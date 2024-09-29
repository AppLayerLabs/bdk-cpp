#pragma once

#include "../utils/utils.h"
#include "cpp/callexecutor.h"

struct DummyHandler {
  Bytes executeCall(auto, Gas&, const evm::Message&) { throw std::runtime_error("TODO"); }

  template<typename M>
  typename M::ReturnType executeCall(auto, Gas&, const cpp::Message<M>&) { throw std::runtime_error("TODO"); }
};

class CallDispatcher {
public:
  using Accounts = boost::unordered_flat_map<Address, NonNullUniquePtr<Account>, SafeHash>;
  using TransferHandler = std::function<void(const Address&, const Address&, const uint256_t&)>;

  CallDispatcher(cpp::CallExecutor cppCallHandler, DummyHandler evmCallHandler, TransferHandler transferHandler, Accounts& accounts)
    : cppCallHandler_(std::move(cppCallHandler)), evmCallHandler_(std::move(evmCallHandler)), transferHandler_(std::move(transferHandler)), accounts_(accounts) {}

  decltype(auto) onCall(auto kind, Gas& gas, auto&& msg) {
    const auto it = accounts_.find(msg.to);

    if (it == accounts_.end()) {
      throw ExecutionFailure("Account not found");
    }

    const auto& account = it->second;

    if (!account->isContract()) {
      throw ExecutionFailure("Not a contract address");
    }

    if (msg.value) {
      transferHandler_(msg.from, msg.to, msg.value);
    }

    if (account->contractType == ContractType::CPP) {
      return cppCallHandler_.executeCall(kind, gas, std::forward<decltype(msg)>(msg));
    } else {
      return evmCallHandler_.executeCall(kind, gas, std::forward<decltype(msg)>(msg));
    }
  }

private:
  Accounts& accounts_;
  cpp::CallExecutor cppCallHandler_;
  DummyHandler evmCallHandler_;
  TransferHandler transferHandler_;
};
