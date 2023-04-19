#ifndef DYNAMICCONTRACT_H
#define DYNAMICCONTRACT_H

#include "contract.h"
#include "contractmanager.h"

/**
 * Native abstraction of a smart contract.
 * All contracts have to inherit this class.
 */
class DynamicContract : public BaseContract {
  private:
    std::unique_ptr<uint256_t> _balance;
    ContractManager::ContractManagerInterface &interface;

    std::unordered_map<std::string, std::function<void(const ethCallInfo& callInfo)>> functions;
    std::unordered_map<std::string, std::function<void(const ethCallInfo& callInfo)>> payableFunctions;
    std::unordered_map<std::string, std::function<std::string(const ethCallInfo& callInfo)>> viewFunctions;

    std::vector<std::reference_wrapper<SafeBase>> usedVariables;
    void registerVariableUse(SafeBase& variable) { usedVariables.emplace_back(variable); }


  protected:
    /// TODO: eth_call from another contract.
    /// Register by using function name + lambda.
    void registerFunction(const std::string& functor, std::function<void(const ethCallInfo& tx)> f) {
      functions[functor] = f;
    }

    void registerPayableFunction(const std::string& functor, std::function<void(const ethCallInfo& tx)> f) {
      payableFunctions[functor] = f;
    }

    void registerViewFunction(const std::string& functor, std::function<std::string(const ethCallInfo& str)> f) {
      viewFunctions[functor] = f;
    }

    virtual void registerContractFunctions() { throw std::runtime_error("Derived Class from Contract does not override registerContractFunctions()"); }

    /// Updates the variables that were used by the contract
    /// Called by ethCall functions and contract constructors.
    /// Flag is set by ContractManager, except for when throwing.
    void updateState(const bool commitToState) {
      if (commitToState) {
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
    DynamicContract(ContractManager::ContractManagerInterface &interface, const std::string& contractName,
                    const Address& address, const Address& creator, const uint64_t& chainId, const std::unique_ptr<DB> &db
    ) : BaseContract(contractName, address, creator, chainId, db), interface(interface) {}

    /**
     * Constructor.
     * @param db
     */
    DynamicContract(ContractManager::ContractManagerInterface &interface, const Address& address, const std::unique_ptr<DB> &db) : BaseContract(address, db), interface(interface) {}

    /**
     * Invoke a contract function using a transaction. automatically differs between payable and non-payable functions.
     * Used by the %State class when calling `processNewBlock()/validateNewBlock()`.
     * @param tx The transaction to use for call.
     * @param commit Whether to commit the changes to the SafeVariables or just simulate the transaction.
     * ContractManager is responsible to set the commit flag (simulate or truly commit variables to state).
     */
    void ethCall(const ethCallInfo& callInfo) override {
      try {
        std::string funcName = std::get<5>(callInfo).substr(0, 4);
        if (this->isPayableFunction(funcName)) {
          auto func = this->payableFunctions.find(funcName);
          if (func == this->payableFunctions.end()) {
            throw std::runtime_error("Functor not found");
          }
          func->second(callInfo);
        } else {
          auto func = this->functions.find(funcName);
          if (func == this->functions.end()) {
            throw std::runtime_error("Functor not found");
          }
          func->second(callInfo);
        }
      } catch (const std::exception& e) {
        updateState(false);
        throw e;
      }
      updateState(this->getCommit());
    };

    /**
     * Do a contract call to a view function
     * @param data
     * @return
     */
    const std::string ethCallView(const ethCallInfo& data) const override {
      try {
        std::string funcName = std::get<5>(data).substr(0, 4);
        auto func = this->viewFunctions.find(funcName);
        if (func == this->viewFunctions.end()) {
          throw std::runtime_error("Functor not found");
        }
        return func->second(data);
      } catch (std::exception &e) {
        throw e;
      }
    }

    /**
     * Check if a functor is registered as a payable function
     */

    bool isPayableFunction(const std::string& functor) const {
      return this->payableFunctions.find(functor) != this->payableFunctions.end();
    }

    /// Tries to cast a contract to a specific type, only const functions can be called on the casted contract
    template <typename T>
    const T* getContract(const Address& address) const {
      return interface.getContract<T>(address);
    }

    void callContract(const Address& address, const ABI::Encoder &encoder, const uint256_t& callValue = 0) {
      ethCallInfo callInfo;
      auto& [from, to, gas, gasPrice, value, data] = callInfo;
      from = this->getContractAddress();
      to = address;
      gas = 0;
      gasPrice = 0;
      value = callValue;
      data = encoder.getRaw();
      interface.callContract(callInfo);
    }


    void sendTokens(const Address& to, const uint256_t& amount) {
      interface.sendTokens(this->getContractAddress(), to, amount);
    }

    friend void registerVariableUse(DynamicContract& contract, SafeBase& variable);
};

#endif //DYNAMICCONTRACT_H