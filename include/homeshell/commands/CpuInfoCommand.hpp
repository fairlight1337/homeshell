/**
 * @file CpuInfoCommand.hpp
 * @brief Display CPU information and status
 *
 * This command displays detailed CPU information including model, architecture,
 * frequencies, cache sizes, features, and current usage statistics.
 * Information is read from /proc/cpuinfo, /sys/devices/system/cpu/, and /proc/stat.
 *
 * @author Homeshell Development Team
 * @date 2025
 */

#pragma once

#include <homeshell/Command.hpp>
#include <homeshell/Status.hpp>

#include <dirent.h>

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace homeshell
{

/**
 * @class CpuInfoCommand
 * @brief Command to display CPU information and status
 *
 * Displays information about:
 * - CPU model, vendor, architecture
 * - Number of physical and logical cores
 * - Current, minimum, and maximum frequencies
 * - CPU cache sizes (L1, L2, L3)
 * - CPU flags and features
 * - CPU governor and scaling driver
 * - Current CPU usage per core
 *
 * Information is read from /proc/cpuinfo, /sys/devices/system/cpu/, and /proc/stat
 *
 * @section Usage
 * @code
 * cpu-info              # Show all CPU information
 * cpu-info -f           # Show only frequency information
 * cpu-info -c           # Show only cache information
 * cpu-info -u           # Show only usage information
 * cpu-info --help       # Show help message
 * @endcode
 */
class CpuInfoCommand : public ICommand
{
public:
    /**
     * @brief Get command name
     * @return Command name "cpu-info"
     */
    std::string getName() const override
    {
        return "cpu-info";
    }

    /**
     * @brief Get command description
     * @return Brief description of the command
     */
    std::string getDescription() const override
    {
        return "Display CPU information and status";
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
     * @brief Execute the cpu-info command
     * @param context Command context with arguments
     * @return Status indicating success or failure
     */
    Status execute(const CommandContext& context) override
    {
        bool show_frequency = false;
        bool show_cache = false;
        bool show_usage = false;
        bool show_all = true;

        // Parse arguments
        for (size_t i = 0; i < context.args.size(); ++i)
        {
            const std::string& arg = context.args[i];

            if (arg == "--help" || arg == "-h")
            {
                showHelp();
                return Status::ok();
            }
            else if (arg == "-f" || arg == "--frequency")
            {
                show_frequency = true;
                show_all = false;
            }
            else if (arg == "-c" || arg == "--cache")
            {
                show_cache = true;
                show_all = false;
            }
            else if (arg == "-u" || arg == "--usage")
            {
                show_usage = true;
                show_all = false;
            }
            else
            {
                return Status::error("Unknown option: " + arg +
                                     "\nUse --help for usage information");
            }
        }

        // If no specific options, show all
        if (show_all)
        {
            show_frequency = true;
            show_cache = true;
            show_usage = true;
        }

        // Parse CPU info
        auto cpu_data = parseCpuInfo();

        // Display basic CPU information (always shown)
        displayBasicInfo(cpu_data);

        if (show_frequency)
        {
            displayFrequencyInfo();
        }

        if (show_cache)
        {
            displayCacheInfo(cpu_data);
        }

        if (show_usage)
        {
            displayUsageInfo();
        }

        return Status::ok();
    }

private:
    /**
     * @brief Display help information
     */
    void showHelp() const
    {
        std::cout << "Usage: cpu-info [OPTIONS]\n\n";
        std::cout << "Display CPU information and current status.\n\n";
        std::cout << "Options:\n";
        std::cout << "  -f, --frequency  Show only frequency information\n";
        std::cout << "  -c, --cache      Show only cache information\n";
        std::cout << "  -u, --usage      Show only usage information\n";
        std::cout << "  -h, --help       Show this help message\n\n";
        std::cout << "Without options, displays all available CPU information.\n";
    }

    /**
     * @brief Read a single-line value from a sysfs file
     * @param path Path to the sysfs file
     * @return Content of the file, or empty string if not readable
     */
    std::string readSysfsValue(const std::string& path) const
    {
        std::ifstream file(path);
        if (!file.is_open())
            return "";

        std::string value;
        std::getline(file, value);

        // Trim whitespace
        size_t start = value.find_first_not_of(" \t\n\r");
        size_t end = value.find_last_not_of(" \t\n\r");

        if (start == std::string::npos)
            return "";

        return value.substr(start, end - start + 1);
    }

    /**
     * @brief Parse /proc/cpuinfo
     * @return Map of CPU properties
     */
    std::map<std::string, std::string> parseCpuInfo() const
    {
        std::map<std::string, std::string> data;
        std::ifstream file("/proc/cpuinfo");
        if (!file.is_open())
            return data;

        std::string line;
        std::set<int> physical_ids;
        int processor_count = 0;
        int core_count = 0;

        while (std::getline(file, line))
        {
            size_t colon = line.find(':');
            if (colon == std::string::npos)
                continue;

            std::string key = line.substr(0, colon);
            std::string value = line.substr(colon + 1);

            // Trim whitespace
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);

            // Store first occurrence of each key
            if (key == "processor")
            {
                processor_count++;
            }
            else if (key == "physical id")
            {
                physical_ids.insert(std::stoi(value));
            }
            else if (key == "cpu cores")
            {
                if (core_count == 0)
                    core_count = std::stoi(value);
            }
            else if (data.find(key) == data.end())
            {
                data[key] = value;
            }
        }

        data["processor_count"] = std::to_string(processor_count);
        data["physical_count"] = std::to_string(physical_ids.empty() ? 1 : physical_ids.size());
        if (core_count > 0)
            data["cores_per_socket"] = std::to_string(core_count);

        return data;
    }

    /**
     * @brief Display basic CPU information
     * @param cpu_data CPU data from /proc/cpuinfo
     */
    void displayBasicInfo(const std::map<std::string, std::string>& cpu_data) const
    {
        std::cout << "=== CPU Information ===\n\n";

        // Model name
        auto it = cpu_data.find("model name");
        if (it != cpu_data.end())
        {
            std::cout << "Model: " << it->second << "\n";
        }

        // Vendor
        it = cpu_data.find("vendor_id");
        if (it != cpu_data.end())
        {
            std::cout << "Vendor: " << it->second << "\n";
        }

        // Architecture
        it = cpu_data.find("cpu family");
        if (it != cpu_data.end())
        {
            std::cout << "Family: " << it->second;
            auto model_it = cpu_data.find("model");
            if (model_it != cpu_data.end())
            {
                std::cout << ", Model: " << model_it->second;
            }
            auto stepping_it = cpu_data.find("stepping");
            if (stepping_it != cpu_data.end())
            {
                std::cout << ", Stepping: " << stepping_it->second;
            }
            std::cout << "\n";
        }

        // Cores
        it = cpu_data.find("processor_count");
        if (it != cpu_data.end())
        {
            std::cout << "Logical Cores: " << it->second << "\n";
        }

        it = cpu_data.find("physical_count");
        if (it != cpu_data.end() && std::stoi(it->second) > 0)
        {
            std::cout << "Physical CPUs: " << it->second << "\n";
        }

        it = cpu_data.find("cores_per_socket");
        if (it != cpu_data.end())
        {
            std::cout << "Cores per Socket: " << it->second << "\n";
        }

        // Flags (show count and first few)
        it = cpu_data.find("flags");
        if (it != cpu_data.end())
        {
            std::istringstream iss(it->second);
            std::vector<std::string> flags;
            std::string flag;
            while (iss >> flag)
            {
                flags.push_back(flag);
            }
            std::cout << "CPU Flags: " << flags.size() << " features";

            // Show some notable flags
            std::vector<std::string> notable = {"sse", "sse2", "sse3", "sse4_1", "sse4_2",
                                                "avx", "avx2", "aes",  "vmx",    "svm"};
            std::vector<std::string> found_notable;
            for (const auto& n : notable)
            {
                if (std::find(flags.begin(), flags.end(), n) != flags.end())
                {
                    found_notable.push_back(n);
                }
            }
            if (!found_notable.empty())
            {
                std::cout << " (";
                for (size_t i = 0; i < found_notable.size(); ++i)
                {
                    if (i > 0)
                        std::cout << ", ";
                    std::cout << found_notable[i];
                }
                std::cout << ")";
            }
            std::cout << "\n";
        }

        // Virtualization
        it = cpu_data.find("flags");
        if (it != cpu_data.end())
        {
            bool has_vmx = it->second.find("vmx") != std::string::npos;
            bool has_svm = it->second.find("svm") != std::string::npos;
            if (has_vmx || has_svm)
            {
                std::cout << "Virtualization: " << (has_vmx ? "VT-x (Intel)" : "AMD-V (AMD)")
                          << "\n";
            }
        }

        std::cout << "\n";
    }

    /**
     * @brief Display frequency information
     */
    void displayFrequencyInfo() const
    {
        std::cout << "=== CPU Frequency ===\n\n";

        // Get frequency info from first CPU
        std::string base_path = "/sys/devices/system/cpu/cpu0/cpufreq/";

        // Current frequency
        std::string cur_freq = readSysfsValue(base_path + "scaling_cur_freq");
        if (!cur_freq.empty())
        {
            double freq_mhz = std::stod(cur_freq) / 1000.0;
            std::cout << "Current: " << std::fixed << std::setprecision(2) << freq_mhz << " MHz\n";
        }

        // Min frequency
        std::string min_freq = readSysfsValue(base_path + "scaling_min_freq");
        if (!min_freq.empty())
        {
            double freq_mhz = std::stod(min_freq) / 1000.0;
            std::cout << "Minimum: " << std::fixed << std::setprecision(2) << freq_mhz << " MHz\n";
        }

        // Max frequency
        std::string max_freq = readSysfsValue(base_path + "scaling_max_freq");
        if (!max_freq.empty())
        {
            double freq_mhz = std::stod(max_freq) / 1000.0;
            std::cout << "Maximum: " << std::fixed << std::setprecision(2) << freq_mhz << " MHz\n";
        }

        // Governor
        std::string governor = readSysfsValue(base_path + "scaling_governor");
        if (!governor.empty())
        {
            std::cout << "Governor: " << governor << "\n";
        }

        // Driver
        std::string driver = readSysfsValue(base_path + "scaling_driver");
        if (!driver.empty())
        {
            std::cout << "Driver: " << driver << "\n";
        }

        // Available governors
        std::string avail_gov = readSysfsValue(base_path + "scaling_available_governors");
        if (!avail_gov.empty())
        {
            std::cout << "Available Governors: " << avail_gov << "\n";
        }

        std::cout << "\n";
    }

    /**
     * @brief Display cache information
     * @param cpu_data CPU data from /proc/cpuinfo
     */
    void displayCacheInfo(const std::map<std::string, std::string>& cpu_data) const
    {
        std::cout << "=== CPU Cache ===\n\n";

        bool found_cache = false;

        // Try to read from sysfs first (more detailed)
        std::string cache_path = "/sys/devices/system/cpu/cpu0/cache/";
        DIR* dir = opendir(cache_path.c_str());
        if (dir)
        {
            std::map<std::string, std::string> caches;
            struct dirent* entry;

            while ((entry = readdir(dir)) != nullptr)
            {
                std::string name = entry->d_name;
                if (name.find("index") == 0)
                {
                    std::string index_path = cache_path + name + "/";
                    std::string level = readSysfsValue(index_path + "level");
                    std::string type = readSysfsValue(index_path + "type");
                    std::string size = readSysfsValue(index_path + "size");

                    if (!level.empty() && !size.empty())
                    {
                        std::string key = "L" + level + " " + type;
                        caches[key] = size;
                        found_cache = true;
                    }
                }
            }
            closedir(dir);

            // Display in order
            for (const auto& cache : caches)
            {
                std::cout << cache.first << ": " << cache.second << "\n";
            }
        }

        // Fallback to /proc/cpuinfo
        if (!found_cache)
        {
            auto it = cpu_data.find("cache size");
            if (it != cpu_data.end())
            {
                std::cout << "Cache Size: " << it->second << "\n";
                found_cache = true;
            }
        }

        if (!found_cache)
        {
            std::cout << "Cache information not available\n";
        }

        std::cout << "\n";
    }

    /**
     * @brief Display CPU usage information
     */
    void displayUsageInfo() const
    {
        std::cout << "=== CPU Usage ===\n\n";

        std::ifstream file("/proc/stat");
        if (!file.is_open())
        {
            std::cout << "Usage information not available\n\n";
            return;
        }

        std::string line;
        int cpu_num = 0;
        std::vector<std::string> cpu_lines;

        while (std::getline(file, line))
        {
            if (line.find("cpu") == 0)
            {
                if (line.find("cpu ") == 0)
                {
                    // Overall CPU line
                    cpu_lines.insert(cpu_lines.begin(), line);
                }
                else
                {
                    // Individual CPU core
                    cpu_lines.push_back(line);
                }
            }
        }

        // Parse and display CPU usage
        for (const auto& cpu_line : cpu_lines)
        {
            std::istringstream iss(cpu_line);
            std::string cpu_name;
            long user, nice, system, idle, iowait, irq, softirq, steal;

            iss >> cpu_name >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal;

            long total = user + nice + system + idle + iowait + irq + softirq + steal;
            long busy = total - idle - iowait;

            double usage = (total > 0) ? (busy * 100.0 / total) : 0.0;

            if (cpu_name == "cpu")
            {
                std::cout << "Overall: " << std::fixed << std::setprecision(1) << usage << "%\n";
            }
            else
            {
                // Extract CPU number
                std::string num = cpu_name.substr(3);
                std::cout << "Core " << num << ":  " << std::fixed << std::setprecision(1) << usage
                          << "%\n";
            }
        }

        std::cout << "\nNote: Usage is instantaneous snapshot from /proc/stat\n";
        std::cout << "\n";
    }
};

} // namespace homeshell
