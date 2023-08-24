#ifndef DYNAMICCONTRACT_H
#define DYNAMICCONTRACT_H

#include <any>
#include "contract.h"
#include "contractmanager.h"
#include "../utils/safehash.h"
#include "../utils/utils.h"

/**
 * Template for a smart contract.
 * All contracts have to inherit this class.
 */
class DynamicContract : public BaseContract {
  private:
   /**
    * Map of non-payable functions that can be called by the contract.
    * The key is the function signature (first 4 hex bytes of keccak).
    * The value is a function that takes a vector of bytes (the arguments) and returns a ReturnType.
    */
    std::unordered_map<
      Functor, std::function<BaseTypes(const ethCallInfo& callInfo)>, SafeHash
    > publicFunctions;

   /**
    * Map of payable functions that can be called by the contract.
    * The key is the function signature (first 4 hex bytes of keccak).
    * The value is a function that takes a vector of bytes (the arguments) and returns a ReturnType.
    */
    std::unordered_map<
      Functor, std::function<BaseTypes(const ethCallInfo& callInfo)>, SafeHash
    > payableFunctions;

   /**
    * Map of view functions that can be called by the contract.
    * The key is the function signature (first 4 hex bytes of keccak).
    * The value is a function that takes a vector of bytes (the arguments) and returns a ReturnType.
    */
    std::unordered_map<
      Functor, std::function<BaseTypes(const ethCallInfo& callInfo)>, SafeHash
    > viewFunctions;

    /**
     * Register a callable function (a function that is called by a transaction),
     * adding it to the callable functions map.
     * @param functor Solidity function signature (first 4 hex bytes of keccak).
     * @param f Function to be called.
     */
    void registerFunction(
      const Functor& functor, std::function<BaseTypes(const ethCallInfo& tx)> f
    ) {
      publicFunctions[functor] = f;
    }

    /**
     * Register a variable that was used by the contract.
     * @param variable Reference to the variable.
     */
    inline void registerVariableUse(SafeBase& variable) { interface.registerVariableUse(variable); }

  protected:
    /// Reference to the contract manager interface.
    ContractManagerInterface& interface;

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
        static BaseTypes createReturnType(T* instance, MemFunc memFunc) {
          return BaseTypes((instance->*memFunc)());
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
      * TODO: Decide if this is the best way to handle void functions.
      */
      static BaseTypes createReturnType(T* instance, MemFunc memFunc) {
          (instance->*memFunc)();
          return BaseTypes{};
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
          this->registerViewFunction(Utils::sha3(Utils::create_view_span(functStr)).view_const(0, 4), [instance, memFunc](const ethCallInfo &callInfo) -> BaseTypes {
            using ReturnType = decltype((instance->*memFunc)());
            if constexpr (Utils::isRangedUint<ReturnType, 256>::value){
              return BaseTypes{static_cast<uint256_t>((instance->*memFunc)())};
            } else if constexpr (Utils::isRangedInt<ReturnType, 256>::value){
              return BaseTypes{static_cast<int256_t>((instance->*memFunc)())};
            } else {
              return BaseTypes{(instance->*memFunc)()};
            }
          });
        }},
        {"nonpayable", [this, functStr, instance, memFunc, funcSignature]() {
          this->registerFunction(Utils::sha3(Utils::create_view_span(functStr)).view_const(0, 4), [instance, memFunc](const ethCallInfo &callInfo) -> BaseTypes {
            using ReturnType = decltype((instance->*memFunc)());
            if constexpr (Utils::isRangedUint<ReturnType, 256>::value){
              return BaseTypes{static_cast<uint256_t>((instance->*memFunc)())};
            } else if constexpr (Utils::isRangedInt<ReturnType, 256>::value){
              return BaseTypes{static_cast<int256_t>((instance->*memFunc)())};
            } else {
              return BaseTypes{(instance->*memFunc)()};
            }
          });
        }},
        {"payable", [this, functStr, instance, memFunc, funcSignature]() {
          this->registerPayableFunction(Utils::sha3(Utils::create_view_span(functStr)).view_const(0, 4), [instance, memFunc](const ethCallInfo &callInfo) -> BaseTypes {
            using ReturnType = decltype((instance->*memFunc)());
            if constexpr (Utils::isRangedUint<ReturnType, 256>::value){
              return BaseTypes{static_cast<uint256_t>((instance->*memFunc)())};
            } else if constexpr (Utils::isRangedInt<ReturnType, 256>::value){
              return BaseTypes{static_cast<int256_t>((instance->*memFunc)())};
            } else {
              return BaseTypes{(instance->*memFunc)()};
            }
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
          this->registerFunction(Utils::sha3(Utils::create_view_span(functStr)).view_const(0, 4), [instance, memFunc](const ethCallInfo &callInfo) -> BaseTypes {
             return RegisterHelper<R, T>::createReturnType(instance, memFunc);
          });
        }},
        {"payable", [this, functStr, instance, memFunc, funcSignature]() {
          this->registerPayableFunction(Utils::sha3(Utils::create_view_span(functStr)).view_const(0, 4), [instance, memFunc](const ethCallInfo &callInfo) -> BaseTypes {
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
    ) -> std::conditional_t<std::is_void<R>::value, BaseTypes, R> {
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
          return BaseTypes{};
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

      for (size_t i = 0; i < types.size(); i++) {
        if (ABI::castUintFunctions.count(types[i]) > 0) {
          uint256_t value = std::any_cast<uint256_t>(decoder.getDataDispatch(i, types[i]));
          dataVector.push_back(ABI::castUintFunctions[types[i]](value));
        }
        else if (ABI::castIntFunctions.count(types[i]) > 0) {
          int256_t value = std::any_cast<int256_t>(decoder.getDataDispatch(i, types[i]));
          dataVector.push_back(ABI::castIntFunctions[types[i]](value));
        }
         else {
          dataVector.push_back(decoder.getDataDispatch(i, types[i]));
        }
      }
        auto result = tryCallFuncWithTuple(instance, memFunc, dataVector, std::index_sequence_for<Args...>());
        return BaseTypes(result);
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

      auto registrationFunc = [this, instance, memFunc, funcSignature](const ethCallInfo &callInfo) -> BaseTypes {
        std::vector<ABI::Types> types = ContractReflectionInterface::getMethodArgumentsTypesABI<decltype(*instance)>(funcSignature);
        ABI::Decoder decoder(types, std::get<6>(callInfo));
        std::vector<std::any> dataVector;
      for (size_t i = 0; i < types.size(); i++) {
        if (ABI::castUintFunctions.count(types[i]) > 0) {
          uint256_t value = std::any_cast<uint256_t>(decoder.getDataDispatch(i, types[i]));
          dataVector.push_back(ABI::castUintFunctions[types[i]](value));
        }
        else if (ABI::castIntFunctions.count(types[i]) > 0) {
          int256_t value = std::any_cast<int256_t>(decoder.getDataDispatch(i, types[i]));
          dataVector.push_back(ABI::castIntFunctions[types[i]](value));
        }
        else {
          dataVector.push_back(decoder.getDataDispatch(i, types[i]));
        }
      }
        auto result = tryCallFuncWithTuple(instance, memFunc, dataVector, std::index_sequence_for<Args...>());
        return BaseTypes(result);
      };

      if (methodMutability == "view") {
        this->registerViewFunction(Utils::sha3(Utils::create_view_span(fullSignature)).view_const(0, 4), registrationFunc);
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
          std::function<BaseTypes(const ethCallInfo& tx)> f) {
    payableFunctions[functor] = f;
  }

    /**
     * Register a view/const function, adding it to the view functions map.
     * @param functor Solidity function signature (first 4 hex bytes of keccak).
     * @param f Function to be called.
     */
    void registerViewFunction(
      const Functor& functor, std::function<BaseTypes(const ethCallInfo& str)> f
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
          if (func == this->payableFunctions.end()) throw std::runtime_error("Functor not found for payable function");
          func->second(callInfo);
        } else {
          // TODO: uncomment to see the horrors of a functor morphing shape every time tests are run
          //std::cout << "funcName: " << funcName.hex().get() << std::endl;
          //for (auto func : this->publicFunctions) {
          //  std::cout << "public: " << func.first.hex().get() << std::endl;
          //}
          auto func = this->publicFunctions.find(funcName);
          if (func == this->publicFunctions.end()) throw std::runtime_error("Functor not found for non-payable function");
          func->second(callInfo);
        }
      } catch (const std::exception& e) {
        throw std::runtime_error(e.what());
      }
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

        BaseTypes result = func->second(data);

        if (std::holds_alternative<BytesEncoded>(result)) {
          return std::get<BytesEncoded>(result).data;
        }
        else {
          ABI::Encoder::EncVar resultVec {result};
          return ABI::Encoder(resultVec).getData();
        }
          
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
        return this->interface.callContractFunction(this->getOrigin(),
                                                    this->getContractAddress(),
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
        return this->interface.callContractFunction(this->getOrigin(),
                                                    this->getContractAddress(),
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
        return this->interface.callContractFunction(this->getOrigin(),
                                                    this->getContractAddress(),
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
        return this->interface.callContractFunction(this->getOrigin(),
                                                    this->getContractAddress(),
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
      try {
        return (static_cast<C*>(this)->*func)(std::forward<const Args&>(args)...);
      } catch (const std::exception& e) {
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
      try {
        return (static_cast<C*>(this)->*func)();
      } catch (const std::exception& e) {
        throw std::runtime_error(e.what());
      }
    }

    /**
    * Call the create function of a contract.
    * @tparam TContract The contract type.
    * @tparam Args The arguments of the contract constructor.
    * @param gas The gas limit.
    * @param gasPrice The gas price.
    * @param value The caller value.
    * @param args The arguments to pass to the constructor.
    * @return The address of the created contract.
    */
    template<typename TContract, typename... Args>
    Address callCreateContract(const uint256_t &gas, const uint256_t &gasPrice, const uint256_t &value, Args&&... args) {
        Utils::safePrint("CallCreateContract being called...");
        ABI::Encoder::EncVar vars = {std::forward<Args>(args)...};
        ABI::Encoder encoder(vars);
        return this->interface.callCreateContract<TContract>(this->getOrigin(), this->getContractAddress(), gas, gasPrice, value, std::move(encoder));
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
