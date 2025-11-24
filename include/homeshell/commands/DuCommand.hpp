/**
 * @file DuCommand.hpp
 * @brief Display disk usage for files and directories
 *
 * This command estimates file space usage, displaying the size of files and
 * directories recursively. Information is gathered by traversing the filesystem
 * and summing file sizes.
 *
 * @author Homeshell Development Team
 * @date 2025
 */

#pragma once

#include <homeshell/Command.hpp>
#include <homeshell/Status.hpp>
#include <homeshell/VirtualFilesystem.hpp>

#include <sys/stat.h>

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace homeshell
{

/**
 * @class DuCommand
 * @brief Command to display disk usage
 *
 * Displays disk usage information for files and directories:
 * - Recursive directory traversal
 * - File and directory size calculations
 * - Human-readable size formatting
 * - Depth control for recursion
 * - Summary and detailed modes
 *
 * @section Usage
 * @code
 * du                    # Show usage for current directory
 * du -h                 # Human-readable sizes
 * du -s /home           # Summary only for /home
 * du --max-depth=2      # Limit recursion to 2 levels
 * du -a                 # Show all files, not just directories
 * du -c dir1 dir2       # Show total at the end
 * du --help             # Show help message
 * @endcode
 */
class DuCommand : public ICommand
{
public:
    /**
     * @brief Get command name
     * @return Command name "du"
     */
    std::string getName() const override
    {
        return "du";
    }

    /**
     * @brief Get command description
     * @return Brief description of the command
     */
    std::string getDescription() const override
    {
        return "Display disk usage for files and directories";
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
     * @brief Execute the du command
     * @param context Command context with arguments
     * @return Status indicating success or failure
     */
    Status execute(const CommandContext& context) override
    {
        bool human_readable = false;
        bool summary_only = false;
        bool show_all = false;
        bool show_total = false;
        int max_depth = -1; // -1 means unlimited
        std::vector<std::string> target_paths;

        // Parse arguments
        for (size_t i = 0; i < context.args.size(); ++i)
        {
            const std::string& arg = context.args[i];

            if (arg == "--help")
            {
                showHelp();
                return Status::ok();
            }
            else if (arg == "--human-readable")
            {
                human_readable = true;
            }
            else if (arg == "--summarize")
            {
                summary_only = true;
            }
            else if (arg == "--all")
            {
                show_all = true;
            }
            else if (arg == "--total")
            {
                show_total = true;
            }
            else if (arg.find("--max-depth=") == 0)
            {
                std::string depth_str = arg.substr(12);
                try
                {
                    max_depth = std::stoi(depth_str);
                    if (max_depth < 0)
                    {
                        return Status::error("Invalid max-depth value: " + depth_str);
                    }
                }
                catch (...)
                {
                    return Status::error("Invalid max-depth value: " + depth_str);
                }
            }
            else if (arg[0] == '-' && arg.length() > 1 && arg[1] != '-')
            {
                // Short options
                for (size_t j = 1; j < arg.length(); ++j)
                {
                    if (arg[j] == 'h')
                    {
                        human_readable = true;
                    }
                    else if (arg[j] == 's')
                    {
                        summary_only = true;
                    }
                    else if (arg[j] == 'a')
                    {
                        show_all = true;
                    }
                    else if (arg[j] == 'c')
                    {
                        show_total = true;
                    }
                    else if (arg[j] == 'd')
                    {
                        // -d N format
                        if (j + 1 < arg.length())
                        {
                            // Rest of this arg is the number
                            std::string depth_str = arg.substr(j + 1);
                            try
                            {
                                max_depth = std::stoi(depth_str);
                                if (max_depth < 0)
                                {
                                    return Status::error("Invalid depth value: " + depth_str);
                                }
                            }
                            catch (...)
                            {
                                return Status::error("Invalid depth value: " + depth_str);
                            }
                            break; // Done parsing this arg
                        }
                        else if (i + 1 < context.args.size())
                        {
                            // Next arg is the number
                            std::string depth_str = context.args[++i];
                            try
                            {
                                max_depth = std::stoi(depth_str);
                                if (max_depth < 0)
                                {
                                    return Status::error("Invalid depth value: " + depth_str);
                                }
                            }
                            catch (...)
                            {
                                return Status::error("Invalid depth value: " + depth_str);
                            }
                            break; // Done parsing this arg
                        }
                        else
                        {
                            return Status::error("Option -d requires an argument");
                        }
                    }
                    else
                    {
                        return Status::error("Unknown option: -" + std::string(1, arg[j]) +
                                             "\nUse --help for usage information");
                    }
                }
            }
            else if (arg[0] == '-')
            {
                return Status::error("Unknown option: " + arg +
                                     "\nUse --help for usage information");
            }
            else
            {
                // It's a path argument
                target_paths.push_back(arg);
            }
        }

        // If no paths specified, use current directory
        if (target_paths.empty())
        {
            target_paths.push_back(".");
        }

        // Calculate and display usage for each path
        uint64_t grand_total = 0;
        auto& vfs = VirtualFilesystem::getInstance();

        for (const auto& path : target_paths)
        {
            // Resolve to absolute path
            auto resolved = vfs.resolvePath(path);

            if (!vfs.exists(resolved.full_path))
            {
                std::cerr << "du: cannot access '" << path << "': No such file or directory\n";
                continue;
            }

            uint64_t total_size = calculateUsage(resolved.full_path, 0, max_depth, summary_only,
                                                 show_all, human_readable);
            grand_total += total_size;
        }

        // Show grand total if requested
        if (show_total && target_paths.size() > 1)
        {
            if (human_readable)
            {
                std::cout << formatSize(grand_total) << "\ttotal\n";
            }
            else
            {
                std::cout << (grand_total / 1024) << "\ttotal\n";
            }
        }

        return Status::ok();
    }

private:
    /**
     * @brief Display help information
     */
    void showHelp() const
    {
        std::cout << "Usage: du [OPTION]... [FILE]...\n\n";
        std::cout << "Summarize disk usage of each FILE, recursively for directories.\n\n";
        std::cout << "Options:\n";
        std::cout << "  -a, --all            Show counts for all files, not just directories\n";
        std::cout << "  -c, --total          Produce a grand total\n";
        std::cout << "  -d N                 Print total for directory only if it is N or fewer "
                     "levels\n";
        std::cout
            << "  -h, --human-readable Print sizes in human readable format (e.g., 1K 234M 2G)\n";
        std::cout << "  -s, --summarize      Display only a total for each argument\n";
        std::cout << "  --max-depth=N        Print total for directory only if it is N or fewer "
                     "levels\n";
        std::cout << "  --help               Show this help message\n\n";
        std::cout << "Examples:\n";
        std::cout << "  du                   # Show usage for current directory\n";
        std::cout << "  du -h                # Human-readable sizes\n";
        std::cout << "  du -s /home          # Summary only\n";
        std::cout << "  du --max-depth=2     # Limit to 2 levels deep\n";
        std::cout << "  du -d 1              # Same as --max-depth=1\n";
        std::cout << "  du -a                # Show all files\n";
        std::cout << "  du -c dir1 dir2      # Show total for multiple directories\n";
        std::cout << "  du -hsc *            # Human-readable summary with total\n";
    }

    /**
     * @brief Format size in human-readable format
     * @param size Size in bytes
     * @return Formatted string
     */
    static std::string formatSize(uint64_t size)
    {
        const char* units[] = {"B", "K", "M", "G", "T", "P"};
        int unit_idx = 0;
        double dsize = static_cast<double>(size);

        // Start from KB since du traditionally shows in KB
        dsize /= 1024.0;
        unit_idx = 1;

        while (dsize >= 1024.0 && unit_idx < 5)
        {
            dsize /= 1024.0;
            unit_idx++;
        }

        std::ostringstream oss;
        if (dsize < 10.0)
        {
            oss << std::fixed << std::setprecision(1) << dsize << units[unit_idx];
        }
        else
        {
            oss << std::fixed << std::setprecision(0) << dsize << units[unit_idx];
        }
        return oss.str();
    }

    /**
     * @brief Calculate disk usage recursively
     * @param path Path to calculate usage for
     * @param current_depth Current recursion depth
     * @param max_depth Maximum depth to recurse (-1 for unlimited)
     * @param summary_only Only show summary for this path
     * @param show_all Show all files, not just directories
     * @param human_readable Format output in human-readable format
     * @return Total size in bytes
     */
    uint64_t calculateUsage(const std::string& path, int current_depth, int max_depth,
                            bool summary_only, bool show_all, bool human_readable) const
    {
        auto& vfs = VirtualFilesystem::getInstance();

        if (!vfs.exists(path))
        {
            return 0;
        }

        bool is_dir = vfs.isDirectory(path);
        uint64_t total_size = 0;

        if (is_dir)
        {
            // Directory - sum all contents
            auto entries = vfs.listDirectory(path);

            for (const auto& entry : entries)
            {
                std::string entry_path = entry.path;

                if (entry.is_directory)
                {
                    // Recursive call for subdirectory
                    uint64_t subdir_size = calculateUsage(entry_path, current_depth + 1, max_depth,
                                                          summary_only, show_all, human_readable);
                    total_size += subdir_size;

                    // Print subdirectory size if:
                    // - Not summary mode (unless at depth 0 which we handle later)
                    // - Within max depth (or no max depth)
                    if (!summary_only && current_depth > 0 &&
                        (max_depth < 0 || current_depth < max_depth))
                    {
                        printSize(subdir_size, entry_path, human_readable);
                    }
                }
                else
                {
                    // File
                    int64_t file_size = entry.size;
                    if (file_size > 0)
                        total_size += static_cast<uint64_t>(file_size);

                    // Print file size if -a option and within depth
                    if (show_all && !summary_only && (max_depth < 0 || current_depth < max_depth))
                    {
                        printSize(file_size > 0 ? static_cast<uint64_t>(file_size) : 0, entry_path,
                                  human_readable);
                    }
                }
            }

            // Print directory total if:
            // - Summary mode and at top level (depth 0)
            // - At top level and not summary mode
            // - At depth limit
            if (current_depth == 0)
            {
                // Always print the top level directory
                printSize(total_size, path, human_readable);
            }
            else if (!summary_only && (max_depth < 0 || current_depth <= max_depth))
            {
                // Print intermediate directories if not in summary mode and within depth
                printSize(total_size, path, human_readable);
            }
        }
        else
        {
            // Single file - read its size from filesystem
            struct stat st;
            if (stat(path.c_str(), &st) == 0)
            {
                total_size = static_cast<uint64_t>(st.st_size);
            }

            if (current_depth == 0 || show_all)
            {
                printSize(total_size, path, human_readable);
            }
        }

        return total_size;
    }

    /**
     * @brief Print size and path
     * @param size Size in bytes
     * @param path Path
     * @param human_readable Format in human-readable format
     */
    void printSize(uint64_t size, const std::string& path, bool human_readable) const
    {
        if (human_readable)
        {
            std::cout << formatSize(size) << "\t" << path << "\n";
        }
        else
        {
            // Traditional du shows in KB (1024-byte blocks)
            uint64_t kb = size / 1024;
            std::cout << kb << "\t" << path << "\n";
        }
    }
};

} // namespace homeshell
