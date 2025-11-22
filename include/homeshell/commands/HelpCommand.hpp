#pragma once

#include <homeshell/Command.hpp>

#include <fmt/color.h>
#include <fmt/core.h>

namespace homeshell
{

/**
 * @brief Display available commands
 *
 * Lists all registered shell commands with their brief descriptions.
 * Provides an overview of shell functionality to users.
 *
 * @details The help command queries the CommandRegistry to display
 *          all available commands in an organized format. Useful for:
 *          - Discovering available commands
 *          - Quick reference for command names
 *          - Understanding shell capabilities
 *
 *          Command syntax:
 *          ```
 *          help
 *          ```
 *
 *          Output format:
 *          ```
 *          Available commands:
 *
 *            cd              - Change current directory
 *            ls              - List directory contents
 *            cat             - Display the contents of a file
 *            ...
 *          ```
 *
 *          The output is formatted with:
 *          - Bold "Available commands:" header
 *          - Cyan-colored command names
 *          - Left-aligned command names (15 char width)
 *          - Descriptions from each command's getDescription()
 *
 * Example usage:
 * ```
 * help                        # Show all commands
 * ```
 *
 * @note Individual command help is typically accessed via:
 *       `command --help` (e.g., `ls --help`)
 */
class HelpCommand : public ICommand
{
public:
    std::string getName() const override
    {
        return "help";
    }

    std::string getDescription() const override
    {
        return "Show available commands";
    }

    CommandType getType() const override
    {
        return CommandType::Synchronous;
    }

    /**
     * @brief Execute the help command
     * @param context Command context (no arguments used)
     * @return Status::ok() always (cannot fail)
     */
    Status execute(const CommandContext& context) override
    {
        fmt::print(fmt::emphasis::bold, "Available commands:\n\n");

        auto& registry = CommandRegistry::getInstance();
        for (const auto& pair : registry.getAll())
        {
            fmt::print("  {:<15} - {}\n", fmt::format(fg(fmt::color::cyan), "{}", pair.first),
                       pair.second->getDescription());
        }

        fmt::print("\n");
        return Status::ok();
    }
};

} // namespace homeshell
