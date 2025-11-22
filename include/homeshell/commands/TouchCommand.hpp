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
 * @brief Create empty file or update timestamp
 *
 * Creates a new empty file if it doesn't exist, or updates the modification
 * timestamp if it does. Works with both regular filesystem and encrypted mounts.
 *
 * @details The touch command is useful for:
 *          - Creating placeholder/template files
 *          - Testing file creation permissions
 *          - Updating file modification times
 *          - Triggering file watchers/build systems
 *
 *          Features:
 *          - Create new empty files
 *          - Update existing file timestamps
 *          - Works with regular filesystem
 *          - Works with encrypted virtual mounts
 *          - Validates target is not a directory
 *
 *          Command syntax:
 *          ```
 *          touch <filename>
 *          ```
 *
 *          Behavior:
 *          - If file doesn't exist → create empty file
 *          - If file exists → update modification timestamp
 *          - If path is directory → error
 *
 *          Errors reported:
 *          - Path is a directory
 *          - Permission denied
 *          - Quota exceeded (in virtual mounts)
 *          - Parent directory doesn't exist
 *
 * Example usage:
 * ```
 * touch newfile.txt           # Create empty file
 * touch /tmp/marker           # Create marker file
 * touch existing.log          # Update timestamp
 * touch /secure/notes.txt     # Create in encrypted mount
 * ```
 *
 * @note Unlike bash touch, this does not support:
 *       - Setting specific timestamps (-t flag)
 *       - Using another file's timestamp (-r flag)
 *       - No-create mode (-c flag)
 *       - Multiple file arguments
 */
class TouchCommand : public ICommand
{
public:
    std::string getName() const override
    {
        return "touch";
    }

    std::string getDescription() const override
    {
        return "Create a new empty file or update timestamp";
    }

    CommandType getType() const override
    {
        return CommandType::Synchronous;
    }

    /**
     * @brief Execute the touch command
     * @param context Command context with filename argument
     * @return Status::ok() on success, Status::error() if operation fails
     */
    Status execute(const CommandContext& context) override
    {
        if (context.args.empty())
        {
            fmt::print(fg(fmt::color::red), "Error: No file specified\n");
            fmt::print("Usage: touch <file>\n");
            return Status::error("No file specified");
        }

        const std::string& path = context.args[0];
        auto& vfs = VirtualFilesystem::getInstance();

        if (vfs.exists(path) && vfs.isDirectory(path))
        {
            fmt::print(fg(fmt::color::red), "Error: '{}' is a directory\n", path);
            return Status::error("Is a directory: " + path);
        }

        if (!vfs.exists(path))
        {
            // Create new empty file
            if (!vfs.writeFile(path, ""))
            {
                fmt::print(fg(fmt::color::red), "Error: Failed to create file '{}'\n", path);
                return Status::error("Failed to create file: " + path);
            }
            fmt::print(fg(fmt::color::green), "File '{}' created\n", path);
        }
        else
        {
            // File exists - update timestamp by re-writing
            std::string content;
            if (vfs.readFile(path, content))
            {
                vfs.writeFile(path, content);
                fmt::print(fg(fmt::color::green), "File '{}' timestamp updated\n", path);
            }
        }

        return Status::ok();
    }
};

} // namespace homeshell
