#pragma once

#include <homeshell/Command.hpp>
#include <homeshell/Status.hpp>
#include <homeshell/VirtualFilesystem.hpp>

#include <fmt/color.h>

#include <string>
#include <vector>

namespace homeshell
{

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
