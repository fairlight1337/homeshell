#pragma once

#include <homeshell/Command.hpp>

#include <fmt/core.h>

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace homeshell
{

/**
 * @brief Display current date and time in ISO 8601 format
 *
 * @details The `datetime` command shows the current system date and time
 * in ISO 8601 format with millisecond precision. This format is
 * machine-readable and suitable for timestamps, logging, and data exchange.
 *
 * **Usage:**
 * @code
 * datetime
 * @endcode
 *
 * **Output Format:**
 * - ISO 8601: `YYYY-MM-DDTHH:MM:SS.mmm`
 * - Y: Year (4 digits)
 * - M: Month (01-12)
 * - D: Day (01-31)
 * - H: Hour (00-23)
 * - M: Minute (00-59)
 * - S: Second (00-59)
 * - m: Millisecond (000-999)
 *
 * **Example Output:**
 * @code
 * 2025-11-22T14:30:25.123
 * @endcode
 *
 * **Use Cases:**
 * - Timestamping log entries
 * - Recording event times
 * - Synchronization verification
 * - Date/time formatting reference
 *
 * @note Uses local system time (not UTC)
 * @note Millisecond precision depends on system clock resolution
 */
class DateTimeCommand : public ICommand
{
public:
    std::string getName() const override
    {
        return "datetime";
    }

    std::string getDescription() const override
    {
        return "Show current date and time in ISO format";
    }

    CommandType getType() const override
    {
        return CommandType::Synchronous;
    }

    Status execute(const CommandContext& context) override
    {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        std::tm tm = *std::localtime(&time);

        // Get milliseconds
        auto ms =
            std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

        // Format: YYYY-MM-DDTHH:MM:SS.mmm (ISO 8601)
        std::ostringstream oss;
        oss << std::setfill('0') << (tm.tm_year + 1900) << "-" << std::setw(2) << (tm.tm_mon + 1)
            << "-" << std::setw(2) << tm.tm_mday << "T" << std::setw(2) << tm.tm_hour << ":"
            << std::setw(2) << tm.tm_min << ":" << std::setw(2) << tm.tm_sec << "." << std::setw(3)
            << ms.count();

        fmt::print("{}\n", oss.str());
        return Status::ok();
    }
};

} // namespace homeshell
