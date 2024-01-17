/*
Copyright (c) [2023] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef SDKTESTSUITE_H
#define SDKTESTSUITE_H

#include "../src/core/storage.h"
#include "../src/core/rdpos.h"
#include "../src/core/state.h"
#include "../src/net/p2p/managernormal.h"
#include "../src/net/http/httpserver.h"
#include "../src/utils/options.h"
#include "../src/utils/db.h"
#include "../src/core/blockchain.h"


/**
 * Struct for accounts used within the SDKTestSuite
 * A simple wrapper around a private key and its corresponding address.
 */
struct TestAccount {
  /// Private key of the account.
  const PrivKey privKey;
  /// Address of the account.
  const Address address;
  /// Empty Account constructor.
  TestAccount() = default;
  /// Account constructor.
  /// @param privKey_ Private key of the account.
  TestAccount(const PrivKey& privKey_) : privKey(privKey_), address(Secp256k1::toAddress(Secp256k1::toPub(privKey))) {}
  /// Create a new random account.
  /// @return A new random account.
  static TestAccount newRandomAccount() {
    return TestAccount(Utils::randBytes(32));
  }
  /// Operator bool to check if the account is not default, use PrivKey::operator bool.
  explicit operator bool() const { return bool(this->privKey); }
};

class SDKTestSuite {
  private:
    std::unique_ptr<Options> options_; ///< Pointer to the options singleton.
    std::unique_ptr<DB> db_; ///< Pointer to the database.
    std::unique_ptr<Storage> storage_; ///< Pointer to the blockchain storage.
    std::unique_ptr<State> state_; ///< Pointer to the blockchain state.
    std::unique_ptr<rdPoS> rdpos_; ///< Pointer to the rdPoS object (consensus).
    std::unique_ptr<P2P::ManagerNormal> p2p_; ///< Pointer to the P2P connection manager.
    std::unique_ptr<HTTPServer> http_; ///< Pointer to the HTTP server.
    const TestAccount chainOwnerAccount_ = TestAccount(Hex::toBytes("0xe89ef6409c467285bcae9f80ab1cfeb3487cfe61ab28fb7d36443e1daa0c2867")); ///< Owner of the chain. (0x00dead00...)

    /// PrivateKeys of the validators for the rdPoS within SDKTestSuite
    const std::vector<PrivKey> validatorPrivKeys_ {
      PrivKey(Hex::toBytes("0x0a0415d68a5ec2df57aab65efc2a7231b59b029bae7ff1bd2e40df9af96418c8")),
      PrivKey(Hex::toBytes("0xb254f12b4ca3f0120f305cabf1188fe74f0bd38e58c932a3df79c4c55df8fa66")),
      PrivKey(Hex::toBytes("0x8a52bb289198f0bcf141688a8a899bf1f04a02b003a8b1aa3672b193ce7930da")),
      PrivKey(Hex::toBytes("0x9048f5e80549e244b7899e85a4ef69512d7d68613a3dba828266736a580e7745")),
      PrivKey(Hex::toBytes("0x0b6f5ad26f6eb79116da8c98bed5f3ed12c020611777d4de94c3c23b9a03f739")),
      PrivKey(Hex::toBytes("0xa69eb3a3a679e7e4f6a49fb183fb2819b7ab62f41c341e2e2cc6288ee22fbdc7")),
      PrivKey(Hex::toBytes("0xd9b0613b7e4ccdb0f3a5ab0956edeb210d678db306ab6fae1e2b0c9ebca1c2c5")),
      PrivKey(Hex::toBytes("0x426dc06373b694d8804d634a0fd133be18e4e9bcbdde099fce0ccf3cb965492f"))
    };
  public:
    /**
     * Initialize all components of a full blockchain node.
     * @param sdkPath Path to the SDK folder.
     * @param accounts (optional) List of accounts to initialize the blockchain with.
     * @param options (optional) Options to initialize the blockchain with.
     */
    SDKTestSuite(const std::string& sdkPath,
                 const std::vector<TestAccount>& accounts = {},
                 const std::unique_ptr<Options>& options = nullptr) {
      // Initialize the DB
      std::string dbPath = sdkPath + "/db";
      if (std::filesystem::exists(dbPath)) {
        std::filesystem::remove_all(dbPath);
      }
      this->db_ = std::make_unique<DB>(dbPath);

      // Create the initial blockchain information (genesis block) and fill DB with it.
      // Genesis Keys:
      // Private: 0xe89ef6409c467285bcae9f80ab1cfeb348  Hash(Hex::toBytes("0x0a0415d68a5ec2df57aab65efc2a7231b59b029bae7ff1bd2e40df9af96418c8")),7cfe61ab28fb7d36443e1daa0c2867
      // Address: 0x00dead00665771855a34155f5e7405489df2c3c6
      Block genesis(Hash(Utils::uint256ToBytes(0)), 1678887537000000, 0);
      genesis.finalize(PrivKey(Hex::toBytes("0xe89ef6409c467285bcae9f80ab1cfeb3487cfe61ab28fb7d36443e1daa0c2867")), 1678887538000000);
      this->db_->put(Utils::stringToBytes("latest"), genesis.serializeBlock(), DBPrefix::blocks);
      this->db_->put(Utils::uint64ToBytes(genesis.getNHeight()), genesis.hash().get(), DBPrefix::blockHeightMaps);
      this->db_->put(genesis.hash().get(), genesis.serializeBlock(), DBPrefix::blocks);

      // Populate rdPoS DB with unique rdPoS, not default.
      for (uint64_t i = 0; i < this->validatorPrivKeys_.size(); ++i) {
        this->db_->put(Utils::uint64ToBytes(i), Address(Secp256k1::toAddress(Secp256k1::toUPub(this->validatorPrivKeys_[i]))).get(),
                DBPrefix::rdPoS);
      }

      // Fill initial accounts with some funds.
      // Populate State DB with one address.
      // Initialize with chain owner account.
      // See ~State for encoding
      const uint256_t desiredBalance("1000000000000000000000");
      {
        Bytes value = Utils::uintToBytes(Utils::bytesRequired(desiredBalance));
        Utils::appendBytes(value, Utils::uintToBytes(desiredBalance));
        value.insert(value.end(), 0x00);
        this->db_->put(this->chainOwnerAccount_.address.get(), value, DBPrefix::nativeAccounts);
      }
      // Populate the remaining accounts.
      for (const TestAccount& account : accounts) {
        Bytes value = Utils::uintToBytes(Utils::bytesRequired(desiredBalance));
        Utils::appendBytes(value, Utils::uintToBytes(desiredBalance));
        value.insert(value.end(), 0x00);
        this->db_->put(account.address.get(), value, DBPrefix::nativeAccounts);
      }
      // Create a default options if none is provided.
      if (options == nullptr) {
        std::vector<std::pair<boost::asio::ip::address, uint64_t>> discoveryNodes;
        this->options_ = std::make_unique<Options>(
          sdkPath,
          "OrbiterSDK/cpp/linux_x86-64/0.1.2",
          1,
          8080,
          8080,
          9999,
          discoveryNodes
        );
      } else {
        this->options_ = std::make_unique<Options>(*options);
      }
      this->storage_ = std::make_unique<Storage>(db_, options_);
      this->rdpos_ = std::make_unique<rdPoS>(db_, storage_, p2p_, options_, state_);
      this->state_ = std::make_unique<State>(db_, storage_, rdpos_, p2p_, options_);
      this->p2p_ = std::make_unique<P2P::ManagerNormal>(boost::asio::ip::address::from_string("127.0.0.1"), rdpos_, options_, storage_, state_);
      this->http_ = std::make_unique<HTTPServer>(state_, storage_, p2p_, options_);
    }

    /**
     * Get a block using its hash
     * @param hash Hash of the block to get.
     * @return A pointer to the block. nullptr if not found.
     */
    const std::shared_ptr<const Block> getBlock(const Hash& hash) const {
      return this->storage_->getBlock(hash);
    }

    /**
     * Get a block using its height
     * @param height Height of the block to get.
     * @return A pointer to the block. nullptr if not found.
     */
    const std::shared_ptr<const Block> getBlock(const uint64_t height) const {
      return this->storage_->getBlock(height);
    }

    /**
     * Create a new valid block, finalize it, and add it to the chain.
     * Returns a pointer to the new block.
     * @param timestamp (optional) Timestamp to use for the block in microseconds.
     * @param txs (optional) List of transactions to include in the block.
     */
    const std::shared_ptr<const Block> advanceChain(const uint64_t& timestamp = 0, const std::vector<TxBlock>& txs = {}) {
      auto validators = rdpos_->getValidators();
      auto randomList = rdpos_->getRandomList();
      Hash blockSignerPrivKey;           // Private key for the block signer.
      std::vector<Hash> orderedPrivKeys; // Private keys for the rdPoS in the order of the random list, limited to rdPoS::minValidators.
      orderedPrivKeys.reserve(4);
      for (const auto& privKey : this->validatorPrivKeys_) {
        if (Secp256k1::toAddress(Secp256k1::toUPub(privKey)) == randomList[0]) {
          blockSignerPrivKey = privKey;
          break;
        }
      }

      for (uint64_t i = 1; i < rdPoS::minValidators + 1; i++) {
        for (const auto& privKey : this->validatorPrivKeys_) {
          if (Secp256k1::toAddress(Secp256k1::toUPub(privKey)) == randomList[i]) {
            orderedPrivKeys.push_back(privKey);
            break;
          }
        }
      }

      // By now we should have randomList[0] privKey in blockSignerPrivKey and the rest in orderedPrivKeys, ordered by the random list.
      // We can proceed with creating the block, transactions have to be **ordered** by the random list.

      // Create a block with 8 TxValidator transactions, 2 for each validator, in order (randomHash and random)
      uint64_t newBlocknHeight = this->storage_->latest()->getNHeight() + 1;
      uint64_t newBlockTimestamp = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
      Hash newBlockPrevHash = this->storage_->latest()->hash();
      Block newBlock(newBlockPrevHash, newBlockTimestamp, newBlocknHeight);
      std::vector<TxValidator> randomHashTxs;
      std::vector<TxValidator> randomTxs;

      std::vector<Hash> randomSeeds(orderedPrivKeys.size(), Hash::random());
      for (uint64_t i = 0; i < orderedPrivKeys.size(); ++i) {
        Address validatorAddress = Secp256k1::toAddress(Secp256k1::toUPub(orderedPrivKeys[i]));
        Bytes hashTxData = Hex::toBytes("0xcfffe746");
        Utils::appendBytes(hashTxData, Utils::sha3(randomSeeds[i].get()));
        Bytes randomTxData = Hex::toBytes("0x6fc5a2d6");
        Utils::appendBytes(randomTxData, randomSeeds[i].get());
        randomHashTxs.emplace_back(
          validatorAddress,
          hashTxData,
          8080,
          newBlocknHeight,
          orderedPrivKeys[i]
        );
        randomTxs.emplace_back(
          validatorAddress,
          randomTxData,
          8080,
          newBlocknHeight,
          orderedPrivKeys[i]
        );
      }
      // Append the transactions to the block.
      for (const auto& tx : randomHashTxs) {
        this->rdpos_->addValidatorTx(tx);
        newBlock.appendTxValidator(tx);
      }
      for (const auto& tx : randomTxs) {
        this->rdpos_->addValidatorTx(tx);
        newBlock.appendTxValidator(tx);
      }
      for (const auto& tx : txs) {
        newBlock.appendTx(tx);
      }

      // Finalize the block
      if (timestamp == 0) {
        newBlock.finalize(blockSignerPrivKey, std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count());
      } else {
        newBlock.finalize(blockSignerPrivKey, timestamp);
      }

      // After finalization, the block should be valid.
      if (!this->state_->validateNextBlock(newBlock)) {
        throw std::runtime_error("SDKTestSuite::advanceBlock: Block is not valid");
      }
      // Process the block
      state_->processNextBlock(std::move(newBlock));
      return this->storage_->latest();
    }

    /**
     * Creates a new TxBlock object based on the provided account and given the current state (for nonce)
     * @param TestAccount from Account to send from. (the private key to sign the transaction will be taken from here)
     * @param Address to Address to send to.
     * @param uint256_t value Amount to send.
     * @param Bytes (optional) data Data to send.
     */
    TxBlock createNewTx(const TestAccount& from, const Address& to, const uint256_t& value, Bytes data = Bytes()) {
      return TxBlock(to,
                 from.address,
                 data,
                 this->options_->getChainID(),
                 this->state_->getNativeNonce(from.address),
                 value,
                 1000000000, // 1 GWEI
                 1000000000, // 1 GWEI
                 21000,
                 from.privKey);
    }

    /**
     * Make a simple transfer transaction and advance the chain with it.
     * @param from Account to send from. (the private key to sign the transaction will be taken from here)
     * @param to Address to send to.
     * @param value Amount to send.
     * @return
     */
    const Hash transfer(const TestAccount& from, const Address& to, const uint256_t& value) {
      TxBlock tx = createNewTx(from, to, value);
      this->advanceChain(0, {tx});
      return tx.hash();
    }

    /**
     * Get all events emitted under a given transaction.
     * @param txHash The hash of the transaction to look for events.
     * @return a vector of events emitted by the transaction.
     */
    const std::vector<Event> getEvents(const Hash& tx) const {
      auto txBlock = this->storage_->getTx(tx);
      return this->state_->getEvents(std::get<0>(txBlock)->hash(), std::get<3>(txBlock), std::get<2>(txBlock));
    }

    /**
     * Get the latest accepted block
     * @return A pointer to the latest accepted block.
     */
    const std::shared_ptr<const Block> getLatestBlock() const {
      return this->storage_->latest();
    }

    /**
     * Get a transaction from the chain using a given hash.
     * @param tx The transaction hash to get.
     * @return A tuple with the found transaction, block hash, index and height, nullptr if the tx was not found
     * @throw std::runtime_error on hash mismatch (should never happen).
     */
    const std::tuple<
      const std::shared_ptr<const TxBlock>, const Hash, const uint64_t, const uint64_t
    > getTx(const Hash& tx) {
      return this->storage_->getTx(tx);
    }

    /**
     * Create a transaction to deploy a new contract and advance the chain with it.
     * Always use the chain owner account to deploy contracts.s
     * @tparam TContract Contract type to deploy.
     * @tparam Args... Arguments to pass to the contract constructor.
     * @return Address of the deployed contract.
     */
    template <typename TContract, typename ...Args>
    const Address deployContract(Args&&... args) {
      TContract::registerContract();
      auto prevContractList = this->state_->getContracts();
      using ContractArgumentTypes = decltype(Utils::removeQualifiers<typename TContract::ConstructorArguments>());

      static_assert(std::is_same_v<ContractArgumentTypes, std::tuple<std::decay_t<Args>...>>, "Invalid contract constructor arguments");
      // Encode the functor
      std::string createSignature = "createNew" + Utils::getRealTypeName<TContract>() + "Contract(";
      createSignature += ContractReflectionInterface::getConstructorArgumentTypesString<TContract>();
      createSignature += ")";
      Functor functor = Utils::sha3(Utils::create_view_span(createSignature)).view_const(0, 4);
      Bytes data(functor.cbegin(), functor.cend());
      // Encode the arguments
      if (sizeof...(args) > 0)
        Utils::appendBytes(data, ABI::Encoder::encodeData<Args...>(std::forward<decltype(args)>(args)...));

      // Create the transaction and advance the chain with it.
      TxBlock createContractTx = createNewTx(this->chainOwnerAccount_, ProtocolContractAddresses.at("ContractManager"), 0, data);

      this->advanceChain(0, {createContractTx});

      // Return the new contract address.
      auto newContractList = this->state_->getContracts();

      // Filter new contract list to find the new contract.
      // TODO: We are assuming that only one contract of the same type is deployed at a time.
      // We need to somehow link a transaction to a newly created contract.
      // This can also be used on eth_getTransactionReceipt contractAddress field.
      Address newContractAddress;
      for (const auto& contract : newContractList) {
        if (std::find(prevContractList.begin(), prevContractList.end(), contract) == prevContractList.end()) {
          newContractAddress = contract.second;
          break;
        }
      }
      return newContractAddress;
    }

    /**
     * Create a transaction to call a contract function and advance the chain with it.
     * Specialization for function withous args
     * @tparam R Return type of the function.
     * @tparam TContract Contract type to call.
     * @param contractAddress Address of the contract to call.
     * @param func Function to call.
     * @param value (optional) Value to send with the transaction.
     * @param testAccount (optional) Account to send the transaction from.
     * @param timestamp (optional) Timestamp to use for the transaction in microseconds.
     */
    template <typename R, typename TContract>
    const Hash callFunction(const Address& contractAddress,
                        R(TContract::*func)(),
                        const uint256_t& value = 0,
                        const TestAccount& testAccount = TestAccount(),
                        const uint64_t& timestamp = 0) {
      /// Create the transaction data
      Hash ret;
      Functor txFunctor = ABI::FunctorEncoder::encode<>(ContractReflectionInterface::getFunctionName(func));
      Bytes txData(txFunctor.cbegin(), txFunctor.cend());
      if (!testAccount) {
        // Use the chain owner account if no account is provided.
        TxBlock tx = this->createNewTx(
            this->getChainOwnerAccount(),
            contractAddress,
            value,
            txData
          );
        ret = tx.hash();
        this->advanceChain(timestamp, {tx});
      } else {
        TxBlock tx = this->createNewTx(
            testAccount,
            contractAddress,
            value,
            txData
          );
        ret = tx.hash();
        this->advanceChain(timestamp, {tx});
      }
      return ret;
    }

    /**
     * Create a transaction to call a contract function and advance the chain with it.
     * Specialization for function with args
     * We are not able to set default values like in the other specialization because of the variadic template.
     * Therefore we need functions with no value/testAccount/timestamp parameters.
     * @tparam R Return type of the function.
     * @tparam TContract Contract type to call.
     * @tparam Args... Arguments to pass to the function.
     * @param contractAddress Address of the contract to call.
     * @param value Value to send with the transaction.
     * @param testAccount Account to send the transaction from.
     * @param timestamp Timestamp to use for the transaction in microseconds.
     * @param func Function to call.
     * @param args Arguments to pass to the function.
     */
    template <typename R, typename TContract, typename ...Args>
    const Hash callFunction(const Address& contractAddress,
                            const uint256_t& value,
                            const TestAccount& testAccount,
                            const uint64_t& timestamp,
                            R(TContract::*func)(const Args&...),
                            const Args&... args) {
      /// Create the transaction data
      Hash ret;
      Functor txFunctor = ABI::FunctorEncoder::encode<Args...>(ContractReflectionInterface::getFunctionName(func));
      Bytes txData(txFunctor.cbegin(), txFunctor.cend());
      Utils::appendBytes(txData, ABI::Encoder::encodeData<Args...>(std::forward<decltype(args)>(args)...));
      if (!testAccount) {
        // Use the chain owner account if no account is provided.
        TxBlock tx = this->createNewTx(
            this->getChainOwnerAccount(),
            contractAddress,
            value,
            txData
          );
        ret = tx.hash();
        this->advanceChain(timestamp, {tx});
      } else {
        TxBlock tx = this->createNewTx(
      this->getChainOwnerAccount(),
          contractAddress,
          value,
        txData
        );
        ret = tx.hash();
        this->advanceChain(timestamp, {tx});
      }
      return ret;
    }

    /**
     * Create a transaction to call a contract function and advance the chain with it.
     * Specialization for function with args with no value/testAccount/timestamp parameters.
     * @tparam R Return type of the function.
     * @tparam TContract Contract type to call.
     * @tparam Args... Arguments to pass to the function.
     * @param contractAddress Address of the contract to call.
     * @param func Function to call.
     * @param args Arguments to pass to the function.
     */
    template <typename R, typename TContract, typename ...Args>
    const Hash callFunction(const Address& contractAddress,
                          R(TContract::*func)(const Args&...),
                          const Args&... args) {
      return this->callFunction(contractAddress, 0, this->getChainOwnerAccount(), 0, func, std::forward<decltype(args)>(args)...);
    }

    /**
     * Create a transaction to call a contract function and advance the chain with it.
     * Specialization for function with args with only value parameter
     * @tparam R Return type of the function.
     * @tparam TContract Contract type to call.
     * @tparam Args... Arguments to pass to the function.
     * @param contractAddress Address of the contract to call.
     * @param value Value to send with the transaction.
     * @param func Function to call.
     * @param args Arguments to pass to the function.
     */
    template <typename R, typename TContract, typename ...Args>
    const Hash callFunction(const Address& contractAddress,
                          uint256_t value,
                          R(TContract::*func)(const Args&...),
                          const Args&... args) {
      return this->callFunction(contractAddress, value, this->getChainOwnerAccount(), 0, func, std::forward<decltype(args)>(args)...);
    }

    /**
     * Create a transaction to call a contract function and advance the chain with it.
     * Specialization for function with args with only testAccount
     * @tparam R Return type of the function.
     * @tparam TContract Contract type to call.
     * @tparam Args... Arguments to pass to the function.
     * @param contractAddress Address of the contract to call.
     * @param testAccount Value to send with the transaction.
     * @param func Function to call.
     * @param args Arguments to pass to the function.
     */
    template <typename R, typename TContract, typename ...Args>
    const Hash callFunction(const Address& contractAddress,
                        const TestAccount& testAccount,
                        R(TContract::*func)(const Args&...),
                        const Args&... args) {
      return this->callFunction(contractAddress, 0, testAccount, 0, func, std::forward<decltype(args)>(args)...);
    }

    /**
     * Create a transaction to call a contract function and advance the chain with it.
     * Specialization for function with args with only timestamp parameter
     * @tparam R Return type of the function.
     * @tparam TContract Contract type to call.
     * @tparam Args... Arguments to pass to the function.
     * @param contractAddress Address of the contract to call.
     * @param timestamp Timestamp to use for the block in microseconds.
     * @param func Function to call.
     * @param args Arguments to pass to the function.
     */
    template <typename R, typename TContract, typename ...Args>
    const Hash callFunction(const Address& contractAddress,
                          uint64_t timestamp,
                          R(TContract::*func)(const Args&...),
                          const Args&... args) {
      return this->callFunction(contractAddress, 0, this->getChainOwnerAccount(), timestamp, func, std::forward<decltype(args)>(args)...);
    }

    /**
     * Create a transaction to call a contract function and advance the chain with it.
     * Specialization for function with args with value and testAccount parameters
     * @tparam R Return type of the function.
     * @tparam TContract Contract type to call.
     * @tparam Args... Arguments to pass to the function.
     * @param contractAddress Address of the contract to call.
     * @param value Value to send with the transaction.
     * @param testAccount Account to send the transaction from.
     * @param func Function to call.
     * @param args Arguments to pass to the function.
     */
    template <typename R, typename TContract, typename ...Args>
    const Hash callFunction(const Address& contractAddress,
                          uint256_t value,
                          const TestAccount& testAccount,
                          R(TContract::*func)(const Args&...),
                          const Args&... args) {
      return this->callFunction(contractAddress, value, testAccount, 0, func, std::forward<decltype(args)>(args)...);
    }

    /**
     * Create a transaction to call a contract function and advance the chain with it.
     * Specialization for function with args with value and timestamp parameters
     * @tparam R Return type of the function.
     * @tparam TContract Contract type to call.
     * @tparam Args... Arguments to pass to the function.
     * @param contractAddress Address of the contract to call.
     * @param value Value to send with the transaction.
     * @param timestamp Timestamp to use for the block in microseconds.
     * @param func Function to call.
     * @param args Arguments to pass to the function.
     */
    template <typename R, typename TContract, typename ...Args>
    const Hash callFunction(const Address& contractAddress,
                          uint256_t value,
                          uint64_t timestamp,
                          R(TContract::*func)(const Args&...),
                          const Args&... args) {
      return this->callFunction(contractAddress, value, this->getChainOwnerAccount(), timestamp, func, std::forward<decltype(args)>(args)...);
    }


    /**
     * Create a transaction to call a contract function and advance the chain with it.
     * Specialization for function with args with value and timestamp parameters
     * @tparam R Return type of the function.
     * @tparam TContract Contract type to call.
     * @tparam Args... Arguments to pass to the function.
     * @param contractAddress Address of the contract to call.
     * @param testAccount Account to send the transaction from.
     * @param timestamp Timestamp to use for the block in microseconds.
     * @param func Function to call.
     * @param args Arguments to pass to the function.
     */
    template <typename R, typename TContract, typename ...Args>
    const Hash callFunction(const Address& contractAddress,
                          const TestAccount& testAccount,
                          uint64_t timestamp,
                          R(TContract::*func)(const Args&...),
                          const Args&... args) {
      return this->callFunction(contractAddress, 0, testAccount, timestamp, func, std::forward<decltype(args)>(args)...);
    }

    /**
     * Call a contract view function with no args and return the result.
     * @tparam R Return type of the function.
     * @tparam TContract Contract type to call.
     * @param contractAddress Address of the contract to call.
     * @param func Function to call.
     * @return The result of the function call. (R)
     */
    template <typename R, typename TContract>
    const R callViewFunction(const Address& contractAddress,
                             R(TContract::*func)() const) {
      /// Create the call data
      ethCallInfoAllocated callData;
      auto& [fromInfo, toInfo, gasInfo, gasPriceInfo, valueInfo, functorInfo, dataInfo] = callData;

      toInfo = contractAddress;
      functorInfo = ABI::FunctorEncoder::encode<>(ContractReflectionInterface::getFunctionName(func));
      dataInfo = Bytes();

      return ABI::Decoder::decodeData<R>(this->state_->ethCall(callData));
    }

    /**
     * Call a contract view function with args and return the result.
     * @tparam R Return type of the function.
     * @tparam TContract Contract type to call.
     * @tparam Args... Arguments to pass to the function.
     * @param contractAddress Address of the contract to call.
     * @param func Function to call.
     * @param args Arguments to pass to the function.
     * @return The result of the function call. (R)
     */
    template <typename R, typename TContract, typename ...Args>
    const R callViewFunction(const Address& contractAddress,
                             R(TContract::*func)(const Args&...) const,
                             const Args&... args) {
      TContract::registerContract();
      /// Create the call data
      ethCallInfoAllocated callData;
      auto& [fromInfo, toInfo, gasInfo, gasPriceInfo, valueInfo, functorInfo, dataInfo] = callData;

      toInfo = contractAddress;
      functorInfo = ABI::FunctorEncoder::encode<Args...>(ContractReflectionInterface::getFunctionName(func));
      dataInfo = ABI::Encoder::encodeData<Args...>(std::forward<decltype(args)>(args)...);

      return std::get<0>(ABI::Decoder::decodeData<R>(this->state_->ethCall(callData)));
    }

    /**
     * Get all the events emitted under the given inputs.
     * @param fromBlock The initial block height to look for.
     * @param toBlock The final block height to look for.
     * @param address The address to look for. If empty, will look for all available addresses.
     * @param topics The topics to filter by. If empty, will look for all available topics.
     * @return A list of matching events, limited by the block and/or log caps set above.
     */
    std::vector<Event> getEvents(
      const uint64_t& fromBlock, const uint64_t& toBlock,
      const Address& address, const std::vector<Hash>& topics
    ) { return this->state_->getEvents(fromBlock, toBlock, address, topics); }

    /**
     * Overload of getEvents() used by "eth_getTransactionReceipts", where
     * parameters are filtered differently (by exact tx, not a range).
     * @param txHash The hash of the transaction to look for events.
     * @param blockIndex The height of the block to look for events.
     * @param txIndex The index of the transaction to look for events.
     * @return A list of matching events, limited by the block and/or log caps set above.
     */
    std::vector<Event> getEvents(
      const Hash& txHash, const uint64_t& blockIndex, const uint64_t& txIndex
    ) { return this->state_->getEvents(txHash, blockIndex, txIndex); }

    /**
     * Get all events emitted by a given confirmed transaction.
     * @param txHash The hash of the transaction to look for events.
     */
    std::vector<Event> getEvents(const Hash& txHash) {
      auto tx = this->storage_->getTx(txHash);
      return this->state_->getEvents(std::get<0>(tx)->hash(), std::get<3>(tx), std::get<2>(tx));
    }

    /**
     * Get a specific event emitted by a given confirmed transaction.
     * Specialization without args (will not filter indexed args)
     * @tparam TContract Contract type to look for.
     * @tparam Args... Arguments to pass to the EventParam.
     * @tparam Flags... Flags to pass to the EventParam.
     * @param txHash The hash of the transaction to look for events.
     * @param func The function to look for.
     * @param anonymous (optional) Whether the event is anonymous or not.
     */
    template <typename TContract, typename... Args, bool... Flags>
    std::vector<Event> getEventsEmittedByTx(const Hash& txHash,
                                           void(TContract::*func)(const EventParam<Args, Flags>&...),
                                           bool anonymous = false) {
      /// Get all the events emitted by the transaction.
      auto eventSignature = ABI::EventEncoder::encodeSignature<Args...>(ContractReflectionInterface::getFunctionName(func));
      std::vector<Hash> topicsToFilter;
      if (!anonymous) {
        topicsToFilter.push_back(eventSignature);
      }

      std::vector<Event> filteredEvents;
      auto allEvents =  this->getEvents(txHash);

      /// Filter the events by the topics
      for (const auto& event : allEvents) {
        if (topicsToFilter.size() == 0) {
          filteredEvents.push_back(event);
        } else {
          if (event.getTopics().size() < topicsToFilter.size()) {
            continue;
          }
          bool match = true;
          for (uint64_t i = 0; i < topicsToFilter.size(); ++i) {
            if (topicsToFilter[i] != event.getTopics()[i]) {
              match = false;
              break;
            }
          }
          if (match) {
            filteredEvents.push_back(event);
          }
        }
      }
      return filteredEvents;
    }

    /**
     * Get a specific event emitted by a given confirmed transaction.
     * Specialization with args (topics)
     * @tparam TContract Contract type to look for.
     * @tparam Args... Arguments to pass to the EventParam.
     * @tparam Flags... Flags to pass to the EventParam.
     * @param txHash The hash of the transaction to look for events.
     * @param func The function to look for.
     * @param anonymous (optional) Whether the event is anonymous or not.
     */
    template <typename TContract, typename... Args, bool... Flags>
    std::vector<Event> getEventsEmittedByTx(const Hash& txHash,
                                           void(TContract::*func)(const EventParam<Args, Flags>&...),
                                           const std::tuple<EventParam<Args, Flags>...>& args,
                                           bool anonymous = false) {
      /// Get all the events emitted by the transaction.
      auto eventSignature = ABI::EventEncoder::encodeSignature<Args...>(ContractReflectionInterface::getFunctionName(func));
      std::vector<Hash> topicsToFilter;
      if (!anonymous) {
        topicsToFilter.push_back(eventSignature);
      }
      std::apply([&](const auto&... param) {
        (..., (param.isIndexed ? topicsToFilter.push_back(ABI::EventEncoder::encodeTopicSignature(param.value)) : void()));
      }, args);

      /// Make topics.size() == 4 if its more than that.
      if (topicsToFilter.size() > 4) {
        topicsToFilter.resize(4);
      }

      std::vector<Event> filteredEvents;
      auto allEvents =  this->getEvents(txHash);
      /// Filter the events by the topics
      for (const auto& event : allEvents) {
        if (topicsToFilter.size() == 0) {
          filteredEvents.push_back(event);
        } else {
          if (event.getTopics().size() < topicsToFilter.size()) {
            continue;
          }
          bool match = true;
          for (uint64_t i = 0; i < topicsToFilter.size(); ++i) {
            if (topicsToFilter[i] != event.getTopics()[i]) {
              match = false;
              break;
            }
          }
          if (match) {
            filteredEvents.push_back(event);
          }
        }
      }
      return filteredEvents;
    }

    /// Chain owner account getter
    const TestAccount& getChainOwnerAccount() const { return this->chainOwnerAccount_; };

    /// Options getter
    const std::unique_ptr<Options>& getOptions() const { return this->options_; };

    /// DB getter
    const std::unique_ptr<DB>& getDB() const { return this->db_; };

    /// Storage getter
    const std::unique_ptr<Storage>& getStorage() const { return this->storage_; };

    /// rdPoS getter
    const std::unique_ptr<rdPoS>& getrdPoS() const { return this->rdpos_; };

    /// State getter
    const std::unique_ptr<State>& getState() const { return this->state_; };

    /// P2P getter
    const std::unique_ptr<P2P::ManagerNormal>& getP2P() const { return this->p2p_; };

    /// HTTP getter
    const std::unique_ptr<HTTPServer>& getHTTP() const { return this->http_; };

    /// Get the native balance of a given address.
    const uint256_t getNativeBalance(const Address& address) const {
      return this->state_->getNativeBalance(address);
    }

    /// Get the nonce of a given address
    const uint64_t getNativeNonce(const Address& address) const {
      return this->state_->getNativeNonce(address);
    }

    /**
     * Initialize the services.
     * Starts P2P and HTTP servers.
     */
    void initializeServices() { this->p2p_->start(); this->http_->start(); }

    /**
     * Stop the services.
     * Stops P2P and HTTP servers.
     */
    void stopServices() { this->http_->stop(); this->p2p_->stop(); }
};







#endif // SDKTESTSUITE_H
