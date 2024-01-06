/*
Copyright (c) [2023] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef DYNAMICCONTRACT_H
#define DYNAMICCONTRACT_H

#include "abi.h"
#include "contract.h"
#include "contractmanager.h"
#include "event.h"
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
      Functor, std::function<void(const ethCallInfo& callInfo)>, SafeHash
    > publicFunctions_;

   /**
    * Map of payable functions that can be called by the contract.
    * The key is the function signature (first 4 hex bytes of keccak).
    * The value is a function that takes a vector of bytes (the arguments) and returns a ReturnType.
    */
    std::unordered_map<
      Functor, std::function<void(const ethCallInfo& callInfo)>, SafeHash
    > payableFunctions_;

   /**
    * Map of view functions that can be called by the contract.
    * The key is the function signature (first 4 hex bytes of keccak).
    * The value is a function that takes a vector of bytes (the arguments) and returns a ReturnType.
    * Function return type is the encoded return value as viewFunctions is only used by eth_call.
    */
    std::unordered_map<
      Functor, std::function<Bytes(const ethCallInfo& callInfo)>, SafeHash
    > viewFunctions_;

    /**
     * Register a callable function (a function that is called by a transaction),
     * adding it to the callable functions map.
     * @param functor Solidity function signature (first 4 hex bytes of keccak).
     * @param f Function to be called.
     */
    void registerFunction(
      const Functor& functor, std::function<void(const ethCallInfo& tx)> f
    ) {
      publicFunctions_[functor] = f;
    }

    /**
     * Register a variable that was used by the contract.
     * @param variable Reference to the variable.
     */
    inline void registerVariableUse(SafeBase& variable) { interface_.registerVariableUse(variable); }

  protected:
    /// Reference to the contract manager interface.
    ContractManagerInterface& interface_;

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
      switch (methodMutability) {
        case FunctionTypes::View: {
          this->registerViewFunction(Utils::sha3(Utils::create_view_span(functStr)).view_const(0, 4), [instance, memFunc](const ethCallInfo &callInfo) -> Bytes {
            using ReturnType = decltype((instance->*memFunc)());
            return ABI::Encoder::encodeData<ReturnType>((instance->*memFunc)());
          });
          break;
        }
        case FunctionTypes::NonPayable: {
          this->registerFunction(Utils::sha3(Utils::create_view_span(functStr)).view_const(0, 4), [instance, memFunc](const ethCallInfo &callInfo) -> void {
            (instance->*memFunc)();
            return;
          });
          break;
        }
        case FunctionTypes::Payable: {
          this->registerPayableFunction(Utils::sha3(Utils::create_view_span(functStr)).view_const(0, 4), [instance, memFunc](const ethCallInfo &callInfo) -> void {
            (instance->*memFunc)();
            return;
          });
          break;
        }
        default: {
          throw std::runtime_error("Invalid function signature.");
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
      switch (methodMutability) {
        case FunctionTypes::View: {
          throw std::runtime_error("View must be const because it does not modify the state.");
        }
        case FunctionTypes::NonPayable: {
          this->registerFunction(
            Utils::sha3(Utils::create_view_span(functStr)).view_const(0, 4),
            [instance, memFunc](const ethCallInfo &callInfo) -> void {
              (instance->*memFunc)();
              return;
            }
          );
          break;
        }
        case FunctionTypes::Payable: {
          this->registerPayableFunction(
            Utils::sha3(Utils::create_view_span(functStr)).view_const(0, 4),
            [instance, memFunc](const ethCallInfo &callInfo) -> void {
              (instance->*memFunc)();
              return;
            }
          );
          break;
        }
        default: {
          throw std::runtime_error("Invalid function signature.");
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
      auto registrationFunc = [this, instance, memFunc, funcSignature](const ethCallInfo &callInfo) {
        using DecayedArgsTuple = std::tuple<std::decay_t<Args>...>;
        DecayedArgsTuple decodedData = ABI::Decoder::decodeData<std::decay_t<Args>...>(std::get<6>(callInfo));
        std::apply([instance, memFunc](auto&&... args) {
            (instance->*memFunc)(std::forward<decltype(args)>(args)...);
        }, decodedData);
      };
      switch (methodMutability) {
        case (FunctionTypes::View): {
          throw std::runtime_error("View must be const because it does not modify the state.");
        }
        case (FunctionTypes::NonPayable): {
          this->registerFunction(functor, registrationFunc);
          break;
        }
        case (FunctionTypes::Payable): {
          this->registerPayableFunction(functor, registrationFunc);
          break;
        }
        default: {
          throw std::runtime_error("Invalid function signature.");
        }
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

      auto registrationFunc = [this, instance, memFunc, funcSignature](const ethCallInfo &callInfo) -> Bytes {
        using ReturnType = decltype((instance->*memFunc)(std::declval<Args>()...));
        using DecayedArgsTuple = std::tuple<std::decay_t<Args>...>;
        DecayedArgsTuple decodedData = ABI::Decoder::decodeData<std::decay_t<Args>...>(std::get<6>(callInfo));
        // Use std::apply to call the member function and encode its return value
        return std::apply([instance, memFunc](Args... args) -> Bytes {
            // Call the member function and return its encoded result
            return ABI::Encoder::encodeData((instance->*memFunc)(std::forward<decltype(args)>(args)...));
        }, decodedData);
      };

      switch (methodMutability) {
        case (FunctionTypes::View): {
          this->registerViewFunction(functor, registrationFunc);
          break;
        }
        case (FunctionTypes::NonPayable): {
          this->registerFunction(functor, registrationFunc);
        }
        case (FunctionTypes::Payable): {
          this->registerPayableFunction(functor, registrationFunc);
          break;
        }
      }
    }

    /**
     * Register a callable and payable function, adding it to the payable functions map.
     * @param functor Solidity function signature (first 4 hex bytes of keccak).
     * @param f Function to be called.
     */
    void registerPayableFunction(
      const Functor& functor, std::function<void(const ethCallInfo& tx)> f
    ) {
      payableFunctions_[functor] = f;
    }

    /**
     * Register a view/const function, adding it to the view functions map.
     * @param functor Solidity function signature (first 4 hex bytes of keccak).
     * @param f Function to be called.
     */
    void registerViewFunction(
      const Functor& functor, std::function<Bytes(const ethCallInfo& str)> f
    ) {
      viewFunctions_[functor] = f;
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
    ) : BaseContract(contractName, address, creator, chainId, db), interface_(interface) {}

    /**
     * Constructor for loading the contract from the database.
     * @param interface Reference to the contract manager interface.
     * @param address The address where the contract will be deployed.
     * @param db Reference to the database object.
     */
    DynamicContract(
      ContractManagerInterface& interface,
      const Address& address, const std::unique_ptr<DB>& db
    ) : BaseContract(address, db), interface_(interface) {}

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
          auto func = this->payableFunctions_.find(funcName);
          if (func == this->payableFunctions_.end()) throw std::runtime_error("Functor not found for payable function");
          func->second(callInfo);
        } else {
          auto func = this->publicFunctions_.find(funcName);
          if (func == this->publicFunctions_.end()) throw std::runtime_error("Functor not found for non-payable function");
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
        auto func = this->viewFunctions_.find(funcName);
        if (func == this->viewFunctions_.end()) throw std::runtime_error("Functor not found");
        return func->second(data);
      } catch (std::exception& e) {
        throw std::runtime_error(e.what());
      }
    }

    /**
     * Emit an event. Remember this is an "empty" event, it lacks state info
     * that will be filled by EventManager::commitEvents().
     * @tparam Args The argument types of the event.
     * @tparam Flags The indexing flags of the event.
     * @param name The event's name.
     * @param args The event's arguments. Defaults to an empty list.
     * @param anonymous Whether the event is anonymous or not. Defaults to false.
     */
    template <typename... Args, bool... Flags>
    void emitEvent(
        const std::string& name,
        const std::tuple<EventParam<Args, Flags>...>& args,
        bool anonymous = false
    ) const {
      Event e(name, this->getContractAddress(), args, anonymous);
      this->interface_.emitContractEvent(e);
    }

    /**
     * Check if a functor is registered as a payable function.
     * @param functor The functor to check.
     * @return `true` if the functor is registered as a payable function, `false` otherwise.
     */
    bool isPayableFunction(const Functor& functor) const {
      return this->payableFunctions_.find(functor) != this->payableFunctions_.end();
    }

    /**
     * Try to cast a contract to a specific type.
     * NOTE: Only const functions can be called on the casted contract.
     * @tparam T The type to cast to.
     * @param address The address of the contract to cast.
     * @return A pointer to the casted contract.
     */
    template <typename T> const T* getContract(const Address& address) const {
      return interface_.getContract<T>(address);
    }

    /**
     * Try to cast a contract to a specific type (non-const).
     * @tparam T The type to cast to.
     * @param address The address of the contract to cast.
     * @return A pointer to the casted contract.
     */
    template <typename T> T* getContract(const Address& address) {
      return interface_.getContract<T>(address);
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
    template <typename R, typename C, typename... Args> R callContractFunction(
      const Address& targetAddr, R(C::*func)(const Args&...), const Args&... args
    ) {
      return this->interface_.callContractFunction(
        this->getOrigin(), this->getContractAddress(),
        targetAddr, 0, func, std::forward<const Args&>(args)...
      );
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
    template <typename R, typename C, typename... Args> R callContractFunction(
      const uint256_t& value, const Address& address, R(C::*func)(const Args&...), const Args&... args
    ) {
      return this->interface_.callContractFunction(
        this->getOrigin(), this->getContractAddress(),
        address, value, func, std::forward<const Args&>(args)...
      );
    }

    /**
     * Call a contract function (non-view) based on the basic requirements of a contract call with no arguments
     * @tparam R The return type of the function.
     * @tparam C The contract type.
     * @param targetAddr The address of the contract to call.
     * @param func The function to call.
     * @return The result of the function.
     */
    template <typename R, typename C> R callContractFunction(
      const Address& targetAddr, R(C::*func)()
    ) {
      return this->interface_.callContractFunction(
        this->getOrigin(), this->getContractAddress(), targetAddr, 0, func
      );
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
    template <typename R, typename C> R callContractFunction(
      const uint256_t& value, const Address& address, R(C::*func)()
    ) {
      return this->interface_.callContractFunction(
        this->getOrigin(), this->getContractAddress(), address, value, func
      );
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
      R (C::*func)(const Args&...), const Args&... args
    ) {
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
    template <typename R, typename C> R callContractFunction(R (C::*func)()) {
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
    template<typename TContract, typename... Args> Address callCreateContract(
      const uint256_t& gas, const uint256_t& gasPrice, const uint256_t& value, Args&&... args
    ) {
      Utils::safePrint("CallCreateContract being called...");
      Bytes encoder;
      if constexpr (sizeof...(Args) > 0) {
        encoder = ABI::Encoder::encodeData(std::forward<Args>(args)...);
      } else {
        encoder = Bytes(32, 0);
      }
      return this->interface_.callCreateContract<TContract>(
        this->getOrigin(), this->getContractAddress(), gas, gasPrice, value, std::move(encoder)
      );
    }

    /**
     * Get the balance of a contract.
     * @param address The address of the contract.
     * @return The balance of the contract.
     */
    uint256_t getBalance(const Address& address) const {
      return interface_.getBalanceFromAddress(address);
    }

    /**
     * Send an amount of tokens from the contract to another address.
     * @param to The address to send the tokens to.
     * @param amount The amount of tokens to send.
     */
    void sendTokens(const Address& to, const uint256_t& amount) {
      interface_.sendTokens(this->getContractAddress(), to, amount);
    }

    /**
     * Register a variable as used by the contract.
     * @param contract The contract that uses the variable.
     * @param variable The variable that is used.
     */
    friend void registerVariableUse(DynamicContract& contract, SafeBase& variable);
};

#endif // DYNAMICCONTRACT_H
