#pragma once

#include <homeshell/Command.hpp>
#include <homeshell/Status.hpp>

#include <fmt/color.h>

#include <csignal>
#include <string>
#include <vector>

namespace homeshell
{

/**
 * @brief Send signals to processes
 *
 * @details The `kill` command sends signals to one or more processes by PID (Process ID).
 * Signals can be used to terminate, interrupt, pause, or otherwise control processes.
 * By default, sends SIGTERM for graceful termination.
 *
 * **Usage:**
 * @code
 * kill [-s SIGNAL] <pid> [pid2 ...]
 * kill -SIGNAL <pid> [pid2 ...]
 * @endcode
 *
 * **Supported Signals:**
 * - `TERM` (15) - Terminate gracefully (default)
 * - `KILL` (9) - Force kill immediately (cannot be caught)
 * - `HUP` (1) - Hangup (reload configuration)
 * - `INT` (2) - Interrupt (like Ctrl+C)
 * - `QUIT` (3) - Quit with core dump
 * - `STOP` (19) - Stop/pause process (cannot be caught)
 * - `CONT` (18) - Continue stopped process
 *
 * **Signal Format Options:**
 * 1. Signal name: `-s TERM`, `-s KILL`
 * 2. Signal number: `-s 15`, `-s 9`
 * 3. Short format: `-TERM`, `-9`
 *
 * **Examples:**
 * @code
 * kill 1234                # Send SIGTERM to PID 1234
 * kill -9 1234             # Force kill PID 1234
 * kill -s KILL 1234        # Force kill (alternative syntax)
 * kill -TERM 1234 5678     # Gracefully terminate multiple processes
 * kill -s HUP 9999         # Send hangup signal (reload config)
 * @endcode
 *
 * **Return Status:**
 * - Success: All signals sent successfully
 * - Error: One or more signals failed to send
 *
 * **Error Handling:**
 * - Invalid PID: Shows error but continues with remaining PIDs
 * - Permission denied: Process owned by different user
 * - Process not found: No such process exists
 *
 * @note Sending signals to processes owned by other users requires root privileges
 * @note SIGKILL (9) and SIGSTOP (19) cannot be caught or ignored by processes
 * @note Use SIGTERM (default) for graceful shutdown before trying SIGKILL
 */
class KillCommand : public ICommand
{
public:
    std::string getName() const override
    {
        return "kill";
    }

    std::string getDescription() const override
    {
        return "Send signal to a process";
    }

    CommandType getType() const override
    {
        return CommandType::Synchronous;
    }

    Status execute(const CommandContext& context) override
    {
        if (context.args.empty())
        {
            fmt::print(fg(fmt::color::red), "Error: No PID specified\n");
            fmt::print("Usage: kill [-s SIGNAL] <pid> [pid2 ...]\n");
            fmt::print("  Signals: TERM (default), KILL, HUP, INT, QUIT, STOP, CONT\n");
            return Status::error("No PID specified");
        }

        int signal = SIGTERM;
        size_t start_idx = 0;

        // Parse signal option
        if (context.args[0] == "-s" && context.args.size() >= 3)
        {
            std::string signal_name = context.args[1];
            signal = parseSignal(signal_name);

            if (signal == -1)
            {
                fmt::print(fg(fmt::color::red), "Error: Unknown signal '{}'\n", signal_name);
                return Status::error("Unknown signal");
            }

            start_idx = 2;
        }
        else if (context.args[0][0] == '-' && context.args[0].size() > 1)
        {
            // Handle -SIGNAL format (e.g., -9, -KILL)
            std::string signal_str = context.args[0].substr(1);
            signal = parseSignal(signal_str);

            if (signal == -1)
            {
                fmt::print(fg(fmt::color::red), "Error: Unknown signal '{}'\n", signal_str);
                return Status::error("Unknown signal");
            }

            start_idx = 1;
        }

        if (start_idx >= context.args.size())
        {
            fmt::print(fg(fmt::color::red), "Error: No PID specified\n");
            return Status::error("No PID specified");
        }

        bool all_success = true;

        // Send signal to each PID
        for (size_t i = start_idx; i < context.args.size(); ++i)
        {
            try
            {
                int pid = std::stoi(context.args[i]);

                if (pid <= 0)
                {
                    fmt::print(fg(fmt::color::red), "Error: Invalid PID {}\n", pid);
                    all_success = false;
                    continue;
                }

                if (::kill(pid, signal) == 0)
                {
                    fmt::print(fg(fmt::color::green), "Signal {} sent to PID {}\n",
                               getSignalName(signal), pid);
                }
                else
                {
                    fmt::print(fg(fmt::color::red), "Error: Failed to send signal to PID {}: {}\n",
                               pid, strerror(errno));
                    all_success = false;
                }
            }
            catch (...)
            {
                fmt::print(fg(fmt::color::red), "Error: Invalid PID '{}'\n", context.args[i]);
                all_success = false;
            }
        }

        return all_success ? Status::ok() : Status::error("Some signals failed");
    }

private:
    int parseSignal(const std::string& signal_str)
    {
        // Try to parse as number first
        try
        {
            return std::stoi(signal_str);
        }
        catch (...)
        {
            // Parse as name
            if (signal_str == "TERM" || signal_str == "15")
                return SIGTERM;
            if (signal_str == "KILL" || signal_str == "9")
                return SIGKILL;
            if (signal_str == "HUP" || signal_str == "1")
                return SIGHUP;
            if (signal_str == "INT" || signal_str == "2")
                return SIGINT;
            if (signal_str == "QUIT" || signal_str == "3")
                return SIGQUIT;
            if (signal_str == "STOP" || signal_str == "19")
                return SIGSTOP;
            if (signal_str == "CONT" || signal_str == "18")
                return SIGCONT;

            return -1;
        }
    }

    std::string getSignalName(int signal)
    {
        switch (signal)
        {
        case SIGTERM:
            return "SIGTERM";
        case SIGKILL:
            return "SIGKILL";
        case SIGHUP:
            return "SIGHUP";
        case SIGINT:
            return "SIGINT";
        case SIGQUIT:
            return "SIGQUIT";
        case SIGSTOP:
            return "SIGSTOP";
        case SIGCONT:
            return "SIGCONT";
        default:
            return std::to_string(signal);
        }
    }
};

} // namespace homeshell
