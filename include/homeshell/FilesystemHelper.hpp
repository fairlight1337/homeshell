#pragma once

#include <algorithm>
#include <filesystem>
#include <string>
#include <system_error>
#include <vector>

namespace homeshell
{

namespace fs = std::filesystem;

/**
 * @brief File metadata information
 *
 * Contains detailed information about a filesystem entry (file, directory, symlink).
 */
struct FileInfo
{
    std::string name;        ///< Entry name (without path)
    bool is_directory;       ///< true if this is a directory
    bool is_regular_file;    ///< true if this is a regular file
    bool is_symlink;         ///< true if this is a symbolic link
    uintmax_t size;          ///< File size in bytes (0 for directories)
    std::string permissions; ///< Unix-style permission string (e.g., "rwxr-xr-x")
};

/**
 * @brief Filesystem utility functions
 *
 * Provides cross-platform filesystem operations using std::filesystem,
 * with additional utility functions for common tasks.
 *
 * @details Features:
 *          - Directory navigation (get/change current directory)
 *          - Directory listing with detailed file information
 *          - File type detection (regular, directory, symlink)
 *          - Path manipulation (absolute paths, resolution)
 *          - Permission string formatting
 *          - Human-readable size formatting
 *          - Error handling via std::error_code
 *
 *          All methods are static; no instance needed.
 *
 * Example usage:
 * @code
 * // List directory
 * auto files = FilesystemHelper::listDirectory("/home/user");
 * for (const auto& file : files) {
 *     fmt::print("{} ({})\n", file.name,
 *                FilesystemHelper::formatSize(file.size));
 * }
 *
 * // Change directory
 * if (FilesystemHelper::changeDirectory("/tmp")) {
 *     auto cwd = FilesystemHelper::getCurrentDirectory();
 * }
 * @endcode
 *
 * @note Uses std::filesystem (C++17) for cross-platform compatibility.
 */
class FilesystemHelper
{
public:
    /**
     * @brief Get the current working directory
     * @return Current working directory path, or "." if error occurs
     */
    static fs::path getCurrentDirectory()
    {
        std::error_code ec;
        auto path = fs::current_path(ec);
        if (ec)
        {
            return fs::path(".");
        }
        return path;
    }

    /**
     * @brief Change the current working directory
     * @param path Target directory path
     * @return true if successful, false otherwise
     */
    static bool changeDirectory(const std::string& path)
    {
        std::error_code ec;
        fs::current_path(path, ec);
        return !ec;
    }

    /**
     * @brief List directory contents
     * @param path Directory path (empty for current directory)
     * @return Vector of FileInfo entries
     */
    static std::vector<FileInfo> listDirectory(const std::string& path = ".")
    {
        std::vector<FileInfo> entries;
        std::error_code ec;

        fs::path dir_path = path.empty() ? fs::current_path(ec) : fs::path(path);

        if (ec || !fs::exists(dir_path, ec) || !fs::is_directory(dir_path, ec))
        {
            return entries;
        }

        for (const auto& entry : fs::directory_iterator(dir_path, ec))
        {
            if (ec)
            {
                continue;
            }

            FileInfo info;
            info.name = entry.path().filename().string();
            info.is_directory = entry.is_directory(ec);
            info.is_regular_file = entry.is_regular_file(ec);
            info.is_symlink = entry.is_symlink(ec);

            if (entry.is_regular_file(ec))
            {
                info.size = entry.file_size(ec);
            }
            else
            {
                info.size = 0;
            }

            info.permissions = getPermissionsString(entry.status(ec).permissions());

            entries.push_back(info);
        }

        // Sort: directories first, then files, alphabetically within each group
        std::sort(entries.begin(), entries.end(),
                  [](const FileInfo& a, const FileInfo& b)
                  {
                      if (a.is_directory != b.is_directory)
                      {
                          return a.is_directory;
                      }
                      return a.name < b.name;
                  });

        return entries;
    }

    /**
     * @brief Check if a path exists
     * @param path Path to check
     * @return true if path exists, false otherwise
     */
    static bool exists(const std::string& path)
    {
        std::error_code ec;
        return fs::exists(path, ec);
    }

    /**
     * @brief Check if a path is a directory
     * @param path Path to check
     * @return true if path is a directory, false otherwise
     */
    static bool isDirectory(const std::string& path)
    {
        std::error_code ec;
        return fs::is_directory(path, ec);
    }

    /**
     * @brief Check if a path is a regular file
     * @param path Path to check
     * @return true if path is a regular file, false otherwise
     */
    static bool isRegularFile(const std::string& path)
    {
        std::error_code ec;
        return fs::is_regular_file(path, ec);
    }

    /**
     * @brief Get the absolute path
     * @param path Relative or absolute path
     * @return Absolute path, or original path if error occurs
     */
    static fs::path getAbsolutePath(const std::string& path)
    {
        std::error_code ec;
        auto abs = fs::absolute(path, ec);
        if (ec)
        {
            return fs::path(path);
        }
        return abs;
    }

    /**
     * @brief Format file size in human-readable format
     * @param size File size in bytes
     * @return Formatted string with appropriate unit (B, KB, MB, GB, TB)
     */
    static std::string formatSize(uintmax_t size)
    {
        const char* units[] = {"B", "KB", "MB", "GB", "TB"};
        int unit_index = 0;
        double display_size = static_cast<double>(size);

        while (display_size >= 1024.0 && unit_index < 4)
        {
            display_size /= 1024.0;
            unit_index++;
        }

        char buffer[32];
        if (unit_index == 0)
        {
            std::snprintf(buffer, sizeof(buffer), "%lu %s", static_cast<unsigned long>(size),
                          units[unit_index]);
        }
        else
        {
            std::snprintf(buffer, sizeof(buffer), "%.1f %s", display_size, units[unit_index]);
        }

        return std::string(buffer);
    }

private:
    static std::string getPermissionsString(fs::perms permissions)
    {
        std::string result;

        // Owner permissions
        result += (permissions & fs::perms::owner_read) != fs::perms::none ? 'r' : '-';
        result += (permissions & fs::perms::owner_write) != fs::perms::none ? 'w' : '-';
        result += (permissions & fs::perms::owner_exec) != fs::perms::none ? 'x' : '-';

        // Group permissions
        result += (permissions & fs::perms::group_read) != fs::perms::none ? 'r' : '-';
        result += (permissions & fs::perms::group_write) != fs::perms::none ? 'w' : '-';
        result += (permissions & fs::perms::group_exec) != fs::perms::none ? 'x' : '-';

        // Others permissions
        result += (permissions & fs::perms::others_read) != fs::perms::none ? 'r' : '-';
        result += (permissions & fs::perms::others_write) != fs::perms::none ? 'w' : '-';
        result += (permissions & fs::perms::others_exec) != fs::perms::none ? 'x' : '-';

        return result;
    }
};

} // namespace homeshell
