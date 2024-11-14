/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef CONTRACT_H
#define CONTRACT_H

#include "../core/dump.h" // core/storage.h, utils/db.h -> utils.h -> strings.h, libs/json.hpp -> cstdint, memory, string, tuple

// Forward declarations.
class ContractHost;
class State;

/// Class that maintains global variables for contracts.
class ContractGlobals {
  protected:
    static Address coinbase_;         ///< Coinbase address (creator of current block).
    static Hash blockHash_;           ///< Current block hash.
    static uint64_t blockHeight_;     ///< Current block height.
    static uint64_t blockTimestamp_;  ///< Current block timestamp.

  public:
    ///@{
    /** Getter. */
    static const Address& getCoinbase() { return ContractGlobals::coinbase_; }
    static const Hash& getBlockHash() { return ContractGlobals::blockHash_; }
    static const uint64_t& getBlockHeight() { return ContractGlobals::blockHeight_; }
    static const uint64_t& getBlockTimestamp() { return ContractGlobals::blockTimestamp_; }
    ///@}

    /// State can update private global vars (e.g. before starting transaction execution).
    friend class State;
};

/// Class that maintains local variables for contracts.
class ContractLocals : public ContractGlobals {
  private:
    // TODO: DONT RELY ON ContractLocals, INSTEAD, USE CONTRACTHOST TO STORE LOCALS
    mutable Address caller_;  ///< Who sent the transaction.
    mutable uint256_t value_; ///< Value sent within the transaction.

  protected:
    ///@{
    /** Getter. */
    const Address& getCaller() const { return this->caller_; }
    const uint256_t& getValue() const { return this->value_; }
    ///@}

    /// ContractCallLogger can update private local vars (e.g. before ethCall() within a contract).
    friend class ContractHost;
};

/// Base class for all contracts.
class BaseContract : public ContractLocals, public Dumpable {
  private:
    const Address contractAddress_;  ///< Address where the contract is deployed.
    const Bytes dbPrefix_;     ///< Prefix for the contract DB.
    const std::string contractName_; ///< Name of the contract, used to identify the Contract Class.
    const Address contractCreator_;  ///< Address of the creator of the contract.
    const uint64_t contractChainId_; ///< Chain where the contract is deployed.

  protected:
    mutable ContractHost* host_ = nullptr; ///< Reference to the ContractHost instance.
    bool reentrancyLock_ = false; ///< Lock (for reentrancy).

  public:
    /**
     * Constructor from scratch.
     * @param contractName The name of the contract.
     * @param address The address where the contract will be deployed.
     * @param creator The address of the creator of the contract.
     * @param chainId The chain where the contract will be deployed.
     */
    BaseContract(const std::string& contractName, const Address& address,
      const Address& creator, const uint64_t& chainId
    ) : contractAddress_(address),
        dbPrefix_([&]() {
              Bytes prefix = DBPrefix::contracts;
              prefix.reserve(prefix.size() + address.size());
              prefix.insert(prefix.end(), address.cbegin(), address.cend());
              return prefix;
            }()),
        contractName_(contractName), contractCreator_(creator), contractChainId_(chainId) {
    }

    DBBatch dump() const override {
      DBBatch batch;
      batch.push_back(Utils::stringToBytes("contractName_"), Utils::stringToBytes(contractName_), this->getDBPrefix());
      batch.push_back(Utils::stringToBytes("contractAddress_"), contractAddress_, this->getDBPrefix());
      batch.push_back(Utils::stringToBytes("contractCreator_"), contractCreator_, this->getDBPrefix());
      batch.push_back(Utils::stringToBytes("contractChainId_"), UintConv::uint64ToBytes(contractChainId_), this->getDBPrefix());
      return batch;
    }

    /**
     * Constructor from load.
     * @param address The address where the contract will be deployed.
     * @param db Pointer to the DB instance.
     */
    BaseContract(const Address &address, const DB& db) :
      contractAddress_(address),
      dbPrefix_([&]() {
       Bytes prefix = DBPrefix::contracts;
       prefix.reserve(prefix.size() + address.size());
       prefix.insert(prefix.end(), address.cbegin(), address.cend());
       return prefix;
      }()),
      contractName_([&]() {
       return Utils::bytesToString(db.get(std::string("contractName_"), dbPrefix_));
      }()),
      contractCreator_([&]() {
       return Address(db.get(std::string("contractCreator_"), dbPrefix_));
      }()),
      contractChainId_([&]() {
       return UintConv::bytesToUint64(db.get(std::string("contractChainId_"), dbPrefix_));
      }())
    {}

    virtual ~BaseContract() = default;  ///< Destructor. All derived classes should override it in order to call DB functions.

    /**
     * Invoke a contract function using a tuple of (from, to, gasLimit, gasPrice,
     * value, data). Should be overriden by derived classes.
     * @param data The tuple of (from, to, gasLimit, gasPrice, value, data).
     * @param host Pointer to the contract host.
     * @throw DynamicException if the derived class does not override this.
     */
    virtual void ethCall(const evmc_message& data, ContractHost* host);

    /**
     * Invoke a contract function and returns the ABI serialized output.
     * To be used by EVM -> CPP calls.
     * @param data The tuple of (from, to, gasLimit, gasPrice, value, data).
     * @param host Pointer to the contract host.
     * @throw DynamicException if the derived class does not override this.
     * @returns The ABI serialized output.
     */
    virtual Bytes evmEthCall(const evmc_message& data, ContractHost* host);

    /**
     * Do a contract call to a view function.
     * Should be overriden by derived classes.
     * @param data The tuple of (from, to, gasLimit, gasPrice, value, data).
     * @param host Pointer to the contract host.
     * @return A string with the answer to the call.
     * @throw DynamicException if the derived class does not override this.
     */
    virtual Bytes ethCallView(const evmc_message &data, ContractHost* host) const;

    ///@{
    /** Getter. */
    inline const Address& getContractAddress() const { return this->contractAddress_; }
    inline const Address& getContractCreator() const { return this->contractCreator_; }
    inline const uint64_t& getContractChainId() const { return this->contractChainId_; }
    inline const std::string& getContractName() const { return this->contractName_; }
    inline const Bytes& getDBPrefix() const { return this->dbPrefix_; }
    ///@}

    /**
     * Create a new DB prefix by appending a new string to the current prefix.
     * @param newPrefix The new prefix to append.
     * @return The new prefix.
     */
    Bytes getNewPrefix(std::string_view newPrefix) const {
      Bytes prefix = this->dbPrefix_;
      prefix.reserve(prefix.size() + newPrefix.size());
      prefix.insert(prefix.end(), newPrefix.cbegin(), newPrefix.cend());
      return prefix;
    }

    Address getOrigin() const; ///< Get the origin of the transaction.
    uint64_t getNonce(const Address& address) const; ///< Get the nonce of an address.
};

#endif // CONTRACT_H
