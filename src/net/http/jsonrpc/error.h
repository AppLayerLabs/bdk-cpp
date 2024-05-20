#ifndef JSONRPC_ERROR_H
#define JSONRPC_ERROR_H

#include <string_view>
#include <stdexcept>
#include <format>

namespace jsonrpc {

class Error : public std::exception {
private:
  int code_;
  std::string message_;

public:
  Error(int code, std::string message)
    : code_(code), message_(std::move(message)) {}

  int code() const noexcept { return code_; }

  std::string_view message() const noexcept { return message_; }

  static Error invalidType(std::string_view exp, std::string_view got) {
    return Error(-32601, std::format("Parsing error: invalid type, exp '{}' - got '{}'", exp, got));
  }

  static Error invalidFormat(std::string_view wrong) {
    return Error(-32601, std::format("Parsing error: '{}' is in invalid format", wrong));
  }

  static Error insufficientValues() {
    return Error(-32601, "Parsing error: insufficient values in array");
  }

  static Error exectionError(std::string_view cause) {
    return Error(-32603, std::string("Execution error: ") + cause.data());
  }
};

} // namespace jsonrpc

#endif // JSONRPC_ERROR_H
