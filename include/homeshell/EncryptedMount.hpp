#pragma once

#include <sqlite3.h>

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace homeshell
{

struct VirtualFileInfo
{
    std::string name;
    std::string path;
    bool is_directory;
    int64_t size;
    int64_t mtime;
};

class EncryptedMount
{
public:
    EncryptedMount(const std::string& name, const std::string& db_path,
                   const std::string& mount_point, int64_t max_size_mb);
    ~EncryptedMount();

    // Mount operations
    bool mount(const std::string& password);
    bool unmount();
    bool is_mounted() const
    {
        return db_ != nullptr;
    }

    // Getters
    std::string getName() const
    {
        return name_;
    }
    std::string getMountPoint() const
    {
        return mount_point_;
    }
    std::string getDbPath() const
    {
        return db_path_;
    }

    // File operations
    bool exists(const std::string& path);
    bool isDirectory(const std::string& path);
    std::vector<VirtualFileInfo> listDirectory(const std::string& path);
    bool readFile(const std::string& path, std::string& content);
    bool writeFile(const std::string& path, const std::string& content);
    bool createDirectory(const std::string& path);
    bool remove(const std::string& path);
    int64_t getUsedSpace();
    int64_t getMaxSpace() const
    {
        return max_size_bytes_;
    }

private:
    bool initializeSchema();
    bool ensureParentDirectory(const std::string& path);
    std::string getParentPath(const std::string& path);
    std::string normalizePath(const std::string& path);

    std::string name_;
    std::string db_path_;
    std::string mount_point_;
    int64_t max_size_bytes_;
    sqlite3* db_;
};

} // namespace homeshell
