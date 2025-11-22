#pragma once

#include <homeshell/Command.hpp>
#include <homeshell/Status.hpp>
#include <homeshell/VirtualFilesystem.hpp>

#include <fmt/color.h>

#include <string>
#include <vector>

namespace homeshell
{

/**
 * @brief Remove files or directories
 *
 * Removes (deletes) files or directories at the specified path.
 * Works with both regular filesystem and encrypted virtual mounts.
 *
 * @details The rm command removes files or empty directories. It does
 *          not have recursive directory removal (no -r flag) for safety.
 *
 *          Features:
 *          - Remove regular files
 *          - Remove empty directories
 *          - Remove files in encrypted virtual mounts
 *          - Validates path exists before removal
 *          - Confirms removal with success message
 *
 *          Command syntax:
 *          ```
 *          rm <path>
 *          ```
 *
 *          Errors reported:
 *          - Path doesn't exist
 *          - Permission denied
 *          - Directory not empty (use rmdir separately for each item)
 *
 * Example usage:
 * ```
 * rm oldfile.txt              # Remove file in current directory
 * rm /home/user/temp.log      # Remove with absolute path
 * rm /secure/oldnotes.txt     # Remove from encrypted mount
 * ```
 *
 * @warning This command permanently deletes files. There is no trash/recycle bin.
 *
 * @note Unlike bash rm, this does not support:
 *       - Recursive directory removal (-r/-R flags)
 *       - Force removal (-f flag)
 *       - Interactive confirmation (-i flag)
 *       - Multiple file arguments
 */
class RmCommand : public ICommand
{
public:
    std::string getName() const override
    {
        return "rm";
    }

    std::string getDescription() const override
    {
        return "Remove files or directories";
    }

    CommandType getType() const override
    {
        return CommandType::Synchronous;
    }

    /**
     * @brief Execute the rm command
     * @param context Command context with file/directory path argument
     * @return Status::ok() on success, Status::error() if removal fails
     */
    Status execute(const CommandContext& context) override
    {
        if (context.args.empty())
        {
            fmt::print(fg(fmt::color::red), "Error: No path specified\n");
            fmt::print("Usage: rm <path>\n");
            return Status::error("No path specified");
        }

        const std::string& path = context.args[0];
        auto& vfs = VirtualFilesystem::getInstance();

        if (!vfs.exists(path))
        {
            fmt::print(fg(fmt::color::red), "Error: '{}' does not exist\n", path);
            return Status::error("Does not exist: " + path);
        }

        if (!vfs.remove(path))
        {
            fmt::print(fg(fmt::color::red), "Error: Failed to remove '{}'\n", path);
            return Status::error("Failed to remove: " + path);
        }

        fmt::print(fg(fmt::color::green), "Removed '{}'\n", path);
        return Status::ok();
    }
};

} // namespace homeshell
