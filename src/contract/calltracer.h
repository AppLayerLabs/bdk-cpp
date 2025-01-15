/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef CONTRACT_CALLTRACER_H
#define CONTRACT_CALLTRACER_H

#include "utils/utils.h"
#include "utils/options.h"
#include "contract/messages/concepts.h"
#include "contract/messages/outofgas.h"
#include "contract/trace/call.h"
#include "contract/messages/traits.h"
#include "contract/abi.h"

template<typename MessageHandler>
class CallTracer {
public:
  CallTracer(MessageHandler handler, IndexingMode indexingMode) 
    : handler_(std::move(handler)), rootCall_(), callStack_(), indexingMode_(indexingMode) {}

  template<concepts::CallMessage Message>
  decltype(auto) onMessage(Message&& msg) {
    using Result = traits::MessageResult<Message>;

    if (indexingMode_ != IndexingMode::RPC_TRACE) {
      return handler_.onMessage(std::forward<Message>(msg));
    }

    trace::Call& callTrace = callStack_.empty()
      ? *(rootCall_ = std::make_unique<trace::Call>())
      : callStack_.top()->calls.emplace_back();

    Gas& gas = msg.gas();
    
    callTrace.type = trace::getMessageCallType(msg);
    callTrace.status = trace::CallStatus::SUCCEEDED;
    callTrace.from = Address(msg.from());
    callTrace.to = Address(msg.to());
    callTrace.value = FixedBytes<32>(Utils::uint256ToBytes(messageValueOrZero(msg)));
    callTrace.gas = uint64_t(gas);

    try {
      callTrace.input = messageInputEncoded(msg);
    } catch (const std::exception& ignored) {}

    callStack_.push(&callTrace);

    try {
      if constexpr (not std::same_as<void, Result>) {
        Result result = handler_.onMessage(std::forward<Message>(msg));

        if constexpr (concepts::PackedMessage<Message>) {
          callTrace.output = ABI::Encoder::encodeData<Result>(result);
        } else {
          callTrace.output = result;
        }

        callTrace.gasUsed = callTrace.gas - uint64_t(gas);
        callStack_.pop();

        return result;
      }

      handler_.onMessage(std::forward<Message>(msg));
    } catch (const OutOfGas& outOfGas) {
      callTrace.status = trace::CallStatus::OUT_OF_GAS;
      callTrace.gasUsed = callTrace.gas - uint64_t(gas);
      callStack_.pop();

      throw outOfGas;
    } catch (const std::exception& error) {
      callTrace.status = trace::CallStatus::EXECUTION_REVERTED;

      if (error.what()) {
        try {
          callTrace.output = ABI::Encoder::encodeError(error.what());
        } catch (const std::exception& ignored) {}
      }

      callTrace.gasUsed = callTrace.gas - uint64_t(gas);
      callStack_.pop();

      throw error;
    }
  }

  decltype(auto) onMessage(concepts::CreateMessage auto&& msg) {
    return handler_.onMessage(std::forward<decltype(msg)>(msg));
  }

  const MessageHandler& handler() const { return handler_; }

  MessageHandler& handler() { return handler_; }

  bool hasCallTrace() const { return rootCall_ != nullptr; }

  const trace::Call& getCallTrace() const { return *rootCall_; }

private:
  MessageHandler handler_;
  std::unique_ptr<trace::Call> rootCall_;
  std::stack<trace::Call*> callStack_;
  IndexingMode indexingMode_;
};

#endif // CONTRACT_CALLTRACER_H
