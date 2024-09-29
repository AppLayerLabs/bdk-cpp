#pragma once

#include "message.h"
#include "../gas.h"
#include "../traits/method.h"
#include "../evm/message.h"
// #include "../dynamiccontract.h"

struct ContractHost;

namespace cpp {

class NestedCallSafeGuard {
public:
  NestedCallSafeGuard(const ContractLocals* contract, const Address& caller, const uint256_t& value) :
    contract_(contract), caller_(contract->caller_), value_(contract->value_) {
  }

  ~NestedCallSafeGuard() {
    contract_->caller_ = caller_;
    contract_->value_ = value_;
  }
private:
  const ContractLocals* contract_;
  Address caller_;
  uint256_t value_;
};

class CallExecutor {
public:
  using Contracts = boost::unordered_flat_map<Address, std::unique_ptr<BaseContract>, SafeHash>&;

  CallExecutor(ContractHost& host, Contracts& contracts)
    : host_(host), contracts_(contracts) {}

  template<typename CallKind, typename M>
  decltype(auto) executeCall(CallKind, Gas& gas, Message<M> msg) {
    if constexpr (std::same_as<CallKind, kind::Delegate>) {
      throw std::runtime_error("TODO: delegate not allowed for C++ contracts");
    }

    gas.use(1000);

    NestedCallSafeGuard guard(msg.caller, msg.caller->caller_, msg.caller->value_);

    auto *contract = getContract<typename M::ClassType>(msg.to);

    // TODO: check nullptr

    auto binded = [&] (auto&&... args) {
      if constexpr (std::same_as<CallKind, kind::Static>) {
        return std::invoke(msg.method.func, *contract, std::forward<decltype(args)>(args)...);
      } else {
        return contract->callContractFunction(&host_, msg.method.func, std::forward<decltype(args)>(args)...);
      }
    };

    return std::apply(binded, std::move(msg.method.args));
  }

  decltype(auto) executeCall(kind::Static, Gas& gas, const evm::Message& msg) {
    throw std::runtime_error("TODO");
  }

  Bytes executeCall(kind::Normal, Gas& gas, const evm::Message& msg);

  decltype(auto) executeCall(kind::Delegate, Gas& gas, const evm::Message& msg) {
    throw std::runtime_error("TODO");
  }

private:
  template<typename C>
  C* getContract(const Address& address) {
    const auto it = contracts_.find(address);

    if (it == contracts_.end()) {
      throw ExecutionFailure("Contract not found"); // TODO: add more error info
    }

    C* ptr = dynamic_cast<C*>(it->second.get());

    if (ptr == nullptr) {
      throw ExecutionFailure("Contract is not of the requested type"); // TODO: add more error info
    }

    return ptr;
  }

  ContractHost& host_;
  Contracts& contracts_;
};

} // namespace cpp
