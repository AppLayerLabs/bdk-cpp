/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef DYNAMICCONTRACT_H
#define DYNAMICCONTRACT_H

#include "abi.h"
#include "contract.h"
#include "contracthost.h"
#include "event.h"
#include "../utils/safehash.h"
#include "../utils/utils.h"

/// Template for a smart contract. All contracts must inherit this class.
class DynamicContract : public BaseContract {
  private:
   /**
    * Map of non-payable functions that can be called by the contract.
    * The key is the function signature (first 4 hex bytes of keccak).
    * The value is a function that takes a vector of bytes (the arguments) and returns a ReturnType.
    */
    ankerl::unordered_dense::map<
      Functor, std::function<void(const evmc_message& callInfo)>, SafeHash
    > publicFunctions_;

   /**
    * Map of payable functions that can be called by the contract.
    * The key is the function signature (first 4 hex bytes of keccak).
    * The value is a function that takes a vector of bytes (the arguments) and returns a ReturnType.
    */
    ankerl::unordered_dense::map<
      Functor, std::function<void(const evmc_message& callInfo)>, SafeHash
    > payableFunctions_;

   /**
    * Map of view functions that can be called by the contract.
    * The key is the function signature (first 4 hex bytes of keccak).
    * The value is a function that takes a vector of bytes (the arguments) and returns a ReturnType.
    * Function return type is the encoded return value as viewFunctions is only used by eth_call.
    */
    ankerl::unordered_dense::map<
      Functor, std::function<Bytes(const evmc_message& callInfo)>, SafeHash
    > viewFunctions_;

    /**
     * Map for all the functions, regardless of mutability (Used by EVM)
     * The reason for having all these functions in a single mapping is because
     * EVM expects a contract call to actually return the ABI serialized data
     * So this map is exactly for that, it is a second representation of all the functions
     * stored in the previous 3 maps but with the ABI serialized data as the return value
     * We still check if we are calling a payable if the evmc_message has value.
     */
    ankerl::unordered_dense::map<
      Functor, std::function<Bytes(const evmc_message& callInfo)>, SafeHash
    > evmFunctions_;

    /**
     * Register a callable function (a function that is called by a transaction),
     * adding it to the callable functions map.
     * @param functor Solidity function signature (first 4 hex bytes of keccak).
     * @param f Function to be called.
     */
    void registerFunction(
      const Functor& functor, const std::function<void(const evmc_message& tx)>& f
    ) {
      publicFunctions_[functor] = f;
    }

    /**
     * Register a EVM callable function (a function that can be called by another EVM contract),
     * adding it to the evmFunctions_ map.
     * @param functor Solidity function signature (first 4 hex bytes of keccak).
     * @param f Function to be called.
     */
    void registerEVMFunction(
      const Functor& functor, const std::function<Bytes(const evmc_message& tx)>& f
    ) {
      evmFunctions_[functor] = f;
    }

    /**
     * Register a variable that was used by the contract.
     * @param variable Reference to the variable.
     */
    inline void registerVariableUse(SafeBase& variable) {
      if (this->host_ == nullptr) {
        throw DynamicException("Contracts going haywire! trying to register variable use without a host_!");
      }
      host_->registerVariableUse(variable);
    }

  protected:
    /**
     * Template for registering a const member function with no arguments.
     * @param funcSignature Solidity function signature.
     * @param memFunc Pointer to the member function.
     * @param methodMutability The mutability of the function.
     * @param instance Pointer to the instance of the class.
     */
    template <typename R, typename T> void registerMemberFunction(
      const std::string& funcSignature, R(T::*memFunc)() const, const FunctionTypes& methodMutability, T* instance
    ) {
      std::string functStr = funcSignature + "()";
      auto registerEvmFunction = [this, instance, memFunc](const evmc_message& callInfo) -> Bytes {
        if constexpr (std::is_same_v<R, void>) {
          // If the function's return type is void, return an empty Bytes object
          (instance->*memFunc)(); // Call the member function without capturing its return
          return Bytes();         // Return an empty Bytes object
        } else {
          // If the function's return type is not void, encode and return its result
          return ABI::Encoder::encodeData<R>((instance->*memFunc)());
        }
      };
      this->registerEVMFunction(Utils::makeFunctor(functStr), registerEvmFunction);
      switch (methodMutability) {
        case FunctionTypes::View: {
          this->registerViewFunction(Utils::makeFunctor(functStr), [instance, memFunc](const evmc_message&) -> Bytes {
            return ABI::Encoder::encodeData<R>((instance->*memFunc)());
          });
          break;
        }
        case FunctionTypes::NonPayable: {
          this->registerFunction(Utils::makeFunctor(functStr), [instance, memFunc](const evmc_message&) -> void {
            (instance->*memFunc)();
            return;
          });
          break;
        }
        case FunctionTypes::Payable: {
          this->registerPayableFunction(Utils::makeFunctor(functStr), [instance, memFunc](const evmc_message&) -> void {
            (instance->*memFunc)();
            return;
          });
          break;
        }
        default: {
          throw DynamicException("Invalid function signature.");
        }
      }
    }

    /**
     * Template for registering a non-const member function with no arguments.
     * @param funcSignature Solidity function signature.
     * @param memFunc Pointer to the member function.
     * @param methodMutability The mutability of the function.
     * @param instance Pointer to the instance of the class.
     */
    template <typename R, typename T> void registerMemberFunction(
      const std::string& funcSignature, R(T::*memFunc)(), const FunctionTypes& methodMutability, T* instance
    ) {
      std::string functStr = funcSignature + "()";
      auto registerEvmFunction = [this, instance, memFunc](const evmc_message& callInfo) -> Bytes {
        if constexpr (std::is_same_v<R, void>) {
          // If the function's return type is void, return an empty Bytes object
          (instance->*memFunc)(); // Call the member function without capturing its return
          return Bytes();         // Return an empty Bytes object
        } else {
          // If the function's return type is not void, encode and return its result
          return ABI::Encoder::encodeData<R>((instance->*memFunc)());
        }
      };
      this->registerEVMFunction(Utils::makeFunctor(functStr), registerEvmFunction);
      switch (methodMutability) {
        case FunctionTypes::View: {
          throw DynamicException("View must be const because it does not modify the state.");
        }
        case FunctionTypes::NonPayable: {
          this->registerFunction(
            Utils::makeFunctor(functStr),
            [instance, memFunc](const evmc_message&) -> void {
              (instance->*memFunc)();
              return;
            }
          );
          break;
        }
        case FunctionTypes::Payable: {
          this->registerPayableFunction(
            Utils::makeFunctor(functStr),
            [instance, memFunc](const evmc_message&) -> void {
              (instance->*memFunc)();
              return;
            }
          );
          break;
        }
        default: {
          throw DynamicException("Invalid function signature.");
        }
      }
    }

    /**
     * Template for registering a non-const member function with arguments.
     * @param funcSignature Solidity function signature.
     * @param memFunc Pointer to the member function.
     * @param methodMutability The mutability of the function.
     * @param instance Pointer to the instance of the class.
     */
    template <typename R, typename... Args, typename T> void registerMemberFunction(
      const std::string& funcSignature, R(T::*memFunc)(Args...), const FunctionTypes& methodMutability, T* instance
    ) {
      Functor functor = ABI::FunctorEncoder::encode<Args...>(funcSignature);
      auto registrationFunc = [this, instance, memFunc, funcSignature](const evmc_message &callInfo) {
        using DecayedArgsTuple = std::tuple<std::decay_t<Args>...>;
        DecayedArgsTuple decodedData = ABI::Decoder::decodeData<std::decay_t<Args>...>(Utils::getFunctionArgs(callInfo));
        std::apply([instance, memFunc](auto&&... args) {
            (instance->*memFunc)(std::forward<decltype(args)>(args)...);
        }, decodedData);
      };
      auto registrationFuncEVM = [this, instance, memFunc, funcSignature](const evmc_message &callInfo) -> Bytes {
        using DecayedArgsTuple = std::tuple<std::decay_t<Args>...>;
        DecayedArgsTuple decodedData = ABI::Decoder::decodeData<std::decay_t<Args>...>(Utils::getFunctionArgs(callInfo));
        if constexpr (std::is_same_v<R, void>) {
          std::apply([instance, memFunc](auto&&... args) {
              (instance->*memFunc)(std::forward<decltype(args)>(args)...);
          }, decodedData);
          return Bytes();
        } else {
          return std::apply([instance, memFunc](auto&&... args) -> Bytes {
            return ABI::Encoder::encodeData((instance->*memFunc)(std::forward<decltype(args)>(args)...));
          }, decodedData);
        }
      };
      this->registerEVMFunction(functor, registrationFuncEVM);
      switch (methodMutability) {
        case FunctionTypes::View:
          throw DynamicException("View must be const because it does not modify the state.");
        case FunctionTypes::NonPayable:
          this->registerFunction(functor, registrationFunc);
          break;
        case FunctionTypes::Payable:
          this->registerPayableFunction(functor, registrationFunc);
          break;
        default:
          throw DynamicException("Invalid function signature.");
      }
    }

    /**
     * Template for registering a const member function with arguments.
     * @param funcSignature Solidity function signature.
     * @param memFunc Pointer to the member function.
     * @param methodMutability The mutability of the function.
     * @param instance Pointer to the instance of the class.
     */
    template <typename R, typename... Args, typename T> void registerMemberFunction(
      const std::string& funcSignature, R(T::*memFunc)(Args...) const, const FunctionTypes& methodMutability, T* instance
    ) {
      Functor functor = ABI::FunctorEncoder::encode<Args...>(funcSignature);
      auto registrationFunc = [this, instance, memFunc, funcSignature](const evmc_message &callInfo) -> Bytes {
        using DecayedArgsTuple = std::tuple<std::decay_t<Args>...>;
        DecayedArgsTuple decodedData = ABI::Decoder::decodeData<std::decay_t<Args>...>(Utils::getFunctionArgs(callInfo));
        // Use std::apply to call the member function and encode its return value
        return std::apply([instance, memFunc](Args... args) -> Bytes {
          // Call the member function and return its encoded result
          return ABI::Encoder::encodeData((instance->*memFunc)(std::forward<decltype(args)>(args)...));
        }, decodedData);
      };
      auto registrationFuncEVM =  [this, instance, memFunc, funcSignature](const evmc_message &callInfo) -> Bytes {
        using DecayedArgsTuple = std::tuple<std::decay_t<Args>...>;
        DecayedArgsTuple decodedData = ABI::Decoder::decodeData<std::decay_t<Args>...>(Utils::getFunctionArgs(callInfo));
        if constexpr (std::is_same_v<R, void>) {
          // If the function's return type is void, call the member function and return an empty Bytes object
          std::apply([instance, memFunc](Args... args) {
            (instance->*memFunc)(std::forward<decltype(args)>(args)...);
          }, decodedData);
          return Bytes(); // Return an empty Bytes object
        } else {
          // If the function's return type is not void, call the member function and return its encoded result
          return std::apply([instance, memFunc](Args... args) -> Bytes {
            return ABI::Encoder::encodeData((instance->*memFunc)(std::forward<decltype(args)>(args)...));
          }, decodedData);
        }
      };
      this->registerEVMFunction(functor, registrationFuncEVM);
      switch (methodMutability) {
        case FunctionTypes::View:
          this->registerViewFunction(functor, registrationFunc);
          break;
        case FunctionTypes::NonPayable:
          this->registerFunction(functor, registrationFunc);
          break;
        case FunctionTypes::Payable:
          this->registerPayableFunction(functor, registrationFunc);
          break;
      }
    }

    /**
     * Register a callable and payable function, adding it to the payable functions map.
     * @param functor Solidity function signature (first 4 hex bytes of keccak).
     * @param f Function to be called.
     */
    void registerPayableFunction(
      const Functor& functor, const std::function<void(const evmc_message& tx)>& f
    ) {
      payableFunctions_[functor] = f;
    }

    /**
     * Register a view/const function, adding it to the view functions map.
     * @param functor Solidity function signature (first 4 hex bytes of keccak).
     * @param f Function to be called.
     */
    void registerViewFunction(
      const Functor& functor, const std::function<Bytes(const evmc_message& str)>& f
    ) {
      viewFunctions_[functor] = f;
    }

    /**
     * Template function for calling the register functions.
     * Should be called by the derived class.
     * @throw DynamicException if the derived class does not override this.
     */
    virtual void registerContractFunctions() {
      throw DynamicException("Derived Class from Contract does not override registerContractFunctions()");
    }

  public:
    /**
     * Constructor for creating the contract from scratch.
     * @param interface Reference to the contract manager interface.
     * @param contractName The name of the contract.
     * @param address The address where the contract will be deployed.
     * @param creator The address of the creator of the contract.
     * @param chainId The chain where the contract wil be deployed.
     */
    DynamicContract(const std::string& contractName,
      const Address& address, const Address& creator, const uint64_t& chainId
    ) : BaseContract(contractName, address, creator, chainId) {}

    /**
     * Constructor for loading the contract from the database.
     * @param interface Reference to the contract manager interface.
     * @param address The address where the contract will be deployed.
     * @param db Reference to the database object.
     */
    DynamicContract(const Address& address, const DB& db) : BaseContract(address, db) {};

    /**
     * Invoke a contract function using a tuple of (from, to, gasLimit, gasPrice, value, data).
     * Automatically differs between payable and non-payable functions.
     * Used by State when calling `processNewBlock()/validateNewBlock()`.
     * @param callInfo Tuple of (from, to, gasLimit, gasPrice, value, data).
     * @throw DynamicException if the functor is not found or the function throws an exception.
     */
    void ethCall(const evmc_message& callInfo, ContractHost* host) final {
      this->host_ = host;
      PointerNullifier nullifier(this->host_);
      try {
        Functor funcName = Utils::getFunctor(callInfo);
        if (this->isPayableFunction(funcName)) {
          auto func = this->payableFunctions_.find(funcName);
          if (func == this->payableFunctions_.end()) throw DynamicException("Functor not found for payable function");
          func->second(callInfo);
        } else {
          // value is a uint8_t[32] C array, we need to check if it's zero in modern C++
          if (!evmc::is_zero(callInfo.value)) {
            // If the value is not zero, we need to throw an exception
            throw DynamicException("Non-payable function called with value");
          }
          auto func = this->publicFunctions_.find(funcName);
          if (func == this->publicFunctions_.end()) throw DynamicException("Functor not found for non-payable function");
          func->second(callInfo);
        }
      } catch (const std::exception& e) {
        throw DynamicException(e.what());
      }
    };

    Bytes evmEthCall(const evmc_message& callInfo, ContractHost* host) final {
      this->host_ = host;
      PointerNullifier nullifier(this->host_);
      try {
        Functor funcName = Utils::getFunctor(callInfo);
        if (this->isPayableFunction(funcName)) {
          auto func = this->evmFunctions_.find(funcName);
          if (func == this->evmFunctions_.end()) throw DynamicException("Functor not found for payable function");
          return func->second(callInfo);
        } else {
          // value is a uint8_t[32] C array, we need to check if it's zero in modern C++
          if (!evmc::is_zero(callInfo.value)) {
            // If the value is not zero, we need to throw an exception
            throw DynamicException("Non-payable function called with value");
          }
          auto func = this->evmFunctions_.find(funcName);
          if (func == this->evmFunctions_.end()) throw DynamicException("Functor not found for non-payable function");
          return func->second(callInfo);
        }
      } catch (const std::exception& e) {
        throw DynamicException(e.what());
      }
    }

    /**
     * Do a contract call to a view function.
     * @param data Tuple of (from, to, gasLimit, gasPrice, value, data).
     * @return The result of the view function.
     * @throw DynamicException if the functor is not found or the function throws an exception.
     */
    Bytes ethCallView(const evmc_message& data, ContractHost* host) const override {
      this->host_ = host;
      PointerNullifier nullifier(this->host_);
      try {
        Functor funcName = Utils::getFunctor(data);
        auto func = this->viewFunctions_.find(funcName);
        if (func == this->viewFunctions_.end()) throw DynamicException("Functor not found");
        return func->second(data);
      } catch (std::exception& e) {
        throw DynamicException(e.what());
      }
    }

    /**
     * Emit an event. Remember this is an "empty" event, it lacks state info
     * that will be filled by EventManager::commitEvents().
     * @tparam Args The argument types of the event.
     * @tparam Flags The indexing flags of the event.
     * @param name The event's name.
     * @param args The event's arguments. Defaults to none (empty tuple).
     * @param anonymous Whether the event is anonymous or not. Defaults to false.
     */
    template <typename... Args, bool... Flags>
    void emitEvent(
      const std::string& name,
      const std::tuple<EventParam<Args, Flags>...>& args = std::make_tuple(),
      bool anonymous = false
    ) const {
      if (this->host_ == nullptr) {
        throw DynamicException("Contracts going haywire! trying to emit a event without a host!");
      }
      this->host_->emitEvent(name, this->getContractAddress(), args, anonymous);
    }

    /**
     * Check if a functor is registered as a payable function.
     * @param functor The functor to check.
     * @return `true` if the functor is registered as a payable function, `false` otherwise.
     */
    bool isPayableFunction(const Functor& functor) const {
      return this->payableFunctions_.contains(functor);
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
    R callContractViewFunction(
      const Address& address, R(C::*func)(const Args&...) const, const Args&... args
    ) const {
      if (this->host_ == nullptr) {
        throw DynamicException("Contracts going haywire! trying to call a contract function without a host!");
      }
      return this->host_->callContractViewFunction(this, address, func, args...);
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
    template <typename R, typename C>
    R callContractViewFunction(
      const Address& address, R(C::*func)() const
    ) const {
      if (this->host_ == nullptr) {
        throw DynamicException("Contracts going haywire! trying to call a contract function without a host!");
      }
      return this->host_->callContractViewFunction(this, address, func);
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
    template <typename R, typename C, typename... Args> R callContractFunction(
      const Address& targetAddr, R(C::*func)(const Args&...), const Args&... args
    ) {
      if (this->host_ == nullptr) {
        throw DynamicException("Contracts going haywire! trying to call a contract function without a host!");
      }
      return this->host_->callContractFunction(this, targetAddr, 0, func, args...);
    }

    /**
     * Call a contract function (non-view) based on the basic requirements of a contract call with the value flag.
     * @tparam R The return type of the function.
     * @tparam C The contract type.
     * @tparam Args The argument types of the function.
     * @param value Flag to send value with the call.
     * @param address The address of the contract to call.
     * @param func The function to call.
     * @param args The arguments to pass to the function.
     * @return The result of the function.
     */
    template <typename R, typename C, typename... Args> R callContractFunction(
      const uint256_t& value, const Address& address, R(C::*func)(const Args&...), const Args&... args
    ) {
      if (this->host_ == nullptr) {
        throw DynamicException("Contracts going haywire! trying to call a contract function without a host!");
      }
      return this->host_->callContractFunction(this, address, value, func, args...);
    }

    /**
     * Call a contract function (non-view) based on the basic requirements of a contract call with no arguments.
     * @tparam R The return type of the function.
     * @tparam C The contract type.
     * @param targetAddr The address of the contract to call.
     * @param func The function to call.
     * @return The result of the function.
     */
    template <typename R, typename C> R callContractFunction(const Address& targetAddr, R(C::*func)()) {
      if (this->host_ == nullptr) {
        throw DynamicException("Contracts going haywire! trying to call a contract function without a host!");
      }
      return this->host_->callContractFunction(this, targetAddr, 0, func);
    }

    /**
     * Call a contract function (non-view) based on the basic requirements of a contract call with the value flag and no arguments.
     * @tparam R The return type of the function.
     * @tparam C The contract type.
     * @param value Flag to send value with the call.
     * @param address The address of the contract to call.
     * @param func The function to call.
     * @return The result of the function.
     */
    template <typename R, typename C> R callContractFunction(
      const uint256_t& value, const Address& address, R(C::*func)()
    ) {
      if (this->host_ == nullptr) {
        throw DynamicException("Contracts going haywire! trying to create a contract without a host!");
      }
      return this->host_->callContractFunction(this, address, value, func);
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
    template <typename R, typename C, typename... Args> R callContractFunction(
      ContractHost* contractHost, R (C::*func)(const Args&...), const Args&... args
    ) {
      try {
        // We don't want to ever overwrite the host_ pointer if it's already set (nested calls)
        if (this->host_ == nullptr) {
          this->host_ = contractHost;
          PointerNullifier nullifier(this->host_);
          return (static_cast<C*>(this)->*func)(args...);
        }
        return (static_cast<C*>(this)->*func)(args...);
      } catch (const std::exception& e) {
        throw DynamicException(e.what());
      }
    }

    /**
     * Wrapper for calling a contract function (non-view) based on the basic requirements of a contract call with no arguments.
     * @tparam R The return type of the function.
     * @tparam C The contract type.
     * @tparam Args The argument types of the function.
     * @param func The function to call.
     * @return The result of the function.
     */
    template <typename R, typename C> R callContractFunction(ContractHost* contractHost, R (C::*func)()) {
      try {
        // We don't want to ever overwrite the host_ pointer if it's already set (nested calls)
        if (this->host_ == nullptr) {
          this->host_ = contractHost;
          PointerNullifier nullifier(this->host_);
          return (static_cast<C*>(this)->*func)();
        }
        return (static_cast<C*>(this)->*func)();
      } catch (const std::exception& e) {
        throw DynamicException(e.what());
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
    template<typename TContract, typename... Args> Address callCreateContract(
      Args&&... args
    ) {
      if (this->host_ == nullptr) {
        throw DynamicException("Contracts going haywire! trying to create a contract without a host!");
      }
      std::string createSignature = "createNew" + Utils::getRealTypeName<TContract>() + "Contract(";
      // Append args
      createSignature += ContractReflectionInterface::getConstructorArgumentTypesString<TContract>();
      createSignature += ")";
      Bytes fullData;
      Utils::appendBytes(fullData, Utils::sha3(Utils::create_view_span(createSignature)));
      fullData.resize(4); // We only need the first 4 bytes for the function signature
      if constexpr (sizeof...(Args) > 0) {
        Utils::appendBytes(fullData, ABI::Encoder::encodeData(std::forward<Args>(args)...));
      }
      return this->host_->callCreateContract<TContract>(this, fullData);
    }

    /**
     * Get the balance of a contract.
     * @param address The address of the contract.
     * @return The balance of the contract.
     */
    uint256_t getBalance(const Address& address) const {
      if (this->host_ == nullptr) {
        throw DynamicException("Contracts going haywire! trying to get balance without a host!");
      }
      return host_->getBalanceFromAddress(address);
    }

    /**
     * Send an amount of tokens from the contract to another address.
     * @param to The address to send the tokens to.
     * @param amount The amount of tokens to send.
     */
    void sendTokens(const Address& to, const uint256_t& amount) {
      if (this->host_ == nullptr) {
        throw DynamicException("Contracts going haywire! trying to send tokens without a host!");
      }
      host_->sendTokens(this, to, amount);
    }

    /**
     * Get the cryptographically secure random number.
     * @return The random number.
     */
    uint256_t getRandom() const {
      if (this->host_ == nullptr) {
        throw DynamicException("Contracts going haywire! trying to get random number without a host!");
      }
      return host_->getRandomValue();
    }

    /**
     * Register a variable as used by the contract.
     * @param contract The contract that uses the variable.
     * @param variable The variable that is used.
     */
    friend void registerVariableUse(DynamicContract& contract, SafeBase& variable);
};

#endif // DYNAMICCONTRACT_H
