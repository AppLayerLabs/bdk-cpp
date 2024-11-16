/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef DYNAMIC_EXCEPTION_H
#define DYNAMIC_EXCEPTION_H

#include <chrono> // ctime
#include <sstream> // ostringstream

/// Abstraction of a custom exception class for dynamic message building and timestamping.
class DynamicException : public std::exception {
  private:
    std::string message_;   ///< The exception message.
    std::string timestamp_; ///< The timestamp of the exception.
    std::string file_;      ///< The file where the exception was thrown.
    int line_;              ///< The line where the exception was thrown.
    std::string function_;  ///< The function where the exception was thrown.

    /// Set the exception timestamp to the current time.
    void setTimestamp() {
      auto now = std::chrono::system_clock::now();
      auto now_c = std::chrono::system_clock::to_time_t(now);
      std::tm tm_buf;
      std::stringstream ss;
      // Using localtime_r for thread safety
      if (localtime_r(&now_c, &tm_buf)) {
        ss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S");
        timestamp_ = ss.str();
      } else {
        timestamp_ = "Error: Unable to get local time";
      }
    }

    /**
     * Build the exception message from the given parts.
     * @tparam Args String types that will form the exception's error message.
     * @param args The message parts to be concatenated.
     * @return The built exception message string.
     */
    template<typename... Args> std::string buildMessage(Args... args) const {
      std::ostringstream stream;
      (stream << ... << args);
      return stream.str();
    }

  public:
    /**
     * Constructor with message only.
     * @tparam Args String types that will form the exception's error message.
     * @param args The message parts to be concatenated.
     */
    template<typename... Args> explicit DynamicException(Args... args)
      : file_(""), line_(0), function_(""), message_(buildMessage(args...))
    { setTimestamp(); }

    /**
     * Constructor with message and stacktrace information.
     * @tparam FirstArg String types that will form the exception's error message.
     * @tparam RestArgs String types that will form the exception's error message.
     * @param firstArg The first part of the message.
     * @param restArgs The rest of the message parts.
     * @param file The file where the exception was thrown.
     * @param line The line where the exception was thrown.
     * @param func The function where the exception was thrown.
     */
    template<typename FirstArg, typename... RestArgs> DynamicException(
      FirstArg firstArg, RestArgs... restArgs, const std::string& file,
      int line, const std::string& func
    ) : file_(file), line_(line), function_(func), message_(buildMessage(firstArg, restArgs...))
    { setTimestamp(); }

    ///@{
    /** Getter. */
    const char* what() const noexcept override { return this->message_.c_str(); }
    std::string getTimestamp() const { return this->timestamp_; }
    const std::string& getFile() const { return this->file_; }
    int getLine() const { return this->line_; }
    const std::string& getFunction() const { return this->function_; }
    ///@}
};

#endif // DYNAMIC_EXCEPTION_H
