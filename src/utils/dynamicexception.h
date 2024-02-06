/*
Copyright (c) [2023-2024] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef DYNAMIC_EXCEPTION_H
#define DYNAMIC_EXCEPTION_H

#include <exception>
#include <string>
#include <sstream>
#include <chrono>
#include <ctime>
#include <iomanip>

/**
* @brief A dynamic exception class that allows for dynamic message building and timestamping.
*/
class DynamicException : public std::exception {
public:
    /**
    * @brief Constructor for the DynamicException class with a dynamic message.
    * @param args The message parts to be concatenated.
    */
    template<typename... Args>
    DynamicException(Args... args)
        : file_(""), line_(0), function_("") {
        message_ = buildMessage(args...);
        setTimestamp();
    }

    /**
    * @brief Constructor for the DynamicException class with a dynamic message and stacktrace information.
    * @param firstArg The first part of the message.
    * @param restArgs The rest of the message parts.
    * @param file The file where the exception was thrown.
    * @param line The line where the exception was thrown.
    * @param func The function where the exception was thrown.
    */
    template<typename FirstArg, typename... RestArgs>
    DynamicException(FirstArg firstArg, RestArgs... restArgs, const std::string& file = "", int line = 0, const std::string& func = "") 
        : file_(file), line_(line), function_(func) {
        std::ostringstream stream;
        stream << firstArg;
        (stream << ... << restArgs);
        message_ = stream.str();
        setTimestamp();
    }

    /**
    * @brief Returns the exception message.
    * @return The exception message.
    */
    const char* what() const noexcept override {
        return message_.c_str();
    }

    /**
    * @brief Returns the timestamp of the exception.
    * @return The timestamp of the exception.
    */
    std::string getTimestamp() const {
        return timestamp_;
    }

    /**
    * @brief Returns the file where the exception was thrown.
    * @return The file where the exception was thrown.
    */
    const std::string& getFile() const {
        return file_;
    }

    /**
    * @brief Returns the line where the exception was thrown.
    * @return The line where the exception was thrown.
    */
    int getLine() const {
        return line_;
    }

    /**
    * @brief Returns the function where the exception was thrown.
    * @return The function where the exception was thrown.
    */
    const std::string& getFunction() const {
        return function_;
    }

private:

    /// The exception message.
    std::string message_;
    /// The timestamp of the exception.
    std::string timestamp_;
    /// The file where the exception was thrown.
    std::string file_;
    /// The line where the exception was thrown.
    int line_;
    /// The function where the exception was thrown.
    std::string function_;

    /**
    * @brief Sets the timestamp of the exception.
    */
    void setTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto now_c = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&now_c), "%Y-%m-%d %H:%M:%S");
        timestamp_ = ss.str();
    }

    /**
    * @brief Builds the exception message from the given parts.
    * @param args The message parts to be concatenated.
    * @return The built exception message.
    */
    template<typename... Args>
    std::string buildMessage(Args... args) {
        std::ostringstream stream;
        ((stream << args), ...);
        return stream.str();
    }
};

#endif // DYNAMIC_EXCEPTION_H
