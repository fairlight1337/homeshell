#pragma once

#include <homeshell/Command.hpp>

#include <fmt/core.h>

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace homeshell
{

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
