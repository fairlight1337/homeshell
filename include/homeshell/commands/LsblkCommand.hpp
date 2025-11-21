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

struct BlockDevice
{
    std::string name;
    std::string size;
    std::string type;
    std::string mountpoint;
    std::string fstype;
    std::string label;
    bool removable;
    bool readonly;
};

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
            fmt::print("{:<12} ", dev.name);
            fmt::print("{:<8} ", dev.size);

            // Color code device types
            if (context.use_colors)
            {
                auto type_color = fmt::color::white;
                if (dev.type == "disk")
                    type_color = fmt::color::cyan;
                else if (dev.type == "part")
                    type_color = fmt::color::green;
                else if (dev.type == "rom")
                    type_color = fmt::color::yellow;
                fmt::print(fg(type_color), "{:<8} ", dev.type);
            }
            else
            {
                fmt::print("{:<8} ", dev.type);
            }

            fmt::print("{:<4} ", dev.readonly ? "1" : "0");
            fmt::print("{:<4} ", dev.removable ? "1" : "0");
            fmt::print("{:<10} ", dev.fstype);

            if (!dev.mountpoint.empty())
            {
                if (context.use_colors)
                    fmt::print(fg(fmt::color::blue), "{}", dev.mountpoint);
                else
                    fmt::print("{}", dev.mountpoint);
            }

            fmt::print("\n");
        }

        fmt::print("\n{} device(s) found\n", devices.size());
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
        std::error_code ec;

        if (!std::filesystem::exists("/sys/block", ec))
        {
            return devices;
        }

        // Enumerate block devices
        for (const auto& entry : std::filesystem::directory_iterator("/sys/block", ec))
        {
            if (!entry.is_directory() && !entry.is_symlink())
                continue;

            std::string name = entry.path().filename().string();

            // Skip loop devices without backing file
            if (name.find("loop") == 0)
            {
                std::string backing_file =
                    readSysFile(entry.path().string() + "/loop/backing_file");
                if (backing_file.empty())
                    continue;
            }

            BlockDevice dev = readBlockDevice(name, entry.path().string());
            devices.push_back(dev);

            // Check for partitions
            auto partitions = getPartitions(name, entry.path().string());
            devices.insert(devices.end(), partitions.begin(), partitions.end());
        }

        // Sort by name
        std::sort(devices.begin(), devices.end(),
                  [](const BlockDevice& a, const BlockDevice& b) { return a.name < b.name; });

        return devices;
    }

    std::vector<BlockDevice> getPartitions(const std::string& parent,
                                           const std::string& parent_path)
    {
        std::vector<BlockDevice> partitions;
        std::error_code ec;

        for (const auto& entry : std::filesystem::directory_iterator(parent_path, ec))
        {
            std::string name = entry.path().filename().string();

            // Check if it's a partition (starts with parent name)
            if (name.find(parent) != 0 || name == parent)
                continue;

            if (entry.is_directory() || entry.is_symlink())
            {
                BlockDevice part = readBlockDevice(name, entry.path().string());
                part.type = "part";
                part.name = "├─" + name;
                partitions.push_back(part);
            }
        }

        return partitions;
    }

    BlockDevice readBlockDevice(const std::string& name, const std::string& device_path)
    {
        BlockDevice dev;
        dev.name = name;

        // Read size
        std::string size_str = readSysFile(device_path + "/size");
        if (!size_str.empty())
        {
            try
            {
                uint64_t sectors = std::stoull(size_str);
                uint64_t bytes = sectors * 512; // Assume 512-byte sectors
                dev.size = FilesystemHelper::formatSize(bytes);
            }
            catch (...)
            {
                dev.size = "0 B";
            }
        }

        // Read device type
        dev.type = "disk";

        // Check if removable
        std::string removable = readSysFile(device_path + "/removable");
        dev.removable = (removable == "1");

        // Check if readonly
        std::string ro = readSysFile(device_path + "/ro");
        dev.readonly = (ro == "1");

        // Try to get filesystem type and mountpoint from /proc/mounts
        getMountInfo(name, dev.mountpoint, dev.fstype);

        return dev;
    }

    void getMountInfo(const std::string& device_name, std::string& mountpoint, std::string& fstype)
    {
        std::ifstream mounts("/proc/mounts");
        if (!mounts)
            return;

        std::string line;
        while (std::getline(mounts, line))
        {
            std::istringstream iss(line);
            std::string dev, mp, fs;

            if (iss >> dev >> mp >> fs)
            {
                // Check if this mount is for our device
                if (dev.find("/dev/" + device_name) != std::string::npos)
                {
                    mountpoint = mp;
                    fstype = fs;
                    return;
                }
            }
        }
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
#endif
};

} // namespace homeshell
