#pragma once

#include <homeshell/EncryptedMount.hpp>
#include <homeshell/FilesystemHelper.hpp>

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace homeshell
{

enum class PathType
{
    Real,   // Regular filesystem
    Virtual // Virtual encrypted mount
};

struct ResolvedPath
{
    PathType type;
    std::string full_path;
    std::string mount_point;         // For virtual paths
    std::string relative_path;       // Path within mount
    EncryptedMount* mount = nullptr; // For virtual paths
};

class VirtualFilesystem
{
public:
    static VirtualFilesystem& getInstance()
    {
        static VirtualFilesystem instance;
        return instance;
    }

    // Mount management
    bool addMount(std::shared_ptr<EncryptedMount> mount);
    bool removeMount(const std::string& name);
    EncryptedMount* getMount(const std::string& name);
    std::vector<std::string> getMountNames() const;

    // Path resolution
    ResolvedPath resolvePath(const std::string& path) const;
    bool isVirtualPath(const std::string& path) const;

    // Directory operations
    std::vector<VirtualFileInfo> listDirectory(const std::string& path);
    bool changeDirectory(const std::string& path, std::string& result_path);
    std::string getCurrentDirectory() const
    {
        return current_directory_;
    }
    void setCurrentDirectory(const std::string& path)
    {
        current_directory_ = path;
    }

    // File operations
    bool exists(const std::string& path);
    bool isDirectory(const std::string& path);
    bool readFile(const std::string& path, std::string& content);
    bool writeFile(const std::string& path, const std::string& content);
    bool createDirectory(const std::string& path);
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
