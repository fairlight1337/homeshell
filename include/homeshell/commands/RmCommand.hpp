#pragma once

#include <homeshell/Command.hpp>
#include <homeshell/Status.hpp>
#include <homeshell/VirtualFilesystem.hpp>

#include <fmt/color.h>

#include <string>
#include <vector>

namespace homeshell
{

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
