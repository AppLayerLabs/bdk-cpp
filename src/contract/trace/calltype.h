#ifndef BDK_CONTRACT_TRACE_CALLTYPE_H
#define BDK_CONTRACT_TRACE_CALLTYPE_H

#include "contract/concepts.h"

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

} // namespace trace


#endif // BDK_CONTRACT_TRACE_CALLTYPE_H
