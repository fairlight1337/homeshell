/**
 * @file PsCommand.hpp
 * @brief Display process status
 *
 * This command displays information about running processes,
 * similar to the Unix `ps` command.
 *
 * @author Homeshell Development Team
 * @date 2025
 */

#pragma once

#include <homeshell/Command.hpp>
#include <homeshell/Status.hpp>

#include <dirent.h>
#include <sys/types.h>
#include <unistd.h>

#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace homeshell
{

class PsCommand : public ICommand
{
public:
    std::string getName() const override
    {
        return "ps";
    }

    std::string getDescription() const override
    {
        return "Display process status";
    }

    CommandType getType() const override
    {
        return CommandType::Synchronous;
    }

    Status execute(const CommandContext& context) override
    {
        if (context.args.size() > 0 && (context.args[0] == "--help" || context.args[0] == "-h"))
        {
            showHelp();
            return Status::ok();
        }

        // Parse options
        bool show_all = false;
        bool show_full = false;

        for (const auto& arg : context.args)
        {
            if (arg == "-a" || arg == "--all" || arg == "aux" || arg == "-aux")
            {
                show_all = true;
            }
            else if (arg == "-f" || arg == "--full")
            {
                show_full = true;
            }
        }

        // Get process list
        auto processes = getProcessList(show_all);

        // Display header
        if (show_full)
        {
            std::cout << std::left << std::setw(8) << "PID" << std::setw(8) << "PPID"
                      << std::setw(10) << "USER" << std::setw(8) << "STATE"
                      << "CMD\n";
        }
        else
        {
            std::cout << std::left << std::setw(8) << "PID" << std::setw(8) << "STATE"
                      << "CMD\n";
        }

        // Display processes
        for (const auto& proc : processes)
        {
            if (show_full)
            {
                std::cout << std::left << std::setw(8) << proc.pid << std::setw(8) << proc.ppid
                          << std::setw(10) << proc.user << std::setw(8) << proc.state << proc.cmd
                          << "\n";
            }
            else
            {
                std::cout << std::left << std::setw(8) << proc.pid << std::setw(8) << proc.state
                          << proc.cmd << "\n";
            }
        }

        return Status::ok();
    }

private:
    struct ProcessInfo
    {
        int pid;
        int ppid;
        std::string user;
        std::string state;
        std::string cmd;
    };

    void showHelp() const
    {
        std::cout << "Usage: ps [OPTION]...\n\n"
                  << "Display process status.\n\n"
                  << "Options:\n"
                  << "  -a, --all       Show processes from all users\n"
                  << "  -f, --full      Show full format listing\n"
                  << "  aux             Show all processes with details\n"
                  << "  --help          Show this help message\n\n"
                  << "Examples:\n"
                  << "  ps\n"
                  << "  ps -a\n"
                  << "  ps -f\n"
                  << "  ps aux\n";
    }

    std::vector<ProcessInfo> getProcessList(bool show_all) const
    {
        std::vector<ProcessInfo> processes;

        DIR* dir = opendir("/proc");
        if (!dir)
        {
            return processes;
        }

        uid_t current_uid = getuid();
        struct dirent* entry;

        while ((entry = readdir(dir)) != nullptr)
        {
            // Check if directory name is a number (PID)
            if (entry->d_type == DT_DIR)
            {
                int pid = atoi(entry->d_name);
                if (pid > 0)
                {
                    ProcessInfo info = getProcessInfo(pid);
                    if (info.pid > 0)
                    {
                        // Filter by user if not showing all
                        if (show_all || info.user == std::to_string(current_uid))
                        {
                            processes.push_back(info);
                        }
                    }
                }
            }
        }

        closedir(dir);
        return processes;
    }

    ProcessInfo getProcessInfo(int pid) const
    {
        ProcessInfo info;
        info.pid = pid;
        info.ppid = 0;
        info.user = "?";
        info.state = "?";
        info.cmd = "?";

        // Read /proc/[pid]/stat
        std::string stat_path = "/proc/" + std::to_string(pid) + "/stat";
        std::ifstream stat_file(stat_path);
        if (stat_file)
        {
            std::string line;
            if (std::getline(stat_file, line))
            {
                // Parse stat file: pid (comm) state ppid ...
                size_t comm_start = line.find('(');
                size_t comm_end = line.find(')');
                if (comm_start != std::string::npos && comm_end != std::string::npos)
                {
                    info.cmd = line.substr(comm_start + 1, comm_end - comm_start - 1);

                    // Parse state and ppid
                    std::istringstream iss(line.substr(comm_end + 2));
                    char state;
                    int ppid;
                    if (iss >> state >> ppid)
                    {
                        info.state = std::string(1, state);
                        info.ppid = ppid;
                    }
                }
            }
        }

        // Read /proc/[pid]/status for UID
        std::string status_path = "/proc/" + std::to_string(pid) + "/status";
        std::ifstream status_file(status_path);
        if (status_file)
        {
            std::string line;
            while (std::getline(status_file, line))
            {
                if (line.substr(0, 4) == "Uid:")
                {
                    std::istringstream iss(line.substr(4));
                    int uid;
                    if (iss >> uid)
                    {
                        info.user = std::to_string(uid);
                    }
                    break;
                }
            }
        }

        // Try to get full command line
        std::string cmdline_path = "/proc/" + std::to_string(pid) + "/cmdline";
        std::ifstream cmdline_file(cmdline_path);
        if (cmdline_file)
        {
            std::string cmdline;
            std::getline(cmdline_file, cmdline, '\0');
            if (!cmdline.empty())
            {
                // Replace null bytes with spaces
                for (char& c : cmdline)
                {
                    if (c == '\0')
                        c = ' ';
                }
                info.cmd = cmdline;
            }
        }

        return info;
    }
};

} // namespace homeshell
