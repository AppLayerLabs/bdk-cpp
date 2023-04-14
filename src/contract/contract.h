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

/**
 * Native abstraction of a smart contract.
 * All contracts have to inherit this class.
 */
class Contract : public ContractGlobals {
  private:
    /* Contract-specific variables */
    std::string contractName;                                  ///< Name of the contract, used to identify the Contract Class.
    Address contractAddress;                                   ///< Address where the contract is deployed.
    Address contractCreator;                                   ///< Address of the creator of the contract.
    uint64_t contractChainId;                                  ///< Chain where the contract is deployed.

    std::unordered_map<std::string, std::function<void(const TxBlock& tx)>> functions;
    std::unordered_map<std::string, std::function<void(const TxBlock& tx, Account& account)>> payableFunctions;
    std::unordered_map<std::string, std::function<std::string(const std::string& str)>> viewFunctions;

    mutable Address origin;          /// Who called the contract
    mutable Address caller;          /// Who sent the tx

    std::vector<std::reference_wrapper<SafeBase>> usedVariables;
    void registerVariableUse(SafeBase& variable) { usedVariables.emplace_back(variable); }
  protected:
    /* Contract-specific variables */
    const std::unique_ptr<DB> &db;                           ///< Reference to the DB where the contract stored deployed.

    /// TODO: eth_call from another contract.
    /// Register by using function name + lambda.
    void registerFunction(const std::string& functor, std::function<void(const TxBlock& tx)> f) {
      functions[functor] = f;
    }

    void registerPayableFunction(const std::string& functor, std::function<void(const TxBlock& tx, Account& account)> f) {
      payableFunctions[functor] = f;
    }

    void registerViewFunction(const std::string& functor, std::function<std::string(const std::string& str)> f) {
      viewFunctions[functor] = f;
    }

    const Address& getOrigin() const { return this->origin; }
    const Address& getCaller() const { return this->caller; }

    virtual void registerContractFunctions() { throw std::runtime_error("Derived Class from Contract does not override registerContractFunctions()"); }

    /// Updates the variables that were used by the contract
    /// Called by ethCall functions and contract constructors.
    void updateState(bool commit) {
      if (commit) {
        for (auto& variable : usedVariables) {
          variable.get().commit();
        }
      } else {
        for (auto& variable : usedVariables) {
          variable.get().revert();
        }
      }
      usedVariables.clear();
    }

  public:
    /**
     * Constructor.
     * @param address The address where the contract will be deployed.
     * @param chainId The chain where the contract wil be deployed.
     * @param contractManager Pointer to the contract manager.
     */
    Contract(const std::string& contractName,
      const Address& address, const Address& creator, const uint64_t& chainId, const std::unique_ptr<DB> &db
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
     Contract(const Address& address, const std::unique_ptr<DB> &db) : contractAddress(address), db(db) {
       this->contractName = db->get("contractName", DBPrefix::contracts + contractAddress.get());
       this->contractCreator = Address(db->get("contractCreator", DBPrefix::contracts + contractAddress.get()), true);
       this->contractChainId = Utils::bytesToUint64(db->get("contractChainId", DBPrefix::contracts + contractAddress.get()));
     }

    ~Contract() {}
    /**
     * Invoke a contract "payable" function using a transaction
     * Used by the State class when calling `processNewBlock()/validateNewBlock()`
     * Can be overridden by child class if they need need to process the function call in a non-standard way (See ContractManager for example)
     * @param tx the Transaction to use for call
     * @param account Reference back to the account within the State class.
     * @param commit Whether to commit the changes to the SafeVariables or just simulate the transaction
     */
     virtual void ethCall(const TxBlock& tx, Account& account, bool commit = false) {
       this->caller = tx.getFrom();
       this->origin = tx.getFrom();
       try {
         std::string funcName = tx.getData().substr(0, 4);
         auto func = this->payableFunctions.find(funcName);
         if (func == this->payableFunctions.end()) {
           throw std::runtime_error("Functor not found");
         }
         func->second(tx, account);
       } catch (const std::exception& e) {
         updateState(false);
         throw e;
       }
      updateState(commit);
     };

    /**
     * Invoke a contract function using a transaction.
     * Used by the %State class when calling `processNewBlock()/validateNewBlock()`.
     * @param tx The transaction to use for call.
     * @param commit Whether to commit the changes to the SafeVariables or just simulate the transaction.
     */
    virtual void ethCall(const TxBlock& tx, bool commit = false) {
      this->caller = tx.getFrom();
      this->origin = tx.getFrom();
      try {
        std::string funcName = tx.getData().substr(0, 4);
        auto func = this->functions.find(funcName);
        if (func == this->functions.end()) {
          throw std::runtime_error("Functor not found");
        }
        func->second(tx);
      } catch (const std::exception& e) {
        updateState(false);
        throw e;
      }
      updateState(commit);
    };

    /**
     * Invoke a const (solidity view) contract function using a data string.
     * Used by RPC for answering `eth_call`.
     * @param data The data string to use for call.
     * @return An encoded %Solidity hex string with the desired function result.
     */
    virtual const std::string ethCall(const std::string& data) const {
      this->origin = Address();
      this->caller = Address();
      try {
        std::string funcName = data.substr(0, 4);
        auto func = this->viewFunctions.find(funcName);
        if (func == this->viewFunctions.end()) {
          throw std::runtime_error("Functor not found");
        }
        return func->second(data);
      } catch (std::exception &e) {
        throw e;
      }
    }

    /// Getters
    const std::string& getContractName() const { return this->contractName; }
    const Address& getContractAddress() const { return this->contractAddress; }
    const Address& getContractCreator() const { return this->contractCreator; }
    const uint64_t& getContractChainId() const { return this->contractChainId; }

    /// Friend
    friend void registerVariableUse(Contract& contract, SafeBase& variable);
};

#endif  // CONTRACT_H
