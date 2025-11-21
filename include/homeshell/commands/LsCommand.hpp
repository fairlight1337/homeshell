#pragma once

#include <homeshell/Command.hpp>
#include <homeshell/FilesystemHelper.hpp>
#include <fmt/core.h>
#include <fmt/color.h>

namespace homeshell
{

class LsCommand : public ICommand
{
public:
    std::string getName() const override
    {
        return "ls";
    }

    std::string getDescription() const override
    {
        return "List directory contents";
    }

    CommandType getType() const override
    {
        return CommandType::Synchronous;
    }

    Status execute(const CommandContext& context) override
    {
        std::string path = ".";
        bool show_details = false;
        bool human_readable = false;

        // Parse arguments
        for (const auto& arg : context.args)
        {
            if (arg == "--help")
            {
                printHelp();
                return Status::ok();
            }
            else if (arg == "-l" || arg == "--long")
            {
                show_details = true;
            }
            else if (arg.length() >= 2 && arg[0] == '-' && arg[1] != '-')
            {
                // Handle combined flags like -lh or single flags like -h
                bool has_help = false;
                bool has_other = false;
                
                for (size_t i = 1; i < arg.length(); ++i)
                {
                    if (arg[i] == 'l')
                    {
                        show_details = true;
                        has_other = true;
                    }
                    else if (arg[i] == 'h')
                    {
                        if (arg.length() == 2)
                        {
                            // -h alone means help
                            has_help = true;
                        }
                        else
                        {
                            // -h with other flags means human-readable
                            human_readable = true;
                            has_other = true;
                        }
                    }
                    else
                    {
                        return Status::error("Unknown flag: -" + std::string(1, arg[i]));
                    }
                }
                
                if (has_help && !has_other)
                {
                    printHelp();
                    return Status::ok();
                }
            }
            else if (arg[0] != '-')
            {
                path = arg;
            }
            else
            {
                return Status::error("Unknown option: " + arg);
            }
        }

        // Check if path exists
        if (!FilesystemHelper::exists(path))
        {
            return Status::error("Path does not exist: " + path);
        }

        if (!FilesystemHelper::isDirectory(path))
        {
            return Status::error("Not a directory: " + path);
        }

        // Get directory contents
        auto entries = FilesystemHelper::listDirectory(path);

        if (entries.empty())
        {
            fmt::print("(empty directory)\n");
            return Status::ok();
        }

        // Display entries
        if (show_details)
        {
            displayDetailed(entries, human_readable);
        }
        else
        {
            displaySimple(entries);
        }

        return Status::ok();
    }

private:
    void printHelp() const
    {
        fmt::print("Usage: ls [OPTIONS] [PATH]\n\n");
        fmt::print("List directory contents\n\n");
        fmt::print("Options:\n");
        fmt::print("  -l, --long    Show detailed listing with permissions and sizes\n");
        fmt::print("  -h            Use human-readable sizes (use with -l)\n");
        fmt::print("  --help        Show this help message\n\n");
        fmt::print("Examples:\n");
        fmt::print("  ls\n");
        fmt::print("  ls -l\n");
        fmt::print("  ls -lh /home/user\n");
    }

    void displaySimple(const std::vector<FileInfo>& entries) const
    {
        for (const auto& entry : entries)
        {
            if (entry.is_directory)
            {
                fmt::print(fg(fmt::color::blue) | fmt::emphasis::bold, "{}/\n", entry.name);
            }
            else if (entry.is_symlink)
            {
                fmt::print(fg(fmt::color::cyan), "{}\n", entry.name);
            }
            else
            {
                fmt::print("{}\n", entry.name);
            }
        }
    }

    void displayDetailed(const std::vector<FileInfo>& entries, bool human_readable) const
    {
        for (const auto& entry : entries)
        {
            char type = entry.is_directory ? 'd' : (entry.is_symlink ? 'l' : '-');
            
            std::string size_str;
            if (human_readable)
            {
                size_str = FilesystemHelper::formatSize(entry.size);
            }
            else
            {
                size_str = std::to_string(entry.size);
            }

            fmt::print("{}{} {:>10}  ", type, entry.permissions, size_str);

            if (entry.is_directory)
            {
                fmt::print(fg(fmt::color::blue) | fmt::emphasis::bold, "{}/\n", entry.name);
            }
            else if (entry.is_symlink)
            {
                fmt::print(fg(fmt::color::cyan), "{}\n", entry.name);
            }
            else
            {
                fmt::print("{}\n", entry.name);
            }
        }
    }
};

} // namespace homeshell

