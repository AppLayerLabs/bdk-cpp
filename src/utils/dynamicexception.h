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
    * @brief Construct a new Dynamic Exception object
    * @param message The message to be displayed when the exception is caught
    * @param file The file where the exception was thrown
    * @param line The line where the exception was thrown
    * @param func The function where the exception was thrown
    */
    template<typename... Args>
    DynamicException(const std::string& message, 
                     const std::string& file = "Unknown File", 
                     int line = -1, 
                     const std::string& func = "Unknown Function");

    /// Getter for the message to be displayed when the exception is caught
    const char* what() const noexcept override;

    /// Getter for the timestamp when the exception was thrown
    std::string getTimestamp() const;

    /// Getter for the file where the exception was thrown
    const std::string& getFile() const;

    /// Getter for the line where the exception was thrown
    int getLine() const;

    /// Getter for the function where the exception was thrown
    const std::string& getFunction() const;

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

