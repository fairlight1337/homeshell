#pragma once

#include <homeshell/Command.hpp>
#include <homeshell/Status.hpp>
#include <homeshell/VirtualFilesystem.hpp>

#include <fmt/color.h>

#include <string>
#include <vector>

namespace homeshell
{

class UnmountCommand : public ICommand
{
public:
    std::string getName() const override
    {
        return "unmount";
    }

    std::string getDescription() const override
    {
        return "Unmount an encrypted storage";
    }

    CommandType getType() const override
    {
        return CommandType::Synchronous;
    }

    Status execute(const CommandContext& context) override
    {
        if (context.args.empty())
        {
            fmt::print(fg(fmt::color::red), "Error: No mount name specified\n");
            fmt::print("Usage: unmount <name>\n");
            return Status::error("No mount name specified");
        }

        std::string name = context.args[0];
        auto& vfs = VirtualFilesystem::getInstance();

        if (!vfs.removeMount(name))
        {
            fmt::print(fg(fmt::color::red), "Error: Mount '{}' not found\n", name);
            return Status::error("Mount not found: " + name);
        }

        fmt::print(fg(fmt::color::green), "Successfully unmounted '{}'\n", name);
        return Status::ok();
    }
};

} // namespace homeshell
