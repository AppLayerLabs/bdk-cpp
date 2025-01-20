#ifndef BDK_CONTRACT_TRACE_CALLSTATUS_H
#define BDK_CONTRACT_TRACE_CALLSTATUS_H

namespace trace {

enum class CallStatus {
  SUCCEEDED,
  EXECUTION_REVERTED,
  OUT_OF_GAS
};

} // namespace trace

#endif // BDK_CONTRACT_TRACE_CALLSTATUS_H
