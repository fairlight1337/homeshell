#pragma once

#include <homeshell/Command.hpp>
#include <homeshell/VirtualFilesystem.hpp>

#include <fmt/core.h>

namespace homeshell
{

/**
 * @brief Change current working directory
 *
 * Changes the shell's current directory to the specified path.
 * Works transparently with both regular filesystem and virtual mounts.
 *
 * @details Features:
 *          - Change to absolute or relative paths
 *          - Navigate to home directory when called without arguments
 *          - Validates that target exists and is a directory
 *          - Updates the shell's working directory context
 *          - Cross-platform home directory detection ($HOME or %USERPROFILE%)
 *
 *          Command syntax:
 *          ```
 *          cd [directory]
 *          ```
 *
 *          Behavior:
 *          - `cd` - Change to home directory
 *          - `cd /path` - Change to absolute path
 *          - `cd dir` - Change to relative path
 *          - `cd ..` - Move up one directory level
 *          - `cd /secure` - Navigate into virtual mount
 *
 * Example usage:
 * ```
 * cd /home/user       # Change to /home/user
 * cd ..               # Go up one level
 * cd                  # Go to home directory
 * cd /secure/docs     # Navigate into encrypted mount
 * ```
 *
 * @note On Windows, %USERPROFILE% is used if $HOME is not set
 */
class CdCommand : public ICommand
{
public:
    std::string getName() const override
    {
        return "cd";
    }

    std::string getDescription() const override
    {
        return "Change current directory";
    }

    CommandType getType() const override
    {
        return CommandType::Synchronous;
    }

    /**
     * @brief Execute the cd command
     * @param context Command context with optional directory argument
     * @return Status::ok() on success, Status::error() if directory invalid
     */
    Status execute(const CommandContext& context) override
    {
        std::string target;
        auto& vfs = VirtualFilesystem::getInstance();

        if (context.args.empty())
        {
            // cd with no args - go to home directory
            const char* home = std::getenv("HOME");
#ifdef _WIN32
            if (!home)
            {
                home = std::getenv("USERPROFILE");
            }
#endif
            if (home)
            {
                target = home;
            }
            else
            {
                return Status::error("HOME directory not set");
            }
        }
        else
        {
            target = context.args[0];
        }

        // Check if target exists and is a directory
        if (!vfs.exists(target))
        {
            return Status::error("Directory does not exist: " + target);
        }

        if (!vfs.isDirectory(target))
        {
            return Status::error("Not a directory: " + target);
        }

        // Change directory
        std::string result_path;
        if (!vfs.changeDirectory(target, result_path))
        {
            return Status::error("Failed to change directory to: " + target);
        }

        // Optionally show new directory
        if (context.verbose)
        {
            fmt::print("Changed to: {}\n", result_path);
        }

        return Status::ok();
    }
};

} // namespace homeshell
