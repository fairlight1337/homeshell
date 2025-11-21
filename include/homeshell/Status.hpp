#pragma once

#include <cstdint>
#include <string>

namespace homeshell
{

/**
 * @brief Status of the operation
 * @details The status of the operation is returned by the functions.
 *          It contains the code and the message of the operation.
 *          The code is the exit code of the operation.
 *          The message is the message of the operation.
 */
struct Status
{
    /**
     * @brief Constructor
     * @param code The exit code of the operation
     * @param message The message of the operation
     */
    Status(int32_t code, std::string message) : code(code), message(message) {}

    /**
     * @brief The exit code of the operation
     */
    int32_t code;

    /**
     * @brief The message of the operation
     */
    std::string message;

    /**
     * @brief Check if the operation was successful
     * @return True if the operation was successful, false otherwise
     */
    bool isSuccess() const
    {
        return code == 0;
    }
};

} // namespace homeshell
