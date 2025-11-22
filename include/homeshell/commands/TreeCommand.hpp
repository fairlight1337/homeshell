#pragma once

#include <homeshell/Command.hpp>
#include <homeshell/Status.hpp>
#include <homeshell/VirtualFilesystem.hpp>

#include <fmt/color.h>
#include <fmt/core.h>

#include <string>
#include <vector>

namespace homeshell
{

/**
 * @brief Display directory tree structure recursively
 *
 * @details The `tree` command provides a visual tree representation of directory
 * hierarchies, showing all files and subdirectories recursively. Directories are
 * displayed first (in blue, bold) followed by files, with ASCII tree characters
 * showing the structure.
 *
 * **Features:**
 * - Recursive directory traversal
 * - Visual tree structure with ASCII art (├──, └──, │)
 * - Directories shown before files (alphabetically sorted within each group)
 * - Color-coded output (directories in blue/bold, files in default color)
 * - Summary statistics (directory and file count)
 * - Works with both regular and virtual encrypted filesystems
 *
 * **Usage:**
 * @code
 * tree [path]
 * @endcode
 *
 * **Parameters:**
 * - `path` - Directory to display (optional, default: current directory)
 *
 * **Example Output:**
 * @code
 * /home/user/project
 * ├── src/
 * │   ├── commands/
 * │   │   ├── CdCommand.hpp
 * │   │   └── LsCommand.hpp
 * │   ├── main.cpp
 * │   └── Shell.cpp
 * ├── tests/
 * │   └── test_commands.cpp
 * ├── CMakeLists.txt
 * └── README.md
 *
 * 2 directories, 6 files
 * @endcode
 *
 * **Use Cases:**
 * - Visualizing project structure
 * - Documenting directory layouts
 * - Finding files in deep hierarchies
 * - Exploring unfamiliar codebases
 * - Verifying directory organization
 *
 * **Tree Structure Symbols:**
 * - `├──` - Item with siblings below
 * - `└──` - Last item in directory
 * - `│` - Vertical line for nested levels
 * - `/` - Directory suffix
 *
 * @note Large directory trees may produce extensive output
 * @note Symbolic links are followed (may cause loops on circular links)
 * @note Works with virtual filesystem paths (e.g., `/secure/`)
 */
class TreeCommand : public ICommand
{
public:
    std::string getName() const override
    {
        return "tree";
    }

    std::string getDescription() const override
    {
        return "Display directory tree structure";
    }

    CommandType getType() const override
    {
        return CommandType::Synchronous;
    }

    Status execute(const CommandContext& context) override
    {
        auto& vfs = VirtualFilesystem::getInstance();

        std::string path = context.args.empty() ? vfs.getCurrentDirectory() : context.args[0];

        if (!vfs.exists(path))
        {
            fmt::print(fg(fmt::color::red), "Error: Path '{}' does not exist\n", path);
            return Status::error("Path not found");
        }

        if (!vfs.isDirectory(path))
        {
            fmt::print(fg(fmt::color::red), "Error: '{}' is not a directory\n", path);
            return Status::error("Not a directory");
        }

        fmt::print(fg(fmt::color::blue) | fmt::emphasis::bold, "{}\n", path);

        int dir_count = 0;
        int file_count = 0;
        printTree(path, "", true, dir_count, file_count);

        fmt::print("\n{} directories, {} files\n", dir_count, file_count);

        return Status::ok();
    }

private:
    void printTree(const std::string& path, const std::string& prefix, bool is_last, int& dir_count,
                   int& file_count)
    {
        auto& vfs = VirtualFilesystem::getInstance();

        auto entries = vfs.listDirectory(path);

        // Sort: directories first, then files, alphabetically
        std::sort(entries.begin(), entries.end(),
                  [](const VirtualFileInfo& a, const VirtualFileInfo& b)
                  {
                      if (a.is_directory != b.is_directory)
                          return a.is_directory > b.is_directory;
                      return a.name < b.name;
                  });

        for (size_t i = 0; i < entries.size(); ++i)
        {
            const auto& entry = entries[i];
            bool is_last_entry = (i == entries.size() - 1);

            // Print tree characters
            fmt::print("{}", prefix);
            fmt::print("{}", is_last_entry ? "└── " : "├── ");

            // Print name with color
            if (entry.is_directory)
            {
                fmt::print(fg(fmt::color::blue) | fmt::emphasis::bold, "{}/\n", entry.name);
                dir_count++;

                // Recurse into directory
                std::string new_path = path;
                if (path != "/" && !path.empty() && path.back() != '/')
                {
                    new_path += "/";
                }
                new_path += entry.name;

                std::string new_prefix = prefix + (is_last_entry ? "    " : "│   ");
                printTree(new_path, new_prefix, is_last_entry, dir_count, file_count);
            }
            else
            {
                fmt::print("{}\n", entry.name);
                file_count++;
            }
        }
    }
};

} // namespace homeshell
