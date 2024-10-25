/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef CONTRACT_CALLTRACER_H
#define CONTRACT_CALLTRACER_H

#include <boost/container/stable_vector.hpp>

#include "../utils/utils.h" // strings.h, bytes/join.h (used in .cpp) libs/zpp_bits.h -> memory

namespace trace {

Bytes encodeRevertReason(std::string_view reason);

std::string decodeRevertReason(bytes::View data);

enum class Status {
  SUCCEEDED,
  EXECUTION_REVERTED,
  OUT_OF_GAS
};

struct Call {
  enum class Type {
    CALL,
    STATICCALL,
    DELEGATECALL
  };

  Call() = default;

  explicit Call(const evmc_message& msg);

  json toJson() const;

  using serialize = zpp::bits::members<10>;

  Type type;
  Status status;
  Address from;
  Address to;
  FixedBytes<32> value;
  uint64_t gas;
  uint64_t gasUsed;
  Bytes input;
  Bytes output;
  boost::container::stable_vector<Call> calls;
};

class CallTracer {
private:
  std::unique_ptr<Call> root_;
  std::deque<Call*> stack_;

  void push(Call call);

  void pop(Bytes output, Status status, uint64_t gasUsed);

public:
  CallTracer() = default;

  explicit CallTracer(Call rootCall);

  bool hasCalls() const noexcept { return bool(root_); }

  bool isFinished() const noexcept { return root_ != nullptr && stack_.empty(); }

  const Call& root() const;

  const Call& current() const;

  void callStarted(Call call);

  void callOutOfGas();

  void callReverted(uint64_t gasUsed);

  void callReverted(Bytes output, uint64_t gasUsed);

  void callSucceeded(Bytes output, uint64_t gasUsed);
};

} // namespace trace

#endif // CONTRACT_CALLTRACER_H
