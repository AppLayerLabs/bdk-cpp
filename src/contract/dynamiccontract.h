/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef DYNAMICCONTRACT_H
#define DYNAMICCONTRACT_H

#include "../utils/evmcconv.h" // getFunctor, getFunctionArgs

#include "contracthost.h" // contractmanager.h -> contract.h, ...utils/utils.h,safehash.h
#include <boost/unordered/unordered_flat_set.hpp>


/// Template for a smart contract. All contracts must inherit this class.
class DynamicContract : public BaseContract {
  private:
    /**
     * Boolean that accompany a PointerNullifier to identify if it has been registered already
     */
    bool nullifiable_ = true;

   /**
    * Set of non-payable functions that can be called by the contract.
    * The key is the function signature (first 4 hex bytes of keccak).
    */
    boost::unordered_flat_set<
      Functor, SafeHash
    > nonpayableFunctions_;

   /**
    * Set of payable functions that can be called by the contract.
    * The key is the function signature (first 4 hex bytes of keccak).
    */
    boost::unordered_flat_set<
      Functor, SafeHash
    > payableFunctions_;

   /**
    * Set of view functions that can be called by the contract.
    * The key is the function signature (first 4 hex bytes of keccak).
    */
    boost::unordered_flat_set<
      Functor, SafeHash
    > viewFunctions_;

    /**
     * Map for all the functions, regardless of mutability
     * The reason for having all these functions in a single mapping is because
     * EVM expects a contract call to actually return the ABI serialized data
     * So this map is exactly for that, it is a second representation of all the functions
     * stored in the previous 3 maps but with the ABI serialized data as the return value
     * We still check if we are calling a payable if the evmc_message has value.
     */
    boost::unordered_flat_map<
      Functor, std::function<Bytes(const evmc_message& callInfo)>, SafeHash
    > evmFunctions_;

    /**
     *  Vector containing all the supported ERC-165 interfaces
     *  See more: https://eips.ethereum.org/EIPS/eip-165
     *  ERC-165 Hashes are directly generated while registering contract functiosn
     *  by calling DynamicContract::registerMemberFunctions().
     *  Where all the functors of the functions inside the arguments of registerMemberFunctions
     *  Are XOR'ed together to generate a single hash compatible with ERC-165.
     *  0x01ffc9a7 is the ERC-165 interface ID for ERC-165 itself.
     *  0x01ffc9a7 == 33540519
     */
    std::vector<Functor> supportedInterfaces_ = { Functor(33540519) };

    /**
     * Register a callable function (a function that is called by a transaction),
     * adding it to the callable functions map.
     * @param functor Solidity function signature (first 4 hex bytes of keccak).
     * @param f Function to be called.
     * @param type The type of the function (payable, non-payable or view).
     */
    void registerFunction(
      const Functor& functor, const std::function<Bytes(const evmc_message& tx)>& f, const FunctionTypes& type
    ) {
      this->evmFunctions_[functor] = f;
      if (type == FunctionTypes::NonPayable) {
        this->nonpayableFunctions_.insert(functor);
      } else if (type == FunctionTypes::Payable) {
        this->payableFunctions_.insert(functor);
      } else if (type == FunctionTypes::View) {
        this->viewFunctions_.insert(functor);
      } else {
        throw DynamicException("Invalid function type for registerFunction.");
      }
    }

    /**
     * Register a variable that was used by the contract.
     * @param variable Reference to the variable.
     */
    inline void registerVariableUse(SafeBase& variable) const {
      if (this->host_ == nullptr) {
        throw DynamicException("Contracts going haywire! trying to register variable use without a host_!");
      }
      host_->registerVariableUse(variable);
    }

    /**
     * Template for registering a const member function with no arguments.
     * @param funcSignature Solidity function signature.
     * @param memFunc Pointer to the member function.
     * @param methodMutability The mutability of the function.
     * @param instance Pointer to the instance of the class.
     */
    template <typename R, typename T> Functor registerMemberFunction(
      const std::string& funcSignature, R(T::*memFunc)() const, const FunctionTypes& methodMutability, T* instance
    ) {
      std::string functStr = funcSignature + "()";
      Functor functor = Utils::makeFunctor(functStr);
      auto registerFunction = [this, instance, memFunc]([[maybe_unused]] const evmc_message& callInfo) -> Bytes {
        if constexpr (std::is_same_v<R, void>) {
          // If the function's return type is void, return an empty Bytes object
          (instance->*memFunc)(); // Call the member function without capturing its return
          return Bytes();         // Return an empty Bytes object
        } else {
          // If the function's return type is not void, encode and return its result
          return ABI::Encoder::encodeData<R>((instance->*memFunc)());
        }
      };
      this->registerFunction(functor, registerFunction, methodMutability);
      return functor;
    }

    /**
     * Template for registering a non-const member function with no arguments.
     * @param funcSignature Solidity function signature.
     * @param memFunc Pointer to the member function.
     * @param methodMutability The mutability of the function.
     * @param instance Pointer to the instance of the class.
     */
    template <typename R, typename T> Functor registerMemberFunction(
      const std::string& funcSignature, R(T::*memFunc)(), const FunctionTypes& methodMutability, T* instance
    ) {
      std::string functStr = funcSignature + "()";
      Functor functor = Utils::makeFunctor(functStr);
      auto registerFunction = [this, instance, memFunc]([[maybe_unused]] const evmc_message& callInfo) -> Bytes {
        if constexpr (std::is_same_v<R, void>) {
          // If the function's return type is void, return an empty Bytes object
          (instance->*memFunc)(); // Call the member function without capturing its return
          return Bytes();         // Return an empty Bytes object
        } else {
          // If the function's return type is not void, encode and return its result
          return ABI::Encoder::encodeData<R>((instance->*memFunc)());
        }
      };
      if (methodMutability == FunctionTypes::View) {
        throw DynamicException("View must be const because it does not modify the state.");
      }
      this->registerFunction(functor, registerFunction, methodMutability);
      return functor;
    }

    /**
     * Template for registering a non-const member function with arguments.
     * @param funcSignature Solidity function signature.
     * @param memFunc Pointer to the member function.
     * @param methodMutability The mutability of the function.
     * @param instance Pointer to the instance of the class.
     */
    template <typename R, typename... Args, typename T> Functor registerMemberFunction(
      const std::string& funcSignature, R(T::*memFunc)(Args...), const FunctionTypes& methodMutability, T* instance
    ) {
      Functor functor = ABI::FunctorEncoder::encode<Args...>(funcSignature);
      auto registrationFunc = [this, instance, memFunc](const evmc_message &callInfo) -> Bytes {
        using DecayedArgsTuple = std::tuple<std::decay_t<Args>...>;
        DecayedArgsTuple decodedData = ABI::Decoder::decodeData<std::decay_t<Args>...>(EVMCConv::getFunctionArgs(callInfo));
        if constexpr (std::is_same_v<R, void>) {
          std::apply([instance, memFunc](auto&&... args) {
              (instance->*memFunc)(std::forward<decltype(args)>(args)...);
          }, decodedData);
          return Bytes();
        } else {
          return std::apply([instance, memFunc](auto&&... args) -> Bytes {
            if constexpr (std::is_same_v<R, void>) {
              (instance->*memFunc)(std::forward<decltype(args)>(args)...);
              return Bytes();
            } else {
              return ABI::Encoder::encodeData((instance->*memFunc)(std::forward<decltype(args)>(args)...));
            }
          }, decodedData);
        }
      };
      if (methodMutability == FunctionTypes::View) {
        throw DynamicException("View must be const because it does not modify the state.");
      }
      this->registerFunction(functor, registrationFunc, methodMutability);
      return functor;
    }

    /**
     * Template for registering a const member function with arguments.
     * @param funcSignature Solidity function signature.
     * @param memFunc Pointer to the member function.
     * @param methodMutability The mutability of the function.
     * @param instance Pointer to the instance of the class.
     */
    template <typename R, typename... Args, typename T> Functor registerMemberFunction(
      const std::string& funcSignature, R(T::*memFunc)(Args...) const, const FunctionTypes& methodMutability, T* instance
    ) {
      Functor functor = ABI::FunctorEncoder::encode<Args...>(funcSignature);
      auto registrationFunc =  [this, instance, memFunc](const evmc_message &callInfo) -> Bytes {
        using DecayedArgsTuple = std::tuple<std::decay_t<Args>...>;
        DecayedArgsTuple decodedData = ABI::Decoder::decodeData<std::decay_t<Args>...>(EVMCConv::getFunctionArgs(callInfo));
        if constexpr (std::is_same_v<R, void>) {
          // If the function's return type is void, call the member function and return an empty Bytes object
          std::apply([instance, memFunc](Args... args) {
            (instance->*memFunc)(std::forward<decltype(args)>(args)...);
          }, decodedData);
          return Bytes(); // Return an empty Bytes object
        } else {
          // If the function's return type is not void, call the member function and return its encoded result
          return std::apply([instance, memFunc](Args... args) -> Bytes {
            if constexpr (std::is_same_v<R, void>) {
              // If the function's return type is void, call the member function and return an empty Bytes object
              (instance->*memFunc)(std::forward<decltype(args)>(args)...);
              return Bytes(); // Return an empty Bytes object
            } else {
              // If the function's return type is not void, call the member function and return its encoded result
              return ABI::Encoder::encodeData((instance->*memFunc)(std::forward<decltype(args)>(args)...));
            }
          }, decodedData);
        }
      };
      this->registerFunction(functor, registrationFunc, methodMutability);
      return functor;
    }

  protected:
    /**
     * Template for registering all functions of a class.
     * @param methods The methods to register.
     */
    template<typename ...Methods>
    void registerMemberFunctions(Methods&& ... methods) {
      Functor xorFunctor;
      // ERC-165 requires that all functions are XOR'ed together to create a single hash
      ((
        xorFunctor ^= this->registerMemberFunction(std::get<0>(methods), std::get<1>(methods), std::get<2>(methods), std::get<3>(methods))
      ), ...);
      this->supportedInterfaces_.push_back(xorFunctor);
    }

    /**
     * Template function for calling the register functions.
     * Should be called by the derived class.
     * @throw DynamicException if the derived class does not override this.
     */
    virtual void registerContractFunctions() {
      throw DynamicException("Derived Class from Contract does not override registerContractFunctions()");
    }

    /**
     * Forcely register a given Functor as a supported interface.
     * @param functor The functor to register.
     */
    void registerInterface(const Functor& functor) {
      this->supportedInterfaces_.push_back(functor);
    }

    /**
     * receive() is a special function when there is ether + no function signature
     * it MUST be overriden by the derived class.
     * @param callInfo The evmc_message containing the call information.
     */
    virtual void receive(const evmc_message& callInfo) {
      // Receive does not exists, so we call fallback...
      this->fallback(callInfo);
    }

    /**
     * fallback() is a special function when there is no function signature.
     * it may return or not return anything, thus Bytes (can return empty Bytes)
     */
    virtual Bytes fallback(const evmc_message& callInfo) {
      Functor funcName = EVMCConv::getFunctor(callInfo);
      throw DynamicException("Function not found for evmEthCall: " + funcName.hex().get() + " and fallback OR receive is not implemented for this contract");
    }

  public:
    /**
     * Constructor for creating the contract from scratch.
     * @param contractName The name of the contract.
     * @param address The address where the contract will be deployed.
     * @param creator The address of the creator of the contract.
     * @param chainId The chain where the contract wil be deployed.
     */
    DynamicContract(const std::string& contractName,
      const Address& address, const Address& creator, const uint64_t& chainId
    ) : BaseContract(contractName, address, creator, chainId) {
      // We need to create the ERC-165 function and register it accordingly
      this->registerMemberFunction("supportsInterface", &DynamicContract::supportsInterface, FunctionTypes::View, this);
    }

    /**
     * Constructor for loading the contract from the database.
     * @param address The address where the contract will be deployed.
     * @param db Reference to the database object.
     */
    DynamicContract(const Address& address, const DB& db) : BaseContract(address, db) {
      this->registerMemberFunction("supportsInterface", &DynamicContract::supportsInterface, FunctionTypes::View, this);
    };

    Bytes evmEthCall(const evmc_message& callInfo, ContractHost* host) final {
      // TODO: properly review the evmc_message to call the appropriate functions (fallback, receive, payable, nonpayable and finally view)
      this->host_ = host;
      PointerNullifier nullifier(this->host_, this->nullifiable_);
      Functor funcName = EVMCConv::getFunctor(callInfo);
      // Check if functor exists
      auto funcIt = this->evmFunctions_.find(funcName);
      if (funcIt == this->evmFunctions_.end()) {
        if (callInfo.flags == EVMC_STATIC) {
          throw DynamicException("Function not found for evmEthCall, cannot call fallback or receive in static context");
        }
        if (callInfo.input_size == 0) {
          this->receive(callInfo);
          // DynamicContract::receive will call this->fallback if derived class does not override it
          return Bytes(); // the function is not found and the functor is zero, we call fallback
        }
        return this->fallback(callInfo); // DynamicContract::fallback will throw an exception if not overridden
      }
      // Now we filter view, payable and non-payable functions
      if (callInfo.flags == EVMC_STATIC) {
        if (!this->viewFunctions_.contains(funcName)) {
          throw DynamicException("Function not found for evmEthCall, cannot call non-view function in static context");
        }
        return funcIt->second(callInfo);
      } else {
        // Delegate call is already treated by whoever calls evmEthCall, so only 0 flags exists
        // If it is a nonpayable function, we must check if the value is zero
        if (this->nonpayableFunctions_.contains(funcName)) {
          if (!evmc::is_zero(callInfo.value)) {
            throw DynamicException("Non-payable function called with value");
          }
        }
        return funcIt->second(callInfo);
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
     * @param address The address of the contract to call.
     * @param func The view function to call.
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
     * @param contractHost Pointer to the contract host.
     * @param func The function to call.
     * @param args The arguments to pass to the function.
     * @return The result of the function.
     */
    template <typename R, typename C, typename... Args> R callContractFunction(
      ContractHost* contractHost, R (C::*func)(const Args&...), const Args&... args
    ) {
      // We don't want to ever overwrite the host_ pointer if it's already set (nested calls)
      PointerNullifier nullifier(this->host_, this->nullifiable_);

      if (this->host_ == nullptr) {
        this->host_ = contractHost;
      }

      return (dynamic_cast<C*>(this)->*func)(args...);
    }

    /**
     * Wrapper for calling a contract function (non-view) based on the basic requirements of a contract call with no arguments.
     * @tparam R The return type of the function.
     * @tparam C The contract type.
     * @param contractHost Pointer to the contract host.
     * @param func The function to call.
     * @return The result of the function.
     */
    template <typename R, typename C> R callContractFunction(ContractHost* contractHost, R (C::*func)()) {
      try {
        // We don't want to ever overwrite the host_ pointer if it's already set (nested calls)
        if (this->host_ == nullptr) {
          this->host_ = contractHost;
          PointerNullifier nullifier(this->host_, this->nullifiable_);
          return (dynamic_cast<C*>(this)->*func)();
        }
        return (dynamic_cast<C*>(this)->*func)();
      } catch (const std::exception& e) {
        throw DynamicException(e.what());
      }
    }

    /**
     * Call the create function of a contract.
     * @tparam TContract The contract type.
     * @tparam Args The arguments of the contract constructor.
     * @param args The arguments to pass to the constructor.
     * @return The address of the created contract.
     */
    template<typename TContract, typename... Args> Address callCreateContract(Args&&... args) {
      if (this->host_ == nullptr) {
        throw DynamicException("Contracts going haywire! trying to create a contract without a host!");
      }
      return this->host_->callCreateContract<TContract>(*this, std::forward<Args>(args)...);
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
      return host_->context().getAccount(address).getBalance();
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
      host_->context().transferBalance(this->getContractAddress(), to, amount);
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

    /**
     * Check if the address is a existing contract.
     * @return
     */
    bool isContract(const Address& address) const {
      if (this->host_ == nullptr) {
        throw DynamicException("Contracts going haywire! trying to check if address is a contract without a host!");
      }
      return host_->context().getAccount(address).getContractType() != ContractType::NOT_A_CONTRACT;
    }

    /**
     * Implementation for the ERC-165 compliance.
     * See more at: https://eips.ethereum.org/EIPS/eip-165
     * @param interfaceId The interface ID to check.
     * @return `true` if the contract supports the interface, `false` otherwise.
     */
    bool supportsInterface(const Bytes4& interfaceId) const {
      Functor interfaceFunctor(UintConv::bytesToUint32(interfaceId));
      return std::ranges::any_of(supportedInterfaces_.begin(), supportedInterfaces_.end(),
        [&interfaceFunctor](const Functor& functor) { return functor == interfaceFunctor; });
    }

    /**
     * Helper function used by
     * This function is utilized to be able to inject a extra function (ERC-165, supportsInterface)
     * Into the ABI generator.
     * @tparam TContract The contract type.
     * @tparam Methods Type to allow template deduction for the methods.
     * @param ctorArgs The constructor arguments string names.
     * @param methods The methods to register.
     */
    template <typename TContract, typename... Methods>
    static void inline registerContractMethods(const std::vector<std::string>& ctorArgs, Methods&&... methods) {
      ContractReflectionInterface::registerContractMethods<
        TContract>(ctorArgs,
        std::make_tuple("supportsInterface", &TContract::supportsInterface, FunctionTypes::View, std::vector<std::string>{"interfaceId"}),
        methods...
      );
    }

    /**
     * Register a variable as used by the contract.
     * @param contract The contract that uses the variable.
     * @param variable The variable that is used.
     */
    friend void registerVariableUse(DynamicContract& contract, SafeBase& variable);
};

#endif // DYNAMICCONTRACT_H
