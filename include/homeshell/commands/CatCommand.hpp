#pragma once

#include <homeshell/Command.hpp>
#include <homeshell/Status.hpp>
#include <homeshell/VirtualFilesystem.hpp>

#include <fmt/color.h>

#include <string>
#include <vector>

namespace homeshell
{

class CatCommand : public ICommand
{
public:
    std::string getName() const override
    {
        return "cat";
    }

    std::string getDescription() const override
    {
        return "Display the contents of a file";
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
            fmt::print("Usage: cat <file>\n");
            return Status::error("No file specified");
        }

        const std::string& path = context.args[0];
        auto& vfs = VirtualFilesystem::getInstance();

        if (!vfs.exists(path))
        {
            fmt::print(fg(fmt::color::red), "Error: File '{}' not found\n", path);
            return Status::error("File not found: " + path);
        }

        if (vfs.isDirectory(path))
        {
            fmt::print(fg(fmt::color::red), "Error: '{}' is a directory\n", path);
            return Status::error("Is a directory: " + path);
        }

        std::string content;
        if (!vfs.readFile(path, content))
        {
            fmt::print(fg(fmt::color::red), "Error: Failed to read file '{}'\n", path);
            return Status::error("Failed to read file: " + path);
        }

        fmt::print("{}", content);
        if (!content.empty() && content.back() != '\n')
        {
            fmt::print("\n");
        }

        return Status::ok();
    }
};

} // namespace homeshell
