#pragma once

#include "message.h"
#include "anycallhandler.h"
#include "../gas.h"
#include "../traits/method.h"
#include "../contractstack.h"
#include "../cpp/message.h"
#include "../../utils/contractreflectioninterface.h"

namespace evm {

class CallExecutor : public evmc::Host {
public:
  using VmStorage = boost::unordered_flat_map<StorageKey, Hash, SafeHash>;
  using Accounts = boost::unordered_flat_map<Address, NonNullUniquePtr<Account>, SafeHash>;

  CallExecutor(AnyCallHandler callHandler, evmc_vm* vm, VmStorage& vmStorage, Accounts& accounts, ContractStack& stack, const Hash& txHash, uint64_t txIndex, const Hash& blockHash, const evmc_tx_context& currentTxContext)
      : callHandler_(std::move(callHandler)), vm_(vm), vmStorage_(vmStorage), accounts_(accounts), stack_(stack), txHash_(txHash), txIndex_(txIndex), blockHash_(blockHash), currentTxContext_(currentTxContext) {}

  Bytes executeCall(kind::Any callKind, Gas& gas, const Message& msg, View<Bytes> code);

  template<typename M>
  M::ReturnType executeCall(auto callKind, Gas& gas, cpp::Message<M> msg, View<Bytes> code) {
    const Bytes input = std::apply([&] <typename... Args> (const Args&... args) {
      const std::string functionName = ContractReflectionInterface::getFunctionName(msg.method.func);

      if (functionName.empty()) {
        throw DynamicException("EVM contract function name is empty (contract not registered?)");
      }

      Bytes res = Utils::makeBytes(Utils::uint32ToBytes(ABI::FunctorEncoder::encode<Args...>(functionName).value));

      if constexpr (sizeof...(Args) > 0) {
        Utils::appendBytes(res, ABI::Encoder::encodeData<Args...>(args...));
      }

      return res;
    }, msg.method.args);

    const Message newMsg {
      .from = msg.from,
      .to = msg.to,
      .value = msg.value,
      .depth = msg.depth,
      .input = input
    };

    const Bytes output = executeCall(callKind, gas, newMsg, code);

    if constexpr (not std::same_as<typename M::ReturnType, void>) {
      return std::get<0>(ABI::Decoder::decodeData<typename M::ReturnType>(output));
    }
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
  AnyCallHandler callHandler_;
  evmc_vm* vm_;
  VmStorage& vmStorage_;
  Accounts& accounts_;
  boost::unordered_flat_map<StorageKey, Hash, SafeHash> transientStorage_;
  ContractStack& stack_;
  uint64_t eventIndex_ = 0;
  const Hash& txHash_;
  const uint64_t txIndex_;
  const Hash& blockHash_;
  const evmc_tx_context& currentTxContext_;
};

} // namespace evm
