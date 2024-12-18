#ifndef BDK_MESSAGES_MESSAGEDISPATCHER_H
#define BDK_MESSAGES_MESSAGEDISPATCHER_H

#include "common.h"
#include "utils/utils.h"
#include "concepts.h"
#include "executioncontext.h"
#include "evmcontractexecutor.h"
#include "cppcontractexecutor.h"

class MessageDispatcher {
public:
  MessageDispatcher(ExecutionContext& context, CppContractExecutor cppExecutor, EvmContractExecutor evmExecutor)
    : context_(context), cppExecutor_(std::move(cppExecutor)), evmExecutor_(std::move(evmExecutor)) {}

  template<concepts::CallMessage M>
  decltype(auto) onMessage(M&& msg) {
    const Account& account = context_.getAccount(messageCodeAddress(msg));

    if (!account.isContract()) {
      throw DynamicException("Not a contract address");
    }

    if (account.contractType == ContractType::CPP) {
      return cppExecutor_.execute(std::forward<M>(msg));
    } else {
      return evmExecutor_.execute(std::forward<M>(msg));
    }
  }

  template<concepts::CreateMessage M>
  Address onMessage(M&& msg) {
    const Address result = std::invoke([&] () {
      if constexpr (concepts::EncodedMessage<M>) {
        return evmExecutor_.execute(std::forward<M>(msg));
      } else {
        return cppExecutor_.execute(std::forward<M>(msg));
      }
    });

    return result;
  }

  CppContractExecutor& cppExecutor() { return cppExecutor_; }

  EvmContractExecutor& evmExecutor() { return evmExecutor_; }

private:
  ExecutionContext& context_;
  CppContractExecutor cppExecutor_;
  EvmContractExecutor evmExecutor_;
};

#endif // BDK_MESSAGES_MESSAGEDISPATCHER_H
