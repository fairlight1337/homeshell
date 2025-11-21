#include <homeshell/EncryptedMount.hpp>

#include <algorithm>
#include <chrono>
#include <filesystem>

namespace homeshell
{

EncryptedMount::EncryptedMount(const std::string& name, const std::string& db_path,
                               const std::string& mount_point, int64_t max_size_mb)
    : name_(name)
    , db_path_(db_path)
    , mount_point_(mount_point)
    , max_size_bytes_(max_size_mb * 1024 * 1024)
    , db_(nullptr)
{
}

EncryptedMount::~EncryptedMount() noexcept
{
    // Note: We don't call unmount() here because during program shutdown,
    // SQLCipher's global state may already be destroyed, causing a segfault.
    // Instead, we just set db_ to nullptr and let the OS clean up file handles.
    // For normal operation (not program shutdown), unmount() should be called
    // explicitly before the object is destroyed.
    db_ = nullptr;
}

bool EncryptedMount::mount(const std::string& password)
{
    if (db_ != nullptr)
    {
        return true; // Already mounted
    }

    // Ensure parent directory exists
    std::filesystem::path db_file_path(db_path_);
    std::filesystem::path parent_dir = db_file_path.parent_path();

    if (!parent_dir.empty() && !std::filesystem::exists(parent_dir))
    {
        std::error_code ec;
        std::filesystem::create_directories(parent_dir, ec);
        if (ec)
        {
            // Failed to create parent directory
            return false;
        }
    }

    // Open/create database
    int rc = sqlite3_open(db_path_.c_str(), &db_);
    if (rc != SQLITE_OK)
    {
        db_ = nullptr;
        return false;
    }

    // Set encryption key
    rc = sqlite3_key(db_, password.c_str(), static_cast<int>(password.length()));
    if (rc != SQLITE_OK)
    {
        sqlite3_close(db_);
        db_ = nullptr;
        return false;
    }

    // Configure SQLCipher for performance and quota
    sqlite3_exec(db_, "PRAGMA journal_mode=WAL", nullptr, nullptr, nullptr);
    sqlite3_exec(db_, "PRAGMA synchronous=NORMAL", nullptr, nullptr, nullptr);

    // Set max page count for quota (4KB pages)
    int64_t max_pages = max_size_bytes_ / 4096;
    std::string quota_sql = "PRAGMA max_page_count = " + std::to_string(max_pages);
    sqlite3_exec(db_, quota_sql.c_str(), nullptr, nullptr, nullptr);

    // Initialize schema if needed
    if (!initializeSchema())
    {
        sqlite3_close(db_);
        db_ = nullptr;
        return false;
    }

    // Create root directory if it doesn't exist
    if (!exists("/"))
    {
        sqlite3_stmt* stmt;
        const char* sql = "INSERT INTO directories (path, parent, mtime) VALUES ('/', '', ?)";
        if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) == SQLITE_OK)
        {
            auto now = std::chrono::system_clock::now().time_since_epoch().count();
            sqlite3_bind_int64(stmt, 1, now);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }
    }

    return true;
}

bool EncryptedMount::unmount()
{
    if (db_ == nullptr)
    {
        return true;
    }

    // Close database connection
    // Note: sqlite3_close() might fail if there are unfinalized statements,
    // but we'll use sqlite3_close_v2() which handles this gracefully
    int rc = sqlite3_close_v2(db_);
    db_ = nullptr;
    
    return (rc == SQLITE_OK);
}

bool EncryptedMount::initializeSchema()
{
    const char* schema = R"(
        CREATE TABLE IF NOT EXISTS files (
            path TEXT PRIMARY KEY,
            content BLOB,
            size INTEGER,
            mtime INTEGER
        );

        CREATE TABLE IF NOT EXISTS directories (
            path TEXT PRIMARY KEY,
            parent TEXT,
            mtime INTEGER
        );

        CREATE INDEX IF NOT EXISTS idx_dir_parent ON directories(parent);
    )";

    char* err_msg = nullptr;
    int rc = sqlite3_exec(db_, schema, nullptr, nullptr, &err_msg);
    if (rc != SQLITE_OK)
    {
        if (err_msg)
        {
            sqlite3_free(err_msg);
        }
        return false;
    }

    return true;
}

std::string EncryptedMount::normalizePath(const std::string& path)
{
    if (path.empty() || path[0] != '/')
    {
        return "/" + path;
    }

    std::string normalized = path;
    // Remove trailing slash unless it's root
    if (normalized.length() > 1 && normalized.back() == '/')
    {
        normalized.pop_back();
    }

    return normalized;
}

bool EncryptedMount::exists(const std::string& path)
{
    if (!db_)
        return false;

    std::string norm_path = normalizePath(path);

    // Check files
    sqlite3_stmt* stmt;
    const char* sql = "SELECT 1 FROM files WHERE path = ? LIMIT 1";
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) == SQLITE_OK)
    {
        sqlite3_bind_text(stmt, 1, norm_path.c_str(), -1, SQLITE_STATIC);
        bool found = (sqlite3_step(stmt) == SQLITE_ROW);
        sqlite3_finalize(stmt);
        if (found)
            return true;
    }

    // Check directories
    sql = "SELECT 1 FROM directories WHERE path = ? LIMIT 1";
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) == SQLITE_OK)
    {
        sqlite3_bind_text(stmt, 1, norm_path.c_str(), -1, SQLITE_STATIC);
        bool found = (sqlite3_step(stmt) == SQLITE_ROW);
        sqlite3_finalize(stmt);
        return found;
    }

    return false;
}

bool EncryptedMount::isDirectory(const std::string& path)
{
    if (!db_)
        return false;

    std::string norm_path = normalizePath(path);

    sqlite3_stmt* stmt;
    const char* sql = "SELECT 1 FROM directories WHERE path = ? LIMIT 1";
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) == SQLITE_OK)
    {
        sqlite3_bind_text(stmt, 1, norm_path.c_str(), -1, SQLITE_STATIC);
        bool is_dir = (sqlite3_step(stmt) == SQLITE_ROW);
        sqlite3_finalize(stmt);
        return is_dir;
    }

    return false;
}

std::vector<VirtualFileInfo> EncryptedMount::listDirectory(const std::string& path)
{
    std::vector<VirtualFileInfo> results;
    if (!db_)
        return results;

    std::string norm_path = normalizePath(path);

    // List subdirectories
    sqlite3_stmt* stmt;
    const char* sql = "SELECT path, mtime FROM directories WHERE parent = ?";
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) == SQLITE_OK)
    {
        sqlite3_bind_text(stmt, 1, norm_path.c_str(), -1, SQLITE_STATIC);
        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            VirtualFileInfo info;
            info.path = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            info.name = info.path.substr(info.path.find_last_of('/') + 1);
            info.is_directory = true;
            info.size = 0;
            info.mtime = sqlite3_column_int64(stmt, 1);
            results.push_back(info);
        }
        sqlite3_finalize(stmt);
    }

    // List files
    // Files are stored with full path, need to filter by directory
    sql = "SELECT path, size, mtime FROM files";
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) == SQLITE_OK)
    {
        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            std::string file_path = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            std::string parent = getParentPath(file_path);

            if (parent == norm_path)
            {
                VirtualFileInfo info;
                info.path = file_path;
                info.name = file_path.substr(file_path.find_last_of('/') + 1);
                info.is_directory = false;
                info.size = sqlite3_column_int64(stmt, 1);
                info.mtime = sqlite3_column_int64(stmt, 2);
                results.push_back(info);
            }
        }
        sqlite3_finalize(stmt);
    }

    return results;
}

bool EncryptedMount::readFile(const std::string& path, std::string& content)
{
    if (!db_)
        return false;

    std::string norm_path = normalizePath(path);

    sqlite3_stmt* stmt;
    const char* sql = "SELECT content FROM files WHERE path = ?";
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) == SQLITE_OK)
    {
        sqlite3_bind_text(stmt, 1, norm_path.c_str(), -1, SQLITE_STATIC);
        if (sqlite3_step(stmt) == SQLITE_ROW)
        {
            const void* blob = sqlite3_column_blob(stmt, 0);
            int size = sqlite3_column_bytes(stmt, 0);
            content.assign(static_cast<const char*>(blob), size);
            sqlite3_finalize(stmt);
            return true;
        }
        sqlite3_finalize(stmt);
    }

    return false;
}

bool EncryptedMount::writeFile(const std::string& path, const std::string& content)
{
    if (!db_)
        return false;

    std::string norm_path = normalizePath(path);

    // Ensure parent directory exists
    if (!ensureParentDirectory(norm_path))
    {
        return false;
    }

    auto now = std::chrono::system_clock::now().time_since_epoch().count();

    sqlite3_stmt* stmt;
    const char* sql =
        "INSERT OR REPLACE INTO files (path, content, size, mtime) VALUES (?, ?, ?, ?)";
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) == SQLITE_OK)
    {
        sqlite3_bind_text(stmt, 1, norm_path.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_blob(stmt, 2, content.data(), static_cast<int>(content.size()), SQLITE_STATIC);
        sqlite3_bind_int64(stmt, 3, static_cast<int64_t>(content.size()));
        sqlite3_bind_int64(stmt, 4, now);

        int rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        return (rc == SQLITE_DONE);
    }

    return false;
}

bool EncryptedMount::createDirectory(const std::string& path)
{
    if (!db_)
        return false;

    std::string norm_path = normalizePath(path);

    if (exists(norm_path))
    {
        return isDirectory(norm_path);
    }

    // Ensure parent exists
    std::string parent = getParentPath(norm_path);
    if (!parent.empty() && parent != "/" && !exists(parent))
    {
        if (!createDirectory(parent))
        {
            return false;
        }
    }

    auto now = std::chrono::system_clock::now().time_since_epoch().count();

    sqlite3_stmt* stmt;
    const char* sql = "INSERT INTO directories (path, parent, mtime) VALUES (?, ?, ?)";
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) == SQLITE_OK)
    {
        sqlite3_bind_text(stmt, 1, norm_path.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, parent.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int64(stmt, 3, now);

        int rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        return (rc == SQLITE_DONE);
    }

    return false;
}

bool EncryptedMount::remove(const std::string& path)
{
    if (!db_)
        return false;

    std::string norm_path = normalizePath(path);

    if (isDirectory(norm_path))
    {
        // Remove directory and all contents
        sqlite3_stmt* stmt;
        const char* sql = "DELETE FROM directories WHERE path = ? OR parent = ?";
        if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) == SQLITE_OK)
        {
            sqlite3_bind_text(stmt, 1, norm_path.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 2, norm_path.c_str(), -1, SQLITE_STATIC);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }

        // Remove files in directory
        sql = "DELETE FROM files WHERE path LIKE ?";
        if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) == SQLITE_OK)
        {
            std::string pattern = norm_path + "/%";
            sqlite3_bind_text(stmt, 1, pattern.c_str(), -1, SQLITE_STATIC);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }

        return true;
    }
    else
    {
        // Remove file
        sqlite3_stmt* stmt;
        const char* sql = "DELETE FROM files WHERE path = ?";
        if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) == SQLITE_OK)
        {
            sqlite3_bind_text(stmt, 1, norm_path.c_str(), -1, SQLITE_STATIC);
            int rc = sqlite3_step(stmt);
            sqlite3_finalize(stmt);
            return (rc == SQLITE_DONE);
        }
    }

    return false;
}

int64_t EncryptedMount::getUsedSpace()
{
    if (!db_)
        return 0;

    sqlite3_stmt* stmt;
    const char* sql = "SELECT SUM(size) FROM files";
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) == SQLITE_OK)
    {
        if (sqlite3_step(stmt) == SQLITE_ROW)
        {
            int64_t used = sqlite3_column_int64(stmt, 0);
            sqlite3_finalize(stmt);
            return used;
        }
        sqlite3_finalize(stmt);
    }

    return 0;
}

bool EncryptedMount::ensureParentDirectory(const std::string& path)
{
    std::string parent = getParentPath(path);
    if (parent.empty() || parent == "/")
    {
        return true;
    }

    if (exists(parent))
    {
        return isDirectory(parent);
    }

    return createDirectory(parent);
}

std::string EncryptedMount::getParentPath(const std::string& path)
{
    std::string norm_path = normalizePath(path);
    size_t last_slash = norm_path.find_last_of('/');

    if (last_slash == 0)
    {
        return "/";
    }
    else if (last_slash != std::string::npos)
    {
        return norm_path.substr(0, last_slash);
    }

    return "";
}

} // namespace homeshell
