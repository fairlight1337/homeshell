/**
 * @file HeadCommand.hpp
 * @brief Display the first lines of files
 *
 * This command outputs the first part of files, by default the first 10 lines.
 * Can read from files or standard input.
 *
 * @author Homeshell Development Team
 * @date 2025
 */

#pragma once

#include <homeshell/Command.hpp>
#include <homeshell/Status.hpp>
#include <homeshell/VirtualFilesystem.hpp>

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace homeshell
{

/**
 * @class HeadCommand
 * @brief Command to display first lines of files
 *
 * Outputs the first N lines of files (default 10).
 *
 * @section Usage
 * @code
 * head file.txt         # Show first 10 lines
 * head -n 20 file.txt   # Show first 20 lines
 * head -5 file.txt      # Show first 5 lines (short form)
 * cat file | head       # Read from stdin
 * head --help           # Show help message
 * @endcode
 */
class HeadCommand : public ICommand
{
public:
    /**
     * @brief Get command name
     * @return Command name "head"
     */
    std::string getName() const override
    {
        return "head";
    }

    /**
     * @brief Get command description
     * @return Brief description of the command
     */
    std::string getDescription() const override
    {
        return "Output the first part of files";
    }

    /**
     * @brief Get command type
     * @return CommandType::Synchronous
     */
    CommandType getType() const override
    {
        return CommandType::Synchronous;
    }

    /**
     * @brief Execute the head command
     * @param context Command context with arguments
     * @return Status indicating success or failure
     */
    Status execute(const CommandContext& context) override
    {
        int num_lines = 10; // Default
        std::vector<std::string> files;

        // Parse arguments
        for (size_t i = 0; i < context.args.size(); ++i)
        {
            const std::string& arg = context.args[i];

            if (arg == "--help")
            {
                showHelp();
                return Status::ok();
            }
            else if (arg == "-n" || arg == "--lines")
            {
                if (i + 1 >= context.args.size())
                {
                    return Status::error("Option " + arg + " requires an argument");
                }
                try
                {
                    num_lines = std::stoi(context.args[++i]);
                    if (num_lines < 0)
                    {
                        return Status::error("Invalid number of lines: " +
                                             std::to_string(num_lines));
                    }
                }
                catch (...)
                {
                    return Status::error("Invalid number of lines: " + context.args[i]);
                }
            }
            else if (arg[0] == '-' && arg.length() > 1 && arg[1] != '-' && isdigit(arg[1]))
            {
                // -N format
                try
                {
                    num_lines = std::stoi(arg.substr(1));
                    if (num_lines < 0)
                    {
                        return Status::error("Invalid number of lines: " + arg);
                    }
                }
                catch (...)
                {
                    return Status::error("Invalid number of lines: " + arg);
                }
            }
            else if (arg[0] == '-')
            {
                return Status::error("Unknown option: " + arg +
                                     "\nUse --help for usage information");
            }
            else
            {
                files.push_back(arg);
            }
        }

        // If no files specified, read from stdin
        if (files.empty())
        {
            return headFromStdin(num_lines);
        }

        // Process each file
        bool first = true;
        for (const auto& file : files)
        {
            if (files.size() > 1)
            {
                if (!first)
                    std::cout << "\n";
                std::cout << "==> " << file << " <==\n";
                first = false;
            }

            auto status = headFromFile(file, num_lines);
            if (!status.isOk())
            {
                std::cerr << "head: " << file << ": " << status.message << "\n";
            }
        }

        return Status::ok();
    }

private:
    /**
     * @brief Display help information
     */
    void showHelp() const
    {
        std::cout << "Usage: head [OPTION]... [FILE]...\n\n";
        std::cout << "Print the first 10 lines of each FILE to standard output.\n";
        std::cout << "With more than one FILE, precede each with a header.\n";
        std::cout << "With no FILE, or when FILE is -, read standard input.\n\n";
        std::cout << "Options:\n";
        std::cout << "  -n, --lines=K  Print the first K lines (default 10)\n";
        std::cout << "  -K             Same as -n K\n";
        std::cout << "  --help         Show this help message\n";
    }

    /**
     * @brief Read and display first N lines from stdin
     * @param num_lines Number of lines to display
     * @return Status
     */
    Status headFromStdin(int num_lines) const
    {
        std::string line;
        int count = 0;

        while (count < num_lines && std::getline(std::cin, line))
        {
            std::cout << line << "\n";
            count++;
        }

        return Status::ok();
    }

    /**
     * @brief Read and display first N lines from file
     * @param path File path
     * @param num_lines Number of lines to display
     * @return Status
     */
    Status headFromFile(const std::string& path, int num_lines) const
    {
        auto& vfs = VirtualFilesystem::getInstance();

        if (!vfs.exists(path))
        {
            return Status::error("No such file or directory");
        }

        if (vfs.isDirectory(path))
        {
            return Status::error("Is a directory");
        }

        std::string content;
        if (!vfs.readFile(path, content))
        {
            return Status::error("Failed to read file");
        }

        // Split into lines and print first N
        std::istringstream iss(content);
        std::string line;
        int count = 0;

        while (count < num_lines && std::getline(iss, line))
        {
            std::cout << line << "\n";
            count++;
        }

        return Status::ok();
    }
};

} // namespace homeshell
