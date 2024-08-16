#ifndef CONTRACT_HOST_H
#define CONTRACT_HOST_H


#include <evmc/evmc.hpp>
#include "../utils/utils.h"
#include "../utils/strings.h"
#include "../utils/hex.h"
#include "../utils/safehash.h"
#include "../utils/db.h"
#include "../core/storage.h"
#include <evmone/evmone.h>
#include "contractstack.h"
#include "../core/rdpos.h"
#include "../utils/contractreflectioninterface.h"
#include "contractmanager.h"
#include "../core/dump.h"
#include "calltracer.h"
#include "../bytes/join.h"


// TODO: EVMC Static Mode Handling
// TODO: Contract creating other contracts (EVM Factories)
// TODO: Proper gas limit tests.

/**
 * The ContractHost class is the class which holds a single line of execution for a contract. (This also includes nested calls)
 * It MUST be unique for each line of execution, as it holds the used stack, accounts, and storage for the contract.
 * The ContractHost class have the following responsibilities:
 * - Be the EVMC Host implementation for EVM contracts
 * - Hold the stack (ContractStack) of the execution.
 * - Allow both C++ and EVM contracts to be called.
 * - Allow interoperability between C++ and EVM contracts.
 * - Process the commit/revert of the stack during **destructor** call.
 */

/**
 * GasLimit Rules
 * Any call: 21000
 * C++ Call: 1000
 * EVM Call: 5000
 * Any EVM Contract: 100000
 * Any CPP Contract: 50000
 */

// Address for static BDKD precompile contracts.
using namespace evmc::literals;
const auto ZERO_ADDRESS = 0x0000000000000000000000000000000000000000_address;
const auto BDK_PRECOMPILE = 0x1000000000000000000000000000100000000001_address;

class ContractHost : public evmc::Host {
  private:
    // We need this because nested calls can call the same contract multiple times
    // Potentially taking advantage of wrong context variables.
    class NestedCallSafeGuard {
      private:
        const ContractLocals* contract_;
        Address caller_;
        uint256_t value_;
      public:
        NestedCallSafeGuard(const ContractLocals* contract, const Address& caller, const uint256_t& value) :
          contract_(contract), caller_(contract->caller_), value_(contract->value_) {
        }
        ~NestedCallSafeGuard() {
          contract_->caller_ = caller_;
          contract_->value_ = value_;
        }
    };
    evmc_vm* vm_;
    DumpManager& manager_;
    Storage& storage_;
    mutable ContractStack stack_;
    mutable RandomGen randomGen_; // Random generator for the contract.
    const evmc_tx_context& currentTxContext_; // MUST be initialized within the constructor.
    boost::unordered_flat_map<Address, std::unique_ptr<BaseContract>, SafeHash>& contracts_;
    boost::unordered_flat_map<Address, NonNullUniquePtr<Account>, SafeHash>& accounts_;
    boost::unordered_flat_map<StorageKey, Hash, SafeHash>& vmStorage_;
    boost::unordered_flat_map<StorageKey, Hash, SafeHash> transientStorage_;
    bool mustRevert_ = true; // We always assume that we must revert until proven otherwise.
    mutable bool evmcThrow_ = false; // Did the EVMC throw an exception?
    mutable std::vector<std::string> evmcThrows_;
    std::vector<Bytes> evmcResults_; /// evmc_result has a uint8_t* and a size, but we need to allocate memory for it.
    uint64_t eventIndex_ = 0;
    const Hash& txHash_;
    const uint64_t txIndex_;
    const Hash& blockHash_;
    int64_t& leftoverGas_; /// Reference to the leftover gas from the transaction.
                            /// The leftoverGas_ is a object given by the State
    TxAdditionalData addTxData_;
    trace::CallTracer callTracer_;

    // Private as this is not available for contracts as it has safety checks
    void transfer(const Address& from, const Address& to, const uint256_t& value);

    /**
     * Set the local variables for a given contract (origin, caller, value).
     * Used everytime *before* and *after* (if nested) a contract call.
     * @param contract The contract to set the local variables for.
     * @param caller The caller address to set.
     * @param value The value to set.
     */
    inline void setContractVars(const ContractLocals* contract,
                                const Address& caller,
                                const uint256_t& value) const {
      contract->caller_ = caller;
      contract->value_ = value;
    }

    inline void deduceGas(const int64_t& gas) const {
      this->leftoverGas_ -= gas;
      if (this->leftoverGas_ < 0) {
        throw DynamicException("ContractHost deduceGas: out of gas");
      }
    }

    evmc::Result createEVMContract(const evmc_message& msg,
                                   const Address& contractAddress,
                                   const evmc_call_kind& kind);

    ContractType decodeContractCallType(const evmc_message& msg) const;
    evmc::Result processBDKPrecompile(const evmc_message& msg);
    evmc::Result callEVMCreate(const evmc_message& msg);
    evmc::Result callEVMCreate2(const evmc_message& msg);
    evmc::Result callEVMContract(const evmc_message& msg);
    evmc::Result callCPPContract(const evmc_message& msg);

  Address computeNewAccountAddress(const Address& fromAddress,
                                   const uint64_t& nonce,
                                   const Hash& salt,
                                   const bytes::View& init_code);

    bool isTracingCalls() const noexcept;

    void traceCallStarted(const evmc_message& msg) noexcept;

    void traceCallFinished(const evmc_result& res) noexcept;

    void traceCallSucceeded(Bytes output, uint64_t gasUsed) noexcept;

    void traceCallReverted(Bytes output, uint64_t gasUsed) noexcept;

    void traceCallReverted(uint64_t gasUsed) noexcept;

    void traceCallOutOfGas() noexcept;

    void saveCallTrace() noexcept;

    void saveTxAdditionalData() noexcept;

  public:
    ContractHost(evmc_vm* vm,
                 DumpManager& manager,
                 Storage& storage,
                 const Hash& randomnessSeed,
                 const evmc_tx_context& currentTxContext,
                 boost::unordered_flat_map<Address, std::unique_ptr<BaseContract>, SafeHash>& contracts,
                 boost::unordered_flat_map<Address, NonNullUniquePtr<Account>, SafeHash>& accounts,
                 boost::unordered_flat_map<StorageKey, Hash, SafeHash>& vmStorage,
                 const Hash& txHash,
                 const uint64_t txIndex,
                 const Hash& blockHash,
                 int64_t& txGasLimit) :
    vm_(vm),
    manager_(manager),
    storage_(storage),
    randomGen_(randomnessSeed),
    currentTxContext_(currentTxContext),
    contracts_(contracts),
    accounts_(accounts),
    vmStorage_(vmStorage),
    txHash_(txHash),
    txIndex_(txIndex),
    blockHash_(blockHash),
    leftoverGas_(txGasLimit),
    addTxData_({.hash = txHash}) {}

    // Rule of five, no copy/move allowed.
    ContractHost(const ContractHost&) = delete;
    ContractHost(ContractHost&&) = delete;
    ContractHost& operator=(const ContractHost&) = delete;
    ContractHost& operator=(ContractHost&&) = delete;
    ~ContractHost() noexcept override;

    static Address deriveContractAddress(const uint64_t& nonce,
                                         const Address& address);

    static Address deriveContractAddress(const Address& fromAddress,
                                         const Hash& salt,
                                         const bytes::View& init_code);

    /// Executes a call
    void execute(const evmc_message& msg, const ContractType& type);

    /// Executes a eth_call RPC method (view)
    /// returns the result of the call
    Bytes ethCallView(const evmc_message& msg, const ContractType& type);

    /// Simulates a call
    void simulate(const evmc_message& msg, const ContractType& type);

    /// EVMC FUNCTIONS
    bool account_exists(const evmc::address& addr) const noexcept final;
    evmc::bytes32 get_storage(const evmc::address& addr, const evmc::bytes32& key) const noexcept final;
    evmc_storage_status set_storage(const evmc::address& addr, const evmc::bytes32& key, const evmc::bytes32& value) noexcept final;
    evmc::uint256be get_balance(const evmc::address& addr) const noexcept final;
    size_t get_code_size(const evmc::address& addr) const noexcept final;
    evmc::bytes32 get_code_hash(const evmc::address& addr) const noexcept final;
    size_t copy_code(const evmc::address& addr, size_t code_offset, uint8_t* buffer_data, size_t buffer_size) const noexcept final;
    bool selfdestruct(const evmc::address& addr, const evmc::address& beneficiary) noexcept final;
    evmc::Result call(const evmc_message& msg) noexcept final;
    evmc_tx_context get_tx_context() const noexcept final;
    evmc::bytes32 get_block_hash(int64_t number) const noexcept final;
    void emit_log(const evmc::address& addr, const uint8_t* data, size_t data_size, const evmc::bytes32 topics[], size_t topics_count) noexcept final;
    evmc_access_status access_account(const evmc::address& addr) noexcept final;
    evmc_access_status access_storage(const evmc::address& addr, const evmc::bytes32& key) noexcept final;
    evmc::bytes32 get_transient_storage(const evmc::address &addr, const evmc::bytes32 &key) const noexcept final;
    void set_transient_storage(const evmc::address &addr, const evmc::bytes32 &key, const evmc::bytes32 &value) noexcept final;
    /// END OF EVMC FUNCTIONS


    /// CONTRACT INTERFACING FUNCTIONS
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
    R callContractViewFunction(const BaseContract* caller, const Address& targetAddr, R(C::*func)(const Args&...) const, const Args&... args) const {
      const auto recipientAccIt = this->accounts_.find(targetAddr);
      if (recipientAccIt == this->accounts_.end()) {
        throw DynamicException(std::string(__func__) + ": Contract Account does not exist - Type: "
          + Utils::getRealTypeName<C>() + " at address: " + targetAddr.hex().get()
        );
      }
      const auto& recipientAcc = *recipientAccIt->second;
      if (!recipientAcc.isContract()) {
        throw DynamicException(std::string(__func__) + ": Contract does not exist - Type: "
          + Utils::getRealTypeName<C>() + " at address: " + targetAddr.hex().get()
        );
      }
      NestedCallSafeGuard guard(caller, caller->caller_, caller->value_);
      switch (recipientAcc.contractType) {
        case ContractType::EVM : {
          this->deduceGas(5000);
          evmc_message msg;
          msg.kind = EVMC_CALL;
          msg.flags = EVMC_STATIC;
          msg.depth = 1;
          msg.gas = this->leftoverGas_;
          msg.recipient = targetAddr.toEvmcAddress();
          msg.sender = caller->getContractAddress().toEvmcAddress();
          auto functionName = ContractReflectionInterface::getFunctionName(func);
          if (functionName.empty()) {
            throw DynamicException("ContractHost::callContractViewFunction: EVM contract function name is empty (contract not registered?)");
          }
          auto functor = ABI::FunctorEncoder::encode<Args...>(functionName);
          Bytes fullData;
          Utils::appendBytes(fullData, Utils::uint32ToBytes(functor.value));
          if constexpr (sizeof...(Args) > 0) {
            Utils::appendBytes(fullData, ABI::Encoder::encodeData<Args...>(args...));
          }
          msg.input_data = fullData.data();
          msg.input_size = fullData.size();
          msg.value = {};
          msg.create2_salt = {};
          msg.code_address = targetAddr.toEvmcAddress();
          /// TODO: OMG this is so ugly, we need to fix this.
          /// A **CONST_CAST** is needed because we can't explicity tell the evmc_execute to do a view call.
          /// Regardless of that, we set flag = 1 to indicate that this is a view/STATIC call.
          evmc::Result result (evmc_execute(this->vm_, &this->get_interface(), const_cast<ContractHost*>(this)->to_context(),
          evmc_revision::EVMC_LATEST_STABLE_REVISION, &msg, recipientAcc.code.data(), recipientAcc.code.size()));
          this->leftoverGas_ = result.gas_left;
          if (result.status_code) {
            auto hexResult = Hex::fromBytes(bytes::View(result.output_data, result.output_data + result.output_size));
            throw DynamicException("ContractHost::callContractViewFunction: EVMC call failed - Type: "
              + Utils::getRealTypeName<C>() + " at address: " + targetAddr.hex().get() + " - Result: " + hexResult.get()
            );
          }
          return std::get<0>(ABI::Decoder::decodeData<R>(bytes::View(result.output_data, result.output_data + result.output_size)));
        } break;
        case ContractType::CPP : {
          this->deduceGas(1000);
          const C* contract = this->getContract<const C>(targetAddr);
          this->setContractVars(contract, caller->getContractAddress(), 0);
          return (contract->*func)(args...);
        }
        default: {
          throw DynamicException("PANIC! ContractHost::callContractViewFunction: Unknown contract type");
        }
      }
    }

    template <typename R, typename C, typename... Args>
    R callContractFunction(
      BaseContract* caller, const Address& targetAddr,
      const uint256_t& value,
      R(C::*func)(const Args&...), const Args&... args) {
      if (this->isTracingCalls()) [[unlikely]] {
        trace::Call callData;

        const uint64_t gas = leftoverGas_;

        callData.type = trace::Call::Type::CALL;
        callData.from = caller->getContractAddress();
        callData.to = targetAddr;
        callData.gas = gas;

        const std::string functionName = ContractReflectionInterface::getFunctionName(func);

        const BytesArr<4> encodedFunctor =
          Utils::uint32ToBytes(ABI::FunctorEncoder::encode<Args...>(functionName).value);

        if constexpr (sizeof...(args) > 0) {
          const Bytes encodedArgs = ABI::Encoder::encodeData<Args...>(args...);
          callData.input = Utils::makeBytes(bytes::join(encodedFunctor, encodedArgs));
        } else {
          callData.input = Utils::makeBytes(encodedFunctor);
        }

        callTracer_.callStarted(std::move(callData));

        try {
          if constexpr (std::same_as<R, void>) {
            callContractFunctionImpl(caller, targetAddr, value, func, args...);
            callTracer_.callSucceeded(Bytes(), gas - leftoverGas_);
            return;
          } else {
            R result = callContractFunctionImpl(caller, targetAddr, value, func, args...);
            const uint64_t gasUsed = gas - leftoverGas_;
            Bytes output = ABI::Encoder::encodeData<R>(result);
            callTracer_.callSucceeded(std::move(output), gasUsed);
            return result;
          }
        } catch (const std::exception& err) {
          Bytes output;

          if (err.what()) {
            output = trace::encodeRevertReason(err.what());
          }

          callTracer_.callReverted(std::move(output), gas - leftoverGas_);
          throw err;
        }
      } else [[likely]] {
        return callContractFunctionImpl(caller, targetAddr, value, func, args...);
      }
    }

    /**
     * Call a contract function. Used by DynamicContract to call other contracts.
     * A given DynamicContract will only call another contract if triggered by a transaction.
     * This will only be called if callContract() or validateCallContractWithTx() was called before.
     * Implementation for @tparam ReturnType NOT being void
     * @tparam R The return type of the function.
     * @tparam C The contract type.
     * @tparam Args The arguments types.
     * @param targetAddr The address of the contract to call.
     * @param value Flag to indicate if the function is payable.,
     * @param func The function to call.
     * @param args The arguments to pass to the function.
     * @return The return value of the function.
     */
    template <typename R, typename C, typename... Args>
    R callContractFunctionImpl(
      BaseContract* caller, const Address& targetAddr,
      const uint256_t& value,
      R(C::*func)(const Args&...), const Args&... args
    ) {
      // 1000 Gas Limit for every C++ contract call!
      auto& recipientAcc = *this->accounts_[targetAddr];
      if (!recipientAcc.isContract()) {
        throw DynamicException(std::string(__func__) + ": Contract does not exist - Type: "
          + Utils::getRealTypeName<C>() + " at address: " + targetAddr.hex().get()
        );
      }
      if (value) {
        this->sendTokens(caller, targetAddr, value);
      }
      NestedCallSafeGuard guard(caller, caller->caller_, caller->value_);
      switch (recipientAcc.contractType) {
        case ContractType::EVM : {
          this->deduceGas(10000);
          evmc_message msg;
          msg.kind = EVMC_CALL;
          msg.flags = 0;
          msg.depth = 1;
          msg.gas = this->leftoverGas_;
          msg.recipient = targetAddr.toEvmcAddress();
          msg.sender = caller->getContractAddress().toEvmcAddress();
          auto functionName = ContractReflectionInterface::getFunctionName(func);
          if (functionName.empty()) {
            throw DynamicException("ContractHost::callContractFunction: EVM contract function name is empty (contract not registered?)");
          }
          auto functor = ABI::FunctorEncoder::encode<Args...>(functionName);
          Bytes fullData;
          Utils::appendBytes(fullData, Utils::uint32ToBytes(functor.value));
          if constexpr (sizeof...(Args) > 0) {
            Utils::appendBytes(fullData, ABI::Encoder::encodeData<Args...>(args...));
          }
          msg.input_data = fullData.data();
          msg.input_size = fullData.size();
          msg.value = Utils::uint256ToEvmcUint256(value);
          msg.create2_salt = {};
          msg.code_address = targetAddr.toEvmcAddress();
          evmc::Result result (evmc_execute(this->vm_, &this->get_interface(), this->to_context(),
          evmc_revision::EVMC_LATEST_STABLE_REVISION, &msg, recipientAcc.code.data(), recipientAcc.code.size()));
          this->leftoverGas_ = result.gas_left;
          if (result.status_code) {
            auto hexResult = Hex::fromBytes(bytes::View(result.output_data, result.output_data + result.output_size));
            throw DynamicException("ContractHost::callContractFunction: EVMC call failed - Type: "
              + Utils::getRealTypeName<C>() + " at address: " + targetAddr.hex().get() + " - Result: " + hexResult.get()
            );
          }
          if constexpr (std::same_as<R, void>) {
            return;
          } else {
            return std::get<0>(ABI::Decoder::decodeData<R>(bytes::View(result.output_data, result.output_data + result.output_size)));
          }
        } break;
        case ContractType::CPP : {
          this->deduceGas(1000);
          C* contract = this->getContract<C>(targetAddr);
          this->setContractVars(contract, caller->getContractAddress(), value);
          try {
            return contract->callContractFunction(this, func, args...);
          } catch (const std::exception& e) {
            throw DynamicException(e.what() + std::string(" - Type: ")
              + Utils::getRealTypeName<C>() + " at address: " + targetAddr.hex().get()
            );
          }
        }
        default : {
          throw DynamicException("PANIC! ContractHost::callContractFunction: Unknown contract type");
        }
      }
    }

    /**
     * Call the createNewContract function of a contract.
     * Used by DynamicContract to create new contracts.
     * @tparam TContract The contract type.
     * @param callValue The caller value.
     * @param encoder The ABI encoder.
     * @return The address of the new contract.
     */
    template <typename TContract> Address callCreateContract(
      BaseContract* caller,
      const Bytes &fullData
    ) {
      // 100k gas limit for every contract creation!
      this->deduceGas(50000);
      evmc_message callInfo;
      // evmc_message:
      // struct evmc_message
      // {
      //   enum evmc_call_kind kind;
      //   uint32_t flags;
      //   int32_t depth;
      //   int64_t gas;
      //   evmc_address recipient;
      //   evmc_address sender;
      //   const uint8_t* input_data;
      //   size_t input_size;
      //   evmc_uint256be value;
      //   evmc_bytes32 create2_salt;
      //   evmc_address code_address;
      // };
      const auto& to = ProtocolContractAddresses.at("ContractManager");
      const auto& from = caller->getContractAddress();
      callInfo.flags = 0;
      callInfo.depth = 1;
      callInfo.gas = this->leftoverGas_;
      callInfo.recipient = to.toEvmcAddress();
      callInfo.sender = from.toEvmcAddress();
      callInfo.input_data = fullData.data();
      callInfo.input_size = fullData.size();
      callInfo.value = {};
      callInfo.create2_salt = {};
      callInfo.code_address = to.toEvmcAddress();
      // Get the ContractManager from the this->accounts_ map
      ContractManager* contractManager = dynamic_cast<ContractManager*>(this->contracts_.at(to).get());
      this->setContractVars(contractManager, from, 0);
      auto& callerNonce = this->accounts_[from]->nonce;
      Address newContractAddress = ContractHost::deriveContractAddress(callerNonce, from);
      this->stack_.registerNonce(from, callerNonce);
      NestedCallSafeGuard guard(caller, caller->caller_, caller->value_);
      contractManager->ethCall(callInfo, this);
      ++callerNonce;
      return newContractAddress;
    }

    /**
     * Get a contract by its address.
     * Used by DynamicContract to access view/const functions of other contracts.
     * @tparam T The contract type.
     * @param address The address of the contract.
     * @return A pointer to the contract.
     * @throw DynamicException if contract is not found or not of the requested type.
     */
    template <typename T> const T* getContract(const Address &address) const {
      auto it = this->contracts_.find(address);
      if (it == this->contracts_.end()) throw DynamicException(
        "ContractManager::getContract: contract at address " +
        address.hex().get() + " not found."
      );
      auto ptr = dynamic_cast<T*>(it->second.get());
      if (ptr == nullptr) throw DynamicException(
        "ContractManager::getContract: Contract at address " +
        address.hex().get() + " is not of the requested type: " + Utils::getRealTypeName<T>()
      );
      return ptr;
    }

    /**
     * Get a contract by its address (non-const).
     * Used by DynamicContract to access view/const functions of other contracts.
     * @tparam T The contract type.
     * @param address The address of the contract.
     * @return A pointer to the contract.
     * @throw DynamicException if contract is not found or not of the requested type.
     */
    template <typename T> T* getContract(const Address& address) {
      auto it = this->contracts_.find(address);
      if (it == this->contracts_.end()) throw DynamicException(
        "ContractManager::getContract: contract at address " +
        address.hex().get() + " not found."
      );
      auto ptr = dynamic_cast<T*>(it->second.get());
      if (ptr == nullptr) throw DynamicException(
        "ContractManager::getContract: Contract at address " +
        address.hex().get() + " is not of the requested type: " + Utils::getRealTypeName<T>()
      );
      return ptr;
    }

    /**
     * Emit an event from a contract. Called by DynamicContract's emitEvent().
     * @param event The event to emit.
     * @throw DynamicException if there's an attempt to emit the event outside a contract call.
     */
    void emitContractEvent(Event&& event);

    /**
     * Get the balance from a given address. Calls populateBalance(), so
     * it's technically the same as getting the balance directly from State.
     * Does NOT consider the current transaction being processed, if there is one.
     * @param address The address to get the balance from.
     * @return The balance of the address.
     */
    uint256_t getBalanceFromAddress(const Address& address) const;

    /**
     * Send tokens to a given address. Used by DynamicContract to send tokens to other contracts.
     * @param from The address from which the tokens will be sent from.
     * @param to The address to send the tokens to.
     * @param amount The amount of tokens to send.
     */
    void sendTokens(const BaseContract* from, const Address& to, const uint256_t& amount);

    /**
     * Get the current nonce of a given Account
     * Returns a REFERENCE to the nonce, so it can be modified.
     * @param acc The address of the account to get the nonce from.
     */
    uint64_t& getNonce(const Address& acc);

    template <typename... Args, bool... Flags>
    void emitEvent(
      const std::string& name,
      const Address& contract,
      const std::tuple<EventParam<Args, Flags>...>& args,
      bool anonymous
    ) {
      Event event(name, // EVM events do not have names
        this->eventIndex_,
        this->txHash_,
        this->txIndex_,
        this->blockHash_,
        this->currentTxContext_.block_number,
        contract,
        args,
        anonymous
      );
      this->eventIndex_++;
      this->stack_.registerEvent(std::move(event));
    }

    void registerNewCPPContract(const Address& addr, BaseContract* contract);
    void registerNewEVMContract(const Address& addr, const uint8_t* code, size_t codeSize);
    void registerVariableUse(SafeBase& variable);

    uint256_t getRandomValue() const {
      return this->randomGen_.operator()();
    }
    /// END OF CONTRACT INTERFACING FUNCTIONS

};

#endif // CONTRACT_HOST_H
