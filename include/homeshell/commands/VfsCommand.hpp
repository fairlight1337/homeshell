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
 * @brief Display virtual filesystem information and status
 *
 * @details The `vfs` command shows information about all currently mounted
 * encrypted virtual filesystems, including storage usage, mount points, and
 * database locations. Useful for monitoring encrypted storage quotas and
 * verifying mount status.
 *
 * **Features:**
 * - Lists all active encrypted mounts
 * - Shows storage usage for each mount (used/max)
 * - Displays usage percentage
 * - Shows mount points and database paths
 * - Color-coded output for readability
 * - Human-readable size formatting (B, KB, MB, GB, TB)
 *
 * **Usage:**
 * @code
 * vfs
 * @endcode
 *
 * **Parameters:**
 * - None (command takes no arguments)
 *
 * **Example Output:**
 * @code
 * Virtual Filesystems:
 *
 * secure
 *   Mount Point: /secure
 *   Database:    /home/user/.homeshell/secure.db
 *   Used:        1.25 MB / 100.00 MB (1.2%)
 *
 * backup
 *   Mount Point: /backup
 *   Database:    /home/user/.homeshell/backup.db
 *   Used:        45.67 MB / 200.00 MB (22.8%)
 *
 * large
 *   Mount Point: /large
 *   Database:    /home/user/.homeshell/large.db
 *   Used:        456.78 MB / 500.00 MB (91.4%)
 * @endcode
 *
 * **If No Mounts:**
 * @code
 * No virtual filesystems mounted.
 * @endcode
 *
 * **Information Displayed:**
 * - **Mount Name** - Unique identifier (cyan, bold)
 * - **Mount Point** - Virtual path where mount is accessible
 * - **Database** - Physical database file location on disk
 * - **Used Space** - Current storage consumption
 * - **Max Space** - Maximum storage quota
 * - **Usage Percentage** - Percentage of quota used
 *
 * **Size Formatting:**
 * - Bytes (B): 0-1023 bytes
 * - Kilobytes (KB): 1024 bytes - 1023 KB
 * - Megabytes (MB): 1024 KB - 1023 MB
 * - Gigabytes (GB): 1024 MB - 1023 GB
 * - Terabytes (TB): 1024 GB and above
 *
 * **Use Cases:**
 * - Monitoring storage quota usage
 * - Verifying mount success
 * - Checking mount point paths
 * - Identifying which mounts need cleanup
 * - Confirming database file locations
 * - Planning storage expansion
 *
 * **Interpreting Usage:**
 * - **< 50%** - Healthy usage level
 * - **50-80%** - Moderate usage, monitor
 * - **80-95%** - High usage, consider cleanup or expansion
 * - **> 95%** - Critical, cleanup required or quota increase
 *
 * **Example Workflow:**
 * @code
 * # Check current mounts
 * vfs
 *
 * # Mount new filesystem
 * mount test ~/test.db /test
 *
 * # Verify it appears
 * vfs
 * # Should now show 'test' mount
 *
 * # Add some files
 * cd /test
 * touch file1.txt file2.txt
 *
 * # Check usage again
 * vfs
 * # Usage should have increased slightly
 *
 * # Clean up
 * unmount test
 * vfs
 * # Mount should be gone
 * @endcode
 *
 * **Notes:**
 * - Shows only currently active mounts (in this session)
 * - Database files persist on disk after unmount
 * - Usage updates in real-time as files are added/removed
 * - Unmounted filesystems don't appear in output
 * - Command is read-only (doesn't modify state)
 *
 * @note Storage usage includes all data and metadata
 * @note Quota is enforced on write operations
 * @note Empty mounts still consume minimal space (database overhead)
 *
 * @see MountCommand for mounting filesystems
 * @see UnmountCommand for unmounting filesystems
 * @see VirtualFilesystem::getUsedSpace() for usage calculation
 */
class VfsCommand : public ICommand
{
public:
    std::string getName() const override
    {
        return "vfs";
    }

    std::string getDescription() const override
    {
        return "Show virtual filesystem information";
    }

    CommandType getType() const override
    {
        return CommandType::Synchronous;
    }

    Status execute(const CommandContext& context) override
    {
        auto& vfs = VirtualFilesystem::getInstance();
        auto mount_names = vfs.getMountNames();

        if (mount_names.empty())
        {
            fmt::print("No virtual filesystems mounted.\n");
            return Status::ok();
        }

        fmt::print(fg(fmt::color::yellow) | fmt::emphasis::bold, "Virtual Filesystems:\n\n");

        for (const auto& name : mount_names)
        {
            auto* mount = vfs.getMount(name);
            if (mount && mount->is_mounted())
            {
                int64_t used = mount->getUsedSpace();
                int64_t max = mount->getMaxSpace();
                double usage_pct = max > 0 ? (100.0 * used / max) : 0.0;

                fmt::print(fg(fmt::color::cyan) | fmt::emphasis::bold, "{}\n", name);
                fmt::print("  Mount Point: {}\n", mount->getMountPoint());
                fmt::print("  Database:    {}\n", mount->getDbPath());
                fmt::print("  Used:        {} / {} ({:.1f}%)\n", formatBytes(used),
                           formatBytes(max), usage_pct);
                fmt::print("\n");
            }
        }

        return Status::ok();
    }

private:
    std::string formatBytes(int64_t bytes) const
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
