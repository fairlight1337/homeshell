#pragma once

#include <homeshell/Command.hpp>
#include <homeshell/EncryptedMount.hpp>
#include <homeshell/PasswordInput.hpp>
#include <homeshell/Status.hpp>
#include <homeshell/VirtualFilesystem.hpp>

#include <fmt/color.h>

#include <memory>
#include <string>
#include <vector>

namespace homeshell
{

/**
 * @brief Mount encrypted storage as virtual filesystem
 *
 * @details The `mount` command creates and mounts an encrypted virtual filesystem
 * using SQLCipher for AES-256 encryption. Once mounted, the virtual filesystem
 * appears as a regular directory path that can be accessed with all filesystem commands.
 *
 * **Features:**
 * - AES-256 encryption via SQLCipher
 * - Block-level encryption for efficient partial updates
 * - Password-based key derivation (PBKDF2)
 * - Configurable storage quota
 * - Automatic database creation on first mount
 * - Secure password prompting (input hidden)
 * - Persistent storage in SQLite database
 *
 * **Usage:**
 * @code
 * mount <name> <db_path> <mount_point> [password] [max_size_mb]
 * @endcode
 *
 * **Parameters:**
 * - `name` - Unique identifier for this mount (required)
 * - `db_path` - Path to encrypted database file (required, will be created if doesn't exist)
 * - `mount_point` - Virtual path where mount appears (required, e.g., `/secure`)
 * - `password` - Encryption password (optional, prompted if omitted - RECOMMENDED)
 * - `max_size_mb` - Maximum storage size in MB (optional, default: 100MB)
 *
 * **Examples:**
 * @code
 * # Mount with password prompt (recommended - secure)
 * mount mysecure ~/.homeshell/secure.db /secure
 * # You'll be prompted: Enter password for mount 'mysecure': ********
 *
 * # Mount with explicit password (NOT RECOMMENDED - visible in history)
 * mount backup ~/backup.db /backup mypassword123 200
 *
 * # Mount with custom size
 * mount large ~/.homeshell/large.db /large "" 500
 * # Empty string triggers password prompt, 500MB quota
 * @endcode
 *
 * **Password Security:**
 * - **Recommended**: Omit password parameter to trigger secure prompt
 * - Password prompt hides input (no echo to terminal)
 * - Passwords NOT stored in shell history when prompted
 * - Command-line passwords ARE visible in history (use with caution)
 *
 * **Storage Management:**
 * - Database grows dynamically as files are added
 * - Storage quota enforced on writes
 * - Use `vfs` command to monitor usage
 * - Files remain encrypted at rest in database
 *
 * **First Mount:**
 * When mounting a non-existent database:
 * 1. Database file is created automatically
 * 2. Encryption key derived from password
 * 3. Empty filesystem initialized
 * 4. Mount point becomes available immediately
 *
 * **Subsequent Mounts:**
 * - Password must match original encryption password
 * - Wrong password returns "Failed to mount" error
 * - All previously stored files become accessible
 *
 * **Error Conditions:**
 * - Insufficient arguments: Shows usage message
 * - Empty password: Not allowed
 * - Invalid max_size_mb: Must be positive integer
 * - Mount name conflict: Name already in use
 * - Database file errors: Permission denied, disk full
 * - Wrong password: Cannot decrypt existing database
 *
 * **Example Workflow:**
 * @code
 * # Create and mount new secure storage
 * mount secrets ~/.homeshell/secrets.db /secrets
 * # Enter password: ********
 * # Successfully mounted 'secrets' at '/secrets'
 *
 * # Use the mount
 * cd /secrets
 * touch passwords.txt
 * echo "sensitive data" > passwords.txt
 *
 * # Check usage
 * vfs
 *
 * # Unmount when done
 * unmount secrets
 * @endcode
 *
 * @note Requires SQLCipher library for encryption
 * @note Mount persists only for current shell session
 * @note Use configuration file for auto-mount on startup
 * @note Database files are portable (can be copied/backed up)
 * @note Quota enforcement prevents runaway storage growth
 *
 * @see UnmountCommand for unmounting
 * @see VfsCommand for viewing mount status
 * @see VirtualFilesystem for implementation details
 */
class MountCommand : public ICommand
{
public:
    std::string getName() const override
    {
        return "mount";
    }

    std::string getDescription() const override
    {
        return "Mount an encrypted storage file";
    }

    CommandType getType() const override
    {
        return CommandType::Synchronous;
    }

    Status execute(const CommandContext& context) override
    {
        if (context.args.size() < 3)
        {
            fmt::print(fg(fmt::color::red), "Error: Insufficient arguments\n");
            fmt::print("Usage: mount <name> <db_path> <mount_point> [password] [max_size_mb]\n");
            fmt::print("  If password is omitted, you will be prompted to enter it.\n");
            return Status::error("Insufficient arguments");
        }

        std::string name = context.args[0];
        std::string db_path = context.args[1];
        std::string mount_point = context.args[2];
        std::string password;
        int64_t max_size_mb = 100;

        // Password is optional - prompt if not provided
        if (context.args.size() >= 4)
        {
            password = context.args[3];
        }
        else
        {
            // Prompt for password
            std::string prompt = fmt::format("Enter password for mount '{}': ", name);
            password = PasswordInput::readPassword(prompt);

            if (password.empty())
            {
                fmt::print(fg(fmt::color::red), "Error: Password cannot be empty\n");
                return Status::error("Empty password");
            }
        }

        // Max size is optional
        if (context.args.size() >= 5)
        {
            try
            {
                max_size_mb = std::stoll(context.args[4]);
            }
            catch (...)
            {
                fmt::print(fg(fmt::color::red), "Error: Invalid max_size_mb value\n");
                return Status::error("Invalid max_size_mb");
            }
        }

        // Create mount
        auto mount = std::make_shared<EncryptedMount>(name, db_path, mount_point, max_size_mb);

        if (!mount->mount(password))
        {
            fmt::print(fg(fmt::color::red), "Error: Failed to mount '{}'\n", name);
            fmt::print("Check that the password is correct and the file is accessible.\n");
            return Status::error("Mount failed");
        }

        // Add to VFS
        auto& vfs = VirtualFilesystem::getInstance();
        if (!vfs.addMount(mount))
        {
            mount->unmount();
            fmt::print(fg(fmt::color::red), "Error: Failed to add mount to VFS\n");
            return Status::error("VFS add failed");
        }

        fmt::print(fg(fmt::color::green), "Successfully mounted '{}' at '{}'\n", name, mount_point);
        return Status::ok();
    }
};

} // namespace homeshell
