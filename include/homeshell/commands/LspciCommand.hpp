#pragma once

#include <homeshell/Command.hpp>
#include <homeshell/Status.hpp>

#include <fmt/color.h>
#include <fmt/core.h>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace homeshell
{

/**
 * @brief PCI device information
 *
 * Contains information about a PCI device detected on the system.
 */
struct PciDevice
{
    std::string slot;       ///< PCI slot address (e.g., "00:1f.2")
    std::string class_name; ///< Device class (e.g., "SATA controller")
    std::string vendor;     ///< Vendor name
    std::string device;     ///< Device name/model
    std::string subsystem;  ///< Subsystem information
};

/**
 * @brief List PCI devices
 *
 * Displays information about PCI buses and connected devices by reading
 * from /sys/bus/pci/devices on Linux systems.
 *
 * @details The lspci command provides:
 *          - PCI slot addresses
 *          - Device class/type
 *          - Vendor and device names
 *          - Subsystem information
 *          - Color-coded output for better readability
 *
 *          Information is gathered from sysfs:
 *          - class (device class code)
 *          - vendor (vendor name)
 *          - device (device name)
 *          - subsystem_vendor, subsystem_device
 *
 * Example output:
 * @code
 * 00:00.0 Host bridge: Intel Corporation Device
 * 00:1f.2 SATA controller: Intel Corporation 8 Series/C220
 * 02:00.0 Network controller: Intel Corporation Wireless
 * @endcode
 *
 * Example usage:
 * @code
 * lspci                       // List all PCI devices
 * @endcode
 *
 * @note Linux only. On other platforms, reports "not supported".
 */
class LspciCommand : public ICommand
{
public:
    std::string getName() const override
    {
        return "lspci";
    }

    std::string getDescription() const override
    {
        return "List PCI devices";
    }

    CommandType getType() const override
    {
        return CommandType::Synchronous;
    }

    /**
     * @brief Execute the lspci command
     * @param context Command context (no arguments used)
     * @return Status::ok() on success, Status::error() if not supported
     */
    Status execute(const CommandContext& context) override
    {
#ifdef __linux__
        auto devices = getPciDevices();

        if (devices.empty())
        {
            fmt::print("No PCI devices found\n");
            return Status::ok();
        }

        // Print devices
        for (const auto& dev : devices)
        {
            if (context.use_colors)
            {
                fmt::print(fg(fmt::color::yellow), "{:<12}", dev.slot);
                fmt::print(fg(fmt::color::cyan), " {:<20}", dev.class_name);
            }
            else
            {
                fmt::print("{:<12}", dev.slot);
                fmt::print(" {:<20}", dev.class_name);
            }
            fmt::print(": ");
            fmt::print("{}", dev.vendor);
            if (!dev.device.empty())
            {
                fmt::print(" {}", dev.device);
            }
            fmt::print("\n");
        }

        fmt::print("\n{} device(s) found\n", devices.size());
        return Status::ok();
#else
        if (context.use_colors)
            fmt::print(fg(fmt::color::red), "Error: lspci is only supported on Linux\n");
        else
            fmt::print("Error: lspci is only supported on Linux\n");
        return Status::error("Unsupported platform");
#endif
    }

private:
#ifdef __linux__
    std::vector<PciDevice> getPciDevices()
    {
        std::vector<PciDevice> devices;
        std::error_code ec;

        if (!std::filesystem::exists("/sys/bus/pci/devices", ec))
        {
            return devices;
        }

        for (const auto& entry : std::filesystem::directory_iterator("/sys/bus/pci/devices", ec))
        {
            if (!entry.is_symlink() && !entry.is_directory())
                continue;

            std::string slot = entry.path().filename().string();
            PciDevice dev = readDeviceInfo(entry.path().string(), slot);

            if (!dev.slot.empty())
            {
                devices.push_back(dev);
            }
        }

        // Sort by slot
        std::sort(devices.begin(), devices.end(),
                  [](const PciDevice& a, const PciDevice& b) { return a.slot < b.slot; });

        return devices;
    }

    PciDevice readDeviceInfo(const std::string& device_path, const std::string& slot)
    {
        PciDevice dev;
        dev.slot = slot;

        // Read class
        std::string class_id = readSysFile(device_path + "/class");
        dev.class_name = getClassName(class_id);

        // Read vendor and device
        dev.vendor = readSysFile(device_path + "/vendor");
        dev.device = readSysFile(device_path + "/device");

        // Try to get human-readable names
        std::string vendor_name = readSysFile(device_path + "/vendor_name");
        if (!vendor_name.empty())
        {
            dev.vendor = vendor_name;
        }

        std::string device_name = readSysFile(device_path + "/device_name");
        if (!device_name.empty())
        {
            dev.device = device_name;
        }

        return dev;
    }

    std::string getClassName(const std::string& class_id)
    {
        if (class_id.empty() || class_id.length() < 6)
            return "Unknown";

        // Extract class code (first 2 hex digits after 0x)
        std::string class_code = class_id.substr(2, 2);

        // Map common PCI class codes to names
        static const std::map<std::string, std::string> class_map = {
            {"00", "Unclassified"},
            {"01", "Mass storage"},
            {"02", "Network controller"},
            {"03", "Display controller"},
            {"04", "Multimedia controller"},
            {"05", "Memory controller"},
            {"06", "Bridge"},
            {"07", "Communication controller"},
            {"08", "Generic system peripheral"},
            {"09", "Input device controller"},
            {"0a", "Docking station"},
            {"0b", "Processor"},
            {"0c", "Serial bus controller"},
            {"0d", "Wireless controller"},
            {"0e", "Intelligent controller"},
            {"0f", "Satellite controller"},
            {"10", "Encryption controller"},
            {"11", "Signal processing controller"},
            {"12", "Processing accelerator"},
            { "13",
              "Non-Essential Instrumentation" }};

        auto it = class_map.find(class_code);
        if (it != class_map.end())
        {
            return it->second;
        }

        return "Class " + class_id;
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
