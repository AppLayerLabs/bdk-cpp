/*g
Copyright (c) [2023-2024] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef EVMHOST_HPP
#define EVMHOST_HPP

#include <evmc/evmc.hpp>
#include "../utils/utils.h"
#include "../utils/strings.h"
#include "../utils/hex.h"
#include "../utils/safehash.h"
#include "../utils/db.h"
#include "storage.h"
#include <evmone/evmone.h>
#include "../utils/randomgen.h"

/**
 * EVM Abstraction for an account
 * An account holds nonce, code, codehash, balance and storage.
 * It always holds a "original" (first) and "current" (second) values
 * These original and current values are used in case of reversions due to contract exceptions
 */
struct EVMAccount {
  std::pair<uint64_t, uint64_t> nonce;  ///< Account nonce.w
  std::pair<Bytes, Bytes> code;         ///< Account code.
  std::pair<Hash, Hash> codeHash;       ///< Account code hash.
  std::pair<uint256_t, uint256_t> balance;  ///< Account balance.
  std::unordered_map<Hash, std::pair<Hash, Hash>, SafeHash> storage;  ///< Account storage.
  std::unordered_map<Hash, Hash, SafeHash> transientStorage; ///< Account transient storage.
};

struct EVMEvent {
  Address creator;
  Bytes data;
  std::vector<Hash> topics;
};


/*
 * Class for the EVMHost
 * used by the State to execute the EVM
 * Everything is public as we want the State to be able to access everything
 */

class EVMHost : public evmc::Host {
public:
  EVMHost(const Storage* storage_, DB* db_, const Options* const options_, evmc_vm* vm_) :  storage(storage_), db(db_), options(options_), vm(vm_) {
    /// Load from DB if we have saved based on the current chain height
    if (db) {
      if (db->has(std::string("latest"), DBPrefix::evmHost)) {
        auto latestSaved = Utils::bytesToUint64(db->get(std::string("latest"), DBPrefix::evmHost));
        if (this->storage->latest()->getNHeight() != latestSaved) {
          throw std::runtime_error("EVMHost: Chain height mismatch, DB is corrupted");
        }

        {
          auto accountsCodeBatch = db->getBatch(DB::makeNewPrefix(DBPrefix::evmHost, "accounts_code"));
          auto accountsCodeHashBatch = db->getBatch(DB::makeNewPrefix(DBPrefix::evmHost, "accounts_hashcode"));
          auto contractAddressesBatch = db->getBatch(DB::makeNewPrefix(DBPrefix::evmHost, "contract_addresses"));

          for (const auto& [key, value] : accountsCodeBatch) {
            this->accounts[Address(key)].code.first = value;
            this->accounts[Address(key)].code.second = value;
          }

          for (const auto& [key, value] : accountsCodeHashBatch) {
            this->accounts[Address(key)].codeHash.first = Hash(value);
            this->accounts[Address(key)].codeHash.second = Hash(value);
          }

          for (const auto& [key, value] : contractAddressesBatch) {
            this->contractAddresses[Hash(key)] = Address(value);
          }
        }

        // We put these into their own scope because they use a lot of memory
        {
          auto accountsStorageBatch = db->getBatch(DB::makeNewPrefix(DBPrefix::evmHost, "accounts_storage"));
          for (const auto& [key, value] : accountsStorageBatch) {
            BytesArrView keyView(key);
            Address addr(keyView.subspan(0, 20));
            Hash realKey(keyView.subspan(20));
            this->accounts[addr].storage[realKey].first = Hash(value);
            this->accounts[addr].storage[realKey].second = Hash(value);
          }
        }
      }
    }
  }

  ~EVMHost() override {
    if (this->db) {
      uint64_t lastestBlockHeight = this->storage->latest()->getNHeight();
      this->db->put(std::string("latest"), Utils::uint64ToBytes(lastestBlockHeight), DBPrefix::evmHost);
      DBBatch batch;
      for (const auto& [address, account] : this->accounts) {
        batch.push_back(address.asBytes(), account.code.first, DB::makeNewPrefix(DBPrefix::evmHost, "accounts_code"));
        batch.push_back(address.asBytes(), account.codeHash.first.asBytes(), DB::makeNewPrefix(DBPrefix::evmHost, "accounts_hashcode"));

        for (const auto& [key, value] : account.storage) {
          // Key for account storage will be address + key
          // Vaue is value.first.asBytes()
          Bytes keyBytes = address.asBytes();
          Utils::appendBytes(keyBytes, key.asBytes());
          batch.push_back(keyBytes, value.second.asBytes(), DB::makeNewPrefix(DBPrefix::evmHost, "accounts_storage"));
        }
      }

      for (const auto& [txHash, address] : this->contractAddresses) {
        batch.push_back(txHash.asBytes(), address.asBytes(), DB::makeNewPrefix(DBPrefix::evmHost, "contract_addresses"));
      }

      this->db->putBatch(batch);
    }
  }

  RandomGen* randomGen = nullptr;
  evmc_vm* vm;
  const Storage* storage; // Pointer to the storage object
  DB * const db; // Pointer to the DB object
  const Options * const options; // Pointer to the options object

  /**
   * Internal variables for the EVMHost
   * Variables that are saved to DB are the following
   * Nonce and balance is handled by the State
   * this->accounts
   * Of the accounts we save:
   *   - code (first)
   *   - codeHash (first)
   *   - storage (hash + first)
   * contractAddresses
   */
  std::unordered_map<Address, EVMAccount, SafeHash> accounts;
  std::vector<Address> accessedAccountsBalances;           // Used to know what accounts were accessed to commit or reverts
  std::vector<Address> accessedAccountsCode;                   // Used to know what accounts were accessed to commit or reverts
  std::vector<Address> accessedAccountsNonces;                   // Used to know what accounts were accessed to commit or reverts
  std::vector<std::pair<Address, Hash>> accessedStorages;  // Used to know what storages were accessed to commit or reverts
  std::unordered_map<Hash, Address, SafeHash> contractAddresses; // Used to know what contract addresses were created based on tx Hash
  std::vector<Hash> recentlyCreatedContracts;              // Used to know what contracts were created to clear
  std::vector<Address> accessedTransients;                 // Used to know what transient storages were accessed to clear
  evmc_tx_context currentTxContext = {};                   // Current transaction context
  Hash currentTxHash;                                      // Current transaction hash
  std::vector<std::array<uint8_t, 32>> m_ecrecover_results; // Used to store the results of ecrecover precompile (so we don't have a memory leak)
  std::vector<Bytes> abiPackResults;                       // Used to store the results of abi precompile (so we don't have a memory leak)
  std::vector<EVMEvent> emittedEvents;                     // Used to store the emitted events by current call
  mutable bool shouldRevert = false;                               // Used to know if we should revert or commit in the case of a exception inside any of the calls below

//  evmc::Result createContract(const ethCallInfo& tx) {
//    const auto& [from, to, gasLimit, gasPrice, value, functor, data, fullData] = tx;
//    // if (from != this->options->getChainOwner()) {
//    //   throw std::runtime_error("Only the chain owner can create contracts");//
//    // }
//
//    const auto contractAddress = deriveContractAddress(this->accounts[from].nonce.second, from);
//    evmc_message creationMsg;
//    creationMsg.kind = evmc_call_kind::EVMC_CREATE;
//    creationMsg.gas = static_cast<uint64_t>(gasLimit);
//    creationMsg.recipient = contractAddress.toEvmcAddress();
//    creationMsg.sender = from.toEvmcAddress();
//    creationMsg.input_data = nullptr;
//    creationMsg.input_size = 0;
//    creationMsg.value = Utils::uint256ToEvmcUint256(value);
//    creationMsg.create2_salt = {};
//    creationMsg.code_address = {};
//    creationMsg.depth = 1;
//    creationMsg.flags = 0;
//
//    auto creationResult = evmc::Result(evmc_execute(this->vm, &this->get_interface(), this->to_context(),
//               evmc_revision::EVMC_LATEST_STABLE_REVISION, &creationMsg,
//               fullData.data(), fullData.size()));
//
//    if (creationResult.status_code) {
//      return creationResult;
//    }
//    // Store contract code into the account
//    Bytes code = Utils::cArrayToBytes(creationResult.output_data, creationResult.output_size);
//    this->accounts[contractAddress].codeHash.second = Utils::sha3(code);
//    this->accounts[contractAddress].code.second = code;
//    // Stored used to revert in case of exception
//    this->recentlyCreatedContracts.push_back(currentTxHash);
//    this->contractAddresses[currentTxHash] = contractAddress;
//    this->accessedAccountsCode.push_back(contractAddress);
//
//    return creationResult;
//  }

//  evmc::Result execute(const ethCallInfo& tx, RandomGen* randomGen_) {
//    this->randomGen = randomGen_;
//    const auto& [from, to, gasLimit, gasPrice, value, functor, data, fullData] = tx;
//
//    if (to == Address()) {
//      return this->createContract(tx);
//    }
//    evmc_message msg;
//    msg.kind = evmc_call_kind::EVMC_CALL;
//    msg.flags = 0;
//    msg.gas = static_cast<int64_t>(gasLimit);
//    msg.recipient = to.toEvmcAddress();
//    msg.sender = from.toEvmcAddress();
//    msg.input_data = fullData.data();
//    msg.input_size = fullData.size();
//    msg.value = Utils::uint256ToEvmcUint256(value);
//    msg.create2_salt = {};
//    msg.depth = 1;
//    msg.code_address = to.toEvmcAddress();
//
//    return evmc::Result(evmc_execute(this->vm, &this->get_interface(), this->to_context(),
//               evmc_revision::EVMC_LATEST_STABLE_REVISION, &msg,
//                accounts[to].code.second.data(), accounts[to].code.second.size()));
//  }


  static Address deriveContractAddress(const uint256_t& nonce, const Address& address) {
    // Contract address is last 20 bytes of sha3 ( rlp ( tx from address + tx nonce ) )
    uint8_t rlpSize = 0xc0;
    rlpSize += 20;
    // As we don't have actually access to the nonce, we will use the number of contracts existing in the chain
    rlpSize += (nonce < 0x80)
      ? 1 : 1 + Utils::bytesRequired(nonce);
    Bytes rlp;
    rlp.insert(rlp.end(), rlpSize);
    rlp.insert(rlp.end(), address.cbegin(), address.cend());
    rlp.insert(rlp.end(), (nonce < 0x80)
      ? (char)nonce
      : (char)0x80 + Utils::bytesRequired(nonce)
    );
    return {Utils::sha3(rlp).view(12)};
  }

  bool isEvmContract(const Address& address) {
    auto it = this->accounts.find(address);
    if (it == this->accounts.end()) {
      return false;
    }
    return !(it->second.code.second.empty());
  }

  //void setTxContext(const ethCallInfo& tx,
  //                  const Hash& blockHash,
  //                  const uint64_t& blockHeight,
  //                  const Address& blockCoinbase,
  //                  const uint64_t& blockTimestamp,
  //                  const uint64_t& blockGasLimit,
  //                  const uint256_t& chainId) {
//
  //    const auto [from, to, gasLimit, gasPrice, value, functor, data, fullData] = tx;
  //    this->currentTxContext.tx_gas_price = Utils::uint256ToEvmcUint256(gasPrice);
  //    this->currentTxContext.tx_origin = from.toEvmcAddress();
  //    this->currentTxContext.block_coinbase = blockCoinbase.toEvmcAddress();
  //    this->currentTxContext.block_number = blockHeight;
  //    this->currentTxContext.block_timestamp = blockTimestamp;
  //    this->currentTxContext.block_gas_limit = blockGasLimit;
  //    this->currentTxContext.block_prev_randao = Utils::uint256ToEvmcUint256(0);
  //    this->currentTxContext.chain_id = Utils::uint256ToEvmcUint256(chainId);
  //    this->currentTxContext.block_base_fee = Utils::uint256ToEvmcUint256(0);
  //    this->currentTxContext.blob_base_fee = Utils::uint256ToEvmcUint256(0);
  //    this->currentTxContext.blob_hashes = nullptr;
  //  }

    bool account_exists(const evmc::address& addr) const noexcept override {
      try {
        Address address(addr);
        return this->accounts.find(address) != this->accounts.end();
      } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        this->shouldRevert = true;
        return false;
      }
    }
    evmc::bytes32 get_storage(const evmc::address& addr, const evmc::bytes32& key) const noexcept override {
      try {
        const auto acc = this->accounts.find(addr);
        if (acc == this->accounts.end()) {
          return {};
        }
        const auto storage = acc->second.storage.find(key);
        if (storage == acc->second.storage.end()) {
          return {};
        }
        return storage->second.second.toEvmcBytes32();
      } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        this->shouldRevert = true;
        return {};
      }
    }

    evmc_storage_status set_storage(const evmc::address& addr, const evmc::bytes32& key, const evmc::bytes32& value) noexcept override {
      auto& oldVal = this->accounts[addr].storage[key];

      try {
        this->accessedStorages.emplace_back(addr, key);
        // bytes32 is an array of uint8_t bytes[32];, Hash .raw() returns a pointer to the start of a std::array<uint8_t, 32>
        // We can can the pointer to a bytes32
        const evmc::bytes32 oldOrig = oldVal.first.toEvmcBytes32();
        const evmc::bytes32 oldCurr = oldVal.second.toEvmcBytes32();

        // Folow EIP-1283
        if (oldCurr == value) {
          return evmc_storage_status::EVMC_STORAGE_ASSIGNED;
        }

        evmc_storage_status status{};
        if (oldOrig == oldCurr) {
          if (!oldCurr) {
            status = evmc_storage_status::EVMC_STORAGE_ADDED;
          } else if (value) {
            status = evmc_storage_status::EVMC_STORAGE_MODIFIED;
          } else {
            status = evmc_storage_status::EVMC_STORAGE_DELETED;
          }
        } else {
          status = evmc_storage_status::EVMC_STORAGE_ASSIGNED;
        }

        oldVal.second = Hash(value);
        return status;
      } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        this->shouldRevert = true;
        return evmc_storage_status::EVMC_STORAGE_MODIFIED;
      }
    }

    evmc::uint256be get_balance(const evmc::address& addr) const noexcept override {
      try {
        const auto acc = this->accounts.find(addr);
        if (acc == this->accounts.end()) {
          return {};
        }
        return Utils::uint256ToEvmcUint256(acc->second.balance.second);
      } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        this->shouldRevert = true;
        return {};
      }
    }

    size_t get_code_size(const evmc::address& addr) const noexcept override {
      try {
        const auto acc = this->accounts.find(addr);
        if (acc == this->accounts.end()) {
          return 0;
        }
        return acc->second.code.second.size();
      } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        this->shouldRevert = true;
        return 0;
      }
    }

    evmc::bytes32 get_code_hash(const evmc::address& addr) const noexcept override {
      try {
        const auto acc = this->accounts.find(addr);
        if (acc == this->accounts.end()) {
          return {};
        }
        return acc->second.codeHash.second.toEvmcBytes32();
      } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        this->shouldRevert = true;
        return {};
      }
    }

    size_t copy_code(const evmc::address& addr, size_t code_offset, uint8_t* buffer_data, size_t buffer_size) const noexcept override {
      try {
        const auto it = accounts.find(addr);
        if (it == accounts.end())
          return 0;

        const auto& code = it->second.code.second;

        if (code_offset >= code.size())
          return 0;

        const auto n = std::min(buffer_size, code.size() - code_offset);

        if (n > 0)
          std::copy_n(&code[code_offset], n, buffer_data);

        return n;
      } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        this->shouldRevert = true;
        return 0;
      }
    }
    bool selfdestruct(const evmc::address& addr, const evmc::address& beneficiary) noexcept override {
      // SelfDestruct is NOT implemented/allowed in Sparq
      this->shouldRevert = true;
      return false;
    }

    evmc::Result call(const evmc_message& msg) noexcept override {
      evmc::Result result (evmc_execute(this->vm, &this->get_interface(), this->to_context(),
               evmc_revision::EVMC_LATEST_STABLE_REVISION, &msg,
               accounts[msg.recipient].code.second.data(), accounts[msg.recipient].code.second.size()));
      return result;
    }

    evmc_tx_context get_tx_context() const noexcept override {
      return this->currentTxContext;
    }

    evmc::bytes32 get_block_hash(int64_t number) const noexcept override {
      try {
        return Utils::uint256ToEvmcUint256(number);
      } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        this->shouldRevert = true;
        return {};
      }
    }

    void emit_log(const evmc::address& addr, const uint8_t* data, size_t data_size, const evmc::bytes32 topics[], size_t topics_count) noexcept override {
      // TODO: Implement after integrating with state
      try {
        this->emittedEvents.push_back({addr, Bytes(data, data + data_size), {topics, topics + topics_count}});
      } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        this->shouldRevert = true;
      }
    }

    evmc_access_status access_account(const evmc::address& addr) noexcept override {
      // Always tell the EVM we are accessing in a warm manner
      return EVMC_ACCESS_WARM;
    }

    evmc_access_status access_storage(const evmc::address& addr, const evmc::bytes32& key) noexcept override {
      // Like before, always tell the EVM we are accessing in a warm manner
      return EVMC_ACCESS_WARM;
    }

    evmc::bytes32 get_transient_storage(const evmc::address &addr, const evmc::bytes32 &key) const noexcept override {
      try {
        const auto acc = this->accounts.find(addr);
        if (acc == this->accounts.end()) {
          return {};
        }
        const auto storage = acc->second.transientStorage.find(key);
        if (storage == acc->second.transientStorage.end()) {
          return {};
        }
        return storage->second.toEvmcBytes32();
      } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        this->shouldRevert = true;
        return {};
      }
    }

    void set_transient_storage(const evmc::address &addr, const evmc::bytes32 &key, const evmc::bytes32 &value) noexcept override {
      try {
        this->accessedTransients.emplace_back(addr);
        this->accounts[addr].transientStorage[key] = Hash(value);
      } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        this->shouldRevert = true;
      }
    }
};


#endif // EVMHOST_HPP