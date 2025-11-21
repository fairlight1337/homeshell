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

    /**
     * @brief Check if the status is OK
     * @return True if OK, false otherwise
     */
    bool isOk() const
    {
        return code == 0;
    }

    /**
     * @brief Check if the status indicates exit
     * @return True if exit status, false otherwise
     */
    bool isExit() const
    {
        return code == -1;
    }

    /**
     * @brief Create an OK status
     * @return OK status
     */
    static Status ok()
    {
        return Status(0, "");
    }

    /**
     * @brief Create an error status
     * @param message Error message
     * @return Error status
     */
    static Status error(const std::string& message)
    {
        return Status(1, message);
    }

    /**
     * @brief Create an exit status
     * @return Exit status
     */
    static Status exit()
    {
        return Status(-1, "exit");
    }
};

} // namespace homeshell
