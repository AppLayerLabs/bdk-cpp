#ifndef JSONRPC_ERROR_H
#define JSONRPC_ERROR_H

#include <string_view>
#include <stdexcept>
#include <format>

namespace jsonrpc {

/// @brief JSON RPC error, refer to https://www.jsonrpc.org/specification#error_object
class Error : public std::exception {
private:
  int code_;
  std::string message_;

public:
  /// @brief error constructor
  /// @param code the error code
  /// @param message the error message
  Error(int code, std::string message)
    : code_(code), message_(std::move(message)) {}

  /// @brief returns the error code
  /// @return the error code
  int code() const noexcept { return code_; }

  /// @brief returns the error message
  /// @return the error message
  std::string_view message() const noexcept { return message_; }

  /// @brief constructs a "invalid type" parsing error
  /// @return the error object with a user friendly message
  static Error invalidType(std::string_view exp, std::string_view got) {
    return Error(-32601, std::format("Parsing error: invalid type, exp '{}' - got '{}'", exp, got));
  }

  /// @brief constructs a "invalid format" parsing error
  /// @return the error object with a user friendly message
  static Error invalidFormat(std::string_view wrong) {
    return Error(-32601, std::format("Parsing error: '{}' is in invalid format", wrong));
  }

  /// @brief constructs a "insufficient values" parsing error for arrays
  /// @return the error object with a user friendly message
  static Error insufficientValues() {
    return Error(-32601, "Parsing error: insufficient values in array");
  }

  /// @brief constructs a generic interal exection error
  /// @return the error object with a user friendly message
  static Error exectionError(std::string_view cause) {
    return Error(-32603, std::string("Execution error: ") + cause.data());
  }
};

} // namespace jsonrpc

#endif // JSONRPC_ERROR_H
