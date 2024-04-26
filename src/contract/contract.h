/*
Copyright (c) [2023-2024] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef CONTRACT_H
#define CONTRACT_H

#include <cstdint>
#include <memory>
#include <string>
#include <tuple>

#include "../utils/db.h"
#include "../utils/strings.h"
#include "../utils/tx.h"
#include "../utils/utils.h"
#include "../utils/dynamicexception.h"
#include "variables/safebase.h"
#include "../core/dump.h"

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
    std::string contractName_; ///< Name of the contract, used to identify the Contract Class.
    const Bytes dbPrefix_;           ///< Prefix for the contract DB.
    Address contractAddress_;  ///< Address where the contract is deployed.
    Address contractCreator_;  ///< Address of the creator of the contract.
    uint64_t contractChainId_; ///< Chain where the contract is deployed.

  protected:
    DB& db_; ///< Reference to the DB instance.
    mutable ContractHost* host_ = nullptr; ///< Reference to the ContractHost instance.

  public:
    bool reentrancyLock_ = false; ///< Lock (for reentrancy).

    /**
     * Constructor from scratch.
     * @param contractName The name of the contract.
     * @param address The address where the contract will be deployed.
     * @param creator The address of the creator of the contract.
     * @param chainId The chain where the contract will be deployed.
     * @param db Pointer to the DB instance.
     */
    BaseContract(const std::string& contractName, const Address& address,
      const Address& creator, const uint64_t& chainId, DB& db
    ) : contractName_(contractName), contractAddress_(address),
        contractCreator_(creator), contractChainId_(chainId), db_(db), dbPrefix_([&]() {
                    Bytes prefix = DBPrefix::contracts;
                    prefix.reserve(prefix.size() + address.size());
                    prefix.insert(prefix.end(), address.cbegin(), address.cend());
                    return prefix;
                  }()) {
    }

    DBBatch dump() const override {
      DBBatch batch;
      batch.push_back(Utils::stringToBytes("contractName_"), Utils::stringToBytes(contractName_), this->getDBPrefix());
      batch.push_back(Utils::stringToBytes("contractAddress_"), contractAddress_.get(), this->getDBPrefix());
      batch.push_back(Utils::stringToBytes("contractCreator_"), contractCreator_.get(), this->getDBPrefix());
      batch.push_back(Utils::stringToBytes("contractChainId_"), Utils::uint64ToBytes(contractChainId_), this->getDBPrefix());
      return batch;
    }

    /**
     * Constructor from load.
     * @param address The address where the contract will be deployed.
     * @param db Pointer to the DB instance.
     */
    BaseContract(const Address &address, DB& db) : contractAddress_(address), db_(db),
                 dbPrefix_([&]() -> Bytes {
                   Bytes prefix = DBPrefix::contracts;
                   prefix.reserve(prefix.size() + contractAddress_.size());
                   prefix.insert(prefix.end(), contractAddress_.cbegin(), contractAddress_.cend());
                   return prefix;
                 }())
    {
      this->contractName_ = Utils::bytesToString(db.get(std::string("contractName_"), this->getDBPrefix()));
      this->contractCreator_ = Address(db.get(std::string("contractCreator_"), this->getDBPrefix()));
      this->contractChainId_ = Utils::bytesToUint64(db.get(std::string("contractChainId_"), this->getDBPrefix()));
    }

    virtual ~BaseContract() = default;  ///< Destructor. All derived classes should override it in order to call DB functions.

    /**
     * Invoke a contract function using a tuple of (from, to, gasLimit, gasPrice,
     * value, data). Should be overriden by derived classes.
     * @param data The tuple of (from, to, gasLimit, gasPrice, value, data).
     * @throw DynamicException if the derived class does not override this.
     */
    virtual void ethCall(const evmc_message& data, ContractHost* host) {
      throw DynamicException("Derived Class from Contract does not override ethCall()");
    }

    /**
     * Invoke a contract function and returns the ABI serialized output.
     * To be used by EVM -> CPP calls.
     * @param data The tuple of (from, to, gasLimit, gasPrice, value, data).
     * @throw DynamicException if the derived class does not override this.
     * @returns The ABI serialized output.
     */
    virtual Bytes evmEthCall(const evmc_message& data, ContractHost* host) {
      throw DynamicException("Derived Class from Contract does not override ethCall()");
    }

    /**
     * Do a contract call to a view function.
     * Should be overriden by derived classes.
     * @param data The tuple of (from, to, gasLimit, gasPrice, value, data).
     * @return A string with the answer to the call.
     * @throw DynamicException if the derived class does not override this.
     */
    virtual Bytes ethCallView(const evmc_message &data, ContractHost* host) const {
      throw DynamicException("Derived Class from Contract does not override ethCallView()");
    }

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
    const Bytes getNewPrefix(const std::string& newPrefix) const {
      Bytes prefix = this->dbPrefix_;
      prefix.reserve(prefix.size() + newPrefix.size());
      prefix.insert(prefix.end(), newPrefix.cbegin(), newPrefix.cend());
      return prefix;
    }

    Address getOrigin() const; ///< Get the origin of the transaction.
    uint64_t getNonce(const Address& address) const; ///< Get the nonce of an address.
};

#endif // CONTRACT_H
