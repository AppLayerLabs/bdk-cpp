#pragma once

#include "message.h"
#include "anycallhandler.h"

namespace evm {

class CallExecutor : public evmc::Host {
public:
  using VmStorage = boost::unordered_flat_map<StorageKey, Hash, SafeHash>;
  using Accounts = boost::unordered_flat_map<Address, NonNullUniquePtr<Account>, SafeHash>;

  CallExecutor(AnyCallHandler callHandler, evmc_vm* vm, VmStorage& vmStorage, Accounts& accounts, ContractStack& stack, const Hash& txHash, const Hash& blockHash, const evmc_tx_context& currentTxContext)
      : callHandler_(std::move(callHandler)), vm_(vm), vmStorage_(vmStorage), accounts_(accounts), stack_(stack), txHash_(txHash), blockHash_(blockHash), currentTxContext_(currentTxContext) {}

  // Bytes executeCall(auto& callHandler, auto kind, Gas& gas, const auto& msg) {    
  // }

  // Bytes executeCall(auto&& callHandler, auto kind, Gas& gas, const CallMessage& msg, bytes::View code) {
  //   static constexpr auto getKind = Utils::Overloaded{
  //     [] (kind::Normal) { return EVMC_CALL; },
  //     [] (kind::Static) { return EVMC_CALL; },
  //     [] (kind::Delegate) { return EVMC_DELEGATECALL; }
  //   };

  //   static constexpr auto getFlags = Utils::Overloaded{
  //     [] (kind::Normal) -> std::uint32_t { return 0; },
  //     [] (kind::Static) -> std::uint32_t { return EVMC_STATIC; },
  //     [] (kind::Delegate) -> std::uint32_t { return 0; }
  //   };

  //   const evmc_message evmcMessage{
  //     .kind = getKind(kind),
  //     .flags = getFlags(kind),
  //     .depth = 0,
  //     .gas = gas.value(),
  //     .recipient = msg.to.toEvmcAddress(),
  //     .sender = msg.from.toEvmcAddress(),
  //     .input_data = msg.input.data(),
  //     .input_size = msg.input.size(),
  //     .value = Utils::uint256ToEvmcUint256(msg.value),
  //     .create2_salt = {},
  //     .code_address = {}
  //   };

  //   return executeCallImpl(evmcMessage, gas, code, );
  // }

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
  const Hash& blockHash_;
  const evmc_tx_context& currentTxContext_;
};

} // namespace evm

