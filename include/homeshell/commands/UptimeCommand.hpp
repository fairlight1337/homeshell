/**
 * @file UptimeCommand.hpp
 * @brief Display system uptime and load average
 *
 * This command displays how long the system has been running, along with
 * the current system load averages. Information is read from /proc/uptime
 * and /proc/loadavg.
 *
 * @author Homeshell Development Team
 * @date 2025
 */

#pragma once

#include <homeshell/Command.hpp>
#include <homeshell/Status.hpp>

#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace homeshell
{

/**
 * @class UptimeCommand
 * @brief Command to display system uptime
 *
 * Displays system uptime and load averages by reading /proc/uptime and /proc/loadavg.
 *
 * @section Usage
 * @code
 * uptime                # Show uptime
 * uptime -p             # Show uptime in pretty format
 * uptime -s             # Show since when (boot time)
 * uptime --help         # Show help message
 * @endcode
 */
class UptimeCommand : public ICommand
{
public:
    /**
     * @brief Get command name
     * @return Command name "uptime"
     */
    std::string getName() const override
    {
        return "uptime";
    }

    /**
     * @brief Get command description
     * @return Brief description of the command
     */
    std::string getDescription() const override
    {
        return "Display system uptime and load average";
    }

    /**
     * @brief Get command type
     * @return CommandType::Synchronous
     */
    CommandType getType() const override
    {
        return CommandType::Synchronous;
    }

    /**
     * @brief Execute the uptime command
     * @param context Command context with arguments
     * @return Status indicating success or failure
     */
    Status execute(const CommandContext& context) override
    {
        bool pretty = false;
        bool since = false;

        // Parse arguments
        for (size_t i = 0; i < context.args.size(); ++i)
        {
            const std::string& arg = context.args[i];

            if (arg == "--help" || arg == "-h")
            {
                showHelp();
                return Status::ok();
            }
            else if (arg == "-p" || arg == "--pretty")
            {
                pretty = true;
            }
            else if (arg == "-s" || arg == "--since")
            {
                since = true;
            }
            else
            {
                return Status::error("Unknown option: " + arg +
                                     "\nUse --help for usage information");
            }
        }

        // Read uptime from /proc/uptime
        std::ifstream uptime_file("/proc/uptime");
        if (!uptime_file.is_open())
        {
            return Status::error("Failed to read /proc/uptime");
        }

        double uptime_seconds;
        uptime_file >> uptime_seconds;
        uptime_file.close();

        if (since)
        {
            // Show boot time
            time_t boot_time = time(nullptr) - static_cast<time_t>(uptime_seconds);
            struct tm* tm_info = localtime(&boot_time);
            char buffer[80];
            strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm_info);
            std::cout << buffer << "\n";
            return Status::ok();
        }

        if (pretty)
        {
            // Pretty format
            displayPrettyUptime(uptime_seconds);
            return Status::ok();
        }

        // Standard format
        displayStandardUptime(uptime_seconds);
        return Status::ok();
    }

private:
    /**
     * @brief Display help information
     */
    void showHelp() const
    {
        std::cout << "Usage: uptime [OPTION]\n\n";
        std::cout << "Display system uptime and load average.\n\n";
        std::cout << "Options:\n";
        std::cout << "  -p, --pretty  Show uptime in pretty format\n";
        std::cout << "  -s, --since   Show system up since (boot time)\n";
        std::cout << "  -h, --help    Show this help message\n";
    }

    /**
     * @brief Display uptime in pretty format
     * @param uptime_seconds Uptime in seconds
     */
    void displayPrettyUptime(double uptime_seconds) const
    {
        int total_seconds = static_cast<int>(uptime_seconds);
        int days = total_seconds / 86400;
        int hours = (total_seconds % 86400) / 3600;
        int minutes = (total_seconds % 3600) / 60;

        std::cout << "up ";
        if (days > 0)
        {
            std::cout << days << " day" << (days > 1 ? "s" : "");
            if (hours > 0 || minutes > 0)
                std::cout << ", ";
        }
        if (hours > 0)
        {
            std::cout << hours << " hour" << (hours > 1 ? "s" : "");
            if (minutes > 0)
                std::cout << ", ";
        }
        if (minutes > 0 || (days == 0 && hours == 0))
        {
            std::cout << minutes << " minute" << (minutes != 1 ? "s" : "");
        }
        std::cout << "\n";
    }

    /**
     * @brief Display uptime in standard format
     * @param uptime_seconds Uptime in seconds
     */
    void displayStandardUptime(double uptime_seconds) const
    {
        // Current time
        time_t now = time(nullptr);
        struct tm* tm_info = localtime(&now);
        char time_buffer[10];
        strftime(time_buffer, sizeof(time_buffer), "%H:%M:%S", tm_info);

        // Uptime
        int total_seconds = static_cast<int>(uptime_seconds);
        int days = total_seconds / 86400;
        int hours = (total_seconds % 86400) / 3600;
        int minutes = (total_seconds % 3600) / 60;

        // Load average
        std::ifstream loadavg_file("/proc/loadavg");
        double load1, load5, load15;
        if (loadavg_file.is_open())
        {
            loadavg_file >> load1 >> load5 >> load15;
            loadavg_file.close();
        }
        else
        {
            load1 = load5 = load15 = 0.0;
        }

        // Number of users (we'll just show 1 for homeshell)
        int users = 1;

        std::cout << " " << time_buffer << " up ";
        if (days > 0)
        {
            std::cout << days << " day" << (days > 1 ? "s" : "") << ", ";
        }
        if (hours > 0 || days > 0)
        {
            std::cout << hours << ":" << std::setfill('0') << std::setw(2) << minutes;
        }
        else
        {
            std::cout << minutes << " min";
        }

        std::cout << ",  " << users << " user" << (users != 1 ? "s" : "");
        std::cout << ",  load average: " << std::fixed << std::setprecision(2) << load1 << ", "
                  << load5 << ", " << load15 << "\n";
    }
};

} // namespace homeshell
