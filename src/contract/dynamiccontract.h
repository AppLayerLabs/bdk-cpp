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
  ContractManager::ContractManagerInterface
      &interface; ///< Reference to the contract manager interface.

  std::unordered_map<std::string,
                     std::function<void(const ethCallInfo &callInfo)>>
      functions; ///< Map of functions that can be called by the contract.
  std::unordered_map<std::string,
                     std::function<void(const ethCallInfo &callInfo)>>
      payableFunctions; ///< Map of payable functions that can be called by the
                        ///< contract.
  std::unordered_map<std::string,
                     std::function<std::string(const ethCallInfo &callInfo)>>
      viewFunctions; ///< Map of view/const functions that can be called by the
                     ///< contract.

  std::vector<std::reference_wrapper<SafeBase>>
      usedVariables; ///< Vector of variables that were used by the contract.

  /**
   * Register a variable that was used by the contract.
   * @param variable Reference to the variable.
   */
  void registerVariableUse(SafeBase &variable) {
    usedVariables.emplace_back(variable);
  }

protected:
  /// TODO: eth_call from another contract.

  /**
   * Register a callable function (a function that is called by a transaction),
   * adding it to the callable functions map.
   * @param functor Solidity function signature.
   * @param f Function to be called.
   */
  void registerFunction(const std::string &functor,
                        std::function<void(const ethCallInfo &tx)> f) {
    functions[functor] = f;
  }

  /**
   * Register a callable and payable function, adding it to the payable
   * functions map.
   * @param functor Solidity function signature.
   * @param f Function to be called.
   */
  void registerPayableFunction(const std::string &functor,
                               std::function<void(const ethCallInfo &tx)> f) {
    payableFunctions[functor] = f;
  }

  /**
   * Register a view/const function, adding it to the view functions map.
   * @param functor Name of the function.
   * @param f Function to be called.
   */
  void
  registerViewFunction(const std::string &functor,
                       std::function<std::string(const ethCallInfo &str)> f) {
    viewFunctions[functor] = f;
  }

  /**
   * Template function for calling the register functions (registerFunction,
   * registerPayableFunction, registerViewFunction). Should be called by the
   * derived class.
   */
  virtual void registerContractFunctions() {
    throw std::runtime_error("Derived Class from Contract does not override "
                             "registerContractFunctions()");
  }

  /**
   * Updates the variables that were used by the contract.
   * Called by ethCall functions and contract constructors.
   * Flag is set by ContractManager, except for when throwing.
   * @param commitToState Whether to commit the changes to the SafeVariables or
   * just simulate the transaction. ContractManager is responsible to set the
   * commit flag (simulate or truly commit variables to state).
   */
  void updateState(const bool commitToState) {
    if (commitToState) {
      for (auto &variable : usedVariables) {
        variable.get().commit();
      }
    } else {
      for (auto &variable : usedVariables) {
        variable.get().revert();
      }
    }
    usedVariables.clear();
  }

public:
  /**
   * Constructor.
   * @param interface Reference to the contract manager interface.
   * @param contractName The name of the contract.
   * @param address The address where the contract will be deployed.
   * @param creator The address of the creator of the contract.
   * @param chainId The chain where the contract wil be deployed.
   * @param db Reference to the database object.
   */
  DynamicContract(ContractManager::ContractManagerInterface &interface,
                  const std::string &contractName, const Address &address,
                  const Address &creator, const uint64_t &chainId,
                  const std::unique_ptr<DB> &db)
      : BaseContract(contractName, address, creator, chainId, db),
        interface(interface) {}

  /**
   * Constructor.
   * @param interface Reference to the contract manager interface.
   * @param address The address where the contract will be deployed.
   * @param db Reference to the database object.
   */
  DynamicContract(ContractManager::ContractManagerInterface &interface,
                  const Address &address, const std::unique_ptr<DB> &db)
      : BaseContract(address, db), interface(interface) {}

  /**
   * Invoke a contract function using a  tuple of (from, to, gasLimit, gasPrice,
   * value, data). Automatically differs
   * between payable and non-payable functions. Used by the %State class when
   * calling `processNewBlock()/validateNewBlock()`.
   * @param callInfo Tuple of (from, to, gasLimit, gasPrice, value, data).
   */
  void ethCall(const ethCallInfo &callInfo) override {
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
    } catch (const std::exception &e) {
      updateState(false);
      throw std::runtime_error(e.what());
    }
    updateState(this->getCommit());
  };

  /**
   * Do a contract call to a view function
   * @param data Tuple of (from, to, gasLimit, gasPrice, value, data).
   * @return The result of the view function. If the function is not found,
   * throws an exception.
   */
  const std::string ethCallView(const ethCallInfo &data) const override {
    try {
      std::string funcName = std::get<5>(data).substr(0, 4);
      auto func = this->viewFunctions.find(funcName);
      if (func == this->viewFunctions.end()) {
        throw std::runtime_error("Functor not found");
      }
      return func->second(data);
    } catch (std::exception &e) {
      throw std::runtime_error(e.what());
    }
  }

  /**
   * Check if a functor is registered as a payable function
   * @param functor The functor to check.
   * @return True if the functor is registered as a payable function, false
   * otherwise.
   */

  bool isPayableFunction(const std::string &functor) const {
    return this->payableFunctions.find(functor) != this->payableFunctions.end();
  }

  /**
   * Tries to cast a contract to a specific type.
   * NOTE: Only const functions can be called on the casted contract.
   * @tparam T The type to cast to.
   * @param address The address of the contract to cast.
   * @return A pointer to the casted contract.
   */
  template <typename T> const T *getContract(const Address &address) const {
    return interface.getContract<T>(address);
  }

  /**
   * Calls a contract based on the basic requirements of a contract call.
   * @param address The address of the contract to call.
   * @param encoder The ABI encoder.
   * @param callValue The value to send with the call.
   */
  void callContract(const Address &address, const ABI::Encoder &encoder,
                    const uint256_t &callValue = 0) {
    ethCallInfo callInfo;
    auto &[from, to, gas, gasPrice, value, data] = callInfo;
    from = this->getContractAddress();
    to = address;
    gas = 0;
    gasPrice = 0;
    value = callValue;
    data = encoder.getRaw();
    interface.callContract(callInfo);
  }

  /**
   * Getter for the balance of a contract.
   * @param address The address of the contract.
   * @return The balance of the contract.
   */
  uint256_t getBalance(const Address &address) const {
    return interface.getBalanceFromAddress(address);
  }

  /**
   * Sends tokens from the contract to another address.
   * @param to The address to send the tokens to.
   * @param amount The amount of tokens to send.
   */
  void sendTokens(const Address &to, const uint256_t &amount) {
    interface.sendTokens(this->getContractAddress(), to, amount);
  }

  /**
   * Registers a variable as used by the contract.
   * @param contract The contract that uses the variable.
   * @param variable The variable that is used.
   */
  friend void registerVariableUse(DynamicContract &contract,
                                  SafeBase &variable);
};

#endif // DYNAMICCONTRACT_H