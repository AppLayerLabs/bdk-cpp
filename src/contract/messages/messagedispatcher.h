#ifndef BDK_MESSAGES_MESSAGEDISPATCHER_H
#define BDK_MESSAGES_MESSAGEDISPATCHER_H

#include "common.h"
#include "bytes/hex.h"
#include "utils/utils.h"
#include "concepts.h"
#include "executioncontext.h"
#include "evmcontractexecutor.h"
#include "cppcontractexecutor.h"
#include "precompiledcontractexecutor.h"

class MessageDispatcher {
public:
  MessageDispatcher(ExecutionContext& context, CppContractExecutor cppExecutor, EvmContractExecutor evmExecutor, PrecompiledContractExecutor precompiledExecutor)
    : context_(context), cppExecutor_(std::move(cppExecutor)), evmExecutor_(std::move(evmExecutor)), precompiledExecutor_(std::move(precompiledExecutor)) {}

  template<concepts::CallMessage M>
  decltype(auto) onMessage(M&& msg) {
    using Result = traits::MessageResult<M>;

    View<Address> codeAddress = messageCodeAddress(msg);

    if (isPrecompiled(codeAddress)) {
      return precompiledExecutor_.execute(std::forward<M>(msg));
    }

    const Account& account = context_.getAccount(codeAddress);

    if (!account.isContract()) {
      throw DynamicException("Not a contract address");
    }

    auto checkpoint = context_.checkpoint();

    if constexpr (concepts::HasValueField<M>) {
      if (msg.value() > 0) {
        context_.transferBalance(msg.from(), msg.to(), msg.value());
      }
    }

    // TODO: to much code repetition, you can do better than this.
    if (account.contractType == ContractType::CPP) {
      if constexpr (std::same_as<Result, void>) {
        cppExecutor_.execute(std::forward<M>(msg));
        checkpoint.commit();
        return;
      } else {
        decltype(auto) result = cppExecutor_.execute(std::forward<M>(msg));
        checkpoint.commit();
        return result;
      }
    } else {
      if constexpr (std::same_as<Result, void>) {
        evmExecutor_.execute(std::forward<M>(msg));
        checkpoint.commit();
        return;
      } else {
        decltype(auto) result = evmExecutor_.execute(std::forward<M>(msg));
        checkpoint.commit();
        return result;
      }
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

  PrecompiledContractExecutor& precompiledExecutor() { return precompiledExecutor_; }

private:
  bool isPrecompiled(View<Address> address) const {
    constexpr Address randomGeneratorAddress = bytes::hex("0x1000000000000000000000000000100000000001");
    return address == randomGeneratorAddress;
  }

  ExecutionContext& context_;
  CppContractExecutor cppExecutor_;
  EvmContractExecutor evmExecutor_;
  PrecompiledContractExecutor precompiledExecutor_;
};

#endif // BDK_MESSAGES_MESSAGEDISPATCHER_H
