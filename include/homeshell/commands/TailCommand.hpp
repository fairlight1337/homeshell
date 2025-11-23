#pragma once

#include <homeshell/Command.hpp>
#include <homeshell/Status.hpp>
#include <homeshell/VirtualFilesystem.hpp>

#include <fmt/color.h>

#include <atomic>
#include <chrono>
#include <deque>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace homeshell
{

/**
 * @brief Display the last part of files
 *
 * The tail command outputs the last part (10 lines by default) of one or more files.
 * It's particularly useful for viewing the end of log files or monitoring files in real-time.
 *
 * @details Features:
 *          - Display last N lines of a file
 *          - Follow mode for monitoring files in real-time
 *          - Works with both regular and encrypted virtual filesystem
 *          - Supports stdin when no file specified
 *          - Cancellable with Ctrl+C in follow mode
 *
 *          Command syntax:
 *          ```
 *          tail [options] <file>
 *          tail [options]           # Read from stdin
 *          ```
 *
 *          Options:
 *          - `-n <num>` - Show last N lines (default: 10)
 *          - `-f` - Follow mode: output appended data as file grows
 *          - `--help` - Show help message
 *
 * Example usage:
 * ```
 * tail file.txt                    # Last 10 lines
 * tail -n 20 file.txt             # Last 20 lines
 * tail -f /var/log/syslog         # Follow log file
 * tail -n 50 -f app.log           # Last 50 lines, then follow
 * cat file.txt | tail -n 5        # Last 5 lines from stdin
 * ```
 *
 * @note Follow mode (-f) continues until interrupted with Ctrl+C
 * @note Works seamlessly with encrypted virtual filesystem paths
 */
class TailCommand : public ICommand
{
public:
    std::string getName() const override
    {
        return "tail";
    }

    std::string getDescription() const override
    {
        return "Display the last part of files";
    }

    CommandType getType() const override
    {
        return CommandType::Synchronous;
    }

    bool supportsCancellation() const override
    {
        return true;
    }

    void cancel() override
    {
        cancelled_.store(true);
    }

    /**
     * @brief Execute the tail command
     * @param context Command context with arguments
     * @return Status::ok() on success, Status::error() on failure
     */
    Status execute(const CommandContext& context) override
    {
        int num_lines = 10;
        bool follow = false;
        std::string filename;

        // Parse arguments
        for (size_t i = 0; i < context.args.size(); ++i)
        {
            if (context.args[i] == "--help")
            {
                showHelp();
                return Status::ok();
            }
            else if (context.args[i] == "-n" && i + 1 < context.args.size())
            {
                try
                {
                    num_lines = std::stoi(context.args[++i]);
                    if (num_lines < 0)
                    {
                        fmt::print(fg(fmt::color::red),
                                   "Error: Number of lines must be positive\n");
                        return Status::error("Invalid line count");
                    }
                }
                catch (...)
                {
                    fmt::print(fg(fmt::color::red), "Error: Invalid number '{}'\n",
                               context.args[i]);
                    return Status::error("Invalid line count");
                }
            }
            else if (context.args[i] == "-f")
            {
                follow = true;
            }
            else if (context.args[i][0] == '-')
            {
                fmt::print(fg(fmt::color::red), "Error: Unknown option '{}'\n", context.args[i]);
                fmt::print("Use --help for usage information\n");
                return Status::error("Unknown option");
            }
            else
            {
                filename = context.args[i];
            }
        }

        // If no filename, read from stdin (for pipe support later)
        if (filename.empty())
        {
            return tailStdin(num_lines);
        }

        // Resolve path through VFS
        auto& vfs = VirtualFilesystem::getInstance();
        auto resolved = vfs.resolvePath(filename);

        // Read file content
        std::string content;
        if (vfs.exists(resolved.full_path))
        {
            if (!vfs.readFile(resolved.full_path, content))
            {
                fmt::print(fg(fmt::color::red), "Error: Failed to read file '{}'\n", filename);
                return Status::error("Read failed");
            }
        }
        else
        {
            // Try regular filesystem
            std::ifstream file(filename);
            if (!file.good())
            {
                fmt::print(fg(fmt::color::red), "Error: Cannot open file '{}'\n", filename);
                return Status::error("File not found");
            }

            std::stringstream buffer;
            buffer << file.rdbuf();
            content = buffer.str();
        }

        // Display last N lines
        displayLastLines(content, num_lines);

        // Follow mode
        if (follow)
        {
            // Follow mode only works with regular files (not VFS)
            if (vfs.exists(resolved.full_path))
            {
                fmt::print(fg(fmt::color::yellow),
                           "Warning: Follow mode not supported for encrypted virtual filesystem\n");
                return Status::ok();
            }

            return followFile(filename, content.size());
        }

        return Status::ok();
    }

private:
    std::atomic<bool> cancelled_{false};

    void showHelp()
    {
        fmt::print("Usage: tail [options] <file>\n");
        fmt::print("       tail [options]           # Read from stdin\n");
        fmt::print("\n");
        fmt::print("Display the last part of files.\n");
        fmt::print("\n");
        fmt::print("Options:\n");
        fmt::print("  -n <num>    Show last N lines (default: 10)\n");
        fmt::print("  -f          Follow mode: output appended data as file grows\n");
        fmt::print("  --help      Show this help message\n");
        fmt::print("\n");
        fmt::print("Examples:\n");
        fmt::print("  tail file.txt              # Last 10 lines\n");
        fmt::print("  tail -n 20 file.txt        # Last 20 lines\n");
        fmt::print("  tail -f /var/log/syslog    # Follow log file\n");
        fmt::print("  tail -n 50 -f app.log      # Last 50 lines, then follow\n");
    }

    /**
     * @brief Display last N lines from content
     * @param content File content
     * @param num_lines Number of lines to display
     */
    void displayLastLines(const std::string& content, int num_lines)
    {
        std::deque<std::string> lines;
        std::stringstream ss(content);
        std::string line;

        // Collect all lines
        while (std::getline(ss, line))
        {
            lines.push_back(line);

            // Keep only last N lines in memory
            if (static_cast<int>(lines.size()) > num_lines)
            {
                lines.pop_front();
            }
        }

        // Display the lines
        for (const auto& l : lines)
        {
            fmt::print("{}\n", l);
        }
    }

    /**
     * @brief Read and display last N lines from stdin
     * @param num_lines Number of lines to display
     * @return Status
     */
    Status tailStdin(int num_lines)
    {
        std::deque<std::string> lines;
        std::string line;

        // Read all lines from stdin
        while (std::getline(std::cin, line))
        {
            lines.push_back(line);

            // Keep only last N lines
            if (static_cast<int>(lines.size()) > num_lines)
            {
                lines.pop_front();
            }
        }

        // Display the lines
        for (const auto& l : lines)
        {
            fmt::print("{}\n", l);
        }

        return Status::ok();
    }

    /**
     * @brief Follow a file and display new lines as they appear
     * @param filename File to follow
     * @param initial_size Initial file size to start from
     * @return Status
     */
    Status followFile(const std::string& filename, size_t initial_size)
    {
        fmt::print(fg(fmt::color::cyan), "==> Following {} (Ctrl+C to stop) <==\n", filename);

        std::ifstream file;
        size_t last_position = initial_size;

        while (!cancelled_.load())
        {
            file.open(filename, std::ios::binary);
            if (!file.good())
            {
                fmt::print(fg(fmt::color::red), "Error: Cannot open file '{}'\n", filename);
                return Status::error("File not accessible");
            }

            // Seek to last known position
            file.seekg(0, std::ios::end);
            size_t current_size = file.tellg();

            // If file grew, read new data
            if (current_size > last_position)
            {
                file.seekg(last_position);
                std::string line;

                while (std::getline(file, line))
                {
                    fmt::print("{}\n", line);
                }

                last_position = current_size;
            }
            // If file truncated (rotated), start from beginning
            else if (current_size < last_position)
            {
                fmt::print(fg(fmt::color::yellow),
                           "\n==> File truncated, restarting from beginning <==\n");
                last_position = 0;
            }

            file.close();

            // Sleep briefly before checking again
            std::this_thread::sleep_for(std::chrono::milliseconds(250));
        }

        fmt::print(fg(fmt::color::cyan), "\n==> Stopped following {} <==\n", filename);
        return Status::ok();
    }
};

} // namespace homeshell
