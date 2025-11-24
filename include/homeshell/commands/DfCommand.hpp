/**
 * @file DfCommand.hpp
 * @brief Display disk filesystem usage
 *
 * This command displays information about filesystem disk space usage including
 * mount points, total size, used space, available space, and usage percentages.
 * Information is read from /proc/mounts and statvfs() system calls.
 *
 * @author Homeshell Development Team
 * @date 2025
 */

#pragma once

#include <homeshell/Command.hpp>
#include <homeshell/Status.hpp>
#include <homeshell/VirtualFilesystem.hpp>

#include <sys/stat.h>
#include <sys/statvfs.h>

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace homeshell
{

/**
 * @class DfCommand
 * @brief Command to display disk filesystem usage
 *
 * Displays information about:
 * - Filesystem device/source
 * - Total size
 * - Used space
 * - Available space
 * - Usage percentage
 * - Mount point
 *
 * Information is read from /proc/mounts and statvfs() system calls
 *
 * @section Usage
 * @code
 * df                    # Show all filesystems
 * df -h                 # Show with human-readable sizes
 * df -a                 # Show all filesystems including pseudo-filesystems
 * df /home              # Show specific mount point
 * df --help             # Show help message
 * @endcode
 */
class DfCommand : public ICommand
{
public:
    /**
     * @brief Get command name
     * @return Command name "df"
     */
    std::string getName() const override
    {
        return "df";
    }

    /**
     * @brief Get command description
     * @return Brief description of the command
     */
    std::string getDescription() const override
    {
        return "Display disk filesystem usage";
    }

    /**
     * @brief Get command type
     * @return CommandType::Synchronous
     */
    CommandType getType() const override
    {
        return CommandType::Synchronous;
    }

    /**
     * @brief Execute the df command
     * @param context Command context with arguments
     * @return Status indicating success or failure
     */
    Status execute(const CommandContext& context) override
    {
        bool human_readable = false;
        bool show_all = false;
        std::vector<std::string> target_paths;

        // Parse arguments
        for (size_t i = 0; i < context.args.size(); ++i)
        {
            const std::string& arg = context.args[i];

            if (arg == "--help")
            {
                showHelp();
                return Status::ok();
            }
            else if (arg == "--human-readable")
            {
                human_readable = true;
            }
            else if (arg == "-a" || arg == "--all")
            {
                show_all = true;
            }
            else if (arg[0] == '-' && arg.length() > 1 && arg[1] != '-')
            {
                // Check for combined flags like -ha or single flags like -h
                for (size_t j = 1; j < arg.length(); ++j)
                {
                    if (arg[j] == 'h')
                    {
                        human_readable = true;
                    }
                    else if (arg[j] == 'a')
                    {
                        show_all = true;
                    }
                    else
                    {
                        return Status::error("Unknown option: -" + std::string(1, arg[j]) +
                                             "\nUse --help for usage information");
                    }
                }
            }
            else if (arg[0] == '-')
            {
                return Status::error("Unknown option: " + arg +
                                     "\nUse --help for usage information");
            }
            else
            {
                // It's a path argument
                target_paths.push_back(arg);
            }
        }

        // Parse mount information
        auto mounts = parseMounts();

        // Filter mounts if specific paths requested
        if (!target_paths.empty())
        {
            std::vector<MountInfo> filtered;
            for (const auto& path : target_paths)
            {
                // Check if path exists first
                struct stat st;
                if (stat(path.c_str(), &st) != 0)
                {
                    std::cerr << "df: '" << path << "': No such file or directory\n";
                    continue;
                }

                // Find mount point for this path
                bool found = false;
                std::string longest_match;
                size_t longest_len = 0;

                for (const auto& mount : mounts)
                {
                    // Check if path is within this mount point
                    size_t mp_len = mount.mount_point.length();
                    if (path == mount.mount_point ||
                        (path.length() > mp_len && path.substr(0, mp_len) == mount.mount_point &&
                         (path[mp_len] == '/' || mount.mount_point == "/")))
                    {
                        // Found a match - keep track of longest match
                        if (mp_len > longest_len)
                        {
                            longest_len = mp_len;
                            found = true;
                        }
                    }
                }

                // Add mounts that match this path
                if (found)
                {
                    for (const auto& mount : mounts)
                    {
                        if (mount.mount_point.length() == longest_len &&
                            (path == mount.mount_point ||
                             (path.length() >= longest_len &&
                              path.substr(0, longest_len) == mount.mount_point)))
                        {
                            // Avoid duplicates
                            bool already_added = false;
                            for (const auto& f : filtered)
                            {
                                if (f.mount_point == mount.mount_point)
                                {
                                    already_added = true;
                                    break;
                                }
                            }
                            if (!already_added)
                            {
                                filtered.push_back(mount);
                            }
                        }
                    }
                }
            }
            mounts = filtered;
        }

        // Filter out pseudo-filesystems unless -a specified
        if (!show_all)
        {
            mounts.erase(std::remove_if(mounts.begin(), mounts.end(),
                                        [](const MountInfo& m) { return isPseudoFilesystem(m); }),
                         mounts.end());
        }

        // Display filesystem information
        displayFilesystems(mounts, human_readable);

        // Also show homeshell virtual filesystems
        displayVirtualFilesystems(human_readable);

        return Status::ok();
    }

private:
    /**
     * @brief Structure to hold mount information
     */
    struct MountInfo
    {
        std::string device;
        std::string mount_point;
        std::string fs_type;
        uint64_t total_size;
        uint64_t used_size;
        uint64_t available_size;
        double usage_percent;
    };

    /**
     * @brief Display help information
     */
    void showHelp() const
    {
        std::cout << "Usage: df [OPTION]... [FILE]...\n\n";
        std::cout << "Show information about the filesystem on which each FILE resides,\n";
        std::cout << "or all filesystems by default.\n\n";
        std::cout << "Options:\n";
        std::cout << "  -a, --all            Include pseudo-filesystems\n";
        std::cout << "  -h, --human-readable Print sizes in human readable format (e.g., 1K 234M "
                     "2G)\n";
        std::cout << "  --help               Show this help message\n\n";
        std::cout << "Examples:\n";
        std::cout << "  df                   # Show all filesystems\n";
        std::cout << "  df -h                # Human-readable sizes\n";
        std::cout << "  df -a                # Include all filesystems\n";
        std::cout << "  df /home             # Show filesystem for /home\n";
    }

    /**
     * @brief Parse mount information from /proc/mounts
     * @return Vector of mount information
     */
    std::vector<MountInfo> parseMounts() const
    {
        std::vector<MountInfo> mounts;
        std::ifstream file("/proc/mounts");
        if (!file.is_open())
            return mounts;

        std::string line;
        std::set<std::string> seen_devices;

        while (std::getline(file, line))
        {
            std::istringstream iss(line);
            std::string device, mount_point, fs_type;

            if (!(iss >> device >> mount_point >> fs_type))
                continue;

            // Decode escaped characters in mount point
            mount_point = decodeOctalEscapes(mount_point);

            // Skip duplicate devices (same device mounted multiple times)
            if (seen_devices.find(device + mount_point) != seen_devices.end())
                continue;
            seen_devices.insert(device + mount_point);

            // Get filesystem statistics
            struct statvfs stat;
            if (statvfs(mount_point.c_str(), &stat) != 0)
                continue;

            MountInfo info;
            info.device = device;
            info.mount_point = mount_point;
            info.fs_type = fs_type;

            // Calculate sizes (statvfs returns in blocks)
            uint64_t block_size = stat.f_frsize;
            info.total_size = stat.f_blocks * block_size;
            info.available_size = stat.f_bavail * block_size;
            info.used_size = (stat.f_blocks - stat.f_bfree) * block_size;

            // Calculate usage percentage
            if (info.total_size > 0)
            {
                info.usage_percent = (info.used_size * 100.0) / info.total_size;
            }
            else
            {
                info.usage_percent = 0.0;
            }

            mounts.push_back(info);
        }

        return mounts;
    }

    /**
     * @brief Check if a filesystem is a pseudo-filesystem
     * @param mount Mount information
     * @return true if pseudo-filesystem, false otherwise
     */
    static bool isPseudoFilesystem(const MountInfo& mount)
    {
        // Pseudo-filesystems to skip by default
        static const std::set<std::string> pseudo_fs = {
            "proc",    "sysfs",    "devpts",     "tmpfs",    "cgroup",     "cgroup2",
            "debugfs", "devtmpfs", "securityfs", "pstore",   "bpf",        "tracefs",
            "fusectl", "mqueue",   "hugetlbfs",  "configfs", "binfmt_misc"};

        if (pseudo_fs.find(mount.fs_type) != pseudo_fs.end())
            return true;

        // Also skip if device doesn't start with / (virtual devices)
        if (!mount.device.empty() && mount.device[0] != '/')
            return true;

        // Skip if total size is 0 (some virtual filesystems)
        if (mount.total_size == 0)
            return true;

        return false;
    }

    /**
     * @brief Decode octal escape sequences in mount point paths
     * @param str String with escape sequences
     * @return Decoded string
     */
    static std::string decodeOctalEscapes(const std::string& str)
    {
        std::string result;
        for (size_t i = 0; i < str.length(); ++i)
        {
            if (str[i] == '\\' && i + 3 < str.length() && isdigit(str[i + 1]) &&
                isdigit(str[i + 2]) && isdigit(str[i + 3]))
            {
                // Octal escape sequence
                int code = (str[i + 1] - '0') * 64 + (str[i + 2] - '0') * 8 + (str[i + 3] - '0');
                result += static_cast<char>(code);
                i += 3;
            }
            else
            {
                result += str[i];
            }
        }
        return result;
    }

    /**
     * @brief Format size in human-readable format
     * @param size Size in bytes
     * @return Formatted string
     */
    static std::string formatSize(uint64_t size)
    {
        const char* units[] = {"B", "K", "M", "G", "T", "P"};
        int unit_idx = 0;
        double dsize = static_cast<double>(size);

        while (dsize >= 1024.0 && unit_idx < 5)
        {
            dsize /= 1024.0;
            unit_idx++;
        }

        std::ostringstream oss;
        if (unit_idx == 0)
        {
            oss << size << units[unit_idx];
        }
        else
        {
            oss << std::fixed << std::setprecision(1) << dsize << units[unit_idx];
        }
        return oss.str();
    }

    /**
     * @brief Display filesystem information
     * @param mounts Vector of mount information
     * @param human_readable Use human-readable sizes
     */
    void displayFilesystems(const std::vector<MountInfo>& mounts, bool human_readable) const
    {
        if (mounts.empty())
        {
            std::cout << "No filesystems to display.\n";
            return;
        }

        // Print header
        std::cout << std::left;
        std::cout << std::setw(25) << "Filesystem";
        if (human_readable)
        {
            std::cout << std::setw(8) << "Size" << std::setw(8) << "Used" << std::setw(8)
                      << "Avail";
        }
        else
        {
            std::cout << std::setw(15) << "1K-blocks" << std::setw(15) << "Used" << std::setw(15)
                      << "Available";
        }
        std::cout << std::setw(8) << "Use%" << std::setw(30) << "Mounted on"
                  << "\n";

        // Print each mount
        for (const auto& mount : mounts)
        {
            std::cout << std::setw(25) << mount.device.substr(0, 24);

            if (human_readable)
            {
                std::cout << std::setw(8) << formatSize(mount.total_size) << std::setw(8)
                          << formatSize(mount.used_size) << std::setw(8)
                          << formatSize(mount.available_size);
            }
            else
            {
                uint64_t total_kb = mount.total_size / 1024;
                uint64_t used_kb = mount.used_size / 1024;
                uint64_t avail_kb = mount.available_size / 1024;

                std::cout << std::setw(15) << total_kb << std::setw(15) << used_kb << std::setw(15)
                          << avail_kb;
            }

            std::cout << std::setw(7) << std::fixed << std::setprecision(0) << mount.usage_percent
                      << "%" << std::setw(30) << mount.mount_point << "\n";
        }
    }

    /**
     * @brief Display homeshell virtual filesystems
     * @param human_readable Use human-readable sizes
     */
    void displayVirtualFilesystems(bool human_readable) const
    {
        auto& vfs = VirtualFilesystem::getInstance();
        auto mount_names = vfs.getMountNames();

        if (mount_names.empty())
            return;

        std::cout << "\nHomeshell Virtual Filesystems:\n";
        std::cout << std::left;
        std::cout << std::setw(25) << "Filesystem";
        if (human_readable)
        {
            std::cout << std::setw(8) << "Size" << std::setw(8) << "Used" << std::setw(8)
                      << "Avail";
        }
        else
        {
            std::cout << std::setw(15) << "1K-blocks" << std::setw(15) << "Used" << std::setw(15)
                      << "Available";
        }
        std::cout << std::setw(8) << "Use%" << std::setw(30) << "Mounted on"
                  << "\n";

        for (const auto& name : mount_names)
        {
            auto* mount = vfs.getMount(name);
            if (!mount)
                continue;

            uint64_t total_size = mount->getMaxSpace();
            uint64_t used_size = mount->getUsedSpace();
            uint64_t avail_size = total_size > used_size ? total_size - used_size : 0;

            std::cout << std::setw(25) << name.substr(0, 24);

            if (human_readable)
            {
                std::cout << std::setw(8) << formatSize(total_size) << std::setw(8)
                          << formatSize(used_size) << std::setw(8) << formatSize(avail_size);
            }
            else
            {
                uint64_t total_kb = total_size / 1024;
                uint64_t used_kb = used_size / 1024;
                uint64_t avail_kb = avail_size / 1024;

                std::cout << std::setw(15) << total_kb << std::setw(15) << used_kb << std::setw(15)
                          << avail_kb;
            }

            double usage = total_size > 0 ? (used_size * 100.0) / total_size : 0.0;
            std::cout << std::setw(7) << std::fixed << std::setprecision(0) << usage << "%"
                      << std::setw(30) << mount->getMountPoint() << "\n";
        }
    }
};

} // namespace homeshell
