#include <homeshell/FileDatabase.hpp>
#include <homeshell/VirtualFilesystem.hpp>

#include <fmt/core.h>

#include <algorithm>
#include <chrono>
#include <cstring>
#include <filesystem>

namespace fs = std::filesystem;

namespace homeshell
{

FileDatabase::FileDatabase(const std::string& db_path)
    : db_path_(db_path)
    , db_(nullptr)
{
}

FileDatabase::~FileDatabase()
{
    close();
}

bool FileDatabase::open()
{
    if (db_)
    {
        return true; // Already open
    }

    // Expand ~ in path
    std::string expanded_path = db_path_;
    if (!db_path_.empty() && db_path_[0] == '~')
    {
        const char* home = getenv("HOME");
        if (home)
        {
            expanded_path = std::string(home) + db_path_.substr(1);
        }
    }

    // Create parent directory if it doesn't exist
    fs::path db_file_path(expanded_path);
    if (db_file_path.has_parent_path())
    {
        std::error_code ec;
        fs::create_directories(db_file_path.parent_path(), ec);
    }

    // Open database
    int rc = sqlite3_open(expanded_path.c_str(), &db_);
    if (rc != SQLITE_OK)
    {
        fmt::print("Error opening database: {}\n", sqlite3_errmsg(db_));
        sqlite3_close(db_);
        db_ = nullptr;
        return false;
    }

    return initSchema();
}

void FileDatabase::close()
{
    if (db_)
    {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

bool FileDatabase::isOpen() const
{
    return db_ != nullptr;
}

bool FileDatabase::initSchema()
{
    if (!db_)
    {
        return false;
    }

    const char* schema = R"(
        CREATE TABLE IF NOT EXISTS files (
            path TEXT PRIMARY KEY,
            is_directory INTEGER NOT NULL,
            size INTEGER NOT NULL,
            mtime INTEGER NOT NULL
        );
        CREATE INDEX IF NOT EXISTS idx_path ON files(path);
    )";

    char* err_msg = nullptr;
    int rc = sqlite3_exec(db_, schema, nullptr, nullptr, &err_msg);

    if (rc != SQLITE_OK)
    {
        fmt::print("Error creating schema: {}\n", err_msg);
        sqlite3_free(err_msg);
        return false;
    }

    return true;
}

DatabaseStats FileDatabase::updateDatabase(const std::vector<std::string>& search_paths,
                                           const std::vector<std::string>& exclude_paths)
{
    DatabaseStats stats;
    auto start_time = std::chrono::high_resolution_clock::now();

    if (!db_)
    {
        return stats;
    }

    // Default paths to scan
    std::vector<std::string> paths_to_scan = search_paths;
    if (paths_to_scan.empty())
    {
        paths_to_scan.push_back(fs::current_path().string());

        // Add virtual filesystem mounts
        auto& vfs = VirtualFilesystem::getInstance();
        auto mount_names = vfs.getMountNames();
        for (const auto& name : mount_names)
        {
            auto* mount = vfs.getMount(name);
            if (mount && mount->is_mounted())
            {
                paths_to_scan.push_back(mount->getMountPoint());
            }
        }
    }

    // Default exclude paths
    std::vector<std::string> excludes = exclude_paths;
    if (excludes.empty())
    {
        excludes = {"/proc", "/sys", "/dev", "/run", "/tmp"};
    }

    // Clear existing data
    sqlite3_exec(db_, "DELETE FROM files", nullptr, nullptr, nullptr);

    // Collect all file entries
    std::vector<FileEntry> entries;
    for (const auto& path : paths_to_scan)
    {
        scanDirectory(path, excludes, entries);
    }

    // Begin transaction for faster inserts
    sqlite3_exec(db_, "BEGIN TRANSACTION", nullptr, nullptr, nullptr);

    // Prepare insert statement
    const char* insert_sql =
        "INSERT OR REPLACE INTO files (path, is_directory, size, mtime) VALUES (?, ?, ?, ?)";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, insert_sql, -1, &stmt, nullptr);

    // Insert all entries
    for (const auto& entry : entries)
    {
        sqlite3_bind_text(stmt, 1, entry.path.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 2, entry.is_directory ? 1 : 0);
        sqlite3_bind_int64(stmt, 3, entry.size);
        sqlite3_bind_int64(stmt, 4, entry.mtime);

        sqlite3_step(stmt);
        sqlite3_reset(stmt);

        if (entry.is_directory)
        {
            stats.total_directories++;
        }
        else
        {
            stats.total_files++;
            stats.total_size += entry.size;
        }
    }

    sqlite3_finalize(stmt);
    sqlite3_exec(db_, "COMMIT", nullptr, nullptr, nullptr);

    auto end_time = std::chrono::high_resolution_clock::now();
    stats.scan_time_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();

    return stats;
}

void FileDatabase::scanDirectory(const std::string& path,
                                 const std::vector<std::string>& exclude_paths,
                                 std::vector<FileEntry>& entries)
{
    if (shouldExclude(path, exclude_paths))
    {
        return;
    }

    auto& vfs = VirtualFilesystem::getInstance();

    // Check if this is a virtual filesystem path
    if (vfs.isVirtualPath(path))
    {
        // Scan virtual filesystem
        if (!vfs.exists(path) || !vfs.isDirectory(path))
        {
            return;
        }

        auto vfs_entries = vfs.listDirectory(path);
        for (const auto& vfs_entry : vfs_entries)
        {
            std::string entry_path = path;
            if (path.back() != '/')
            {
                entry_path += "/";
            }
            entry_path += vfs_entry.name;

            FileEntry file_entry;
            file_entry.path = entry_path;
            file_entry.is_directory = vfs_entry.is_directory;
            file_entry.size = vfs_entry.size;
            file_entry.mtime = vfs_entry.mtime;

            entries.push_back(file_entry);

            if (vfs_entry.is_directory)
            {
                scanDirectory(entry_path, exclude_paths, entries);
            }
        }
    }
    else
    {
        // Scan regular filesystem
        std::error_code ec;
        if (!fs::exists(path, ec))
        {
            return;
        }

        if (!fs::is_directory(path, ec))
        {
            // If it's a file, just add it
            FileEntry file_entry;
            file_entry.path = path;
            file_entry.is_directory = false;
            file_entry.size = fs::file_size(path, ec);
            file_entry.mtime = 0;

            auto ftime = fs::last_write_time(path, ec);
            if (!ec)
            {
                file_entry.mtime =
                    std::chrono::duration_cast<std::chrono::seconds>(ftime.time_since_epoch())
                        .count();
            }

            entries.push_back(file_entry);
            return;
        }

        // Add the directory itself
        FileEntry dir_entry;
        dir_entry.path = path;
        dir_entry.is_directory = true;
        dir_entry.size = 0;
        dir_entry.mtime = 0;

        auto ftime = fs::last_write_time(path, ec);
        if (!ec)
        {
            dir_entry.mtime =
                std::chrono::duration_cast<std::chrono::seconds>(ftime.time_since_epoch()).count();
        }

        entries.push_back(dir_entry);

        // Recursively scan subdirectories
        std::error_code iter_ec;
        auto iter_end = fs::recursive_directory_iterator();
        auto iter = fs::recursive_directory_iterator(
            path, fs::directory_options::skip_permission_denied, iter_ec);

        if (iter_ec)
        {
            // Failed to create iterator
            return;
        }

        for (; iter != iter_end; ++iter)
        {
            std::error_code entry_ec;
            const auto& dir_entry = *iter;

            if (entry_ec)
            {
                continue;
            }

            std::string entry_path = dir_entry.path().string();
            if (shouldExclude(entry_path, exclude_paths))
            {
                continue;
            }

            FileEntry file_entry;
            file_entry.path = entry_path;
            file_entry.is_directory = dir_entry.is_directory(entry_ec);
            file_entry.size = 0;
            file_entry.mtime = 0;

            if (!entry_ec && dir_entry.is_regular_file(entry_ec))
            {
                file_entry.size = dir_entry.file_size(entry_ec);
            }

            if (!entry_ec)
            {
                auto ftime = dir_entry.last_write_time(entry_ec);
                if (!entry_ec)
                {
                    file_entry.mtime =
                        std::chrono::duration_cast<std::chrono::seconds>(ftime.time_since_epoch())
                            .count();
                }
            }

            entries.push_back(file_entry);
        }
    }
}

bool FileDatabase::shouldExclude(const std::string& path,
                                 const std::vector<std::string>& exclude_paths)
{
    for (const auto& exclude : exclude_paths)
    {
        if (path.find(exclude) == 0)
        {
            return true;
        }
    }
    return false;
}

std::vector<FileEntry> FileDatabase::search(const std::string& pattern, bool case_sensitive,
                                            int limit)
{
    std::vector<FileEntry> results;

    if (!db_)
    {
        return results;
    }

    std::string sql_pattern = wildcardToSqlPattern(pattern);
    std::string query;

    if (case_sensitive)
    {
        query = "SELECT path, is_directory, size, mtime FROM files WHERE path GLOB ? LIMIT ?";
    }
    else
    {
        query = "SELECT path, is_directory, size, mtime FROM files WHERE path LIKE ? LIMIT ?";
    }

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, query.c_str(), -1, &stmt, nullptr);

    if (rc != SQLITE_OK)
    {
        return results;
    }

    sqlite3_bind_text(stmt, 1, sql_pattern.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, limit);

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        FileEntry entry;
        entry.path = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        entry.is_directory = sqlite3_column_int(stmt, 1) != 0;
        entry.size = sqlite3_column_int64(stmt, 2);
        entry.mtime = sqlite3_column_int64(stmt, 3);

        results.push_back(entry);
    }

    sqlite3_finalize(stmt);
    return results;
}

std::string FileDatabase::wildcardToSqlPattern(const std::string& pattern)
{
    std::string sql_pattern;
    sql_pattern.reserve(pattern.size() + 2);

    // If pattern doesn't start with /, add wildcard prefix
    if (pattern.empty() || pattern[0] != '/')
    {
        sql_pattern += '%';
    }

    for (char c : pattern)
    {
        if (c == '*')
        {
            sql_pattern += '%';
        }
        else if (c == '?')
        {
            sql_pattern += '_';
        }
        else
        {
            sql_pattern += c;
        }
    }

    // Add wildcard suffix if pattern doesn't end with a specific match
    if (pattern.empty() || (pattern.back() != '/' && pattern.find('.') == std::string::npos))
    {
        sql_pattern += '%';
    }

    return sql_pattern;
}

DatabaseStats FileDatabase::getStats()
{
    DatabaseStats stats;

    if (!db_)
    {
        return stats;
    }

    const char* query = "SELECT COUNT(*) FILTER (WHERE is_directory = 0), "
                        "COUNT(*) FILTER (WHERE is_directory = 1), "
                        "SUM(size) FROM files";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, query, -1, &stmt, nullptr);

    if (rc == SQLITE_OK && sqlite3_step(stmt) == SQLITE_ROW)
    {
        stats.total_files = sqlite3_column_int64(stmt, 0);
        stats.total_directories = sqlite3_column_int64(stmt, 1);
        stats.total_size = sqlite3_column_int64(stmt, 2);
    }

    sqlite3_finalize(stmt);
    return stats;
}

bool FileDatabase::clear()
{
    if (!db_)
    {
        return false;
    }

    char* err_msg = nullptr;
    int rc = sqlite3_exec(db_, "DELETE FROM files", nullptr, nullptr, &err_msg);

    if (rc != SQLITE_OK)
    {
        sqlite3_free(err_msg);
        return false;
    }

    return true;
}

} // namespace homeshell
