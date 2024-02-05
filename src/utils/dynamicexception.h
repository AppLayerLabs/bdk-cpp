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

/**
* @brief A dynamic exception class that records the timestamp, file, line, and function where the exception was thrown.
*/
class DynamicException : public std::exception {
public:

    /**
    * @brief Construct a new DynamicException object with a message
    * @param message The message to be displayed when the exception is caught
    */
    template<typename... Args>
    DynamicException(Args... args);

    /**
    * @brief Construct a new DynamicException object with a message and stack trace information
    * @tparam FirstArg The type of the first argument
    * @tparam RestArgs The types of the rest of the arguments
    * @param firstArg The first part of the message
    * @param restArgs The rest of the parts of the message
    * @param file The file where the exception was thrown
    * @param line The line where the exception was thrown
    * @param func The function where the exception was thrown
    */
    template<typename FirstArg, typename... RestArgs>
    DynamicException(FirstArg firstArg, RestArgs... restArgs, const std::string& file = "", int line = 0, const std::string& func = "");

    /// Getter for the message to be displayed when the exception is caught
    const char* what() const noexcept override;

    /// Getter for the timestamp when the exception was thrown
    std::string getTimestamp() const;

    /// Setter for the timestamp when the exception was thrown
    void setTimestamp();

    /// Getter for the file where the exception was thrown
    const std::string& getFile() const;

    /// Getter for the line where the exception was thrown
    int getLine() const;

    /// Getter for the function where the exception was thrown
    const std::string& getFunction() const;

    /**
    * @brief Concatenates the arguments into a single message
    * @tparam Args The types of the arguments to be concatenated into the message
    * @param args The arguments to be concatenated into the message
    * @return The concatenated message
    */
    template<typename... Args>
    std::string buildMessage(Args... args);

private:

    /// The message to be displayed when the exception is caught
    std::string message_;

    /// The timestamp when the exception was thrown
    std::string timestamp_;

    /// The file where the exception was thrown
    std::string file_;

    /// The line where the exception was thrown
    int line_;

    /// The function where the exception was thrown
    std::string function_;
};


#include "dynamicexception.tpp" // Include template implementation

#endif // DYNAMIC_EXCEPTION_H

