/**
 * @file MvCommand.hpp
 * @brief Move or rename files and directories
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

class MvCommand : public ICommand
{
public:
    std::string getName() const override
    {
        return "mv";
    }
    std::string getDescription() const override
    {
        return "Move or rename files and directories";
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
        bool verbose = false;
        std::vector<std::string> files;

        for (const auto& arg : context.args)
        {
            if (arg == "-v")
                verbose = true;
            else if (arg[0] != '-')
                files.push_back(arg);
        }

        if (files.size() < 2)
        {
            std::cerr << "mv: missing file operand\n";
            return Status::error("Missing operands");
        }

        std::string dest = files.back();
        files.pop_back();

        // Move each source
        for (const auto& src : files)
        {
            if (!std::filesystem::exists(src))
            {
                std::cerr << "mv: cannot stat '" << src << "': No such file or directory\n";
                continue;
            }

            try
            {
                std::filesystem::path dest_path = dest;
                if (std::filesystem::is_directory(dest))
                {
                    dest_path /= std::filesystem::path(src).filename();
                }

                std::filesystem::rename(src, dest_path);

                if (verbose)
                {
                    std::cout << "renamed '" << src << "' -> '" << dest_path.string() << "'\n";
                }
            }
            catch (const std::exception& e)
            {
                std::cerr << "mv: " << e.what() << "\n";
            }
        }

        return Status::ok();
    }

private:
    void showHelp() const
    {
        std::cout << "Usage: mv [OPTION]... SOURCE DEST\n"
                  << "   or: mv [OPTION]... SOURCE... DIRECTORY\n\n"
                  << "Move or rename files and directories.\n\n"
                  << "Options:\n"
                  << "  -v         Verbose output\n"
                  << "  --help     Show this help message\n";
    }
};

} // namespace homeshell
