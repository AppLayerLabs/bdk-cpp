#ifndef CONTRACT_CALLTRACER_H
#define CONTRACT_CALLTRACER_H

#include <memory>
#include <list>
#include <boost/container/stable_vector.hpp>
#include "../utils/utils.h"
#include "../utils/strings.h"
#include "../libs/zpp_bits.h"

namespace trace {

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
  Address from;
  Address to;
  FixedBytes<32> value;
  uint64_t gas;
  uint64_t gasUsed;
  Bytes input;
  Bytes output;
  std::string error;
  boost::container::stable_vector<Call> calls;
};

class CallTracer {
private:
  std::unique_ptr<Call> root_;
  std::deque<Call*> stack_;

  void traceOutInternal(bytes::View output, uint64_t gasUsed, std::string error);

public:
  CallTracer() = default;

  explicit CallTracer(Call rootCall);

  bool hasCalls() const noexcept { return bool(root_); }

  bool isFinished() const noexcept { return root_ != nullptr && stack_.empty(); }

  const Call& root() const;

  const Call& current() const;

  void traceIn(Call call);

  void traceOut(bytes::View output, uint64_t gasUsed);

  void traceError(std::string error, uint64_t gasUsed);
};

} // namespace trace

#endif // CONTRACT_CALLTRACER_H
