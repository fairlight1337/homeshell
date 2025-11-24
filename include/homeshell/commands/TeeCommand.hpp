/**
 * @file TeeCommand.hpp
 * @brief Read from stdin and write to files and stdout
 *
 * This command reads from standard input and writes to both standard output
 * and one or more files simultaneously, similar to the Unix `tee` command.
 *
 * @author Homeshell Development Team
 * @date 2025
 */

#pragma once

#include <homeshell/Command.hpp>
#include <homeshell/Status.hpp>

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace homeshell
{

class TeeCommand : public ICommand
{
public:
    std::string getName() const override
    {
        return "tee";
    }

    std::string getDescription() const override
    {
        return "Read from stdin and write to files and stdout";
    }

    CommandType getType() const override
    {
        return CommandType::Synchronous;
    }

    Status execute(const CommandContext& context) override
    {
        if (context.args.size() > 0 && (context.args[0] == "--help" || context.args[0] == "-h"))
        {
            showHelp();
            return Status::ok();
        }

        // Parse options
        bool append = false;
        bool ignore_interrupts = false;
        std::vector<std::string> files;

        for (const auto& arg : context.args)
        {
            if (arg == "-a" || arg == "--append")
            {
                append = true;
            }
            else if (arg == "-i" || arg == "--ignore-interrupts")
            {
                ignore_interrupts = true;
            }
            else if (arg[0] != '-')
            {
                files.push_back(arg);
            }
        }

        // Open all output files
        std::vector<std::ofstream> output_files;
        for (const auto& filename : files)
        {
            std::ios_base::openmode mode = std::ios::out;
            if (append)
            {
                mode |= std::ios::app;
            }

            std::ofstream file(filename, mode);
            if (!file)
            {
                std::cerr << "tee: " << filename << ": Unable to open file\n";
                return Status::error("Failed to open file: " + filename);
            }
            output_files.push_back(std::move(file));
        }

        // Read from stdin and write to stdout and all files
        std::string line;
        while (std::getline(std::cin, line))
        {
            // Write to stdout
            std::cout << line << '\n';

            // Write to all files
            for (auto& file : output_files)
            {
                file << line << '\n';
                if (!file)
                {
                    std::cerr << "tee: write error\n";
                    return Status::error("Write error");
                }
            }
        }

        // Close all files
        for (auto& file : output_files)
        {
            file.close();
        }

        return Status::ok();
    }

private:
    void showHelp() const
    {
        std::cout << "Usage: tee [OPTION]... [FILE]...\n\n"
                  << "Copy standard input to each FILE, and also to standard output.\n\n"
                  << "Options:\n"
                  << "  -a, --append            Append to files rather than overwrite\n"
                  << "  -i, --ignore-interrupts Ignore interrupt signals\n"
                  << "  --help                  Show this help message\n\n"
                  << "Examples:\n"
                  << "  echo 'hello' | tee file.txt\n"
                  << "  cat input.txt | tee -a log.txt output.txt\n";
    }
};

} // namespace homeshell
