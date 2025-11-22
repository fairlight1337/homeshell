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
 * @brief Update file database for fast searching with locate command
 *
 * @details The `updatedb` command scans the filesystem and builds a SQLite database
 * of all files and directories for fast searching. Similar to the Unix `updatedb` command,
 * it creates an index that can be quickly searched with the `locate` command.
 *
 * **Features:**
 * - Recursive filesystem scanning
 * - SQLite database storage
 * - Excludes system directories by default (/proc, /sys, /dev, /run, /tmp)
 * - Scans both regular and virtual encrypted filesystems
 * - Progress statistics after completion
 * - Configurable search and exclude paths
 *
 * **Usage:**
 * @code
 * updatedb [options]
 * @endcode
 *
 * **Options:**
 * - `--path <dir>` - Add directory to scan (can be specified multiple times)
 * - `--exclude <dir>` - Exclude directory from scan (can be specified multiple times)
 * - `--help` - Show help message
 *
 * **Examples:**
 * @code
 * # Update database with current directory
 * updatedb
 *
 * # Update with specific paths
 * updatedb --path /home --path /usr
 *
 * # Update with custom excludes
 * updatedb --exclude /home/user/large_dir
 *
 * # Scan everything including virtual mounts
 * updatedb --path / --path /secure
 * @endcode
 *
 * **Output Statistics:**
 * @code
 * Updating file database...
 *
 * Database update complete!
 * Files indexed:       12,345
 * Directories indexed:  1,234
 * Total size:          45.67 MB
 * Time taken:           2.34 seconds
 * Database:            ~/.homeshell/locate.db
 * @endcode
 *
 * **Default Behavior:**
 * - Scans current directory
 * - Includes all mounted virtual filesystems
 * - Excludes: /proc, /sys, /dev, /run, /tmp
 * - Database: ~/.homeshell/locate.db
 *
 * **Performance Notes:**
 * - Scanning large directory trees can take time
 * - Database is optimized for search performance
 * - Subsequent updates replace all previous data
 * - Virtual filesystem entries are included automatically
 *
 * **Use Cases:**
 * - Quickly find files without manual searching
 * - Index project directories for fast lookups
 * - Search encrypted storage contents
 * - Locate configuration files across system
 *
 * @note Database file created automatically on first run
 * @note Requires write access to ~/.homeshell directory
 * @note Virtual filesystem must be mounted before updatedb
 * @see LocateCommand for searching the database
 */
class UpdatedbCommand : public ICommand
{
public:
    std::string getName() const override
    {
        return "updatedb";
    }

    std::string getDescription() const override
    {
        return "Update file database for locate command";
    }

    CommandType getType() const override
    {
        return CommandType::Synchronous;
    }

    Status execute(const CommandContext& context) override
    {
        std::vector<std::string> search_paths;
        std::vector<std::string> exclude_paths;

        // Parse arguments
        for (size_t i = 0; i < context.args.size(); ++i)
        {
            if (context.args[i] == "--help")
            {
                showHelp();
                return Status::ok();
            }
            else if (context.args[i] == "--path" && i + 1 < context.args.size())
            {
                search_paths.push_back(context.args[++i]);
            }
            else if (context.args[i] == "--exclude" && i + 1 < context.args.size())
            {
                exclude_paths.push_back(context.args[++i]);
            }
            else
            {
                fmt::print(fg(fmt::color::red), "Error: Unknown option '{}'\n", context.args[i]);
                fmt::print("Use --help for usage information\n");
                return Status::error("Unknown option");
            }
        }

        // Open database
        FileDatabase db("~/.homeshell/locate.db");
        if (!db.open())
        {
            fmt::print(fg(fmt::color::red), "Error: Failed to open database\n");
            return Status::error("Failed to open database");
        }

        fmt::print(fg(fmt::color::cyan), "Updating file database...\n\n");

        // Update database
        auto stats = db.updateDatabase(search_paths, exclude_paths);

        // Display statistics
        fmt::print(fg(fmt::color::green) | fmt::emphasis::bold, "Database update complete!\n\n");

        fmt::print(fg(fmt::color::yellow), "Files indexed:       ");
        fmt::print("{:>10}\n", formatNumber(stats.total_files));

        fmt::print(fg(fmt::color::yellow), "Directories indexed: ");
        fmt::print("{:>10}\n", formatNumber(stats.total_directories));

        fmt::print(fg(fmt::color::yellow), "Total size:          ");
        fmt::print("{:>10}\n", formatSize(stats.total_size));

        fmt::print(fg(fmt::color::yellow), "Time taken:          ");
        fmt::print("{:>10.2f} seconds\n", stats.scan_time_ms / 1000.0);

        fmt::print(fg(fmt::color::yellow), "Database:            ");
        fmt::print("~/.homeshell/locate.db\n");

        db.close();
        return Status::ok();
    }

private:
    void showHelp()
    {
        fmt::print("Usage: updatedb [options]\n\n");
        fmt::print("Update file database for fast searching with locate command.\n\n");
        fmt::print("Options:\n");
        fmt::print("  --path <dir>     Add directory to scan (can be used multiple times)\n");
        fmt::print("  --exclude <dir>  Exclude directory from scan (can be used multiple times)\n");
        fmt::print("  --help           Show this help message\n\n");
        fmt::print("Examples:\n");
        fmt::print("  updatedb                      # Scan current directory\n");
        fmt::print("  updatedb --path /home         # Scan /home directory\n");
        fmt::print("  updatedb --exclude /tmp       # Exclude /tmp from scan\n");
    }

    std::string formatNumber(int64_t num)
    {
        std::string result = std::to_string(num);
        int insert_pos = result.length() - 3;
        while (insert_pos > 0)
        {
            result.insert(insert_pos, ",");
            insert_pos -= 3;
        }
        return result;
    }

    std::string formatSize(int64_t bytes)
    {
        const char* units[] = {"B", "KB", "MB", "GB", "TB"};
        int unit_idx = 0;
        double size = static_cast<double>(bytes);

        while (size >= 1024.0 && unit_idx < 4)
        {
            size /= 1024.0;
            unit_idx++;
        }

        if (unit_idx == 0)
        {
            return fmt::format("{} {}", static_cast<int64_t>(size), units[unit_idx]);
        }
        else
        {
            return fmt::format("{:.2f} {}", size, units[unit_idx]);
        }
    }
};

} // namespace homeshell
