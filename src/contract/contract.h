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

/// Class that maintains global variables for contracts.
class ContractGlobals {
  protected:
    static Address coinbase;          ///< Coinbase address (creator of current block).
    static uint256_t blockHeight;     ///< Current block height.
    static uint256_t blockTimestamp;  ///< Current block timestamp.

  public:
    /// Getter for `coinbase`.
    static const Address& getCoinbase() { return coinbase; }

    /// Getter for `blockHeight`.
    static const uint256_t& getBlockHeight() { return blockHeight; }

    /// Getter for `getBlockTimestamp`.
    static const uint256_t& getBlockTimestamp() { return blockTimestamp; }

    /// State is a friend as it can update private global vars (e.g. before ethCall() with a TxBlock).
    friend State;
};

/// Class that maintains local variables for contracts.
class ContractLocals : public ContractGlobals {
  private:
    mutable Address origin;       ///< Who called the contract.
    mutable Address caller;       ///< Who sent the transaction.
    mutable uint256_t value;      ///< Value sent within the transaction.
    mutable bool commit = false;  ///< Indicates whether the contract should commit to variables.

  protected:
    /// Getter for `origin`.
    const Address& getOrigin() const { return this->origin; }

    /// Getter for `caller`.
    const Address& getCaller() const { return this->caller; }

    /// Getter for `value`.
    const uint256_t& getValue() const { return this->value; }

    /// Getter for `commit`.
    bool getCommit() const { return this->commit; }

    /// ContractManager is a friend as it can update private local vars (e.g. before ethCall() within a contract).
    friend class ContractManager;

    /// ContractManagerInterface is a friend as it can set the commit flag.
    friend class ContractManagerInterface;
};

/// Base class for all contracts.
class BaseContract : public ContractLocals {
  private:
    std::string contractName; ///< Name of the contract, used to identify the contract class.
    Address contractAddress;  ///< Address where the contract is deployed.
    Address contractCreator;  ///< Address of the creator of the contract.
    uint64_t contractChainId; ///< Chain where the contract is deployed.

  protected:
    const std::unique_ptr<DB>& db; ///< Pointer to the DB instance.

  public:
    /**
     * Constructor from scratch.
     * @param contractName The contract's name.
     * @param address The address where the contract will be deployed.
     * @param creator The address of the creator of the contract.
     * @param chainId The chain where the contract wil be deployed.
     * @param db Pointer to the database.
     */
    BaseContract(
      const std::string& contractName, const Address& address,
      const Address& creator, const uint64_t& chainId,
      const std::unique_ptr<DB>& db
    ) : contractName(contractName), contractAddress(address),
      contractCreator(creator), contractChainId(chainId), db(db)
    {
      db->put("contractName", contractName, DBPrefix::contracts + contractAddress.get());
      db->put("contractAddress", contractAddress.get(), DBPrefix::contracts + contractAddress.get());
      db->put("contractCreator", contractCreator.get(), DBPrefix::contracts + contractAddress.get());
      db->put("contractChainId", Utils::uint64ToBytes(contractChainId), DBPrefix::contracts + contractAddress.get());
    }

    /**
     * Constructor for loading from the database.
     * @param address The address where the contract will be deployed.
     * @param db Pointer to the DB instance.
     */
    BaseContract(const Address& address, const std::unique_ptr<DB>& db)
      : contractAddress(address), db(db)
    {
      this->contractName = db->get("contractName", DBPrefix::contracts + contractAddress.get());
      this->contractCreator = Address(
        db->get("contractCreator", DBPrefix::contracts + contractAddress.get()), true
      );
      this->contractChainId = Utils::bytesToUint64(
        db->get("contractChainId", DBPrefix::contracts + contractAddress.get())
      );
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
    virtual const std::string ethCallView(const ethCallInfo& data) const {
      throw std::runtime_error("Derived Class from Contract does not override ethCall()");
    }

    /// Getter for `contractAddress`.
    const Address& getContractAddress() const { return this->contractAddress; }

    /// Getter for `contractCreator`.
    const Address& getContractCreator() const { return this->contractCreator; }

    /// Getter for `contractChainId`.
    const uint64_t& getContractChainId() const { return this->contractChainId; }

    /// Getter for `contractName`.
    const std::string& getContractName() const { return this->contractName; }
};

#endif // CONTRACT_H
