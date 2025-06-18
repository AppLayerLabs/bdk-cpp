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

  template<concepts::Message M, typename R = traits::MessageResult<M>>
  R onMessage(M&& msg) {
    auto checkpoint = context_.checkpoint();

    if constexpr (std::same_as<R, void>) {
      dispatchMessage(std::forward<M>(msg));
      checkCppContractReverted();
      checkpoint.commit();
    } else {
      R result = dispatchMessage(std::forward<M>(msg));
      checkCppContractReverted();
      checkpoint.commit();
      return result;
    }
  }

  Address dispatchMessage(concepts::CreateMessage auto&& msg) requires concepts::EncodedMessage<decltype(msg)> {
    return evmExecutor_.execute(std::forward<decltype(msg)>(msg));
  }

  Address dispatchMessage(concepts::CreateMessage auto&& msg) requires concepts::PackedMessage<decltype(msg)> {
    return cppExecutor_.execute(std::forward<decltype(msg)>(msg));
  }

  decltype(auto) dispatchMessage(concepts::CallMessage auto&& msg) {
    if constexpr (!concepts::DelegateCallMessage<decltype(msg)>) {
      // If it is NOT a DELEGATECALL
      // We need to transfer the funds before executing the message
      transferFunds(msg);
    }

    if (precompiledExecutor_.isPrecompiled(msg.to())) {
      return precompiledExecutor_.execute(std::forward<decltype(msg)>(msg));
    }
    auto account = context_.getAccount(messageCodeAddress(msg));

    switch (account.getContractType()) {
      case ContractType::CPP:
        return dispatchCppCall(std::forward<decltype(msg)>(msg));
        break;

      case ContractType::EVM:
        return dispatchEvmCall(std::forward<decltype(msg)>(msg));
        break;
      default: {} // Do nothing
    }

    if constexpr (concepts::EncodedMessage<decltype(msg)>) {
      if (isPayment(msg)) {
        return Bytes();
      }
    }

    if constexpr (concepts::DelegateCallMessage<decltype(msg)>) {
      // Delegate calls can never be Packed so we dont need to check for PackedMessage
      throw DynamicException("Attempt to invoke non-contract or inexistent address from: " + msg.to().hex(true).get() + " to: " + msg.from().hex(true).get() + " calldata: " + Hex::fromBytes(msg.input()).forRPC() + " this was a delegated call with code address: " + msg.codeAddress().hex(true).get());
    }
    if constexpr (concepts::HasInputField<decltype(msg)>) {
      throw DynamicException("Attempt to invoke non-contract or inexistent address from: " + msg.to().hex(true).get() + " to: " + msg.from().hex(true).get() + " calldata: " + Hex::fromBytes(msg.input()).forRPC());
    }
    if constexpr (concepts::PackedMessage<decltype(msg)>) {
      // If it is a packed message, we need to decode the packed arguments
      if constexpr (std::is_same_v<decltype(msg.args()), std::tuple<>>) {
        throw DynamicException("Attempt to invoke non-contract or inexistent address from: " + msg.to().hex(true).get() + " to: " + msg.from().hex(true).get() + " apparently no calldata/arguments");
      } else {
        Bytes packedArgs = ABI::Encoder::encodeData(msg.args());
        throw DynamicException("Attempt to invoke non-contract or inexistent address from: " + msg.to().hex(true).get() + " to: " + msg.from().hex(true).get() + " calldata: " + Hex::fromBytes(packedArgs).forRPC());
      }
    }
    throw DynamicException("Attempt to invoke non-contract or inexistent address from: " + msg.to().hex(true).get() + " to: " + msg.from().hex(true).get() + " apparently no calldata/arguments");
  }

  decltype(auto) dispatchCppCall(auto&& msg) {
    try {
      return cppExecutor_.execute(std::forward<decltype(msg)>(msg));
    } catch (const std::exception&) {
      cppContractReverted_ = true;
      throw;
    }
  }

  decltype(auto) dispatchEvmCall(auto&& msg) {
    return evmExecutor_.execute(std::forward<decltype(msg)>(msg));
  }

  CppContractExecutor& cppExecutor() { return cppExecutor_; }

  EvmContractExecutor& evmExecutor() { return evmExecutor_; }

  PrecompiledContractExecutor& precompiledExecutor() { return precompiledExecutor_; }

private:
  void transferFunds(const concepts::Message auto& msg) {
    const uint256_t value = messageValueOrZero(msg);
    if (value > 0) {
      context_.transferBalance(msg.from(), msg.to(), value);
    }
  }

  bool isPayment(const concepts::EncodedMessage auto& msg) const {
    return msg.input().size() == 0;
  }

  bool isPayment(const concepts::PackedMessage auto& msg) const {
    return false;
  }

  void checkCppContractReverted() const {
    if (cppContractReverted_) [[unlikely]] {
      throw DynamicException("Reverted due to C++ call failure");
    }
  }

  ExecutionContext& context_;
  CppContractExecutor cppExecutor_;
  EvmContractExecutor evmExecutor_;
  PrecompiledContractExecutor precompiledExecutor_;
  bool cppContractReverted_ = false;
};

#endif // BDK_MESSAGES_MESSAGEDISPATCHER_H
