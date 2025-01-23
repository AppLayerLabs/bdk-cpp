#ifndef BDK_MESSAGES_EXECUTIONREVERTED_H
#define BDK_MESSAGES_EXECUTIONREVERTED_H

#include "utils/dynamicexception.h"

namespace messages {

struct ExecutionReverted : std::runtime_error {
  explicit ExecutionReverted(std::string_view msg)
    : std::runtime_error(str.data()) {}
};

} // namespace messages

#endif  // BDK_MESSAGES_EXECUTIONREVERTED_H
