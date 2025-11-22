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
 * @brief Unmount encrypted virtual filesystem
 *
 * @details The `unmount` command cleanly dismounts an encrypted virtual filesystem
 * that was previously mounted with the `mount` command. All data is flushed to disk
 * and the mount point becomes unavailable.
 *
 * **Features:**
 * - Clean shutdown of encrypted mount
 * - Automatic file buffer flushing
 * - SQLite database closure
 * - Mount point removal from virtual filesystem
 * - Safe operation (data persisted before unmount)
 *
 * **Usage:**
 * @code
 * unmount <name>
 * @endcode
 *
 * **Parameters:**
 * - `name` - Name of the mount to unmount (required, must match mount name)
 *
 * **Examples:**
 * @code
 * # Unmount a previously mounted filesystem
 * unmount mysecure
 * # Successfully unmounted 'mysecure'
 *
 * # Unmount with shell exiting to current directory first
 * cd ~
 * unmount backup
 * @endcode
 *
 * **Unmount Process:**
 * 1. Verify mount exists and is mounted
 * 2. Flush all pending writes to database
 * 3. Close SQLite database connection
 * 4. Remove mount from virtual filesystem registry
 * 5. Mount point path becomes unavailable
 *
 * **Data Safety:**
 * - All file operations are committed before unmount
 * - Database remains intact and encrypted on disk
 * - Can be remounted later with correct password
 * - No data loss occurs during proper unmount
 *
 * **Error Conditions:**
 * - No mount name specified: Shows usage message
 * - Mount not found: Mount name doesn't exist or already unmounted
 * - Current directory is inside mount: Not automatically handled (change directory first)
 *
 * **Important Notes:**
 * - Changes are saved automatically during operation (not just on unmount)
 * - Unmounting is not required for data safety (but recommended)
 * - If shell exits without unmount, OS cleans up resources
 * - Cannot unmount if mount doesn't exist
 *
 * **Example Workflow:**
 * @code
 * # Mount, use, and unmount
 * mount temp ~/temp.db /temp
 * cd /temp
 * touch file.txt
 * cd ~
 * unmount temp
 *
 * # Later, remount to access same files
 * mount temp ~/temp.db /temp
 * ls /temp
 * # file.txt is still there
 * @endcode
 *
 * **Best Practices:**
 * 1. Change out of mount directory before unmounting
 * 2. Ensure no open file handles in mount
 * 3. Wait for long-running operations to complete
 * 4. Check `vfs` command to verify unmount succeeded
 *
 * @note Data is encrypted and persisted in database file
 * @note Unmount does not delete the database file
 * @note Mount can be re-established later with same password
 * @note Shell will not prevent unmounting active directories
 *
 * @see MountCommand for mounting
 * @see VfsCommand for viewing active mounts
 */
class UnmountCommand : public ICommand
{
public:
    std::string getName() const override
    {
        return "unmount";
    }

    std::string getDescription() const override
    {
        return "Unmount an encrypted storage";
    }

    CommandType getType() const override
    {
        return CommandType::Synchronous;
    }

    Status execute(const CommandContext& context) override
    {
        if (context.args.empty())
        {
            fmt::print(fg(fmt::color::red), "Error: No mount name specified\n");
            fmt::print("Usage: unmount <name>\n");
            return Status::error("No mount name specified");
        }

        std::string name = context.args[0];
        auto& vfs = VirtualFilesystem::getInstance();

        if (!vfs.removeMount(name))
        {
            fmt::print(fg(fmt::color::red), "Error: Mount '{}' not found\n", name);
            return Status::error("Mount not found: " + name);
        }

        fmt::print(fg(fmt::color::green), "Successfully unmounted '{}'\n", name);
        return Status::ok();
    }
};

} // namespace homeshell
