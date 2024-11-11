#pragma once

#include "../../utils/strings.h"

namespace evm {

struct Message {
  Address from;
  Address to;
  uint256_t value;
  uint32_t depth;
  View<Bytes> input;
};

struct CreateMessage {
  Address from;
  uint256_t value;
  uint32_t depth;
  View<Bytes> code;
  std::optional<Hash> salt;
};

} // namespace evm
