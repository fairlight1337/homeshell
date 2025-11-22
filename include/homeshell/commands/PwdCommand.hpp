#pragma once

#include <homeshell/Command.hpp>
#include <homeshell/VirtualFilesystem.hpp>

#include <fmt/core.h>

namespace homeshell
{

/**
 * @brief Print working directory
 *
 * Displays the shell's current working directory path.
 * Shows both regular filesystem and virtual mount paths.
 *
 * @details The pwd command outputs the absolute path of the current
 *          directory to standard output. It works transparently across
 *          regular filesystem boundaries and virtual mount points.
 *
 *          Command syntax:
 *          ```
 *          pwd
 *          ```
 *
 *          Output format:
 *          ```
 *          /current/working/directory
 *          ```
 *
 * Example usage:
 * ```
 * $ cd /home/user/documents
 * $ pwd
 * /home/user/documents
 *
 * $ cd /secure
 * $ pwd
 * /secure
 * ```
 *
 * @note Unlike bash pwd, this does not support -L or -P flags for
 *       logical vs. physical paths.
 */
class PwdCommand : public ICommand
{
public:
    std::string getName() const override
    {
        return "pwd";
    }

    std::string getDescription() const override
    {
        return "Print working directory";
    }

    CommandType getType() const override
    {
        return CommandType::Synchronous;
    }

    /**
     * @brief Execute the pwd command
     * @param context Command context (no arguments used)
     * @return Status::ok() always (cannot fail)
     */
    Status execute(const CommandContext& context) override
    {
        auto& vfs = VirtualFilesystem::getInstance();
        fmt::print("{}\n", vfs.getCurrentDirectory());
        return Status::ok();
    }
};

} // namespace homeshell
