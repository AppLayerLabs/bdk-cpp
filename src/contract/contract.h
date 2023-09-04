/*
Copyright (c) [2023] [Sparq Network]

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
#include "variables/safebase.h"

// Forward declarations.
class ContractCallLogger;
class State;

/// Class that maintains global variables for contracts.
class ContractGlobals {
  protected:
    static Address coinbase_;         ///< Coinbase address (creator of current block).
    static Hash blockHash_;           ///< Current block hash.
    static uint256_t blockHeight_;    ///< Current block height.
    static uint256_t blockTimestamp_; ///< Current block timestamp.

  public:
    /// Getter for `coinbase_`.
    static const Address& getCoinbase() { return this->coinbase_; }

    /// Getter for `blockHash_`.
    static const Hash& getBlockHash() { return this->blockHash_; }

    /// Getter for `blockHeight_`.
    static const uint256_t& getBlockHeight() { return this->blockHeight_; }

    /// Getter for `getBlockTimestamp_`.
    static const uint256_t& getBlockTimestamp() { return this->blockTimestamp_; }

    /// State is a friend as it can update private global vars (e.g. before ethCall() with a TxBlock).
    friend State;
};

/// Class that maintains local variables for contracts.
class ContractLocals : public ContractGlobals {
  private:
    mutable Address origin_;       ///< Who called the contract.
    mutable Address caller_;       ///< Who sent the transaction.
    mutable uint256_t value_;      ///< Value sent within the transaction.

  protected:
    /// Getter for `origin`.
    const Address& getOrigin() const { return this->origin_; }

    /// Getter for `caller`.
    const Address& getCaller() const { return this->caller_; }

    /// Getter for `value`.
    const uint256_t& getValue() const { return this->value_; }

    /// ContractCallLogger is a friend as it can update private local vars (e.g. before ethCall() within a contract).
    friend class ContractCallLogger;
};

/// Base class for all contracts.
class BaseContract : public ContractLocals {
  private:
    /* Contract-specific variables */
    std::string contractName_; ///< Name of the contract, used to identify the Contract Class.
    Bytes dbPrefix_;           ///< Prefix for the contract DB.
    Address contractAddress_;  ///< Address where the contract is deployed.
    Address contractCreator_;  ///< Address of the creator of the contract.
    uint64_t contractChainId_; ///< Chain where the contract is deployed.

  protected:
    bool reentrancyLock_ = false;    ///< Lock (for reentrancy).
    const std::unique_ptr<DB> &db_; ///< Pointer to the DB instance.

  public:
    /**
     * Constructor from scratch.
     * @param contractName The name of the contract.
     * @param address The address where the contract will be deployed.
     * @param creator The address of the creator of the contract.
     * @param chainId The chain where the contract will be deployed.
     * @param db Pointer to the DB instance.
     */
    BaseContract(const std::string &contractName, const Address &address,
      const Address &creator, const uint64_t &chainId, const std::unique_ptr<DB> &db
    ) : contractName_(contractName), contractAddress_(address),
      contractCreator_(creator), contractChainId_(chainId), db_(db)
    {
      dbPrefix_ = [&]() -> Bytes {
        Bytes prefix = DBPrefix::contracts;
        prefix.reserve(prefix.size() + contractAddress_.size());
        prefix.insert(prefix.end(), contractAddress_.cbegin(), contractAddress_.cend());
        return prefix;
      }();
      db->put(std::string("contractName_"), contractName_, this->getDBPrefix());
      db->put(std::string("contractAddress_"), contractAddress_.get(), this->getDBPrefix());
      db->put(std::string("contractCreator_"), contractCreator_.get(), this->getDBPrefix());
      db->put(std::string("contractChainId_"), Utils::uint64ToBytes(contractChainId_), this->getDBPrefix());
    }

    /**
     * Constructor from load.
     * @param address The address where the contract will be deployed.
     * @param db Pointer to the DB instance.
     */
    BaseContract(const Address &address, const std::unique_ptr<DB> &db) : contractAddress_(address), db_(db) {
      this->dbPrefix_ = [&]() -> Bytes {
        Bytes prefix = DBPrefix::contracts;
        prefix.reserve(prefix.size() + contractAddress_.size());
        prefix.insert(prefix.end(), contractAddress_.cbegin(), contractAddress_.cend());
        return prefix;
      }();
      this->contractName_ = Utils::bytesToString(db->get(std::string("contractName_"), this->getDBPrefix()));
      this->contractCreator_ = Address(db->get(std::string("contractCreator_"), this->getDBPrefix()));
      this->contractChainId_ = Utils::bytesToUint64(db->get(std::string("contractChainId_"), this->getDBPrefix()));
    }

    /**
     * Destructor.
     * All derived classes should override it in order to call DB functions.
     */
    virtual ~BaseContract() {}

    /**
     * Invoke a contract function using a tuple of (from, to, gasLimit, gasPrice,
     * value, data). Should be overriden by derived classes.
     * @param data The tuple of (from, to, gasLimit, gasPrice, value, data).
     * @throw std::runtime_error if the derived class does not override this.
     */
    virtual void ethCall(const ethCallInfo& data) {
      throw std::runtime_error("Derived Class from Contract does not override ethCall()");
    }

    /**
     * Do a contract call to a view function.
     * Should be overriden by derived classes.
     * @param data The tuple of (from, to, gasLimit, gasPrice, value, data).
     * @return A string with the answer to the call.
     * @throw std::runtime_error if the derived class does not override this.
     */
    virtual const Bytes ethCallView(const ethCallInfo &data) const {
      throw std::runtime_error(
          "Derived Class from Contract does not override ethCall()");
    }

    /// Getter for `contractAddress`.
    const Address& getContractAddress() const { return this->contractAddress_; }

    /// Getter for `contractCreator`.
    const Address& getContractCreator() const { return this->contractCreator_; }

    /// Getter for `contractChainId`.
    const uint64_t& getContractChainId() const { return this->contractChainId_; }

    /// Getter for `contractName_`.
    const std::string& getContractName() const { return this->contractName_; }

    /// Getter for `dbPrefix`.
    const Bytes &getDBPrefix() const { return this->dbPrefix_; }

    /**
     * Creates a new DB prefix appending a new string to the current prefix.
     * @param newPrefix The new prefix to append.
     * @return The new prefix.
     */
    const Bytes getNewPrefix(const std::string &newPrefix) const {
      Bytes prefix = this->dbPrefix_;
      prefix.reserve(prefix.size() + newPrefix.size());
      prefix.insert(prefix.end(), newPrefix.cbegin(), newPrefix.cend());
      return prefix;
    }
};

#endif // CONTRACT_H
