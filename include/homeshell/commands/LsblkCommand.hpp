#pragma once

#include <homeshell/Command.hpp>
#include <homeshell/FilesystemHelper.hpp>
#include <homeshell/Status.hpp>

#include <fmt/color.h>
#include <fmt/core.h>
#include <sys/stat.h>

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace homeshell
{

/**
 * @brief Block device information
 *
 * Contains information about a block device (disk/partition) on the system.
 */
struct BlockDevice
{
    std::string name;       ///< Device name (e.g., "sda", "sda1", "nvme0n1")
    std::string size;       ///< Device size (human-readable, e.g., "500G")
    std::string type;       ///< Device type ("disk", "part", "loop", "rom")
    std::string mountpoint; ///< Mount point if mounted (e.g., "/", "/home")
    std::string fstype;     ///< Filesystem type (e.g., "ext4", "ntfs", "vfat")
    std::string label;      ///< Filesystem label if set
    bool removable;         ///< true if removable media (USB, SD card)
    bool readonly;          ///< true if mounted read-only
};

/**
 * @brief List block devices
 *
 * Displays information about block devices (disks, partitions, etc.) by reading
 * from /sys/block and /sys/class/block on Linux systems.
 *
 * @details The lsblk command provides comprehensive information about:
 *          - Physical disks and partitions
 *          - Device sizes and types
 *          - Mount points and filesystem types
 *          - Removable media detection
 *          - Read-only status
 *          - Color-coded output
 *
 *          Output includes:
 *          - NAME: Device name
 *          - SIZE: Capacity (human-readable)
 *          - TYPE: disk/part/loop/rom
 *          - RO: Read-only flag
 *          - RM: Removable flag
 *          - FSTYPE: Filesystem type
 *          - MOUNTPOINT: Where mounted
 *
 * Example output:
 * @code
 * NAME         SIZE     TYPE     RO   RM   FSTYPE     MOUNTPOINT
 * sda          500G     disk     0    0
 * ├─sda1       512M     part     0    0    vfat       /boot/efi
 * ├─sda2       100G     part     0    0    ext4       /
 * └─sda3       400G     part     0    0    ext4       /home
 * nvme0n1      256G     disk     0    0
 * └─nvme0n1p1  256G     part     0    0    ntfs
 * @endcode
 *
 * Example usage:
 * @code
 * lsblk                       // List all block devices
 * @endcode
 *
 * @note Linux only. On other platforms, reports "not supported".
 */
class LsblkCommand : public ICommand
{
public:
    std::string getName() const override
    {
        return "lsblk";
    }

    std::string getDescription() const override
    {
        return "List block devices";
    }

    CommandType getType() const override
    {
        return CommandType::Synchronous;
    }

    /**
     * @brief Execute the lsblk command
     * @param context Command context (no arguments used)
     * @return Status::ok() on success, Status::error() if not supported
     */
    Status execute(const CommandContext& context) override
    {
#ifdef __linux__
        auto devices = getBlockDevices();

        if (devices.empty())
        {
            fmt::print("No block devices found\n");
            return Status::ok();
        }

        // Print header
        if (context.use_colors)
        {
            fmt::print(fg(fmt::color::yellow) | fmt::emphasis::bold,
                       "{:<12} {:<8} {:<8} {:<4} {:<4} {:<10} {}\n", "NAME", "SIZE", "TYPE", "RO",
                       "RM", "FSTYPE", "MOUNTPOINT");
            fmt::print(fg(fmt::color::yellow), "{}\n", std::string(70, '-'));
        }
        else
        {
            fmt::print("{:<12} {:<8} {:<8} {:<4} {:<4} {:<10} {}\n", "NAME", "SIZE", "TYPE", "RO",
                       "RM", "FSTYPE", "MOUNTPOINT");
            fmt::print("{}\n", std::string(70, '-'));
        }

        // Print devices
        for (const auto& dev : devices)
        {
            if (context.use_colors)
            {
                // Color code by type
                auto color = fmt::color::white;
                if (dev.type == "disk")
                    color = fmt::color::cyan;
                else if (dev.type == "part")
                    color = fmt::color::green;
                else if (dev.type == "loop")
                    color = fmt::color::yellow;

                fmt::print(fg(color), "{:<12}", dev.name);
            }
            else
            {
                fmt::print("{:<12}", dev.name);
            }

            fmt::print(" {:<8} {:<8} {:<4} {:<4} {:<10} {}\n", dev.size, dev.type,
                       dev.readonly ? "1" : "0", dev.removable ? "1" : "0", dev.fstype,
                       dev.mountpoint);
        }

        fmt::print("\n");
        return Status::ok();
#else
        if (context.use_colors)
            fmt::print(fg(fmt::color::red), "Error: lsblk is only supported on Linux\n");
        else
            fmt::print("Error: lsblk is only supported on Linux\n");
        return Status::error("Unsupported platform");
#endif
    }

private:
#ifdef __linux__
    std::vector<BlockDevice> getBlockDevices()
    {
        std::vector<BlockDevice> devices;

        if (!std::filesystem::exists("/sys/class/block"))
        {
            return devices;
        }

        for (const auto& entry : std::filesystem::directory_iterator("/sys/class/block"))
        {
            std::string name = entry.path().filename().string();

            // Skip loop devices without backing files
            if (name.find("loop") == 0)
            {
                std::string backing_file =
                    readSysFile("/sys/class/block/" + name + "/loop/backing_file");
                if (backing_file.empty())
                    continue;
            }

            BlockDevice dev;
            dev.name = name;
            dev.size = formatSize(readSysFile("/sys/class/block/" + name + "/size"));
            dev.removable = (readSysFile("/sys/class/block/" + name + "/removable") == "1");
            dev.readonly = (readSysFile("/sys/class/block/" + name + "/ro") == "1");

            // Determine type
            if (name.find("loop") == 0)
                dev.type = "loop";
            else if (name.find("sr") == 0 || name.find("cdrom") == 0)
                dev.type = "rom";
            else if (std::isdigit(name.back()) ||
                     name.find("nvme") != std::string::npos && name.find("p") != std::string::npos)
                dev.type = "part";
            else
                dev.type = "disk";

            // Get filesystem info
            std::string dev_path = "/dev/" + name;
            if (std::filesystem::exists(dev_path))
            {
                // Try to get mount info from /proc/mounts
                std::ifstream mounts("/proc/mounts");
                std::string line;
                while (std::getline(mounts, line))
                {
                    if (line.find(dev_path) == 0)
                    {
                        std::istringstream iss(line);
                        std::string device, mountpoint, fstype;
                        iss >> device >> mountpoint >> fstype;
                        dev.mountpoint = mountpoint;
                        dev.fstype = fstype;
                        break;
                    }
                }

                // If not mounted, try to detect filesystem type from blkid
                if (dev.fstype.empty())
                {
                    // This would require running blkid command or reading superblock
                    // For now, leave empty
                }
            }

            devices.push_back(dev);
        }

        // Sort devices (disks first, then partitions)
        std::sort(devices.begin(), devices.end(),
                  [](const BlockDevice& a, const BlockDevice& b)
                  {
                      if (a.type != b.type)
                      {
                          if (a.type == "disk")
                              return true;
                          if (b.type == "disk")
                              return false;
                      }
                      return a.name < b.name;
                  });

        return devices;
    }

    std::string readSysFile(const std::string& path)
    {
        std::ifstream file(path);
        if (!file)
            return "";

        std::string content;
        std::getline(file, content);

        // Trim whitespace
        content.erase(0, content.find_first_not_of(" \t\r\n"));
        content.erase(content.find_last_not_of(" \t\r\n") + 1);

        return content;
    }

    std::string formatSize(const std::string& sectors_str)
    {
        if (sectors_str.empty())
            return "0B";

        try
        {
            uint64_t sectors = std::stoull(sectors_str);
            uint64_t bytes = sectors * 512; // Most devices use 512-byte sectors

            return FilesystemHelper::formatSize(bytes);
        }
        catch (...)
        {
            return "0B";
        }
    }
#endif
};

} // namespace homeshell
