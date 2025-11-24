/**
 * @file HostnameCommand.hpp
 * @brief Display or set the system hostname
 *
 * This command displays the current hostname of the system. It reads from
 * /etc/hostname and uses gethostname() system call.
 *
 * @author Homeshell Development Team
 * @date 2025
 */

#pragma once

#include <homeshell/Command.hpp>
#include <homeshell/Status.hpp>

#include <unistd.h>

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace homeshell
{

/**
 * @class HostnameCommand
 * @brief Command to display system hostname
 *
 * Displays the current system hostname using gethostname() system call.
 *
 * @section Usage
 * @code
 * hostname              # Show hostname
 * hostname --help       # Show help message
 * @endcode
 */
class HostnameCommand : public ICommand
{
public:
    /**
     * @brief Get command name
     * @return Command name "hostname"
     */
    std::string getName() const override
    {
        return "hostname";
    }

    /**
     * @brief Get command description
     * @return Brief description of the command
     */
    std::string getDescription() const override
    {
        return "Display the system hostname";
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
     * @brief Execute the hostname command
     * @param context Command context with arguments
     * @return Status indicating success or failure
     */
    Status execute(const CommandContext& context) override
    {
        // Parse arguments
        for (size_t i = 0; i < context.args.size(); ++i)
        {
            const std::string& arg = context.args[i];

            if (arg == "--help" || arg == "-h")
            {
                showHelp();
                return Status::ok();
            }
            else
            {
                return Status::error("Unknown option: " + arg +
                                     "\nUse --help for usage information");
            }
        }

        // Get hostname using system call
        char hostname[256];
        if (gethostname(hostname, sizeof(hostname)) == 0)
        {
            std::cout << hostname << "\n";
            return Status::ok();
        }
        else
        {
            return Status::error("Failed to get hostname");
        }
    }

private:
    /**
     * @brief Display help information
     */
    void showHelp() const
    {
        std::cout << "Usage: hostname [OPTION]\n\n";
        std::cout << "Display the system hostname.\n\n";
        std::cout << "Options:\n";
        std::cout << "  -h, --help    Show this help message\n";
    }
};

} // namespace homeshell
