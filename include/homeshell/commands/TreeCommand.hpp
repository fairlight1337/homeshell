#pragma once

#include <homeshell/Command.hpp>
#include <homeshell/Status.hpp>
#include <homeshell/VirtualFilesystem.hpp>
#include <fmt/color.h>
#include <fmt/core.h>
#include <vector>
#include <string>

namespace homeshell
{

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
    void printTree(const std::string& path, const std::string& prefix, bool is_last,
                   int& dir_count, int& file_count)
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

