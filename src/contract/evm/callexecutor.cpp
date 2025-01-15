#include "callexecutor.h"
#include "bytes/cast.h"

template<typename M>
using map_value_t = decltype(std::declval<std::ranges::range_reference_t<M>>().second);

template<typename M>
using map_value_reference_t = std::add_lvalue_reference_t<
  std::conditional_t<
    std::is_const_v<std::remove_reference_t<M>>,
    std::add_const_t<map_value_t<M>>,
    map_value_t<M>>>;

template<std::ranges::borrowed_range M>
constexpr std::optional<std::reference_wrapper<std::remove_reference_t<map_value_reference_t<M>>>> get(M&& map, const auto& key) noexcept {
  const auto it = map.find(key);

  if (it == map.end()) {
    return std::nullopt;
  }

  return it->second;
}

constexpr evmc_call_kind getCallKind(kind::Any callKind) noexcept {
  return std::visit(Utils::Overloaded{
    [] (kind::Delegate) { return EVMC_DELEGATECALL; },
    [] (kind::Normal) { return EVMC_CALL; },
    [] (kind::Static) { return EVMC_CALL; }
  }, callKind);
}

constexpr uint32_t getCallFlags(kind::Any callKind) noexcept {
  return std::visit(Utils::Overloaded{
    [] (kind::Delegate) -> uint32_t { return 0; },
    [] (kind::Normal) -> uint32_t { return 0; },
    [] (kind::Static) -> uint32_t { return EVMC_STATIC; }
  }, callKind);
}

namespace evm {

Bytes CallExecutor::executeCall(kind::Any callKind, Gas& gas, const Message& msg, View<Bytes> code) {
  const evmc_message evmcMsg{
    .kind = getCallKind(callKind),
    .flags = getCallFlags(callKind),
    .depth = msg.depth,
    .gas = int64_t(gas),
    .recipient = bytes::cast<evmc_address>(msg.to),
    .sender = bytes::cast<evmc_address>(msg.from),
    .input_data = msg.input.data(),
    .input_size = msg.input.size(),
    .value = EVMCConv::uint256ToEvmcUint256(msg.value),
    .create2_salt = {},
    .code_address = {}
  };

  evmc::Result result(::evmc_execute(this->vm_,
                                     &this->get_interface(),
                                     this->to_context(),
                                     evmc_revision::EVMC_LATEST_STABLE_REVISION,
                                     &evmcMsg,
                                     code.data(),
                                     code.size()));

  gas.use(evmcMsg.gas - result.gas_left);

  View<Bytes> output(result.output_data, result.output_size);

  switch (result.status_code) {
    case EVMC_SUCCESS:
      return Utils::makeBytes(output);

    case EVMC_REVERT:
      throw ExecutionReverted("TODO");
    
    case EVMC_OUT_OF_GAS:
      throw OutOfGas();

    default:
      throw ExecutionFailure("TODO");
  }
}

bool CallExecutor::account_exists(const evmc::address& addr) const noexcept {
  return accounts_.find(addr) != accounts_.end();
}

evmc::bytes32 CallExecutor::get_storage(const evmc::address& addr, const evmc::bytes32& key) const noexcept {
  return get(vmStorage_, StorageKey(addr, key))
    .transform([] (const Hash& hash) { return bytes::cast<evmc::bytes32>(hash); })
    .value_or(evmc::bytes32{});
}

evmc_storage_status CallExecutor::set_storage(const evmc::address& addr, const evmc::bytes32& key, const evmc::bytes32& value) noexcept {
  const StorageKey storageKey(addr, key);
  auto& storageValue = vmStorage_[storageKey];
  stack_.registerStorageChange(storageKey, storageValue);
  storageValue = Hash(value);
  return EVMC_STORAGE_MODIFIED;
}

evmc::uint256be CallExecutor::get_balance(const evmc::address& addr) const noexcept {
  return get(accounts_, addr)
    .transform([] (const auto& account) { return EVMCConv::uint256ToEvmcUint256(account.get()->balance); })
    .value_or(evmc::uint256be{});
}

size_t CallExecutor::get_code_size(const evmc::address& addr) const noexcept {
  return get(accounts_, addr)
    .transform([] (const auto& account) { return account.get()->code.size(); })
    .value_or(0);
}

evmc::bytes32 CallExecutor::get_code_hash(const evmc::address& addr) const noexcept {
  return get(accounts_, addr)
    .transform([] (const auto& account) { return bytes::cast<evmc::bytes32>(account.get()->codeHash); })
    .value_or(evmc::bytes32{});
}

size_t CallExecutor::copy_code(const evmc::address& addr, size_t code_offset, uint8_t* buffer_data, size_t buffer_size) const noexcept {
  const auto thenCopyCode = [&] (const auto& account) -> size_t {
    View<Bytes> code = account.get()->code;

    if (code_offset < code.size()) {
      const size_t n = std::min(buffer_size, code.size() - code_offset);
      if (n > 0)
        std::copy_n(&code[code_offset], n, buffer_data);
      return n;
    }

    return 0;
  };

  return get(accounts_, addr)
    .transform(thenCopyCode)
    .value_or(0);
}

bool CallExecutor::selfdestruct(const evmc::address& addr, const evmc::address& beneficiary) noexcept {
  return false;
}

evmc_tx_context CallExecutor::get_tx_context() const noexcept {
  return currentTxContext_;
}

evmc::bytes32 CallExecutor::get_block_hash(int64_t number) const noexcept {
  return EVMCConv::uint256ToEvmcUint256(number);
}

void CallExecutor::emit_log(const evmc::address& addr, const uint8_t* data, size_t data_size, const evmc::bytes32 topics[], size_t topics_count) noexcept {
  try {
    std::vector<Hash> topics_;
    for (uint64_t i = 0; i < topics_count; i++) {
      topics_.emplace_back(topics[i]);
    }
    Event event("", // EVM events do not have names
                this->eventIndex_,
                this->txHash_,
                this->txIndex_,
                this->blockHash_,
                this->currentTxContext_.block_number,
                addr,
                Bytes(data, data + data_size),
                topics_,
                (topics_count == 0)
      );
    ++this->eventIndex_;
    this->stack_.registerEvent(std::move(event));
  } catch (const std::exception& ignored) {}
}

evmc_access_status CallExecutor::access_account(const evmc::address& addr) noexcept {
  return EVMC_ACCESS_WARM;
}

evmc_access_status CallExecutor::access_storage(const evmc::address& addr, const evmc::bytes32& key) noexcept {
  return EVMC_ACCESS_WARM;
}

evmc::bytes32 CallExecutor::get_transient_storage(const evmc::address &addr, const evmc::bytes32 &key) const noexcept {
  return get(transientStorage_, StorageKey(addr, key))
    .transform([] (const Hash& hash) { return bytes::cast<evmc::bytes32>(hash); })
    .value_or(evmc::bytes32{});
}

void CallExecutor::set_transient_storage(const evmc::address &addr, const evmc::bytes32 &key, const evmc::bytes32 &value) noexcept {
  transientStorage_.emplace(StorageKey(addr, key), value);
}

evmc::Result CallExecutor::call(const evmc_message& msg) noexcept {
  Message callMsg;
  CreateMessage createMsg;
  evmc_status_code status;
  std::variant<Bytes, Address> result = Bytes();
  kind::Any callKind;

  Gas gas(msg.gas);

  if (msg.kind == EVMC_DELEGATECALL) {
    callKind = kind::DELEGATE;
  } else if (msg.flags == EVMC_STATIC) {
    callKind = kind::STATIC;
  } else {
    callKind = kind::NORMAL;
  }

  try {
    switch (msg.kind) {
      case EVMC_CALL:
      case EVMC_DELEGATECALL:
        callMsg.from = Address(msg.sender);
        callMsg.to = Address(msg.recipient);
        callMsg.value = EVMCConv::evmcUint256ToUint256(msg.value);
        callMsg.depth = msg.depth;
        callMsg.input = View<Bytes>(msg.input_data, msg.input_size);
        result = callHandler_.onCall(callKind, gas, callMsg);
        break;

      case EVMC_CREATE2:
        [[fallthrough]];

      case EVMC_CREATE:
        [[fallthrough]];

      case EVMC_CALLCODE:
        assert(false); // TODO
    }

    status = EVMC_SUCCESS;

  } catch (const OutOfGas&) {
    status = EVMC_OUT_OF_GAS;
  } catch (const ExecutionReverted&) {
    // TODO: encode error if exists
    status = EVMC_REVERT;
  } catch (const std::exception&) {
    // TODO: encode error if exists
    status = EVMC_FAILURE;
  }

  return std::visit(Utils::Overloaded{
    [&] (Bytes output) { return evmc::Result(status, int64_t(gas), 0, output.data(), output.size()); },
    [&] (const Address& createAddress) { return evmc::Result(status, int64_t(gas), 0, createAddress.toEvmcAddress()); },
  }, std::move(result));
}

} // namespace evm
