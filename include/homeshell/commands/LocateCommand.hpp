#pragma once

#include <homeshell/Command.hpp>
#include <homeshell/FileDatabase.hpp>
#include <homeshell/Status.hpp>

#include <fmt/color.h>
#include <fmt/core.h>

#include <string>
#include <vector>

namespace homeshell
{

/**
 * @brief Find files by name using the file database
 *
 * @details The `locate` command searches the file database created by `updatedb`
 * for files and directories matching a pattern. Similar to the Unix `locate` command,
 * it provides very fast file lookups without scanning the filesystem.
 *
 * **Features:**
 * - Lightning-fast database search
 * - Wildcard pattern matching (*, ?)
 * - Case-insensitive search by default
 * - Color-coded output (directories in blue, files in default color)
 * - Limited results to prevent overwhelming output
 * - Search both regular and virtual filesystem entries
 *
 * **Usage:**
 * @code
 * locate [options] <pattern>
 * @endcode
 *
 * **Options:**
 * - `-i` - Case-insensitive search (default)
 * - `-c` - Case-sensitive search
 * - `-l <n>` - Limit results to n entries (default: 1000)
 * - `--help` - Show help message
 *
 * **Pattern Matching:**
 * - `*` - Matches any number of characters
 * - `?` - Matches exactly one character
 * - No wildcards - Searches for substring anywhere in path
 *
 * **Examples:**
 * @code
 * # Find all .cpp files
 * locate *.cpp
 *
 * # Find files with "config" in the name
 * locate config
 *
 * # Find files in specific directory
 * locate /usr/bin/*
 *
 * # Case-sensitive search
 * locate -c README.md
 *
 * # Limit to 50 results
 * locate -l 50 *.txt
 *
 * # Find test files
 * locate test_*.cpp
 * @endcode
 *
 * **Example Output:**
 * @code
 * locate *.hpp
 * /home/user/project/include/Command.hpp
 * /home/user/project/include/Shell.hpp
 * /home/user/project/include/Status.hpp
 * /secure/include/Config.hpp
 *
 * Found 4 matches
 * @endcode
 *
 * **Search Behavior:**
 * - By default, pattern matches anywhere in the path
 * - Prepend `/` to match from filesystem root
 * - Append specific extension to match exact file types
 * - Case-insensitive by default for convenience
 *
 * **Performance:**
 * - Database indexed search (extremely fast)
 * - No filesystem access required
 * - Instant results even for millions of files
 * - Limited to 1000 results by default
 *
 * **Database Requirements:**
 * - Database must exist (run `updatedb` first)
 * - Database at ~/.homeshell/locate.db
 * - Database reflects state at last `updatedb` run
 * - Re-run `updatedb` to refresh file list
 *
 * **Use Cases:**
 * - Quick file lookup across entire system
 * - Find configuration files
 * - Locate source code files
 * - Search encrypted storage
 * - Find documentation or scripts
 *
 * **Limitations:**
 * - Only finds files indexed by `updatedb`
 * - Results may be stale if files changed since last update
 * - Excluded directories won't appear in results
 * - New files won't appear until `updatedb` runs again
 *
 * @note Must run `updatedb` before first use
 * @note Database must be updated periodically for accuracy
 * @note Directories shown in blue, files in default color
 * @see UpdatedbCommand for building the file database
 */
class LocateCommand : public ICommand
{
public:
    std::string getName() const override
    {
        return "locate";
    }

    std::string getDescription() const override
    {
        return "Find files by name in database";
    }

    CommandType getType() const override
    {
        return CommandType::Synchronous;
    }

    Status execute(const CommandContext& context) override
    {
        bool case_sensitive = false;
        int limit = 1000;
        std::string pattern;

        // Parse arguments
        for (size_t i = 0; i < context.args.size(); ++i)
        {
            if (context.args[i] == "--help")
            {
                showHelp();
                return Status::ok();
            }
            else if (context.args[i] == "-c")
            {
                case_sensitive = true;
            }
            else if (context.args[i] == "-i")
            {
                case_sensitive = false;
            }
            else if (context.args[i] == "-l" && i + 1 < context.args.size())
            {
                try
                {
                    limit = std::stoi(context.args[++i]);
                    if (limit <= 0)
                    {
                        fmt::print(fg(fmt::color::red), "Error: Limit must be positive\n");
                        return Status::error("Invalid limit");
                    }
                }
                catch (...)
                {
                    fmt::print(fg(fmt::color::red), "Error: Invalid limit value\n");
                    return Status::error("Invalid limit");
                }
            }
            else if (pattern.empty())
            {
                pattern = context.args[i];
            }
            else
            {
                fmt::print(fg(fmt::color::red), "Error: Multiple patterns not supported\n");
                return Status::error("Multiple patterns");
            }
        }

        if (pattern.empty())
        {
            fmt::print(fg(fmt::color::red), "Error: No search pattern specified\n");
            fmt::print("Usage: locate [options] <pattern>\n");
            fmt::print("Use --help for more information\n");
            return Status::error("No pattern");
        }

        // Open database
        FileDatabase db("~/.homeshell/locate.db");
        if (!db.open())
        {
            fmt::print(fg(fmt::color::red),
                       "Error: Failed to open database. Run 'updatedb' first.\n");
            return Status::error("Database not found");
        }

        // Search database
        auto results = db.search(pattern, case_sensitive, limit);

        if (results.empty())
        {
            if (context.use_colors)
            {
                fmt::print(fg(fmt::color::yellow), "No matches found\n");
            }
            else
            {
                fmt::print("No matches found\n");
            }
            return Status::ok();
        }

        // Display results
        for (const auto& entry : results)
        {
            if (context.use_colors && entry.is_directory)
            {
                fmt::print(fg(fmt::color::blue) | fmt::emphasis::bold, "{}/\n", entry.path);
            }
            else
            {
                fmt::print("{}\n", entry.path);
            }
        }

        // Show count
        if (context.use_colors)
        {
            fmt::print("\n");
            if (results.size() >= static_cast<size_t>(limit))
            {
                fmt::print(fg(fmt::color::yellow), "Found {} matches (limited to {})\n",
                           results.size(), limit);
            }
            else
            {
                fmt::print(fg(fmt::color::green), "Found {} matches\n", results.size());
            }
        }
        else
        {
            fmt::print("\n");
            if (results.size() >= static_cast<size_t>(limit))
            {
                fmt::print("Found {} matches (limited to {})\n", results.size(), limit);
            }
            else
            {
                fmt::print("Found {} matches\n", results.size());
            }
        }

        db.close();
        return Status::ok();
    }

private:
    void showHelp()
    {
        fmt::print("Usage: locate [options] <pattern>\n\n");
        fmt::print("Find files by searching the database created by updatedb.\n\n");
        fmt::print("Options:\n");
        fmt::print("  -i           Case-insensitive search (default)\n");
        fmt::print("  -c           Case-sensitive search\n");
        fmt::print("  -l <n>       Limit results to n entries (default: 1000)\n");
        fmt::print("  --help       Show this help message\n\n");
        fmt::print("Pattern matching:\n");
        fmt::print("  *            Match any characters\n");
        fmt::print("  ?            Match single character\n");
        fmt::print("  text         Match substring anywhere in path\n\n");
        fmt::print("Examples:\n");
        fmt::print("  locate *.cpp            # Find all C++ files\n");
        fmt::print("  locate config           # Find files with 'config' in name\n");
        fmt::print("  locate /usr/bin/*       # Find all files in /usr/bin\n");
        fmt::print("  locate -c README.md     # Case-sensitive search\n");
        fmt::print("  locate -l 50 *.txt      # Limit to 50 results\n");
    }
};

} // namespace homeshell
