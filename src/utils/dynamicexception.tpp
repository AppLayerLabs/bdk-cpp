/*
Copyright (c) [2023-2024] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include <iomanip>

template<typename... Args>
DynamicException::DynamicException(Args... args) {
    message_ = buildMessage(args...);
    setTimestamp();
}

template<typename FirstArg, typename... RestArgs>
DynamicException::DynamicException(FirstArg firstArg, RestArgs... restArgs, const std::string& file, int line, const std::string& func)
    : file_(file), line_(line), function_(func) {
    std::ostringstream stream;
    stream << firstArg;  // Add the first part of the message
    (stream << ... << restArgs);  // Add the rest of the message parts
    message_ = stream.str();

    setTimestamp();
}

template<typename... Args>
std::string DynamicException::buildMessage(Args... args) {
    std::ostringstream stream;
    ((stream << args), ...);  // Fold expression to concatenate args
    return stream.str();
}

void DynamicException::setTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto now_c = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&now_c), "%Y-%m-%d %H:%M:%S");
    timestamp_ = ss.str();
}

const char* DynamicException::what() const noexcept {
    return message_.c_str();
}

std::string DynamicException::getTimestamp() const {
    return timestamp_.c_str();
}

const std::string& DynamicException::getFile() const {
    return file_;
}

int DynamicException::getLine() const {
    return line_;
}

const std::string& DynamicException::getFunction() const {
    return function_;
}
