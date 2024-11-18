/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef CONTRACT_CALLTRACER_H
#define CONTRACT_CALLTRACER_H

#include <boost/container/stable_vector.hpp>

#include "../utils/utils.h" // strings.h, bytes/join.h (used in .cpp) libs/zpp_bits.h -> memory

/// Namespace for tracing-related classes and functions.
namespace trace {

/**
 * Encode the reason for a call being reverted into raw bytes.
 * @param reason The revert reason to encode.
 * @return The revert reason as a raw bytes string.
 */
Bytes encodeRevertReason(std::string_view reason);

/**
 * Decode the raw bytes string that states the reason for a call being reverted.
 * @param data The data to decode.
 * @return The revert reason as a readable string.
 */
std::string decodeRevertReason(bytes::View data);

/// Enum for the call's status.
enum class Status { SUCCEEDED, EXECUTION_REVERTED, OUT_OF_GAS };

/// Abstraction of a contract call.
struct Call {
  using serialize = zpp::bits::members<10>; ///< Typedef for the serialization struct.

  /// Enum for the call's type.
  enum class Type { CALL, STATICCALL, DELEGATECALL };

  Type type;  ///< The call's type.
  Status status;  ///< The call's status.
  Address from; ///< The address that made the call.
  Address to; ///< The address that received the call.
  FixedBytes<32> value; ///< The value spent during the call.
  uint64_t gas; ///< The call's gas limit.
  uint64_t gasUsed; ///< The call's cumulative used gas.
  Bytes input; ///< The call's input data.
  Bytes output; ///< The call's output data.
  boost::container::stable_vector<Call> calls;  ///< A list of nested calls originated from this one.

  Call() = default; ///< Constructor (default).

  /**
   * Constructor based on a call message.
   * @param msg The call message to parse.
   */
  explicit Call(const evmc_message& msg);

  json toJson() const; ///< Serialize the call data to a JSON object.
};

/**
 * Get the respective call type from a given message.
 * @param msg The message to parse.
 * @return The call type.
 */
static Call::Type getCallType(const evmc_message& msg) {
  using enum Call::Type;
  if (msg.kind == EVMC_CALL) return (msg.flags == EVMC_STATIC) ? STATICCALL : CALL;
  if (msg.kind == EVMC_DELEGATECALL) return DELEGATECALL;
  throw DynamicException("evmc_message is not from a function call");
}

/// Abstraction of a contract call tracer.
class CallTracer {
  private:
    std::unique_ptr<Call> root_;  ///< The root call (the one that initiated the trace).
    std::deque<Call*> stack_; ///< The list of subsequent calls that are being traced.

    /**
     * Add a call to the stack.
     * @param call The call to add.
     */
    void push(Call call);

    /**
     * Remove a call from the stack.
     * @param output The output of the call.
     * @param status The status of the call.
     * @param gasUsed The cumulative gas used in the call.
     * @throw DynamicException if a trace hasn't started yet (stack is empty).
     */
    void pop(Bytes output, Status status, uint64_t gasUsed);

  public:
    CallTracer() = default; ///< Constructor (default).

    /**
     * Constructor based on a root call.
     * @param rootCall The call that starts the trace.
     */
    explicit CallTracer(Call rootCall);

    /**
     * Check if calls are being traced at the moment.
     * @return `true` if tracer has a root call, `false` otherwise.
     */
    bool hasCalls() const noexcept { return bool(root_); }

    /**
     * Check if a call trace has finished (no root call, stack is empty).
     * @return `true` is call trace has finished, `false` otherwise.
     */
    bool isFinished() const noexcept { return root_ != nullptr && stack_.empty(); }

    const Call& root() const; ///< Getter.
    const Call& current() const;  ///< Getter.

    /**
     * Signal that a call has started (push into the stack).
     * @param call The call to signal.
     */
    void callStarted(Call call);

    /// Signal that a call has run out of gas (pop out of the stack).
    void callOutOfGas();

    /**
     * Signal that a call was reverted (pop out of stack).
     * @param gasUsed The cumulative gas used by the call.
     */
    void callReverted(uint64_t gasUsed);

    /**
     * Overload of callReverted() that accepts the call's output bytes.
     * @param output The call's output bytes.
     * @param gasUsed The cumulative gas used by the call.
     */
    void callReverted(Bytes output, uint64_t gasUsed);

    /**
     * Signal that a call has succeeded (pop out of stack).
     * @param output The call's output bytes.
     * @param gasUsed The cumulative gas used by the call.
     */
    void callSucceeded(Bytes output, uint64_t gasUsed);
};

} // namespace trace

#endif // CONTRACT_CALLTRACER_H
