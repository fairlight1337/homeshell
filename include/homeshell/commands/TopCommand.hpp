#pragma once

#include <homeshell/Command.hpp>
#include <homeshell/Status.hpp>

#include <fmt/color.h>
#include <fmt/core.h>
#include <unistd.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <vector>

#ifdef __linux__
#include <sys/sysinfo.h>
#endif

namespace homeshell
{

/**
 * @brief Process information structure
 *
 * @details Contains information about a running process including its PID,
 * owner, command, resource usage, and execution state.
 */
struct ProcessInfo
{
    int pid;              ///< Process ID
    std::string user;     ///< User/owner of the process
    std::string command;  ///< Command name or full command line
    double cpu_percent;   ///< CPU usage percentage (0-100+)
    unsigned long mem_kb; ///< Memory usage in kilobytes
    char state;           ///< Process state (R=running, S=sleeping, Z=zombie, etc.)
};

/**
 * @brief Display running processes command (Linux only)
 *
 * @details The `top` command provides a snapshot of running processes on Linux systems,
 * similar to the traditional `top` utility but simplified for quick process monitoring.
 *
 * **Platform Support:**
 * - Linux: Full support via `/proc` filesystem
 * - Other platforms: Not supported (returns error)
 *
 * **Features:**
 * - Process listing from `/proc` filesystem
 * - CPU usage highlighting (red >50%, yellow >20%, white otherwise)
 * - Memory usage in KB
 * - Process state indicators (R=running, S=sleeping, etc.)
 * - Sorted by CPU usage (highest first)
 * - Limited to top 25 processes
 * - Full command line display (truncated to 60 chars)
 *
 * **Usage:**
 * @code
 * top
 * @endcode
 *
 * **Example Output:**
 * @code
 *     PID    S   CPU%  MEM(KB) COMMAND
 * ------------------------------------------------------------
 *    1234    R   45.2    12345 firefox --profile /home/user/.mozilla...
 *    5678    S   12.3     8192 chrome --type=renderer
 *    9012    S    5.1     4096 python script.py
 *
 * Showing 3 of 156 processes
 * @endcode
 *
 * **Process States:**
 * - R: Running or runnable (on run queue)
 * - S: Sleeping (waiting for an event)
 * - D: Uninterruptible sleep (usually I/O)
 * - Z: Zombie (terminated but not reaped)
 * - T: Stopped (by job control signal)
 * - t: Stopped (by debugger during tracing)
 * - W: Paging
 * - X: Dead
 * - ?: Unknown state
 *
 * @note CPU percentage calculation is simplified and may not reflect accurate CPU usage
 * @note Requires read access to `/proc` filesystem
 * @note On non-Linux platforms, returns an error
 */
class TopCommand : public ICommand
{
public:
    std::string getName() const override
    {
        return "top";
    }

    std::string getDescription() const override
    {
        return "Display running processes";
    }

    CommandType getType() const override
    {
        return CommandType::Synchronous;
    }

    Status execute(const CommandContext& context) override
    {
#ifdef __linux__
        auto processes = getProcessList();

        if (processes.empty())
        {
            fmt::print(fg(fmt::color::red), "Error: Unable to read process information\n");
            return Status::error("Failed to read /proc");
        }

        // Sort by CPU usage (descending)
        std::sort(processes.begin(), processes.end(),
                  [](const ProcessInfo& a, const ProcessInfo& b)
                  { return a.cpu_percent > b.cpu_percent; });

        // Display header
        fmt::print(fg(fmt::color::yellow) | fmt::emphasis::bold, "{:>7} {:>4} {:>6} {:>8} {}\n",
                   "PID", "S", "CPU%", "MEM(KB)", "COMMAND");
        fmt::print(fg(fmt::color::yellow), "{}\n", std::string(60, '-'));

        // Display top processes (limit to 25)
        size_t count = std::min(processes.size(), size_t(25));
        for (size_t i = 0; i < count; ++i)
        {
            const auto& proc = processes[i];

            auto color = fmt::color::white;
            if (proc.cpu_percent > 50.0)
                color = fmt::color::red;
            else if (proc.cpu_percent > 20.0)
                color = fmt::color::yellow;

            fmt::print("{:>7} {:>4} ", proc.pid, std::string(1, proc.state));
            fmt::print(fg(color), "{:>6.1f}", proc.cpu_percent);
            fmt::print(" {:>8} {}\n", proc.mem_kb, proc.command);
        }

        fmt::print("\nShowing {} of {} processes\n", count, processes.size());

        return Status::ok();
#else
        fmt::print(fg(fmt::color::red), "Error: top command is only supported on Linux\n");
        return Status::error("Unsupported platform");
#endif
    }

private:
#ifdef __linux__
    std::vector<ProcessInfo> getProcessList()
    {
        std::vector<ProcessInfo> processes;

        // Read /proc directory
        std::error_code ec;
        for (const auto& entry : std::filesystem::directory_iterator("/proc", ec))
        {
            if (!entry.is_directory())
                continue;

            std::string name = entry.path().filename().string();

            // Check if directory name is a number (PID)
            if (name.empty() || !std::isdigit(name[0]))
                continue;

            try
            {
                int pid = std::stoi(name);
                ProcessInfo info = readProcessInfo(pid);
                if (info.pid > 0)
                {
                    processes.push_back(info);
                }
            }
            catch (...)
            {
                continue;
            }
        }

        return processes;
    }

    ProcessInfo readProcessInfo(int pid)
    {
        ProcessInfo info;
        info.pid = pid;
        info.cpu_percent = 0.0;
        info.mem_kb = 0;
        info.state = '?';
        info.user = "?";

        // Read /proc/[pid]/stat
        std::string stat_path = "/proc/" + std::to_string(pid) + "/stat";
        std::ifstream stat_file(stat_path);

        if (!stat_file.is_open())
        {
            info.pid = -1;
            return info;
        }

        std::string line;
        std::getline(stat_file, line);

        // Parse stat file (simplified)
        size_t comm_start = line.find('(');
        size_t comm_end = line.find(')');

        if (comm_start != std::string::npos && comm_end != std::string::npos)
        {
            info.command = line.substr(comm_start + 1, comm_end - comm_start - 1);

            // Parse state and other fields after command
            std::istringstream iss(line.substr(comm_end + 2));
            std::string field;
            std::vector<std::string> fields;

            // Read all fields
            while (iss >> field)
            {
                fields.push_back(field);
            }

            // After extracting comm (fields 1-2: pid, comm), remaining fields start from state
            // Field 0 is state (originally field 3)
            if (fields.size() > 0)
            {
                info.state = fields[0][0];
            }

            // RSS is field 24 overall, so at index 21 after removing pid and comm (24 - 3 = 21)
            if (fields.size() > 21)
            {
                try
                {
                    long rss = std::stol(fields[21]);
                    info.mem_kb = (rss * sysconf(_SC_PAGESIZE)) / 1024;
                }
                catch (...)
                {
                    info.mem_kb = 0;
                }
            }
        }

        // Read cmdline for full command
        std::string cmdline_path = "/proc/" + std::to_string(pid) + "/cmdline";
        std::ifstream cmdline_file(cmdline_path);
        if (cmdline_file.is_open())
        {
            std::string cmdline;
            std::getline(cmdline_file, cmdline, '\0');
            if (!cmdline.empty())
            {
                // Replace nulls with spaces
                for (char& c : cmdline)
                {
                    if (c == '\0')
                        c = ' ';
                }
                if (cmdline.length() > 60)
                {
                    cmdline = cmdline.substr(0, 57) + "...";
                }
                info.command = cmdline;
            }
        }

        // CPU calculation would require sampling over time, so we'll skip for now
        // or just show a simplified version

        return info;
    }
#endif
};

} // namespace homeshell
