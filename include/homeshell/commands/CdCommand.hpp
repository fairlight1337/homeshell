#pragma once

#include <homeshell/Command.hpp>
#include <homeshell/FilesystemHelper.hpp>

#include <fmt/core.h>

namespace homeshell
{

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

    Status execute(const CommandContext& context) override
    {
        std::string target;

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
        if (!FilesystemHelper::exists(target))
        {
            return Status::error("Directory does not exist: " + target);
        }

        if (!FilesystemHelper::isDirectory(target))
        {
            return Status::error("Not a directory: " + target);
        }

        // Change directory
        if (!FilesystemHelper::changeDirectory(target))
        {
            return Status::error("Failed to change directory to: " + target);
        }

        // Optionally show new directory
        if (context.verbose)
        {
            fmt::print("Changed to: {}\n", FilesystemHelper::getCurrentDirectory().string());
        }

        return Status::ok();
    }
};

} // namespace homeshell
