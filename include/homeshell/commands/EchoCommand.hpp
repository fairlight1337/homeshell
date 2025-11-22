#pragma once

#include <homeshell/Command.hpp>

#include <fmt/core.h>

#include <iostream>

namespace homeshell
{

/**
 * @brief Echo text to standard output
 *
 * Prints all arguments to stdout, separated by spaces, followed by a newline.
 * Commonly used for displaying messages, debugging, or generating output.
 *
 * @details The echo command is one of the simplest shell commands.
 *          It outputs its arguments verbatim, making it useful for:
 *          - Displaying messages to users
 *          - Testing output redirection
 *          - Generating content for pipes
 *          - Debugging shell scripts
 *
 *          Command syntax:
 *          ```
 *          echo [arguments...]
 *          ```
 *
 *          Behavior:
 *          - Arguments are joined with single spaces
 *          - Output always ends with newline
 *          - No arguments produces empty line
 *          - No escape sequence processing
 *
 * Example usage:
 * ```
 * echo Hello World           # Output: Hello World
 * echo "Multiple words"      # Output: Multiple words
 * echo                       # Output: (empty line)
 * echo test > file.txt       # Redirect to file
 * ```
 *
 * @note Unlike bash echo, this does not support:
 *       - -n flag (suppress trailing newline)
 *       - -e flag (interpret escape sequences)
 *       - -E flag (disable escape sequences)
 */
class EchoCommand : public ICommand
{
public:
    std::string getName() const override
    {
        return "echo";
    }

    std::string getDescription() const override
    {
        return "Echo the arguments";
    }

    CommandType getType() const override
    {
        return CommandType::Synchronous;
    }

    /**
     * @brief Execute the echo command
     * @param context Command context with arguments to echo
     * @return Status::ok() always (cannot fail)
     */
    Status execute(const CommandContext& context) override
    {
        for (size_t i = 0; i < context.args.size(); ++i)
        {
            if (i > 0)
            {
                fmt::print(" ");
            }
            fmt::print("{}", context.args[i]);
        }
        fmt::print("\n");
        return Status::ok();
    }
};

} // namespace homeshell
