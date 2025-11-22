#pragma once

#include <sqlite3.h>

#include <chrono>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace homeshell
{

/**
 * @brief File information stored in the locate database
 */
struct FileEntry
{
    std::string path;  ///< Full file path
    bool is_directory; ///< Whether the entry is a directory
    int64_t size;      ///< File size in bytes (0 for directories)
    int64_t mtime;     ///< Last modification time (Unix timestamp)
};

/**
 * @brief Statistics about the file database
 */
struct DatabaseStats
{
    int64_t total_files = 0;       ///< Total number of files indexed
    int64_t total_directories = 0; ///< Total number of directories indexed
    int64_t total_size = 0;        ///< Total size of all files in bytes
    double scan_time_ms = 0.0;     ///< Time taken to scan in milliseconds
};

/**
 * @brief SQLite database for file indexing and fast searching
 *
 * @details The FileDatabase class provides fast file lookup capabilities
 * similar to the Unix `locate` command. It maintains a SQLite database
 * of all files and directories in the filesystem for quick searches.
 *
 * **Features:**
 * - SQLite-based file index
 * - Fast pattern matching queries
 * - Stores file metadata (path, type, size, mtime)
 * - Efficient indexing for substring searches
 * - Support for both regular and virtual filesystems
 *
 * **Database Schema:**
 * - `files` table: path, is_directory, size, mtime
 * - Index on path column for fast lookups
 *
 * **Usage:**
 * @code
 * FileDatabase db("~/.homeshell/locate.db");
 *
 * if (db.open()) {
 *     // Update database
 *     auto stats = db.updateDatabase({"/home", "/usr"}, {"/tmp"});
 *     fmt::print("Indexed {} files\n", stats.total_files);
 *
 *     // Search for files
 *     auto results = db.search("*.cpp");
 *     for (const auto& entry : results) {
 *         fmt::print("{}\n", entry.path);
 *     }
 *
 *     db.close();
 * }
 * @endcode
 *
 * @note Database file is created automatically if it doesn't exist
 * @note Requires SQLite3 library
 */
class FileDatabase
{
public:
    /**
     * @brief Construct a FileDatabase with specified database path
     * @param db_path Path to SQLite database file
     */
    explicit FileDatabase(const std::string& db_path);

    /**
     * @brief Destructor - closes database if open
     */
    ~FileDatabase();

    /**
     * @brief Open the database connection
     * @return true if successful, false on error
     */
    bool open();

    /**
     * @brief Close the database connection
     */
    void close();

    /**
     * @brief Check if database is currently open
     * @return true if database is open, false otherwise
     */
    bool isOpen() const;

    /**
     * @brief Update the database by scanning filesystem
     *
     * @param search_paths Directories to scan (default: {"/", virtual mounts})
     * @param exclude_paths Directories to skip (default: {"/proc", "/sys", "/dev"})
     * @return Statistics about the update operation
     *
     * @details Recursively scans specified directories and updates the database
     * with all files and directories found. Existing entries are replaced.
     *
     * @note Can take a long time for large directory trees
     * @note Virtual filesystem paths are also scanned
     */
    DatabaseStats updateDatabase(const std::vector<std::string>& search_paths = {},
                                 const std::vector<std::string>& exclude_paths = {});

    /**
     * @brief Search for files matching a pattern
     *
     * @param pattern Search pattern (supports wildcards: *, ?)
     * @param case_sensitive Whether search is case-sensitive (default: false)
     * @param limit Maximum number of results (default: 1000)
     * @return Vector of matching file entries
     *
     * @details Searches the database for files matching the given pattern.
     * Wildcards are supported: `*` matches any characters, `?` matches single character.
     *
     * **Examples:**
     * - `*.cpp` - All C++ source files
     * - `test_*.cpp` - Test files
     * - `/usr/bin/*` - All files in /usr/bin
     * - `*config*` - Any path containing "config"
     *
     * @note Case-insensitive by default for easier searching
     * @note Limited to 1000 results by default to prevent overwhelming output
     */
    std::vector<FileEntry> search(const std::string& pattern, bool case_sensitive = false,
                                  int limit = 1000);

    /**
     * @brief Get statistics about the current database
     * @return Database statistics (counts, sizes)
     */
    DatabaseStats getStats();

    /**
     * @brief Clear all entries from the database
     * @return true if successful, false on error
     */
    bool clear();

private:
    std::string db_path_;
    sqlite3* db_;

    /**
     * @brief Initialize database schema
     * @return true if successful, false on error
     */
    bool initSchema();

    /**
     * @brief Scan a directory recursively and collect file entries
     * @param path Directory to scan
     * @param exclude_paths Paths to exclude
     * @param entries Output vector for collected entries
     */
    void scanDirectory(const std::string& path, const std::vector<std::string>& exclude_paths,
                       std::vector<FileEntry>& entries);

    /**
     * @brief Check if a path should be excluded
     * @param path Path to check
     * @param exclude_paths List of excluded paths
     * @return true if path should be excluded
     */
    bool shouldExclude(const std::string& path, const std::vector<std::string>& exclude_paths);

    /**
     * @brief Convert wildcard pattern to SQL LIKE pattern
     * @param pattern Wildcard pattern with * and ?
     * @return SQL LIKE pattern with % and _
     */
    std::string wildcardToSqlPattern(const std::string& pattern);
};

} // namespace homeshell
