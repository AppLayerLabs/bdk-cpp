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

  protected:
    const Address& getOrigin() const { return this->origin; }
    const Address& getCaller() const { return this->caller; }
    const uint256_t& getValue() const { return this->value; }

    friend class ContractManager; /// ContractManager updates the contract locals before calling ethCall within a contract.
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
     * Invoke a contract "payable" function using a transaction
     * Used by the State/ContractManager class when calling `processNewBlock()`
     * Should be overriden by derived classes.
     * @param tx the Transaction to use for call
     * @param account Reference back to the account within the State class.
     * @param commit Whether to commit the changes to the SafeVariables or just simulate the transaction
     */
    virtual void ethCall(const TxBlock& tx, const uint256_t& txValue) { throw std::runtime_error("Derived Class from Contract does not override ethCall()"); }

    /**
     * Invoke a contract function using a transaction.
     * Should be overriden by derived classes.
     * @param tx The transaction to use for call.
     * @param commit Whether to commit the changes to the SafeVariables or just simulate the transaction.
     */
    virtual void ethCall(const TxBlock& tx) { throw std::runtime_error("Derived Class from Contract does not override ethCall()"); }

    /**
     * Invoke a contract function and simulate it.
     * Should be overriden by derived classes.
     * @param callInfo callInfo: tuple of (from, to, gasLimit, gasPrice, value, data)
     */
    virtual void ethCall(const ethCallInfo& callInfo) { throw std::runtime_error("Derived Class from Contract does not override ethCall()"); }

    /**
     * Do a contract call to a view function
     * Should be overriden by derived classes.
     * @param data
     * @return
     */
    virtual const std::string ethCall(const std::string& data) const { throw std::runtime_error("Derived Class from Contract does not override ethCall()"); }

    const Address& getContractAddress() const { return this->contractAddress; }
    const Address& getContractCreator() const { return this->contractCreator; }
    const uint64_t& getContractChainId() const { return this->contractChainId; }
    const std::string& getContractName() const { return this->contractName; }
};

/**
 * Native abstraction of a smart contract.
 * All contracts have to inherit this class.
 */
class DynamicContract : public BaseContract {
  private:
    std::unordered_map<std::string, std::function<void(const TxBlock& tx)>> functions;
    std::unordered_map<std::string, std::function<void(const TxBlock& tx)>> payableFunctions;
    std::unordered_map<std::string, std::function<std::string(const std::string& str)>> viewFunctions;

    std::vector<std::reference_wrapper<SafeBase>> usedVariables;
    void registerVariableUse(SafeBase& variable) { usedVariables.emplace_back(variable); }

  protected:
    /// TODO: eth_call from another contract.
    /// Register by using function name + lambda.
    void registerFunction(const std::string& functor, std::function<void(const TxBlock& tx)> f) {
      functions[functor] = f;
    }

    void registerPayableFunction(const std::string& functor, std::function<void(const TxBlock& tx)> f) {
      payableFunctions[functor] = f;
    }

    void registerViewFunction(const std::string& functor, std::function<std::string(const std::string& str)> f) {
      viewFunctions[functor] = f;
    }

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
    DynamicContract(const std::string& contractName,
      const Address& address, const Address& creator, const uint64_t& chainId, const std::unique_ptr<DB> &db
    ) : BaseContract(contractName, address, creator, chainId, db) {}

    /**
     * Constructor.
     * @param db
     */
    DynamicContract(const Address& address, const std::unique_ptr<DB> &db) : BaseContract(address, db) {}

    /**
     * Invoke a contract "payable" function using a transaction
     * Used by the State class when calling `processNewBlock()/validateNewBlock()`
     * Can be overridden by child class if they need need to process the function call in a non-standard way (See ContractManager for example)
     * @param tx the Transaction to use for call
     * @param account Reference back to the account within the State class.
     * @param commit Whether to commit the changes to the SafeVariables or just simulate the transaction
     */
     void ethCall(const TxBlock& tx, const uint256_t& txValue) override {
       try {
         std::string funcName = tx.getData().substr(0, 4);
         auto func = this->payableFunctions.find(funcName);
         if (func == this->payableFunctions.end()) {
           throw std::runtime_error("Functor not found");
         }
         func->second(tx);
       } catch (const std::exception& e) {
         updateState(false);
         throw e;
       }
       updateState(true);
     };

    /**
     * Invoke a contract function using a transaction.
     * Used by the %State class when calling `processNewBlock()/validateNewBlock()`.
     * @param tx The transaction to use for call.
     * @param commit Whether to commit the changes to the SafeVariables or just simulate the transaction.
     */
    void ethCall(const TxBlock& tx) override {
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
      updateState(true);
    };

    /**
     * Invoke a contract function and simulate it.
     * @param callInfo callInfo: tuple of (from, to, gasLimit, gasPrice, value, data)
     */
    void ethCall(const ethCallInfo& callInfo) override {
      auto [from, to, gasLimit, gasPrice, value, data] = callInfo;
      PrivKey mockupPrivKey(Hex::toBytes("2a616d189193e56994f22993ac4eb4dca0e2652afdc95d240739837ab83b21e2"));
      Address mockupAddr = Secp256k1::toAddress(Secp256k1::toUPub(mockupPrivKey));
      TxBlock tx(
          this->getContractAddress(),
          mockupAddr,
          data,
          this->getContractChainId(),
          0,
          value,
          gasPrice,
          gasPrice,
          gasLimit,
          mockupPrivKey
        );

      try {
        std::string funcName = tx.getData().substr(0, 4);
        if (value) {
          auto it = this->payableFunctions.find(funcName);
          if (it == this->payableFunctions.end()) {
            throw std::runtime_error("Functor not found");
          }
          it->second(tx);
        } else {
          auto it = this->functions.find(funcName);
          if (it == this->functions.end()) {
            throw std::runtime_error("Functor not found");
          }
          it->second(tx);
        }
      } catch (const std::exception& e) {
        updateState(false);
        throw e;
      }
      updateState(false);
    }

    /**
     * Do a contract call to a view function
     * @param data
     * @return
     */
    const std::string ethCall(const std::string& data) const override {
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

    /// Friend
    friend void registerVariableUse(DynamicContract& contract, SafeBase& variable);
};

#endif  // CONTRACT_H
