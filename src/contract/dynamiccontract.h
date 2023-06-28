#ifndef DYNAMICCONTRACT_H
#define DYNAMICCONTRACT_H

#include <any>
#include "contract.h"
#include "contractmanager.h"
#include "../utils/safehash.h"

/**
 * Template for a smart contract.
 * All contracts have to inherit this class.
 */
class DynamicContract : public BaseContract {
  private:
    /// Reference to the contract manager interface.
    ContractManagerInterface& interface;

    /**
    * Variant type for the possible return types of a non-payable/payable function.
    * The return type can be a single value or a vector of values.
    */
    typedef std::variant<Byte, Bytes, uint16_t, std::vector<uint16_t>, uint32_t, std::vector<uint32_t>,
      uint64_t, std::vector<uint64_t>, uint256_t, std::vector<uint256_t>, Address, std::vector<Address>,
      bool, std::vector<bool>, std::string, std::vector<std::string>> ReturnType;

    /**
    * Map of non-payable functions that can be called by the contract.
    * The key is the function signature (first 4 hex bytes of keccak).
    * The value is a function that takes a vector of bytes (the arguments) and returns a ReturnType.
    */
    std::unordered_map<
    Functor, std::function<ReturnType(const ethCallInfo& callInfo)>, SafeHash
  > publicFunctions;

  /**
  * Map of payable functions that can be called by the contract.
  * The key is the function signature (first 4 hex bytes of keccak).
  * The value is a function that takes a vector of bytes (the arguments) and returns a ReturnType.
  */
  std::unordered_map<
    Functor, std::function<ReturnType(const ethCallInfo& callInfo)>, SafeHash
  > payableFunctions;

    /**
    * Map of view functions that can be called by the contract.
    * The key is the function signature (first 4 hex bytes of keccak).
    * The value is a function that takes a vector of bytes (the arguments) and returns a ReturnType.
    */
    std::unordered_map<
      Functor, std::function<Bytes(const ethCallInfo& callInfo)>, SafeHash
    > viewFunctions;

    /// Vector of variables that were used by the contract.
    std::vector<std::reference_wrapper<SafeBase>> usedVariables;

    /**
     * Register a callable function (a function that is called by a transaction),
     * adding it to the callable functions map.
     * @param functor Solidity function signature (first 4 hex bytes of keccak).
     * @param f Function to be called.
     */
    void registerFunction(const Functor& functor,
                         std::function<ReturnType(const ethCallInfo& tx)> f) {
      publicFunctions[functor] = f;
    }

    /**
     * Register a variable that was used by the contract.
     * @param variable Reference to the variable.
     */
    inline void registerVariableUse(SafeBase& variable) { usedVariables.emplace_back(variable); }

  protected:
    /**
     * Helper function for registering a payable/non-payable function.
     * @tparam R Return type of the function.
     * @tparam T Class type.
     * @tparam MemFunc Member function type.
     */
    template <typename R, typename T>
    struct RegisterHelper {
        template <typename MemFunc>
        /**
        * Create a ReturnType object by calling the member function.
        * @param instance Pointer to the instance of the class.
        * @param memFunc Pointer to the member function.
        * @return A ReturnType object.
        */
        static ReturnType createReturnType(T* instance, MemFunc memFunc) {
            return ReturnType((instance->*memFunc)());
        }
    };

    /**
    * Helper function for registering a void payable/non-payable function.
    * @tparam T Class type.
    * @tparam MemFunc Member function type.
    */
    template <typename T>
    struct RegisterHelper<void, T> {
        template <typename MemFunc>
        /**
        * Create a ReturnType object by calling the member function.
        * @param instance Pointer to the instance of the class.
        * @param memFunc Pointer to the member function.
        * @return A ReturnType object (default constructed).
        */
        static ReturnType createReturnType(T* instance, MemFunc memFunc) {
            (instance->*memFunc)(); // Call the function even though it's void
            // Handle void case here: return a default ReturnType, or throw an exception, etc.
            return ReturnType{}; // I don't know the best way to handle this
        }
    };

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

      std::string functStr = funcSignature + "()";

      const std::unordered_map<std::string, std::function<void()>> mutabilityActions = {
        {"view", [this, functStr, instance, memFunc, funcSignature]() {
          this->registerViewFunction(Utils::sha3(Utils::create_view_span(functStr)).view_const(0, 4), [instance, memFunc](const ethCallInfo &callInfo) -> Bytes {
            return (instance->*memFunc)();
          });
        }},
        {"nonpayable", [this, functStr, instance, memFunc, funcSignature]() {
          this->registerFunction(Utils::sha3(Utils::create_view_span(functStr)).view_const(0, 4), [instance, memFunc](const ethCallInfo &callInfo) -> ReturnType {
            return ReturnType{(instance->*memFunc)()};
          });
        }},
        {"payable", [this, functStr, instance, memFunc, funcSignature]() {
          this->registerPayableFunction(Utils::sha3(Utils::create_view_span(functStr)).view_const(0, 4), [instance, memFunc](const ethCallInfo &callInfo) -> ReturnType {
            return ReturnType{(instance->*memFunc)()};
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
     * Template for registering a non-const member function with no arguments.
     * @param funcSignature Solidity function signature.
     * @param memFunc Pointer to the member function.
     * @param instance Pointer to the instance of the class.
     */
    template <typename R, typename T> void registerMemberFunction(
      const std::string& funcSignature, R(T::*memFunc)(), T* instance
    ) {
      bool hasArgs = ContractReflectionInterface::methodHasArguments<decltype(*instance)>(funcSignature);
      std::string methodMutability = ContractReflectionInterface::getMethodMutability<decltype(*instance)>(funcSignature);
      if (hasArgs) throw std::runtime_error("Invalid function signature.");

      std::string functStr = funcSignature + "()";

      const std::unordered_map<std::string, std::function<void()>> mutabilityActions = {
        {"view", []() { throw std::runtime_error("View must be const because it does not modify the state."); }},
        {"nonpayable", [this, functStr, instance, memFunc, funcSignature]() {
          this->registerFunction(Utils::sha3(Utils::create_view_span(functStr)).view_const(0, 4), [instance, memFunc](const ethCallInfo &callInfo) -> ReturnType {
             return RegisterHelper<R, T>::createReturnType(instance, memFunc);
          });
        }},
        {"payable", [this, functStr, instance, memFunc, funcSignature]() {
          this->registerPayableFunction(Utils::sha3(Utils::create_view_span(functStr)).view_const(0, 4), [instance, memFunc](const ethCallInfo &callInfo) -> ReturnType {
             return RegisterHelper<R, T>::createReturnType(instance, memFunc);
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
     * @tparam T Type of the class.
     * @tparam R Return type of the function.
     * @tparam Args Argument types of the function.
     * @tparam Is Indices of the arguments.
     * @param instance Pointer to the instance of the class.
     * @param memFunc Pointer to the member function.
     * @param dataVec Vector of arguments.
     * @return Return value of the function.
     */
    template <typename T, typename R, typename... Args, std::size_t... Is> auto tryCallFuncWithTuple(
      T* instance, R(T::*memFunc)(Args...), const std::vector<std::any>& dataVec, std::index_sequence<Is...>
    ) -> std::conditional_t<std::is_void<R>::value, ReturnType, R> {
      if (sizeof...(Args) > dataVec.size()) {
        throw std::runtime_error(
          "Not enough arguments provided for function. Expected: " +
          std::to_string(sizeof...(Args)) + ", Actual: " + std::to_string(dataVec.size())
        );
      }

      try {
        if constexpr (!std::is_void<R>::value) {
          // If R is not void, just return the result as before
          return (instance->*memFunc)(std::any_cast<Args>(dataVec[Is])...);
        } else {
          // If R is void, perform the function call but then return an empty variant
          (instance->*memFunc)(std::any_cast<Args>(dataVec[Is])...);
          return ReturnType{};
        }
      } catch (const std::bad_any_cast& ex) {
        std::string errorMessage = "Mismatched argument types. Attempted casting failed with: ";
        ((errorMessage += (
          "\nAttempted to cast to type: " + std::string(typeid(Args).name()) + ", Actual type: " +
          (dataVec[Is].has_value() ? std::string(dataVec[Is].type().name()) : "Empty any")
        )), ...);
        throw std::runtime_error(errorMessage);
      }
    }

    /**
     * Template helper function for calling a const member function with no arguments.
     * @tparam T Type of the class.
     * @tparam R Return type of the function.
     * @tparam Args Argument types of the function.
     * @tparam Is Index sequence for the arguments.
     * @param instance Pointer to the instance of the class.
     * @param memFunc Pointer to the member function.
     * @param dataVec Vector of anys containing the arguments.
     * @return The return value of the function.
     */
    template <typename T, typename R, typename... Args, std::size_t... Is> auto tryCallFuncWithTuple(
      T* instance, R(T::*memFunc)(Args...) const, const std::vector<std::any>& dataVec, std::index_sequence<Is...>
    ) {
        if (sizeof...(Args) != dataVec.size()) throw std::runtime_error(
          "Not enough arguments provided for function. Expected: " +
          std::to_string(sizeof...(Args)) + ", Actual: " + std::to_string(dataVec.size())
        );
        try {
          return (instance->*memFunc)(std::any_cast<Args>(dataVec[Is])...);
        } catch (const std::bad_any_cast& ex) {
          std::string errorMessage = "Mismatched argument types. Attempted casting failed with: ";
          ((errorMessage += (
            "\nAttempted to cast to type: " + std::string(typeid(Args).name()) + ", Actual type: " +
            (dataVec[Is].has_value() ? std::string(dataVec[Is].type().name()) : "Empty any")
          )), ...);
          throw std::runtime_error(errorMessage);
        }
    }

    /**
     * Template for registering a non-const member function with arguments.
     * @param funcSignature Solidity function signature.
     * @param memFunc Pointer to the member function.
     * @param instance Pointer to the instance of the class.
     */
    template <typename R, typename... Args, typename T> void registerMemberFunction(
      const std::string& funcSignature, R(T::*memFunc)(Args...), T* instance
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
        if (types.size() != sizeof...(Args)) throw std::runtime_error(
          "Mismatched argument types in function " + funcSignature + ". Expected: " +
          std::to_string(sizeof...(Args)) + ", Actual: " + std::to_string(types.size())
        );
        ABI::Decoder decoder(types, std::get<6>(callInfo));
        std::vector<std::any> dataVector;
        for (size_t i = 0; i < types.size(); i++) dataVector.push_back(decoder.getDataDispatch(i, types[i]));
        auto result = tryCallFuncWithTuple(instance, memFunc, dataVector, std::index_sequence_for<Args...>());
        return ReturnType(result);
      };

      if (methodMutability == "view") {
        throw std::runtime_error("View must be const because it does not modify the state.");
      } else if (methodMutability == "nonpayable") {
        this->registerFunction(Utils::sha3(Utils::create_view_span(fullSignature)).view_const(0, 4), registrationFunc);
      } else if (methodMutability == "payable") {
        this->registerPayableFunction(Utils::sha3(Utils::create_view_span(fullSignature)).view_const(0, 4), registrationFunc);
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
        ABI::Decoder decoder(types, std::get<6>(callInfo));
        std::vector<std::any> dataVector;
        for (size_t i = 0; i < types.size(); i++) dataVector.push_back(decoder.getDataDispatch(i, types[i]));
        auto result = tryCallFuncWithTuple(instance, memFunc, dataVector, std::index_sequence_for<Args...>());
        return ReturnType(result);
      };

      auto viewRegistrationFunc = [this, instance, memFunc, funcSignature](const ethCallInfo &callInfo) -> Bytes {
        std::vector<ABI::Types> types = ContractReflectionInterface::getMethodArgumentsTypesABI<decltype(*instance)>(funcSignature);
        ABI::Decoder decoder(types, std::get<6>(callInfo));
        std::vector<std::any> dataVector;
        for (size_t i = 0; i < types.size(); i++) dataVector.push_back(decoder.getDataDispatch(i, types[i]));
        auto result = tryCallFuncWithTuple(instance, memFunc, dataVector, std::index_sequence_for<Args...>());
        return result;
      };

      if (methodMutability == "view") {
        this->registerViewFunction(Utils::sha3(Utils::create_view_span(fullSignature)).view_const(0, 4), viewRegistrationFunc);
      } else if (methodMutability == "nonpayable") {
        this->registerFunction(Utils::sha3(Utils::create_view_span(fullSignature)).view_const(0, 4), registrationFunc);
      } else if (methodMutability == "payable") {
        this->registerPayableFunction(Utils::sha3(Utils::create_view_span(fullSignature)).view_const(0, 4), registrationFunc);
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
    const Functor& functor,
          std::function<ReturnType(const ethCallInfo& tx)> f) {
    payableFunctions[functor] = f;
  }

    /**
     * Register a view/const function, adding it to the view functions map.
     * @param functor Solidity function signature (first 4 hex bytes of keccak).
     * @param f Function to be called.
     */
    void registerViewFunction(
      const Functor& functor, std::function<Bytes(const ethCallInfo& str)> f
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
      ContractManagerInterface& interface,
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
      ContractManagerInterface& interface,
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
        Functor funcName = std::get<5>(callInfo);
        if (this->isPayableFunction(funcName)) {
          auto func = this->payableFunctions.find(funcName);
          if (func == this->payableFunctions.end()) throw std::runtime_error("Functor not found");
          func->second(callInfo);
        } else {
          auto func = this->publicFunctions.find(funcName);
          if (func == this->publicFunctions.end()) throw std::runtime_error("Functor not found");
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
    const Bytes ethCallView(const ethCallInfo& data) const override {
      try {
        Functor funcName = std::get<5>(data);
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
    bool isPayableFunction(const Functor& functor) const {
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
    * Try to cast a contract to a specific type (non-const).
    * @tparam T The type to cast to.
    * @param address The address of the contract to cast.
    * @return A pointer to the casted contract.
    */
    template <typename T> T* getContract(const Address& address) {
      return interface.getContract<T>(address);
    }

    /**
    * Call a contract view function based on the basic requirements of a contract call.
    * @tparam R The return type of the view function.
    * @tparam C The contract type.
    * @tparam Args The argument types of the view function.
    * @param address The address of the contract to call.
    * @param func The view function to call.
    * @param args The arguments to pass to the view function.
    * @return The result of the view function.
    */
    template <typename R, typename C, typename... Args>
    R callContractViewFunction(const Address& address, R(C::*func)(const Args&...) const, const Args&... args) const {
        const C* contract = this->getContract<C>(address);
        return (contract->*func)(std::forward<const Args&>(args)...);
    }

    /**
    * Call a contract view function based on the basic requirements of a contract call.
    * @tparam R The return type of the view function.
    * @tparam C The contract type.
    * @param address The address of the contract to call.
    * @param func The view function to call.
    * @return The result of the view function.
    */
    template <typename R, typename C>
    R callContractViewFunction(const Address& address, R(C::*func)() const) const {
        const C* contract = this->getContract<C>(address);
        return (contract->*func)();
    }

    /**
    * Call a contract function (non-view) based on the basic requirements of a contract call.
    * @tparam R The return type of the function.
    * @tparam C The contract type.
    * @tparam Args The argument types of the function.
    * @param targetAddr The address of the contract to call.
    * @param func The function to call.
    * @param args The arguments to pass to the function.
    * @return The result of the function.
    */
    template <typename R, typename C, typename... Args>
    R callContractFunction(const Address& targetAddr, R(C::*func)(const Args&...), const Args&... args) {
        return this->interface.callContractFunction(this->getContractAddress(),
                                                    targetAddr, 
                                                    0, 
                                                    func, 
                                                    std::forward<const Args&>(args)...);
    }

    /**
    * Call a contract function (non-view) based on the basic requirements of a contract call with the value flag
    * @tparam R The return type of the function.
    * @tparam C The contract type.
    * @tparam Args The argument types of the function.
    * @param value Flag to send value with the call.
    * @param address The address of the contract to call.
    * @param func The function to call.
    * @param args The arguments to pass to the function.
    * @return The result of the function.
    */
    template <typename R, typename C, typename... Args>
    R callContractFunction(const uint256_t& value, const Address& address, R(C::*func)(const Args&...), const Args&... args) {
        return this->interface.callContractFunction(this->getContractAddress(),
                                                    address, 
                                                    value, 
                                                    func, 
                                                    std::forward<const Args&>(args)...);
    }

    /**
    * Call a contract function (non-view) based on the basic requirements of a contract call with no arguments
    * @tparam R The return type of the function.
    * @tparam C The contract type.
    * @param targetAddr The address of the contract to call.
    * @param func The function to call.
    * @return The result of the function.
    */
    template <typename R, typename C>
    R callContractFunction(const Address& targetAddr, R(C::*func)()) {
        return this->interface.callContractFunction(this->getContractAddress(),
                                                    targetAddr, 
                                                    0, 
                                                    func);
    }

    /**
    * Call a contract function (non-view) based on the basic requirements of a contract call with the value flag and no arguments
    * @tparam R The return type of the function.
    * @tparam C The contract type.
    * @param value Flag to send value with the call.
    * @param address The address of the contract to call.
    * @param func The function to call.
    * @return The result of the function.
    */
    template <typename R, typename C>
    R callContractFunction(const uint256_t& value, const Address& address, R(C::*func)()) {
        return this->interface.callContractFunction(this->getContractAddress(),
                                                    address, 
                                                    value, 
                                                    func);
    }

    /**
    * Wrapper for calling a contract function (non-view) based on the basic requirements of a contract call.
    * @tparam R The return type of the function.
    * @tparam C The contract type.
    * @tparam Args The argument types of the function.
    * @param func The function to call.
    * @param args The arguments to pass to the function.
    * @return The result of the function.
    */
    template <typename R, typename C, typename... Args>
    R callContractFunction(R (C::*func)(const Args&...), const Args&... args) {
      struct UpdateState {
        C* contract;
        UpdateState(C* contract) : contract(contract) {}
        ~UpdateState() { contract->updateState(contract->getCommit()); }
      } updateState(static_cast<C*>(this));
      
      try {
        return (static_cast<C*>(this)->*func)(std::forward<const Args&>(args)...);
      } catch (const std::exception& e) {
        this->updateState(false);
        throw std::runtime_error(e.what());
      }
    }

    /**
    * Wrapper for calling a contract function (non-view) based on the basic requirements of a contract call with no arguments
    * @tparam R The return type of the function.
    * @tparam C The contract type.
    * @tparam Args The argument types of the function.
    * @param func The function to call.
    * @return The result of the function.
    */
    template <typename R, typename C>
    R callContractFunction(R (C::*func)()) {
      struct UpdateState {
        C* contract;
        UpdateState(C* contract) : contract(contract) {}
        ~UpdateState() { contract->updateState(contract->getCommit()); }
      } updateState(static_cast<C*>(this));
      
      try {
        return (static_cast<C*>(this)->*func)();
      } catch (const std::exception& e) {
        this->updateState(false);
        throw std::runtime_error(e.what());
      }
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
