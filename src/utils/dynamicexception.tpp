/*
Copyright (c) [2023-2024] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include <iomanip>

template<typename... Args>
DynamicException::DynamicException(const std::string& message, 
                                   const std::string& file, 
                                   int line, 
                                   const std::string& func) 
    : message_(message), file_(file), line_(line), function_(func) {
    // Record the current time as the timestamp
    std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
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
