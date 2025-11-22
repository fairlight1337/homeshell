#pragma once

#include <homeshell/Command.hpp>
#include <homeshell/FilesystemHelper.hpp>
#include <homeshell/VirtualFilesystem.hpp>

#include <fmt/color.h>
#include <fmt/core.h>

namespace homeshell
{

/**
 * @brief List directory contents
 *
 * Displays files and directories in a specified path, with support for
 * detailed listings and human-readable sizes. Works transparently with
 * both regular filesystem and encrypted virtual mounts.
 *
 * @details Features:
 *          - Simple listing (names only)
 *          - Detailed listing with permissions, size, and timestamps (-l)
 *          - Human-readable file sizes (-h with -l)
 *          - Color-coded output (blue for directories, cyan for symlinks)
 *          - Unified view of regular and virtual filesystems
 *
 *          Command syntax:
 *          ```
 *          ls [OPTIONS] [PATH]
 *          ```
 *
 *          Options:
 *          - `-l, --long`: Show detailed listing with permissions, size, mtime
 *          - `-h`: Human-readable sizes (KB, MB, GB) - requires -l
 *          - `--help`: Show help message
 *
 *          Output format (simple):
 *          ```
 *          file1.txt  dir1/  file2.txt
 *          ```
 *
 *          Output format (detailed -l):
 *          ```
 *          -rw-r--r--  1024  2024-11-22 15:30  file1.txt
 *          drwxr-xr-x  4096  2024-11-22 14:20  dir1/
 *          ```
 *
 *          Output format (detailed -lh):
 *          ```
 *          -rw-r--r--  1.0K  2024-11-22 15:30  file1.txt
 *          drwxr-xr-x  4.0K  2024-11-22 14:20  dir1/
 *          ```
 *
 * Example usage:
 * ```
 * ls                    # List current directory
 * ls /home              # List /home directory
 * ls -l                 # Detailed listing
 * ls -lh /var/log       # Human-readable sizes
 * ls /secure            # List encrypted mount
 * ```
 */
class LsCommand : public ICommand
{
public:
    std::string getName() const override
    {
        return "ls";
    }

    std::string getDescription() const override
    {
        return "List directory contents";
    }

    CommandType getType() const override
    {
        return CommandType::Synchronous;
    }

    /**
     * @brief Execute the ls command
     * @param context Command context with optional path and flags
     * @return Status indicating success or failure
     */
    Status execute(const CommandContext& context) override
    {
        std::string path = ".";
        bool show_details = false;
        bool human_readable = false;

        // Parse arguments
        for (const auto& arg : context.args)
        {
            if (arg == "--help")
            {
                printHelp();
                return Status::ok();
            }
            else if (arg == "-l" || arg == "--long")
            {
                show_details = true;
            }
            else if (arg.length() >= 2 && arg[0] == '-' && arg[1] != '-')
            {
                // Handle combined flags like -lh or single flags like -h
                bool has_help = false;
                bool has_other = false;

                for (size_t i = 1; i < arg.length(); ++i)
                {
                    if (arg[i] == 'l')
                    {
                        show_details = true;
                        has_other = true;
                    }
                    else if (arg[i] == 'h')
                    {
                        if (arg.length() == 2)
                        {
                            // -h alone means help
                            has_help = true;
                        }
                        else
                        {
                            // -h with other flags means human-readable
                            human_readable = true;
                            has_other = true;
                        }
                    }
                    else
                    {
                        return Status::error("Unknown flag: -" + std::string(1, arg[i]));
                    }
                }

                if (has_help && !has_other)
                {
                    printHelp();
                    return Status::ok();
                }
            }
            else if (arg[0] != '-')
            {
                path = arg;
            }
            else
            {
                return Status::error("Unknown option: " + arg);
            }
        }

        // Check if path exists
        auto& vfs = VirtualFilesystem::getInstance();

        if (!vfs.exists(path))
        {
            return Status::error("Path does not exist: " + path);
        }

        if (!vfs.isDirectory(path))
        {
            return Status::error("Not a directory: " + path);
        }

        // Get directory contents
        auto entries = vfs.listDirectory(path);

        if (entries.empty())
        {
            fmt::print("(empty directory)\n");
            return Status::ok();
        }

        // Display entries
        if (show_details)
        {
            displayDetailed(entries, human_readable);
        }
        else
        {
            displaySimple(entries);
        }

        return Status::ok();
    }

private:
    void printHelp() const
    {
        fmt::print("Usage: ls [OPTIONS] [PATH]\n\n");
        fmt::print("List directory contents\n\n");
        fmt::print("Options:\n");
        fmt::print("  -l, --long    Show detailed listing with permissions and sizes\n");
        fmt::print("  -h            Use human-readable sizes (use with -l)\n");
        fmt::print("  --help        Show this help message\n\n");
        fmt::print("Examples:\n");
        fmt::print("  ls\n");
        fmt::print("  ls -l\n");
        fmt::print("  ls -lh /home/user\n");
    }

    void displaySimple(const std::vector<VirtualFileInfo>& entries) const
    {
        for (const auto& entry : entries)
        {
            if (entry.is_directory)
            {
                fmt::print(fg(fmt::color::blue) | fmt::emphasis::bold, "{}/\n", entry.name);
            }
            else
            {
                fmt::print("{}\n", entry.name);
            }
        }
    }

    void displayDetailed(const std::vector<VirtualFileInfo>& entries, bool human_readable) const
    {
        for (const auto& entry : entries)
        {
            char type = entry.is_directory ? 'd' : '-';

            std::string size_str;
            if (human_readable)
            {
                size_str = FilesystemHelper::formatSize(entry.size);
            }
            else
            {
                size_str = std::to_string(entry.size);
            }

            fmt::print("{} {:>10}  ", type, size_str);

            if (entry.is_directory)
            {
                fmt::print(fg(fmt::color::blue) | fmt::emphasis::bold, "{}/\n", entry.name);
            }
            else
            {
                fmt::print("{}\n", entry.name);
            }
        }
    }
};

} // namespace homeshell
