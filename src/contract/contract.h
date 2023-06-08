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
class ContractManager;
class State;

/// Global variables for contracts, such as current block Height, timestamp and
/// coinbase
class ContractGlobals {
protected:
  /* Global variables */
  static Address coinbase; ///< Coinbase address (creator of current block).
  static uint256_t blockHeight;    ///< Current block height.
  static uint256_t blockTimestamp; ///< Current block timestamp.
public:
  /**
   * Getter for `coinbase`.
   * @return The coinbase address (creator of current block).
   */
  static const Address &getCoinbase() { return coinbase; }

  /**
   * Getter for `blockHeight`.
   * @return The current block height.
   */
  static const uint256_t &getBlockHeight() { return blockHeight; }

  /**
   * Getter for `blockTimestamp`.
   * @return The current block timestamp.
   */
  static const uint256_t &getBlockTimestamp() { return blockTimestamp; }

  // Forward declaration of friend classes.
  friend State; // State can update private global vars, this is done before
                // calling ethCall() with a TxBlock.
};

class ContractLocals : public ContractGlobals {
private:
  mutable Address origin;  ///< Who called the contract
  mutable Address caller;  ///< Who sent the tx
  mutable uint256_t value; ///< Value sent with the tx
  mutable bool commit =
      false; ///< Tells if the contract should commit to variables or not.

protected:
  /**
   * Getter for `origin`.
   * @return The origin address (who called the contract).
   */
  const Address &getOrigin() const { return this->origin; }

  /**
   * Getter for `caller`.
   * @return The caller address (who sent the tx).
   */
  const Address &getCaller() const { return this->caller; }

  /**
   * Getter for `value`.
   * @return The value sent with the tx.
   */
  const uint256_t &getValue() const { return this->value; }

  /**
   * Getter for `commit` flag.
   * @return The commit flag.
   */
  bool getCommit() const { return this->commit; }

  // Forward declaration of friend classes.
  friend class ContractManager; /// ContractManager updates the contract locals
                                /// before calling ethCall within a contract.
  friend class ContractManagerInterface; /// ContractManagerInterface can set
                                         /// the commit flag.
};

/**
 * Base class for all contracts.
 */

class BaseContract : public ContractLocals {
private:
  /* Contract-specific variables */
  std::string contractName; ///< Name of the contract, used to identify the
                            ///< Contract Class.
  Bytes dbPrefix;           ///< Prefix for the contract DB.
  Address contractAddress;  ///< Address where the contract is deployed.
  Address contractCreator;  ///< Address of the creator of the contract.
  uint64_t contractChainId; ///< Chain where the contract is deployed.
protected:
  const std::unique_ptr<DB> &db; ///< Pointer to the DB instance.
public:
  /**
   * Constructor.
   * @param address The address where the contract will be deployed.
   * @param chainId The chain where the contract wil be deployed.
   * @param contractManager Pointer to the contract manager.
   */
  BaseContract(const std::string &contractName, const Address &address,
               const Address &creator, const uint64_t &chainId,
               const std::unique_ptr<DB> &db)
      : contractName(contractName), contractAddress(address),
        contractCreator(creator), contractChainId(chainId), db(db) {
    dbPrefix = [&]() -> Bytes {
      Bytes prefix = DBPrefix::contracts;
      prefix.reserve(prefix.size() + contractAddress.size());
      prefix.insert(prefix.end(), contractAddress.cbegin(), contractAddress.cend());
      return prefix;
    }();
    db->put(std::string("contractName"), contractName, this->getDBPrefix());
    db->put(std::string("contractAddress"), contractAddress.get(), this->getDBPrefix());
    db->put(std::string("contractCreator"), contractCreator.get(), this->getDBPrefix());
    db->put(std::string("contractChainId"), Utils::uint64ToBytes(contractChainId), this->getDBPrefix());
  }

  /**
   * Constructor.
   * @param address The address where the contract will be deployed.
   * @param db Pointer to the DB instance.
   */
  BaseContract(const Address &address, const std::unique_ptr<DB> &db)
      : contractAddress(address), db(db) {
    this->dbPrefix = [&]() -> Bytes {
      Bytes prefix = DBPrefix::contracts;
      prefix.reserve(prefix.size() + contractAddress.size());
      prefix.insert(prefix.end(), contractAddress.cbegin(), contractAddress.cend());
      return prefix;
    }();
    this->contractName = Utils::bytesToString(db->get(std::string("contractName"), this->getDBPrefix()));
    this->contractCreator = Address(db->get(std::string("contractCreator"), this->getDBPrefix()));
    this->contractChainId = Utils::bytesToUint64(db->get(std::string("contractChainId"), this->getDBPrefix()));
  }

  /**
   * Destructor.
   */

  /// All derived classes should override the destructor in order to call DB
  /// functions.

  virtual ~BaseContract() {}

  /**
   * Invoke a contract function using a tuple of (from, to, gasLimit, gasPrice,
   * value, data). Should be overriden by derived classes.
   * @param data The tuple of (from, to, gasLimit, gasPrice, value, data).
   * @throws std::runtime_error if the derived class does not override this
   */
  virtual void ethCall(const ethCallInfo &data) {
    throw std::runtime_error(
        "Derived Class from Contract does not override ethCall()");
  }

  /**
   * Do a contract call to a view function
   * Should be overriden by derived classes.
   * @param data The tuple of (from, to, gasLimit, gasPrice, value, data).
   * @return
   * @throws std::runtime_error if the derived class does not override this
   */
  virtual const Bytes ethCallView(const ethCallInfo &data) const {
    throw std::runtime_error(
        "Derived Class from Contract does not override ethCall()");
  }

  /**
   * Getter for `contractAddress`.
   * @return The contract address.
   */
  const Address &getContractAddress() const { return this->contractAddress; }

  /**
   * Getter for `contractCreator`.
   * @return The contract creator address.
   */
  const Address &getContractCreator() const { return this->contractCreator; }

  /**
   * Getter for `contractChainId`.
   * @return The contract chain id.
   */
  const uint64_t &getContractChainId() const { return this->contractChainId; }

  /**
   * Getter for `contractName`.
   * @return The contract name.
   */
  const std::string &getContractName() const { return this->contractName; }

  /**
   * Getter for `dbPrefix`.
   * @return The contract dbPrefix.
   */
  const Bytes &getDBPrefix() const { return this->dbPrefix; }

  /**
   * Creates a new DB prefix appending a new string to the current prefix.
   * @param newPrefix The new prefix to append.
   * @return The new prefix.
   */
  const Bytes getNewPrefix(const std::string &newPrefix) const {
    Bytes prefix = this->dbPrefix;
    prefix.reserve(prefix.size() + newPrefix.size());
    prefix.insert(prefix.end(), newPrefix.cbegin(), newPrefix.cend());
    return prefix;
  }
};

#endif // CONTRACT_H
