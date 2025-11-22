#pragma once

#include <sqlite3.h>

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace homeshell
{

/**
 * @brief Virtual file information structure
 *
 * Contains metadata about a file or directory in the virtual filesystem.
 * Used when listing directory contents.
 */
struct VirtualFileInfo
{
    std::string name;  ///< File or directory name (without path)
    std::string path;  ///< Full path within the virtual filesystem
    bool is_directory; ///< true if this is a directory, false if file
    int64_t size;      ///< File size in bytes (0 for directories)
    int64_t mtime;     ///< Last modification time (Unix timestamp)
};

/**
 * @brief Encrypted virtual filesystem mount
 *
 * Provides an encrypted storage backend using SQLCipher (AES-256 encryption).
 * Files are stored as BLOBs in an encrypted SQLite database with full
 * directory hierarchy support.
 *
 * @details Each mount represents a separate encrypted volume with:
 *          - Unique name for identification
 *          - Mount point for path resolution
 *          - Password-based encryption (AES-256)
 *          - Configurable size quota
 *          - Full filesystem operations (create, read, write, delete)
 *
 *          Security features:
 *          - Block-level encryption via SQLCipher
 *          - Password required for mounting
 *          - Automatic database initialization
 *          - Safe cleanup during unmount
 *
 *          Storage layout:
 *          - Database file on regular filesystem
 *          - Internal schema with files and metadata tables
 *          - Hierarchical directory structure
 *
 * Example usage:
 * ```cpp
 * auto mount = std::make_shared<EncryptedMount>(
 *     "secure", "/home/user/secure.db", "/secure", 100);
 * if (mount->mount("my_password")) {
 *     mount->writeFile("/secure/secret.txt", "confidential data");
 * }
 * ```
 */
class EncryptedMount
{
public:
    /**
     * @brief Construct an encrypted mount
     * @param name Unique name for this mount
     * @param db_path Path to the SQLCipher database file
     * @param mount_point Virtual path prefix (e.g., "/secure")
     * @param max_size_mb Maximum storage size in megabytes
     */
    EncryptedMount(const std::string& name, const std::string& db_path,
                   const std::string& mount_point, int64_t max_size_mb);

    /**
     * @brief Destructor - safely cleans up without calling SQLite during static destruction
     */
    ~EncryptedMount() noexcept;

    /**
     * @brief Mount the encrypted volume
     * @param password Password for decryption
     * @return true if mount successful, false if password incorrect or database error
     */
    bool mount(const std::string& password);

    /**
     * @brief Unmount the encrypted volume
     * @return true if unmount successful, false on error
     */
    bool unmount();

    /**
     * @brief Check if mount is currently mounted
     * @return true if mounted and accessible, false otherwise
     */
    bool is_mounted() const
    {
        return db_ != nullptr;
    }

    /**
     * @brief Get the mount name
     * @return Mount name identifier
     */
    std::string getName() const
    {
        return name_;
    }

    /**
     * @brief Get the mount point path
     * @return Virtual path prefix for this mount
     */
    std::string getMountPoint() const
    {
        return mount_point_;
    }

    /**
     * @brief Get the database file path
     * @return Path to the SQLCipher database file
     */
    std::string getDbPath() const
    {
        return db_path_;
    }

    /**
     * @brief Check if a file or directory exists
     * @param path Path within the mount (relative to mount point)
     * @return true if path exists, false otherwise
     */
    bool exists(const std::string& path);

    /**
     * @brief Check if a path is a directory
     * @param path Path within the mount
     * @return true if path is a directory, false if file or doesn't exist
     */
    bool isDirectory(const std::string& path);

    /**
     * @brief List directory contents
     * @param path Directory path within the mount
     * @return Vector of file information structures for directory contents
     */
    std::vector<VirtualFileInfo> listDirectory(const std::string& path);

    /**
     * @brief Read entire file contents
     * @param path File path within the mount
     * @param[out] content Buffer to receive file contents
     * @return true if read successful, false if file not found or error
     */
    bool readFile(const std::string& path, std::string& content);

    /**
     * @brief Write file contents
     * @param path File path within the mount
     * @param content Data to write
     * @return true if write successful, false on error or quota exceeded
     */
    bool writeFile(const std::string& path, const std::string& content);

    /**
     * @brief Create a directory
     * @param path Directory path to create within the mount
     * @return true if creation successful, false if already exists or error
     */
    bool createDirectory(const std::string& path);

    /**
     * @brief Remove a file or directory
     * @param path Path to remove within the mount
     * @return true if removal successful, false if not found or error
     */
    bool remove(const std::string& path);

    /**
     * @brief Get current storage usage
     * @return Number of bytes currently used
     */
    int64_t getUsedSpace();

    /**
     * @brief Get maximum storage capacity
     * @return Maximum number of bytes allowed (quota)
     */
    int64_t getMaxSpace() const
    {
        return max_size_bytes_;
    }

private:
    /**
     * @brief Initialize database schema on first mount
     * @return true if schema creation successful, false on error
     */
    bool initializeSchema();

    /**
     * @brief Ensure parent directory exists for a path
     * @param path Path whose parent should exist
     * @return true if parent exists or was created, false on error
     */
    bool ensureParentDirectory(const std::string& path);

    /**
     * @brief Get parent directory path
     * @param path Full path
     * @return Parent directory path
     */
    std::string getParentPath(const std::string& path);

    /**
     * @brief Normalize a path (remove trailing slashes, resolve dots)
     * @param path Path to normalize
     * @return Normalized path string
     */
    std::string normalizePath(const std::string& path);

    std::string name_;        ///< Unique mount name
    std::string db_path_;     ///< Path to SQLCipher database file
    std::string mount_point_; ///< Virtual path prefix
    int64_t max_size_bytes_;  ///< Maximum storage quota in bytes
    sqlite3* db_;             ///< SQLite/SQLCipher database handle
};

} // namespace homeshell
