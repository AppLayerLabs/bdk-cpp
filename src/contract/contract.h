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


/// Global variables for contracts, such as current block Height, timestamp and coinbase
class ContractGlobals {
  protected:
    /* Global variables */
    static Address coinbase;                ///< Coinbase address (creator of current block).
    static uint256_t blockHeight;           ///< Current block height.
    static uint256_t blockTimestamp;        ///< Current block timestamp.
  public:
    /// Getter for `coinbase`.
    static const Address& getCoinbase() { return coinbase; }

    /// Getter for `blockHeight`.
    static const uint256_t& getBlockHeight() { return blockHeight; }

    /// Getter for `blockTimestamp`.
    static const uint256_t& getBlockTimestamp() { return blockTimestamp; }

    friend State; // State can update private global vars, this is done before calling ethCall() with a TxBlock.
};

class ContractLocals : public ContractGlobals {
  private:
    mutable Address origin;          /// Who called the contract
    mutable Address caller;          /// Who sent the tx
    mutable uint256_t value;         /// Value sent with the tx
    mutable bool commit = false;     /// Tells if the contract should commit to variables or not.

  protected:
    const Address& getOrigin() const { return this->origin; }
    const Address& getCaller() const { return this->caller; }
    const uint256_t& getValue() const { return this->value; }
    bool getCommit() const { return this->commit; }

    friend class ContractManager; /// ContractManager updates the contract locals before calling ethCall within a contract.
    friend class ContractManagerInterface; /// ContractManagerInterface can set the commit flag.
};

/**
 * Base class for all contracts.
 */

class BaseContract : public ContractLocals {
  private:
    /* Contract-specific variables */
    std::string contractName;                                  ///< Name of the contract, used to identify the Contract Class.
    Address contractAddress;                                   ///< Address where the contract is deployed.
    Address contractCreator;                                   ///< Address of the creator of the contract.
    uint64_t contractChainId;                                  ///< Chain where the contract is deployed.
  protected:
    const std::unique_ptr<DB>& db;                                    ///< Pointer to the DB instance.
  public:
    /**
     * Constructor.
     * @param address The address where the contract will be deployed.
     * @param chainId The chain where the contract wil be deployed.
     * @param contractManager Pointer to the contract manager.
     */
    BaseContract(const std::string& contractName, const Address& address, const Address& creator, const uint64_t& chainId, const std::unique_ptr<DB> &db
    ) : contractName(contractName), contractAddress(address), contractCreator(creator), contractChainId(chainId), db(db) {
      db->put("contractName", contractName, DBPrefix::contracts + contractAddress.get());
      db->put("contractAddress", contractAddress.get(), DBPrefix::contracts + contractAddress.get());
      db->put("contractCreator", contractCreator.get(), DBPrefix::contracts + contractAddress.get());
      db->put("contractChainId", Utils::uint64ToBytes(contractChainId), DBPrefix::contracts + contractAddress.get());
    }

    /**
     * Constructor.
     * @param db
     */
    BaseContract(const Address& address, const std::unique_ptr<DB> &db) : contractAddress(address), db(db) {
      this->contractName = db->get("contractName", DBPrefix::contracts + contractAddress.get());
      this->contractCreator = Address(db->get("contractCreator", DBPrefix::contracts + contractAddress.get()), true);
      this->contractChainId = Utils::bytesToUint64(db->get("contractChainId", DBPrefix::contracts + contractAddress.get()));
    }

    /// All derived classes should override the destructor in order to call DB functions.
    virtual ~BaseContract() { }

    /**
     * Invoke a contract function using a transaction. (automatically differs between payable and non-payable functions)
     * Should be overriden by derived classes.
     * @param tx The transaction to use for call.
     * @param commit Whether to commit the changes to the SafeVariables or just simulate the transaction.
     */
    virtual void ethCall(const ethCallInfo& data) { throw std::runtime_error("Derived Class from Contract does not override ethCall()"); }


    /**
     * Do a contract call to a view function
     * Should be overriden by derived classes.
     * @param data
     * @return
     */
    virtual const std::string ethCallView(const ethCallInfo& data) const { throw std::runtime_error("Derived Class from Contract does not override ethCall()"); }

    const Address& getContractAddress() const { return this->contractAddress; }
    const Address& getContractCreator() const { return this->contractCreator; }
    const uint64_t& getContractChainId() const { return this->contractChainId; }
    const std::string& getContractName() const { return this->contractName; }
};

#endif  // CONTRACT_H
