#pragma once

#include <homeshell/Command.hpp>
#include <homeshell/Status.hpp>
#include <homeshell/VirtualFilesystem.hpp>

#include <fmt/color.h>

#include <string>
#include <vector>

namespace homeshell
{

/**
 * @brief Display file contents
 *
 * Reads and displays the entire contents of a file to standard output.
 * Works with both regular files and files in encrypted virtual mounts.
 *
 * @details The cat command reads a file and outputs its contents verbatim.
 *          It can handle text and binary files, and ensures output ends
 *          with a newline if the file doesn't.
 *
 *          Features:
 *          - Read regular filesystem files
 *          - Read encrypted virtual filesystem files
 *          - Automatic newline appending
 *          - Error handling for missing/invalid files
 *
 *          Command syntax:
 *          ```
 *          cat <filename>
 *          ```
 *
 *          Errors reported:
 *          - File not found
 *          - Path is a directory (not a file)
 *          - Read permission denied
 *          - Failed to read file content
 *
 * Example usage:
 * ```
 * cat myfile.txt              # Display myfile.txt
 * cat /etc/hosts              # Display system file
 * cat /secure/secrets.txt     # Display encrypted file
 * ```
 *
 * @note Unlike bash cat, this does not support:
 *       - Multiple file arguments
 *       - Reading from stdin (-)
 *       - Options like -n (line numbers), -b (blank lines), etc.
 */
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

    /**
     * @brief Execute the cat command
     * @param context Command context with filename argument
     * @return Status::ok() on success, Status::error() if file invalid
     */
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
