#include "callexecutor.h"

constexpr decltype(auto) findAnd(auto&& map, const auto& key, auto andThen, auto orElse) noexcept {
  const auto it = map.find(key);

  if (it == map.end()) {
    return std::invoke(orElse);
  }

  return std::invoke(andThen, it->second);
}

static evmc::bytes32 thenToBytes32(const Hash& hash) noexcept {
  return hash.toEvmcBytes32();
}

template<typename T>
static T orDefault() noexcept {
  return T{};
}

namespace evm {

Bytes CallExecutor::executeCall(kind::Any callKind, Gas& gas, const Message& msg) {

}

bool CallExecutor::account_exists(const evmc::address& addr) const noexcept override final {
  return accounts_.find(addr) != accounts_.end();
}

evmc::bytes32 CallExecutor::get_storage(const evmc::address& addr, const evmc::bytes32& key) const noexcept override final {
  return findAnd(vmStorage_, StorageKey(addr, key), thenToBytes32, orDefault<evmc::bytes32>);
}

evmc_storage_status CallExecutor::set_storage(const evmc::address& addr, const evmc::bytes32& key, const evmc::bytes32& value) noexcept override final {
  const StorageKey storageKey(addr, key);
  auto& storageValue = vmStorage_[storageKey];
  stack_.registerStorageChange(storageKey, storageValue);
  storageValue = Hash(value);
  return EVMC_STORAGE_MODIFIED;
}

evmc::uint256be CallExecutor::get_balance(const evmc::address& addr) const noexcept override final {
  return findAnd(accounts_, addr,
    [] (const auto& account) { return Utils::uint256ToEvmcUint256(account->balance); },
    orDefault<evmc::uint256be>);
}

size_t CallExecutor::get_code_size(const evmc::address& addr) const noexcept override final {
  return findAnd(accounts_, addr,
    [] (const auto& account) { return account->code.size(); },
    orDefault<size_t>);
}

evmc::bytes32 CallExecutor::get_code_hash(const evmc::address& addr) const noexcept override final {
  return findAnd(accounts_, addr,
    [] (const auto& account) { return account->codeHash.toEvmcBytes32(); },
    orDefault<evmc::bytes32>);
}

size_t CallExecutor::copy_code(const evmc::address& addr, size_t code_offset, uint8_t* buffer_data, size_t buffer_size) const noexcept override final {
  return findAnd();
}

bool CallExecutor::selfdestruct(const evmc::address& addr, const evmc::address& beneficiary) noexcept override final {

}

evmc_tx_context CallExecutor::get_tx_context() const noexcept override final {

}

evmc::bytes32 CallExecutor::get_block_hash(int64_t number) const noexcept override final {

}

void CallExecutor::emit_log(const evmc::address& addr, const uint8_t* data, size_t data_size, const evmc::bytes32 topics[], size_t topics_count) noexcept override final {

}

evmc_access_status CallExecutor::access_account(const evmc::address& addr) noexcept override final {

}

evmc_access_status CallExecutor::access_storage(const evmc::address& addr, const evmc::bytes32& key) noexcept override final {

}

evmc::bytes32 CallExecutor::get_transient_storage(const evmc::address &addr, const evmc::bytes32 &key) const noexcept override final {

}

void CallExecutor::set_transient_storage(const evmc::address &addr, const evmc::bytes32 &key, const evmc::bytes32 &value) noexcept override final {

}

evmc::Result CallExecutor::call(const evmc_message& msg) noexcept override final {

}


} // namespace evm