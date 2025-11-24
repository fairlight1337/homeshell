/**
 * @file CpCommand.hpp
 * @brief Copy files and directories
 */

#pragma once

#include <homeshell/Command.hpp>
#include <homeshell/Status.hpp>

#include <sys/stat.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace homeshell
{

class CpCommand : public ICommand
{
public:
    std::string getName() const override
    {
        return "cp";
    }
    std::string getDescription() const override
    {
        return "Copy files and directories";
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
        bool recursive = false, verbose = false;
        std::vector<std::string> files;

        for (const auto& arg : context.args)
        {
            if (arg == "-r" || arg == "-R")
                recursive = true;
            else if (arg == "-v")
                verbose = true;
            else if (arg[0] != '-')
                files.push_back(arg);
        }

        if (files.size() < 2)
        {
            std::cerr << "cp: missing file operand\n";
            return Status::error("Missing operands");
        }

        std::string dest = files.back();
        files.pop_back();

        // Copy each source
        for (const auto& src : files)
        {
            if (!std::filesystem::exists(src))
            {
                std::cerr << "cp: cannot stat '" << src << "': No such file or directory\n";
                continue;
            }

            if (std::filesystem::is_directory(src) && !recursive)
            {
                std::cerr << "cp: -r not specified; omitting directory '" << src << "'\n";
                continue;
            }

            try
            {
                std::filesystem::path dest_path = dest;
                if (std::filesystem::is_directory(dest))
                {
                    dest_path /= std::filesystem::path(src).filename();
                }

                if (std::filesystem::is_directory(src))
                {
                    std::filesystem::copy(src, dest_path, std::filesystem::copy_options::recursive);
                }
                else
                {
                    std::filesystem::copy_file(src, dest_path,
                                               std::filesystem::copy_options::overwrite_existing);
                }

                if (verbose)
                {
                    std::cout << "'" << src << "' -> '" << dest_path.string() << "'\n";
                }
            }
            catch (const std::exception& e)
            {
                std::cerr << "cp: " << e.what() << "\n";
            }
        }

        return Status::ok();
    }

private:
    void showHelp() const
    {
        std::cout << "Usage: cp [OPTION]... SOURCE DEST\n"
                  << "   or: cp [OPTION]... SOURCE... DIRECTORY\n\n"
                  << "Copy files and directories.\n\n"
                  << "Options:\n"
                  << "  -r, -R     Copy directories recursively\n"
                  << "  -v         Verbose output\n"
                  << "  --help     Show this help message\n";
    }
};

} // namespace homeshell
