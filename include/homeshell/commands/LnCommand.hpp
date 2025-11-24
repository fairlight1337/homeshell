/**
 * @file LnCommand.hpp
 * @brief Create symbolic and hard links
 */

#pragma once

#include <homeshell/Command.hpp>
#include <homeshell/Status.hpp>

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace homeshell
{

class LnCommand : public ICommand
{
public:
    std::string getName() const override
    {
        return "ln";
    }
    std::string getDescription() const override
    {
        return "Create symbolic and hard links";
    }
    CommandType getType() const override
    {
        return CommandType::Synchronous;
    }

    Status execute(const CommandContext& context) override
    {
        if (context.args.empty() || context.args[0] == "--help" || context.args[0] == "-h")
        {
            showHelp();
            return Status::ok();
        }

        // Parse options
        bool symbolic = false, force = false, verbose = false;
        std::vector<std::string> files;

        for (const auto& arg : context.args)
        {
            if (arg == "-s")
                symbolic = true;
            else if (arg == "-f")
                force = true;
            else if (arg == "-v")
                verbose = true;
            else if (arg[0] != '-')
                files.push_back(arg);
        }

        if (files.size() < 2)
        {
            std::cerr << "ln: missing file operand\n";
            return Status::error("Missing operands");
        }

        std::string link_name = files.back();
        files.pop_back();

        // Create links
        for (const auto& target : files)
        {
            if (!symbolic && !std::filesystem::exists(target))
            {
                std::cerr << "ln: failed to access '" << target << "': No such file or directory\n";
                continue;
            }

            if (!symbolic && std::filesystem::is_directory(target))
            {
                std::cerr << "ln: '" << target << "': hard link not allowed for directory\n";
                continue;
            }

            try
            {
                std::filesystem::path link_path = link_name;
                if (std::filesystem::is_directory(link_name))
                {
                    link_path /= std::filesystem::path(target).filename();
                }

                if (std::filesystem::exists(link_path))
                {
                    if (force)
                    {
                        std::filesystem::remove(link_path);
                    }
                    else
                    {
                        std::cerr << "ln: failed to create link '" << link_path.string()
                                  << "': File exists\n";
                        continue;
                    }
                }

                if (symbolic)
                {
                    std::filesystem::create_symlink(target, link_path);
                }
                else
                {
                    std::filesystem::create_hard_link(target, link_path);
                }

                if (verbose)
                {
                    std::cout << "'" << link_path.string() << "' -> '" << target << "'\n";
                }
            }
            catch (const std::exception& e)
            {
                std::cerr << "ln: " << e.what() << "\n";
            }
        }

        return Status::ok();
    }

private:
    void showHelp() const
    {
        std::cout << "Usage: ln [OPTION]... TARGET LINK_NAME\n"
                  << "   or: ln [OPTION]... TARGET... DIRECTORY\n\n"
                  << "Create a link to TARGET with the name LINK_NAME.\n\n"
                  << "Options:\n"
                  << "  -s         Create symbolic link instead of hard link\n"
                  << "  -f         Force overwrite\n"
                  << "  -v         Verbose output\n"
                  << "  --help     Show this help message\n";
    }
};

} // namespace homeshell
