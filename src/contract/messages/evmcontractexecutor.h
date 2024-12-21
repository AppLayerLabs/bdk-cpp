#ifndef BDK_MESSAGES_EVMCONTRACTEXECUTOR_H
#define BDK_MESSAGES_EVMCONTRACTEXECUTOR_H

#include <evmc/evmc.hpp>
#include "utils/hash.h"
#include "utils/contractreflectioninterface.h"
#include "contract/contractstack.h"
#include "anyencodedmessagehandler.h"
#include "executioncontext.h"
#include "traits.h"

class EvmContractExecutor : public evmc::Host {
public:
  EvmContractExecutor(
    AnyEncodedMessageHandler messageHandler, ExecutionContext& context, evmc_vm *vm)
      : messageHandler_(messageHandler), context_(context), vm_(vm), transientStorage_(), depth_(0) {}

  EvmContractExecutor(ExecutionContext& context, evmc_vm *vm)
      : context_(context), vm_(vm), transientStorage_(), depth_(0) {}

  void setMessageHandler(AnyEncodedMessageHandler messageHandler) { messageHandler_ = messageHandler; }

  Bytes execute(EncodedCallMessage& msg);

  Bytes execute(EncodedStaticCallMessage& msg);

  Bytes execute(EncodedDelegateCallMessage& msg);

  template<concepts::PackedMessage M>
    requires concepts::CallMessage<M>
  auto execute(M&& msg) -> traits::MessageResult<M> {
    // TODO: can this apply be a free common function for encoding not encoded messages?
    // TODO: what about the whole process of converting a packed msg to a encoded msg?
    const Bytes encodedInput = std::apply([&] <typename... Args> (const Args&... args) {
      const std::string functionName = ContractReflectionInterface::getFunctionName(msg.method());

      if (functionName.empty()) {
        throw DynamicException("EVM contract function name is empty (contract not registered?)");
      }

      Bytes res = Utils::makeBytes(Utils::uint32ToBytes(ABI::FunctorEncoder::encode<Args...>(functionName).value));

      if constexpr (sizeof...(Args) > 0) {
        Utils::appendBytes(res, ABI::Encoder::encodeData<Args...>(args...));
      }

      return res;
    }, msg.args());

    Bytes output;

    if constexpr (concepts::StaticCallMessage<M>) {
      EncodedStaticCallMessage encodedMessage(msg.from(), msg.to(), msg.gas(), encodedInput);
      output = this->execute(encodedMessage);
    } else if constexpr (concepts::DelegateCallMessage<M>) {
      EncodedDelegateCallMessage encodedMessage(msg.from(), msg.to(), msg.gas(), msg.value(), encodedInput, msg.codeAddress());
      output = this->execute(encodedMessage);
    } else {
      EncodedCallMessage encodedMessage(msg.from(), msg.to(), msg.gas(), msg.value(), encodedInput);
      output = this->execute(encodedMessage);
    }

    if constexpr (not std::same_as<traits::MessageResult<M>, void>) {
      return std::get<0>(ABI::Decoder::decodeData<traits::MessageResult<M>>(output));
    }
  }

  Address execute(EncodedCreateMessage& msg);

  Address execute(EncodedSaltCreateMessage& msg);

  decltype(auto) execute(auto&& msg) {
    return execute(msg);
  }

  bool account_exists(const evmc::address& addr) const noexcept override final;

  evmc::bytes32 get_storage(const evmc::address& addr, const evmc::bytes32& key) const noexcept override final;

  evmc_storage_status set_storage(const evmc::address& addr, const evmc::bytes32& key, const evmc::bytes32& value) noexcept override final;

  evmc::uint256be get_balance(const evmc::address& addr) const noexcept override final;

  size_t get_code_size(const evmc::address& addr) const noexcept override final;

  evmc::bytes32 get_code_hash(const evmc::address& addr) const noexcept override final;

  size_t copy_code(const evmc::address& addr, size_t code_offset, uint8_t* buffer_data, size_t buffer_size) const noexcept override final;

  bool selfdestruct(const evmc::address& addr, const evmc::address& beneficiary) noexcept override final;

  evmc_tx_context get_tx_context() const noexcept override final;

  evmc::bytes32 get_block_hash(int64_t number) const noexcept override final;

  void emit_log(const evmc::address& addr, const uint8_t* data, size_t data_size, const evmc::bytes32 topics[], size_t topics_count) noexcept override final;

  evmc_access_status access_account(const evmc::address& addr) noexcept override final;

  evmc_access_status access_storage(const evmc::address& addr, const evmc::bytes32& key) noexcept override final;

  evmc::bytes32 get_transient_storage(const evmc::address &addr, const evmc::bytes32 &key) const noexcept override final;

  void set_transient_storage(const evmc::address &addr, const evmc::bytes32 &key, const evmc::bytes32 &value) noexcept override final;

  evmc::Result call(const evmc_message& msg) noexcept override final;

private:
  AnyEncodedMessageHandler messageHandler_;
  ExecutionContext& context_;
  evmc_vm *vm_;
  boost::unordered_flat_map<StorageKey, Hash, SafeHash, SafeCompare> transientStorage_;
  uint64_t depth_;
};

#endif // BDK_MESSAGES_EVMCONTRACTEXECUTOR_H
