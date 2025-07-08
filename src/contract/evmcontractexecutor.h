#ifndef BDK_MESSAGES_EVMCONTRACTEXECUTOR_H
#define BDK_MESSAGES_EVMCONTRACTEXECUTOR_H

#include <evmc/evmc.hpp>
#include "utils/hash.h"
#include "utils/contractreflectioninterface.h"
#include "contract/contractstack.h"
#include "anyencodedmessagehandler.h"
#include "executioncontext.h"
#include "traits.h"
#include "utils/options.h"

class EvmContractExecutor : public evmc::Host {
public:
  EvmContractExecutor(
    AnyEncodedMessageHandler messageHandler, ExecutionContext& context, evmc_vm *vm, IndexingMode indexingMode)
      : messageHandler_(messageHandler), context_(context), vm_(vm), transientStorage_(), depth_(0), indexingMode_(indexingMode), deepestError_(nullptr) {}

  EvmContractExecutor(ExecutionContext& context, evmc_vm *vm, IndexingMode indexingMode)
      : context_(context), vm_(vm), transientStorage_(), depth_(0), indexingMode_(indexingMode), deepestError_(nullptr) {}

  void setMessageHandler(AnyEncodedMessageHandler messageHandler) { messageHandler_ = messageHandler; }

  Bytes execute(EncodedCallMessage& msg);

  Bytes execute(EncodedStaticCallMessage& msg);

  Bytes execute(EncodedDelegateCallMessage& msg);

  template<concepts::PackedMessage M>
    requires concepts::CallMessage<M>
  auto execute(M&& msg) -> traits::MessageResult<M> {
    const Bytes input = messageInputEncoded(msg);
    Bytes output;

    if constexpr (concepts::StaticCallMessage<M>) {
      EncodedStaticCallMessage encodedMessage(msg.from(), msg.to(), msg.gas(), input);
      output = this->execute(encodedMessage);
    } else if constexpr (concepts::DelegateCallMessage<M>) {
      EncodedDelegateCallMessage encodedMessage(msg.from(), msg.to(), msg.gas(), msg.value(), input, msg.codeAddress());
      output = this->execute(encodedMessage);
    } else {
      EncodedCallMessage encodedMessage(msg.from(), msg.to(), msg.gas(), msg.value(), input);
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
  IndexingMode indexingMode_;
  uint64_t depth_;
  std::unique_ptr<VMExecutionError> deepestError_;
  Bytes executeEvmcMessage(evmc_vm* vm, const evmc_host_interface* host, evmc_host_context* context, const evmc_message& msg, Gas& gas, View<Bytes> code);
  void createContractImpl(auto& msg, ExecutionContext& context, View<Address> contractAddress, evmc_vm *vm, evmc::Host& host, uint64_t depth);
};

#endif // BDK_MESSAGES_EVMCONTRACTEXECUTOR_H
