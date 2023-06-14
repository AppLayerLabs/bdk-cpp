#ifndef DYNAMICCONTRACT_H
#define DYNAMICCONTRACT_H

#include "contract.h"
#include "contractmanager.h"
#include "../utils/contractreflectioninterface.h"

/**
 * Template for a smart contract.
 * All contracts have to inherit this class.
 */
class DynamicContract : public BaseContract {
private:
  ContractManagerInterface
      &interface; ///< Reference to the contract manager interface.

    /// Map of functions that can be called by the contract.
    std::unordered_map<
      std::string, std::function<void(const ethCallInfo& callInfo)>
    > functions;

    /// Map of payable functions that can be called by the contract.
    std::unordered_map<
      std::string, std::function<void(const ethCallInfo& callInfo)>
    > payableFunctions;

    /// Map of view/const functions that can be called by the contract.
    std::unordered_map<
      std::string, std::function<std::string(const ethCallInfo& callInfo)>
    > viewFunctions;

    /// Vector of variables that were used by the contract.
    std::vector<std::reference_wrapper<SafeBase>> usedVariables;

    /**
     * Register a callable function (a function that is called by a transaction),
     * adding it to the callable functions map.
     * @param functor Solidity function signature (first 4 hex bytes of keccak).
     * @param f Function to be called.
     */
    void registerFunction(
      const std::string& functor, std::function<void(const ethCallInfo& tx)> f
    ) {
      functions[functor] = f;
    }

    /**
     * Register a variable that was used by the contract.
     * @param variable Reference to the variable.
     */
    inline void registerVariableUse(SafeBase& variable) { usedVariables.emplace_back(variable); }

  protected:
    /**
     * Template for registering a const member function with no arguments.
     * @param funcSignature Solidity function signature.
     * @param memFunc Pointer to the member function.
     * @param instance Pointer to the instance of the class.
     */
    template <typename R, typename T> void registerMemberFunction(
      const std::string& funcSignature, R(T::*memFunc)() const, T* instance
    ) {
      bool hasArgs = ContractReflectionInterface::methodHasArguments<decltype(*instance)>(funcSignature);
      std::string methodMutability = ContractReflectionInterface::getMethodMutability<decltype(*instance)>(funcSignature);
      if (hasArgs) throw std::runtime_error("Invalid function signature.");

      const std::unordered_map<std::string, std::function<void()>> mutabilityActions = {
        {"view", [this, instance, memFunc, funcSignature]() {
          this->registerViewFunction(Utils::sha3(funcSignature + "()").get().substr(0, 4), [instance, memFunc](const ethCallInfo &callInfo) {
            return (instance->*memFunc)();
          });
        }},
        {"nonpayable", [this, instance, memFunc, funcSignature]() {
          this->registerFunction(Utils::sha3(funcSignature + "()").get().substr(0, 4), [instance, memFunc](const ethCallInfo &callInfo) {
            return (instance->*memFunc)();
          });
        }},
        {"payable", [this, instance, memFunc, funcSignature]() {
          this->registerPayableFunction(Utils::sha3(funcSignature + "()").get().substr(0, 4), [instance, memFunc](const ethCallInfo &callInfo) {
            return (instance->*memFunc)();
          });
        }}
      };

    if (hasArgs) {
      throw std::runtime_error("Invalid function signature.");
    }

    const std::unordered_map<std::string, std::function<void()>> mutabilityActions = {
      {"view", []() { throw std::runtime_error("View must be const because it does not modify the state."); }},
      {"nonpayable", [this, instance, memFunc, funcSignature]() {
        this->registerFunction(Utils::sha3(funcSignature + "()").get().substr(0, 4), [instance, memFunc](const ethCallInfo &callInfo) {
          return (instance->*memFunc)();
        });
      }},
      {"payable", [this, instance, memFunc, funcSignature]() {
        this->registerPayableFunction(Utils::sha3(funcSignature + "()").get().substr(0, 4), [instance, memFunc](const ethCallInfo &callInfo) {
          return (instance->*memFunc)();
        });
      }}
    };

    auto actionIt = mutabilityActions.find(methodMutability);
    if (actionIt != mutabilityActions.end()) {
      actionIt->second();
    } else {
      throw std::runtime_error("Invalid function signature.");
    }
  }

  /**
   * Template helper function for calling a non-const member function with no arguments.
    * @param instance Pointer to the instance of the class.
    * @param memFunc Pointer to the member function.
    * @param dataVec Vector of anys containing the arguments.
    * @param Is Index sequence for the arguments.
    * @return The return value of the function.
    */
  template <typename T, typename R, typename... Args, std::size_t... Is>
  auto tryCallFuncWithTuple(T* instance, R(T::*memFunc)(Args...),
                            const std::vector<std::any>& dataVec,
                            std::index_sequence<Is...>) {
    if (sizeof...(Args) > dataVec.size()) {
      throw std::runtime_error("Not enough arguments provided for function. Expected: " +
                              std::to_string(sizeof...(Args)) + ", Actual: " + std::to_string(dataVec.size()));
    }

    try {
      return (instance->*memFunc)(std::any_cast<Args>(dataVec[Is])...);
    } catch (const std::bad_any_cast& ex) {
      std::string errorMessage = "Mismatched argument types. Attempted casting failed with: ";
      ((errorMessage += ("\nAttempted to cast to type: " + std::string(typeid(Args).name()) +
                        ", Actual type: " +
                        (dataVec[Is].has_value() ? std::string(dataVec[Is].type().name()) : "Empty any"))), ...);
      throw std::runtime_error(errorMessage);
    }
  }

  /**
   * Template helper function for calling a const member function with no arguments.
   * @param instance Pointer to the instance of the class.
   * @param memFunc Pointer to the member function.
   * @param dataVec Vector of anys containing the arguments.
   * @param Is Index sequence for the arguments.
   * @return The return value of the function.
   */
  template <typename T, typename R, typename... Args, std::size_t... Is>
  auto tryCallFuncWithTuple(T* instance, R(T::*memFunc)(Args...) const, 
                            const std::vector<std::any>& dataVec,
                            std::index_sequence<Is...>) {
      if (sizeof...(Args) != dataVec.size()) {
          throw std::runtime_error("Not enough arguments provided for function. Expected: " +
                                  std::to_string(sizeof...(Args)) + ", Actual: " + std::to_string(dataVec.size()));
      }
      try {
          return (instance->*memFunc)(std::any_cast<Args>(dataVec[Is])...);
      } catch (const std::bad_any_cast& ex) {
          std::string errorMessage = "Mismatched argument types. Attempted casting failed with: ";
          ((errorMessage += ("\nAttempted to cast to type: " + std::string(typeid(Args).name()) +
                            ", Actual type: " +
                            (dataVec[Is].has_value() ? std::string(dataVec[Is].type().name()) : "Empty any"))), ...);
          throw std::runtime_error(errorMessage);
      }
  }


  /**
   * Template for registering a non-const member function with arguments.
   * @param funcSignature Solidity function signature.
   * @param memFunc Pointer to the member function.
   * @param instance Pointer to the instance of the class.
   */
  template <typename R, typename... Args, typename T>
  void registerMemberFunction(const std::string& funcSignature, R(T::*memFunc)(Args...), T* instance) {
    std::vector<std::string> args = ContractReflectionInterface::getMethodArgumentsTypesString<decltype(*instance)>(funcSignature);
    std::string methodMutability = ContractReflectionInterface::getMethodMutability<decltype(*instance)>(funcSignature);
    std::ostringstream fullSignatureStream;
    fullSignatureStream << funcSignature << "(";
    if (!args.empty()) {
      std::copy(args.begin(), args.end() - 1, std::ostream_iterator<std::string>(fullSignatureStream, ","));
      fullSignatureStream << args.back();
    }
    fullSignatureStream << ")";

    std::string fullSignature = fullSignatureStream.str();

    auto registrationFunc = [this, instance, memFunc, funcSignature](const ethCallInfo &callInfo) {
      std::vector<ABI::Types> types = ContractReflectionInterface::getMethodArgumentsTypesABI<decltype(*instance)>(funcSignature);
      if (types.size() != sizeof...(Args)) {
        throw std::runtime_error("Mismatched argument types in function " + funcSignature + ". Expected: " +
                                 std::to_string(sizeof...(Args)) + ", Actual: " + std::to_string(types.size()));
      }
      ABI::Decoder decoder(types, std::get<5>(callInfo).substr(4));
      std::vector<std::any> dataVector;
      for (size_t i = 0; i < types.size(); i++) {
        dataVector.push_back(decoder.getDataDispatch(i, types[i]));
      }
      return tryCallFuncWithTuple(instance, memFunc, dataVector, std::index_sequence_for<Args...>());
    };

    if (methodMutability == "view") {
      throw std::runtime_error("View must be const because it does not modify the state.");
    } else if (methodMutability == "nonpayable") {
      this->registerFunction(Utils::sha3(fullSignature).get().substr(0, 4), registrationFunc);
    } else if (methodMutability == "payable") {
      this->registerPayableFunction(Utils::sha3(fullSignature).get().substr(0, 4), registrationFunc);
    } else {
      throw std::runtime_error("Invalid function signature.");
    }
  }

  /**
   * Template for registering a const member function with arguments.
   * @param funcSignature Solidity function signature.
   * @param memFunc Pointer to the member function.
   * @param instance Pointer to the instance of the class.
   */
  template <typename R, typename... Args, typename T>
  void registerMemberFunction(const std::string& funcSignature, R(T::*memFunc)(Args...) const, T* instance) {
    std::vector<std::string> args = ContractReflectionInterface::getMethodArgumentsTypesString<decltype(*instance)>(funcSignature);
    std::string methodMutability = ContractReflectionInterface::getMethodMutability<decltype(*instance)>(funcSignature);
    std::ostringstream fullSignatureStream;
    fullSignatureStream << funcSignature << "(";
    if (!args.empty()) {
      std::copy(args.begin(), args.end() - 1, std::ostream_iterator<std::string>(fullSignatureStream, ","));
      fullSignatureStream << args.back();
    }
    fullSignatureStream << ")";

    std::string fullSignature = fullSignatureStream.str();

    auto registrationFunc = [this, instance, memFunc, funcSignature](const ethCallInfo &callInfo) {
      std::vector<ABI::Types> types = ContractReflectionInterface::getMethodArgumentsTypesABI<decltype(*instance)>(funcSignature);
      ABI::Decoder decoder(types, std::get<5>(callInfo).substr(4));
      std::vector<std::any> dataVector;
      for (size_t i = 0; i < types.size(); i++) {
        dataVector.push_back(decoder.getDataDispatch(i, types[i]));
      }
      return tryCallFuncWithTuple(instance, memFunc, dataVector, std::index_sequence_for<Args...>());
    };

    if (methodMutability == "view") {
      this->registerViewFunction(Utils::sha3(fullSignature).get().substr(0, 4), registrationFunc);
    } else if (methodMutability == "nonpayable") {
      this->registerFunction(Utils::sha3(fullSignature).get().substr(0, 4), registrationFunc);
    } else if (methodMutability == "payable") {
      this->registerPayableFunction(Utils::sha3(fullSignature).get().substr(0, 4), registrationFunc);
    } else {
      throw std::runtime_error("Invalid function signature.");
    }
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
  DynamicContract(ContractManagerInterface &interface,
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
  DynamicContract(ContractManagerInterface &interface, const Address &address,
  const std::unique_ptr<DB> &db): BaseContract(address, db), interface(interface) {}

  /**
   * Invoke a contract function using a  tuple of (from, to, gasLimit, gasPrice,
   * value, data). Automatically differs
   * between payable and non-payable functions. Used by the %State class when
   * calling `processNewBlock()/validateNewBlock()`.
   * @param callInfo Tuple of (from, to, gasLimit, gasPrice, value, data).
   * @throws std::runtime_error if the functor is not found.
   * @throws std::runtime_error if the function throws an exception.
   */
  void ethCall(const ethCallInfo &callInfo) override {
    try {
      std::string funcName = std::get<5>(callInfo).substr(0, 4);
      if (this->isPayableFunction(funcName)) {
        auto func = this->payableFunctions.find(funcName);
        if (func == this->payableFunctions.end()) {
          throw std::runtime_error("Functor not found for payable function");
        }
        func->second(callInfo);
      } else {
        auto func = this->functions.find(funcName);
        if (func == this->functions.end()) {
          throw std::runtime_error("Functor not found for non-payable function");
        }
    }

  /**
   * Do a contract call to a view function
   * @param data Tuple of (from, to, gasLimit, gasPrice, value, data).
   * @return The result of the view function. If the function is not found,
   * throws an exception.
   * @throws std::runtime_error if the functor is not found.
   * @throws std::runtime_error if the function throws an exception.
   */
  const std::string ethCallView(const ethCallInfo &data) const override {
    try {
      std::string funcName = std::get<5>(data).substr(0, 4);
      auto func = this->viewFunctions.find(funcName);
      if (func == this->viewFunctions.end()) {
        throw std::runtime_error("Functor not found for view function");
      }
    }

    /**
     * Template for registering a const member function with arguments.
     * @param funcSignature Solidity function signature.
     * @param memFunc Pointer to the member function.
     * @param instance Pointer to the instance of the class.
     */
    template <typename R, typename... Args, typename T> void registerMemberFunction(
      const std::string& funcSignature, R(T::*memFunc)(Args...) const, T* instance
    ) {
      std::vector<std::string> args = ContractReflectionInterface::getMethodArgumentsTypesString<decltype(*instance)>(funcSignature);
      std::string methodMutability = ContractReflectionInterface::getMethodMutability<decltype(*instance)>(funcSignature);
      std::ostringstream fullSignatureStream;
      fullSignatureStream << funcSignature << "(";
      if (!args.empty()) {
        std::copy(args.begin(), args.end() - 1, std::ostream_iterator<std::string>(fullSignatureStream, ","));
        fullSignatureStream << args.back();
      }
      fullSignatureStream << ")";

      std::string fullSignature = fullSignatureStream.str();

      auto registrationFunc = [this, instance, memFunc, funcSignature](const ethCallInfo &callInfo) {
        std::vector<ABI::Types> types = ContractReflectionInterface::getMethodArgumentsTypesABI<decltype(*instance)>(funcSignature);
        ABI::Decoder decoder(types, std::get<5>(callInfo).substr(4));
        std::vector<std::any> dataVector;
        for (size_t i = 0; i < types.size(); i++) dataVector.push_back(decoder.getDataDispatch(i, types[i]));
        return tryCallFuncWithTuple(instance, memFunc, dataVector, std::index_sequence_for<Args...>());
      };

      if (methodMutability == "view") {
        this->registerViewFunction(Utils::sha3(fullSignature).get().substr(0, 4), registrationFunc);
      } else if (methodMutability == "nonpayable") {
        this->registerFunction(Utils::sha3(fullSignature).get().substr(0, 4), registrationFunc);
      } else if (methodMutability == "payable") {
        this->registerPayableFunction(Utils::sha3(fullSignature).get().substr(0, 4), registrationFunc);
      } else {
        throw std::runtime_error("Invalid function signature.");
      }
    }

    /**
     * Register a callable and payable function, adding it to the payable functions map.
     * @param functor Solidity function signature (first 4 hex bytes of keccak).
     * @param f Function to be called.
     */
    void registerPayableFunction(
      const std::string& functor, std::function<void(const ethCallInfo& tx)> f
    ) {
      payableFunctions[functor] = f;
    }

    /**
     * Register a view/const function, adding it to the view functions map.
     * @param functor Solidity function signature (first 4 hex bytes of keccak).
     * @param f Function to be called.
     */
    void registerViewFunction(
      const std::string& functor, std::function<std::string(const ethCallInfo& str)> f
    ) {
      viewFunctions[functor] = f;
    }

    /**
     * Template function for calling the register functions.
     * Should be called by the derived class.
     * @throw std::runtime_error if the derived class does not override this.
     */
    virtual void registerContractFunctions() {
      throw std::runtime_error(
        "Derived Class from Contract does not override registerContractFunctions()"
      );
    }

    /**
     * Update the variables that were used by the contract.
     * Called by ethCall functions and contract constructors.
     * Flag is set by ContractManager, except for when throwing.
     * @param commitToState If `true`, commits the changes made to SafeVariables to the state.
     *                      If `false`, just simulates the transaction.
     *                      ContractManager is responsible for setting this.
     */
    void updateState(const bool commitToState) {
      for (auto& var : usedVariables) {
        if (commitToState) var.get().commit(); else var.get().revert();
      }
      usedVariables.clear();
    }

  public:
    /**
     * Constructor for creating the contract from scratch.
     * @param interface Reference to the contract manager interface.
     * @param contractName The name of the contract.
     * @param address The address where the contract will be deployed.
     * @param creator The address of the creator of the contract.
     * @param chainId The chain where the contract wil be deployed.
     * @param db Reference to the database object.
     */
    DynamicContract(
      ContractManager::ContractManagerInterface& interface,
      const std::string& contractName, const Address& address,
      const Address& creator, const uint64_t& chainId,
      const std::unique_ptr<DB>& db
    ) : BaseContract(contractName, address, creator, chainId, db), interface(interface) {}

    /**
     * Constructor for loading the contract from the database.
     * @param interface Reference to the contract manager interface.
     * @param address The address where the contract will be deployed.
     * @param db Reference to the database object.
     */
    DynamicContract(
      ContractManager::ContractManagerInterface& interface,
      const Address& address, const std::unique_ptr<DB>& db
    ) : BaseContract(address, db), interface(interface) {}

    /**
     * Invoke a contract function using a tuple of (from, to, gasLimit, gasPrice, value, data).
     * Automatically differs between payable and non-payable functions.
     * Used by State when calling `processNewBlock()/validateNewBlock()`.
     * @param callInfo Tuple of (from, to, gasLimit, gasPrice, value, data).
     * @throw std::runtime_error if the functor is not found or the function throws an exception.
     */
    void ethCall(const ethCallInfo& callInfo) override {
      try {
        std::string funcName = std::get<5>(callInfo).substr(0, 4);
        if (this->isPayableFunction(funcName)) {
          auto func = this->payableFunctions.find(funcName);
          if (func == this->payableFunctions.end()) throw std::runtime_error("Functor not found");
          func->second(callInfo);
        } else {
          auto func = this->functions.find(funcName);
          if (func == this->functions.end()) throw std::runtime_error("Functor not found");
          func->second(callInfo);
        }
      } catch (const std::exception& e) {
        updateState(false);
        throw std::runtime_error(e.what());
      }
      updateState(this->getCommit());
    };

    /**
     * Do a contract call to a view function.
     * @param data Tuple of (from, to, gasLimit, gasPrice, value, data).
     * @return The result of the view function.
     * @throw std::runtime_error if the functor is not found or the function throws an exception.
     */
    const std::string ethCallView(const ethCallInfo& data) const override {
      try {
        std::string funcName = std::get<5>(data).substr(0, 4);
        auto func = this->viewFunctions.find(funcName);
        if (func == this->viewFunctions.end()) throw std::runtime_error("Functor not found");
        return func->second(data);
      } catch (std::exception& e) {
        throw std::runtime_error(e.what());
      }
    }

    /**
     * Check if a functor is registered as a payable function.
     * @param functor The functor to check.
     * @return `true` if the functor is registered as a payable function, `false` otherwise.
     */
    bool isPayableFunction(const std::string& functor) const {
      return this->payableFunctions.find(functor) != this->payableFunctions.end();
    }

    /**
     * Try to cast a contract to a specific type.
     * NOTE: Only const functions can be called on the casted contract.
     * @tparam T The type to cast to.
     * @param address The address of the contract to cast.
     * @return A pointer to the casted contract.
     */
    template <typename T> const T *getContract(const Address& address) const {
      return interface.getContract<T>(address);
    }

    /**
     * Call a contract based on the basic requirements of a contract call.
     * @param address The address of the contract to call.
     * @param encoder The ABI encoder.
     * @param callValue The value to send with the call.
     */
    void callContract(
      const Address& address, const ABI::Encoder& encoder, const uint256_t& callValue = 0
    ) {
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

    /**
     * Get the balance of a contract.
     * @param address The address of the contract.
     * @return The balance of the contract.
     */
    uint256_t getBalance(const Address& address) const {
      return interface.getBalanceFromAddress(address);
    }

    /**
     * Send an amount of tokens from the contract to another address.
     * @param to The address to send the tokens to.
     * @param amount The amount of tokens to send.
     */
    void sendTokens(const Address& to, const uint256_t& amount) {
      interface.sendTokens(this->getContractAddress(), to, amount);
    }

    /**
     * Register a variable as used by the contract.
     * @param contract The contract that uses the variable.
     * @param variable The variable that is used.
     */
    friend void registerVariableUse(DynamicContract& contract, SafeBase& variable);
};

#endif // DYNAMICCONTRACT_H
