#pragma once

#include <homeshell/Command.hpp>
#include <homeshell/Status.hpp>
#include <homeshell/VirtualFilesystem.hpp>

#include <fmt/color.h>

#include <string>
#include <vector>

namespace homeshell
{

class TouchCommand : public ICommand
{
public:
    std::string getName() const override
    {
        return "touch";
    }

    std::string getDescription() const override
    {
        return "Create a new empty file or update timestamp";
    }

    CommandType getType() const override
    {
        return CommandType::Synchronous;
    }

    Status execute(const CommandContext& context) override
    {
        if (context.args.empty())
        {
            fmt::print(fg(fmt::color::red), "Error: No file specified\n");
            fmt::print("Usage: touch <file>\n");
            return Status::error("No file specified");
        }

        const std::string& path = context.args[0];
        auto& vfs = VirtualFilesystem::getInstance();

        if (vfs.exists(path) && vfs.isDirectory(path))
        {
            fmt::print(fg(fmt::color::red), "Error: '{}' is a directory\n", path);
            return Status::error("Is a directory: " + path);
        }

        if (!vfs.exists(path))
        {
            // Create new empty file
            if (!vfs.writeFile(path, ""))
            {
                fmt::print(fg(fmt::color::red), "Error: Failed to create file '{}'\n", path);
                return Status::error("Failed to create file: " + path);
            }
            fmt::print(fg(fmt::color::green), "File '{}' created\n", path);
        }
        else
        {
            // File exists - update timestamp by re-writing
            std::string content;
            if (vfs.readFile(path, content))
            {
                vfs.writeFile(path, content);
                fmt::print(fg(fmt::color::green), "File '{}' timestamp updated\n", path);
            }
        }

        return Status::ok();
    }
};

} // namespace homeshell
