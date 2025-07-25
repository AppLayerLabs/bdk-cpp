#ifndef BDK_CONTRACT_TRACE_CALL_H
#define BDK_CONTRACT_TRACE_CALL_H

#include <boost/container/stable_vector.hpp>
#include "libs/zpp_bits.h"
#include "utils/bytes.h"
#include "utils/fixedbytes.h"
#include "utils/address.h"
#include "utils/utils.h"
#include "callstatus.h"
#include "calltype.h"

namespace trace {

struct Call {
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

#endif // BDK_CONTRACT_TRACE_CALL_H
