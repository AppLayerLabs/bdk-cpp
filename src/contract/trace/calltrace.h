#ifndef BDK_CONTRACT_CALLTRACE_H
#define BDK_CONTRACT_CALLTRACE_H

#include <boost/container/stable_vector.hpp>
#include "libs/zpp_bits.h"
#include "utils/bytes.h"
#include "utils/fixedbytes.h"
#include "utils/address.h"
#include "utils/utils.h"
#include "contract/messages/concepts.h"

namespace trace {

enum class CallType {
  CALL,
  STATICCALL,
  DELEGATECALL
};

template<concepts::CallMessage M>
constexpr CallType getMessageCallType(const M& msg) {
  if constexpr (concepts::DelegateCallMessage<M>) {
    return CallType::DELEGATECALL;
  } else if (concepts::StaticCallMessage<M>) {
    return CallType::STATICCALL;
  } else {
    return CallType::CALL;
  }
}

enum class CallStatus {
  SUCCEEDED,
  EXECUTION_REVERTED,
  OUT_OF_GAS
};

struct Call {
  Call() = default;

  explicit Call(const concepts::CallMessage auto& msg)
    : type(getMessageCallType(msg)),
      status(CallStatus::SUCCEEDED),
      from(msg.from()),
      to(msg.to()),
      value(Utils::uint256ToBytes(messageValueOrZero(msg))),
      gas(msg.gas().value()),
      gasUsed(0),
      input(messageInputEncoded(msg)),
      output(),
      calls() {}

  json toJson() const;

  using serialize = zpp::bits::members<10>;

  CallType type;
  CallStatus status;
  Address from;
  Address to;
  FixedBytes<32> value;
  uint64_t gas;
  uint64_t gasUsed;
  Bytes input;
  Bytes output;
  boost::container::stable_vector<Call> calls;
};

} // namespace trace

#endif // BDK_CONTRACT_CALLTRACE_H
