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
 * @brief Create a new directory
 *
 * Creates a new directory at the specified path. Works with both
 * regular filesystem and encrypted virtual mounts.
 *
 * @details The mkdir command creates a single directory. It does not
 *          create parent directories (no -p flag support yet).
 *
 *          Features:
 *          - Create directory in regular filesystem
 *          - Create directory in encrypted virtual mounts
 *          - Validates path doesn't already exist
 *          - Confirms creation with success message
 *
 *          Command syntax:
 *          ```
 *          mkdir <directory>
 *          ```
 *
 *          Errors reported:
 *          - Directory already exists
 *          - Parent directory doesn't exist
 *          - Permission denied
 *          - Quota exceeded (in virtual mounts)
 *
 * Example usage:
 * ```
 * mkdir newdir               # Create in current directory
 * mkdir /home/user/docs      # Create with absolute path
 * mkdir /secure/private      # Create in encrypted mount
 * ```
 *
 * @note Unlike bash mkdir, this does not support:
 *       - Creating parent directories (-p flag)
 *       - Setting permissions during creation (-m flag)
 *       - Multiple directory arguments
 */
class MkdirCommand : public ICommand
{
public:
    std::string getName() const override
    {
        return "mkdir";
    }

    std::string getDescription() const override
    {
        return "Create a new directory";
    }

    CommandType getType() const override
    {
        return CommandType::Synchronous;
    }

    /**
     * @brief Execute the mkdir command
     * @param context Command context with directory path argument
     * @return Status::ok() on success, Status::error() if creation fails
     */
    Status execute(const CommandContext& context) override
    {
        if (context.args.empty())
        {
            fmt::print(fg(fmt::color::red), "Error: No directory specified\n");
            fmt::print("Usage: mkdir <directory>\n");
            return Status::error("No directory specified");
        }

        const std::string& path = context.args[0];
        auto& vfs = VirtualFilesystem::getInstance();

        if (vfs.exists(path))
        {
            fmt::print(fg(fmt::color::red), "Error: '{}' already exists\n", path);
            return Status::error("Already exists: " + path);
        }

        if (!vfs.createDirectory(path))
        {
            fmt::print(fg(fmt::color::red), "Error: Failed to create directory '{}'\n", path);
            return Status::error("Failed to create directory: " + path);
        }

        fmt::print(fg(fmt::color::green), "Directory '{}' created\n", path);
        return Status::ok();
    }
};

} // namespace homeshell
