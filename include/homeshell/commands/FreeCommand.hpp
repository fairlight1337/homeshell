/**
 * @file FreeCommand.hpp
 * @brief Display memory usage information
 *
 * This command displays information about system memory (RAM) and swap usage.
 * Information is read from /proc/meminfo.
 *
 * @author Homeshell Development Team
 * @date 2025
 */

#pragma once

#include <homeshell/Command.hpp>
#include <homeshell/Status.hpp>

#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace homeshell
{

/**
 * @class FreeCommand
 * @brief Command to display memory usage
 *
 * Displays memory and swap usage by reading /proc/meminfo.
 *
 * @section Usage
 * @code
 * free                  # Show memory in KB
 * free -h               # Human-readable format
 * free -m               # Show in MB
 * free -g               # Show in GB
 * free --help           # Show help message
 * @endcode
 */
class FreeCommand : public ICommand
{
public:
    /**
     * @brief Get command name
     * @return Command name "free"
     */
    std::string getName() const override
    {
        return "free";
    }

    /**
     * @brief Get command description
     * @return Brief description of the command
     */
    std::string getDescription() const override
    {
        return "Display memory usage information";
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
     * @brief Execute the free command
     * @param context Command context with arguments
     * @return Status indicating success or failure
     */
    Status execute(const CommandContext& context) override
    {
        bool human_readable = false;
        int unit_divisor = 1; // 1 = KB (default), 1024 = MB, 1024*1024 = GB
        std::string unit_label = "Ki";

        // Parse arguments
        for (size_t i = 0; i < context.args.size(); ++i)
        {
            const std::string& arg = context.args[i];

            if (arg == "--help")
            {
                showHelp();
                return Status::ok();
            }
            else if (arg[0] == '-' && arg.length() > 1 && arg[1] != '-')
            {
                // Short options
                for (size_t j = 1; j < arg.length(); ++j)
                {
                    if (arg[j] == 'h')
                    {
                        human_readable = true;
                        unit_label = "";
                    }
                    else if (arg[j] == 'm')
                    {
                        unit_divisor = 1024;
                        unit_label = "Mi";
                    }
                    else if (arg[j] == 'g')
                    {
                        unit_divisor = 1024 * 1024;
                        unit_label = "Gi";
                    }
                    else if (arg[j] == 'b')
                    {
                        unit_divisor = 1;
                        unit_label = "";
                        human_readable = false; // Bytes mode
                    }
                    else
                    {
                        return Status::error("Unknown option: -" + std::string(1, arg[j]) +
                                             "\nUse --help for usage information");
                    }
                }
            }
            else if (arg == "--human")
            {
                human_readable = true;
                unit_label = "";
            }
            else if (arg == "--mega")
            {
                unit_divisor = 1024;
                unit_label = "Mi";
            }
            else if (arg == "--giga")
            {
                unit_divisor = 1024 * 1024;
                unit_label = "Gi";
            }
            else
            {
                return Status::error("Unknown option: " + arg +
                                     "\nUse --help for usage information");
            }
        }

        // Read memory info from /proc/meminfo
        auto meminfo = parseMemInfo();
        if (meminfo.empty())
        {
            return Status::error("Failed to read /proc/meminfo");
        }

        displayMemoryInfo(meminfo, human_readable, unit_divisor, unit_label);
        return Status::ok();
    }

private:
    /**
     * @brief Display help information
     */
    void showHelp() const
    {
        std::cout << "Usage: free [OPTION]\n\n";
        std::cout << "Display amount of free and used memory in the system.\n\n";
        std::cout << "Options:\n";
        std::cout << "  -b, --bytes       Show output in bytes\n";
        std::cout << "  -k, --kilo        Show output in kilobytes (default)\n";
        std::cout << "  -m, --mega        Show output in megabytes\n";
        std::cout << "  -g, --giga        Show output in gigabytes\n";
        std::cout << "  -h, --human       Show human-readable output\n";
        std::cout << "  --help            Show this help message\n";
    }

    /**
     * @brief Parse /proc/meminfo
     * @return Map of memory info keys to values (in KB)
     */
    std::map<std::string, uint64_t> parseMemInfo() const
    {
        std::map<std::string, uint64_t> info;
        std::ifstream file("/proc/meminfo");
        if (!file.is_open())
            return info;

        std::string line;
        while (std::getline(file, line))
        {
            std::istringstream iss(line);
            std::string key;
            uint64_t value;
            std::string unit;

            if (iss >> key >> value)
            {
                // Remove trailing colon from key
                if (!key.empty() && key.back() == ':')
                    key.pop_back();
                info[key] = value; // Values are in KB
            }
        }

        return info;
    }

    /**
     * @brief Format size for display
     * @param kb Size in kilobytes
     * @param human_readable Use human-readable format
     * @param divisor Unit divisor
     * @return Formatted string
     */
    std::string formatSize(uint64_t kb, bool human_readable, int divisor) const
    {
        if (divisor == 1 && !human_readable)
        {
            // Bytes mode
            return std::to_string(kb * 1024);
        }

        double value = static_cast<double>(kb) / divisor;

        if (human_readable)
        {
            // Human readable: choose best unit
            const char* units[] = {"Ki", "Mi", "Gi", "Ti"};
            int unit_idx = 0;
            double hr_value = static_cast<double>(kb);

            while (hr_value >= 1024.0 && unit_idx < 3)
            {
                hr_value /= 1024.0;
                unit_idx++;
            }

            std::ostringstream oss;
            oss << std::fixed << std::setprecision(1) << hr_value << units[unit_idx];
            return oss.str();
        }
        else
        {
            std::ostringstream oss;
            oss << static_cast<uint64_t>(value);
            return oss.str();
        }
    }

    /**
     * @brief Display memory information
     * @param meminfo Memory info map
     * @param human_readable Use human-readable format
     * @param divisor Unit divisor
     * @param unit_label Unit label for header
     */
    void displayMemoryInfo(const std::map<std::string, uint64_t>& meminfo, bool human_readable,
                           int divisor, const std::string& unit_label) const
    {
        uint64_t mem_total = meminfo.count("MemTotal") ? meminfo.at("MemTotal") : 0;
        uint64_t mem_free = meminfo.count("MemFree") ? meminfo.at("MemFree") : 0;
        uint64_t mem_available = meminfo.count("MemAvailable") ? meminfo.at("MemAvailable") : 0;
        uint64_t buffers = meminfo.count("Buffers") ? meminfo.at("Buffers") : 0;
        uint64_t cached = meminfo.count("Cached") ? meminfo.at("Cached") : 0;
        uint64_t swap_total = meminfo.count("SwapTotal") ? meminfo.at("SwapTotal") : 0;
        uint64_t swap_free = meminfo.count("SwapFree") ? meminfo.at("SwapFree") : 0;

        uint64_t mem_used = mem_total - mem_free - buffers - cached;
        uint64_t swap_used = swap_total - swap_free;

        // Print header
        std::cout << std::setw(16) << std::left << "" << std::setw(13) << std::right << "total"
                  << std::setw(13) << "used" << std::setw(13) << "free" << std::setw(13) << "shared"
                  << std::setw(13) << "buff/cache" << std::setw(13) << "available"
                  << "\n";

        // Print memory line
        std::cout << std::setw(16) << std::left << "Mem:" << std::setw(13) << std::right
                  << formatSize(mem_total, human_readable, divisor) << std::setw(13)
                  << formatSize(mem_used, human_readable, divisor) << std::setw(13)
                  << formatSize(mem_free, human_readable, divisor) << std::setw(13) << "0"
                  << std::setw(13) << formatSize(buffers + cached, human_readable, divisor)
                  << std::setw(13) << formatSize(mem_available, human_readable, divisor) << "\n";

        // Print swap line
        std::cout << std::setw(16) << std::left << "Swap:" << std::setw(13) << std::right
                  << formatSize(swap_total, human_readable, divisor) << std::setw(13)
                  << formatSize(swap_used, human_readable, divisor) << std::setw(13)
                  << formatSize(swap_free, human_readable, divisor) << "\n";
    }
};

} // namespace homeshell
