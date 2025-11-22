#pragma once

#include <homeshell/Command.hpp>

namespace homeshell
{

/**
 * @brief Exit the shell
 *
 * Terminates the shell session, returning control to the parent process.
 *
 * @details The exit command cleanly shuts down the shell by:
 *          - Returning a special exit status
 *          - Allowing the shell REPL to terminate gracefully
 *          - Triggering cleanup routines (unmounting, history save, etc.)
 *
 *          Command syntax:
 *          ```
 *          exit
 *          ```
 *
 *          Behavior:
 *          - Always succeeds
 *          - Returns Status::exit() to signal shell termination
 *          - No arguments accepted
 *          - Encrypted mounts are automatically unmounted
 *          - Command history is saved
 *
 * Example usage:
 * ```
 * exit                        # Exit the shell
 * ```
 *
 * @note Unlike bash exit, this does not support:
 *       - Exit codes (exit 1, exit 127, etc.)
 *       - Always exits with code 0
 */
class ExitCommand : public ICommand
{
public:
    std::string getName() const override
    {
        return "exit";
    }

    std::string getDescription() const override
    {
        return "Exit the shell";
    }

    CommandType getType() const override
    {
        return CommandType::Synchronous;
    }

    /**
     * @brief Execute the exit command
     * @param context Command context (no arguments used)
     * @return Status::exit() to signal shell termination
     */
    Status execute(const CommandContext& context) override
    {
        return Status::exit();
    }
};

} // namespace homeshell
