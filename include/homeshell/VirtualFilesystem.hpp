#pragma once

#include <homeshell/EncryptedMount.hpp>
#include <homeshell/FilesystemHelper.hpp>

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace homeshell
{

/**
 * @brief Type of filesystem path
 */
enum class PathType
{
    Real,   ///< Regular filesystem path
    Virtual ///< Path within encrypted virtual mount
};

/**
 * @brief Result of path resolution
 *
 * Contains information about whether a path points to the real filesystem
 * or a virtual encrypted mount, along with the resolved absolute path.
 */
struct ResolvedPath
{
    PathType type;                   ///< Type of path (real or virtual)
    std::string full_path;           ///< Fully resolved absolute path
    std::string mount_point;         ///< Mount point name (for virtual paths)
    std::string relative_path;       ///< Path within the mount (for virtual paths)
    EncryptedMount* mount = nullptr; ///< Pointer to mount (for virtual paths)
};

/**
 * @brief Virtual filesystem with encrypted mount support
 *
 * Provides a unified interface to both the regular filesystem and encrypted
 * virtual mounts. Implements a singleton pattern to manage all mounts globally.
 *
 * @details The VirtualFilesystem allows transparent access to encrypted storage
 *          through mount points. Paths beginning with a mount point name are
 *          automatically routed to the encrypted storage backend. All other
 *          paths are handled by the regular filesystem.
 *
 *          Example mount structure:
 *          - /real/path/file.txt  → regular filesystem
 *          - /secure/file.txt     → encrypted mount named "secure"
 *
 *          This class is thread-safe through the singleton pattern but does not
 *          provide internal synchronization for mount operations.
 */
class VirtualFilesystem
{
public:
    /**
     * @brief Get the singleton instance
     * @return Reference to the global VirtualFilesystem instance
     */
    static VirtualFilesystem& getInstance()
    {
        static VirtualFilesystem instance;
        return instance;
    }

    /**
     * @brief Add an encrypted mount to the filesystem
     * @param mount Shared pointer to initialized encrypted mount
     * @return true if mount added successfully, false if name conflict
     */
    bool addMount(std::shared_ptr<EncryptedMount> mount);

    /**
     * @brief Remove and unmount an encrypted mount
     * @param name Name of the mount to remove
     * @return true if mount removed successfully, false if not found
     */
    bool removeMount(const std::string& name);

    /**
     * @brief Get a mount by name
     * @param name Name of the mount to retrieve
     * @return Pointer to mount, or nullptr if not found
     */
    EncryptedMount* getMount(const std::string& name);

    /**
     * @brief Get names of all mounted volumes
     * @return Vector of mount names
     */
    std::vector<std::string> getMountNames() const;

    /**
     * @brief Resolve a path to its real or virtual location
     * @param path Path to resolve (can be relative or absolute)
     * @return ResolvedPath containing path type and full path information
     */
    ResolvedPath resolvePath(const std::string& path) const;

    /**
     * @brief Check if a path points to a virtual mount
     * @param path Path to check
     * @return true if path is within a virtual mount, false otherwise
     */
    bool isVirtualPath(const std::string& path) const;

    /**
     * @brief List directory contents
     * @param path Directory path (real or virtual)
     * @return Vector of file information structures
     */
    std::vector<VirtualFileInfo> listDirectory(const std::string& path);

    /**
     * @brief Change the current working directory
     * @param path New directory path
     * @param[out] result_path Resolved absolute path of new directory
     * @return true if directory change successful, false otherwise
     */
    bool changeDirectory(const std::string& path, std::string& result_path);

    /**
     * @brief Get the current working directory
     * @return Current directory path
     */
    std::string getCurrentDirectory() const
    {
        return current_directory_;
    }

    /**
     * @brief Set the current working directory
     * @param path New current directory path
     */
    void setCurrentDirectory(const std::string& path)
    {
        current_directory_ = path;
    }

    /**
     * @brief Check if a file or directory exists
     * @param path Path to check (real or virtual)
     * @return true if path exists, false otherwise
     */
    bool exists(const std::string& path);

    /**
     * @brief Check if a path is a directory
     * @param path Path to check (real or virtual)
     * @return true if path is a directory, false otherwise
     */
    bool isDirectory(const std::string& path);

    /**
     * @brief Read entire file contents
     * @param path File path (real or virtual)
     * @param[out] content File contents
     * @return true if read successful, false otherwise
     */
    bool readFile(const std::string& path, std::string& content);

    /**
     * @brief Write file contents
     * @param path File path (real or virtual)
     * @param content Data to write
     * @return true if write successful, false otherwise
     */
    bool writeFile(const std::string& path, const std::string& content);

    /**
     * @brief Create a directory
     * @param path Directory path to create (real or virtual)
     * @return true if creation successful, false otherwise
     */
    bool createDirectory(const std::string& path);

    /**
     * @brief Remove a file or directory
     * @param path Path to remove (real or virtual)
     * @return true if removal successful, false otherwise
     */
    bool remove(const std::string& path);

    ~VirtualFilesystem() noexcept
    {
        try
        {
            // During program shutdown, we cannot safely call SQLite functions
            // because SQLCipher's global state may already be destroyed.
            // Just clear the map and let the OS clean up file handles.
            mounts_.clear();
        }
        catch (...)
        {
            // Swallow any exceptions in destructor to prevent terminate()
        }
    }

private:
    VirtualFilesystem()
        : current_directory_(FilesystemHelper::getCurrentDirectory().string())
    {
    }
    VirtualFilesystem(const VirtualFilesystem&) = delete;
    VirtualFilesystem& operator=(const VirtualFilesystem&) = delete;

    std::string resolveRelativePath(const std::string& path) const;

    std::map<std::string, std::shared_ptr<EncryptedMount>> mounts_;
    std::string current_directory_;
};

} // namespace homeshell
