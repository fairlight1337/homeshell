#pragma once

#include <homeshell/Command.hpp>
#include <homeshell/Status.hpp>

#include <fmt/color.h>
#include <fmt/core.h>

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace homeshell
{

/**
 * @brief USB device information
 *
 * Contains all relevant information about a USB device detected on the system.
 */
struct UsbDevice
{
    std::string bus;          ///< USB bus number
    std::string device;       ///< Device number on bus
    std::string vendor_id;    ///< Vendor ID (hex)
    std::string product_id;   ///< Product ID (hex)
    std::string vendor_name;  ///< Human-readable vendor name
    std::string product_name; ///< Human-readable product name
    std::string speed;        ///< Connection speed (e.g., "480M", "5000M")
};

/**
 * @brief List USB devices
 *
 * Displays information about USB buses and connected devices by reading
 * from /sys/bus/usb/devices on Linux systems.
 *
 * @details The lsusb command provides:
 *          - USB bus and device numbers
 *          - Vendor and product IDs (hex format)
 *          - Device names (vendor and product)
 *          - Connection speed
 *          - Color-coded output for better readability
 *
 *          Command syntax:
 *          @code
 *          lsusb
 *          @endcode
 *
 *          Output format:
 *          @code
 *          Bus 001 Device 002: ID 8087:0024 Intel Corp. Integrated Rate Matching Hub
 *          Bus 001 Device 001: ID 1d6b:0002 Linux Foundation 2.0 root hub
 *          Bus 002 Device 003: ID 046d:c52b Logitech, Inc. Unifying Receiver
 *          @endcode
 *
 *          Information is gathered from sysfs:
 *          - busnum (bus number)
 *          - devnum (device number)
 *          - idVendor (vendor ID)
 *          - idProduct (product ID)
 *          - manufacturer (vendor name)
 *          - product (product name)
 *          - speed (connection speed)
 *
 * Example usage:
 * @code
 * lsusb                       // List all USB devices
 * @endcode
 *
 * @note Linux only. On other platforms, reports "not supported".
 */
class LsusbCommand : public ICommand
{
public:
    std::string getName() const override
    {
        return "lsusb";
    }

    std::string getDescription() const override
    {
        return "List USB devices";
    }

    CommandType getType() const override
    {
        return CommandType::Synchronous;
    }

    /**
     * @brief Execute the lsusb command
     * @param context Command context (no arguments used)
     * @return Status::ok() on success, Status::error() if not supported
     */
    Status execute(const CommandContext& context) override
    {
#ifdef __linux__
        auto devices = getUsbDevices();

        if (devices.empty())
        {
            fmt::print("No USB devices found\n");
            return Status::ok();
        }

        // Print header
        if (context.use_colors)
        {
            fmt::print(fg(fmt::color::yellow) | fmt::emphasis::bold, "{:<8} {:<8} {:<10} {}\n",
                       "Bus", "Device", "ID", "Description");
            fmt::print(fg(fmt::color::yellow), "{}\n", std::string(70, '-'));
        }
        else
        {
            fmt::print("{:<8} {:<8} {:<10} {}\n", "Bus", "Device", "ID", "Description");
            fmt::print("{}\n", std::string(70, '-'));
        }

        // Print devices
        for (const auto& dev : devices)
        {
            fmt::print("Bus {:>3} Device {:>3}: {}:{} ", dev.bus, dev.device, dev.vendor_id,
                       dev.product_id);

            if (!dev.vendor_name.empty() || !dev.product_name.empty())
            {
                if (context.use_colors)
                    fmt::print(fg(fmt::color::cyan), "{}", dev.vendor_name);
                else
                    fmt::print("{}", dev.vendor_name);

                if (!dev.vendor_name.empty() && !dev.product_name.empty())
                {
                    fmt::print(" ");
                }
                fmt::print("{}", dev.product_name);
            }

            if (!dev.speed.empty())
            {
                if (context.use_colors)
                    fmt::print(fg(fmt::color::green), " [{}]", dev.speed);
                else
                    fmt::print(" [{}]", dev.speed);
            }

            fmt::print("\n");
        }

        fmt::print("\n{} device(s) found\n", devices.size());
        return Status::ok();
#else
        if (context.use_colors)
            fmt::print(fg(fmt::color::red), "Error: lsusb is only supported on Linux\n");
        else
            fmt::print("Error: lsusb is only supported on Linux\n");
        return Status::error("Unsupported platform");
#endif
    }

private:
#ifdef __linux__
    std::vector<UsbDevice> getUsbDevices()
    {
        std::vector<UsbDevice> devices;
        std::error_code ec;

        if (!std::filesystem::exists("/sys/bus/usb/devices", ec))
        {
            return devices;
        }

        for (const auto& entry : std::filesystem::directory_iterator("/sys/bus/usb/devices", ec))
        {
            if (!entry.is_directory())
                continue;

            std::string path = entry.path().string();

            // Skip root hubs and interfaces
            if (path.find(":") != std::string::npos)
                continue;

            UsbDevice dev = readDeviceInfo(path);
            if (!dev.vendor_id.empty() && !dev.product_id.empty())
            {
                devices.push_back(dev);
            }
        }

        // Sort by bus and device number
        std::sort(devices.begin(), devices.end(),
                  [](const UsbDevice& a, const UsbDevice& b)
                  {
                      if (a.bus != b.bus)
                          return std::stoi(a.bus) < std::stoi(b.bus);
                      return std::stoi(a.device) < std::stoi(b.device);
                  });

        return devices;
    }

    UsbDevice readDeviceInfo(const std::string& device_path)
    {
        UsbDevice dev;

        dev.bus = readSysFile(device_path + "/busnum");
        dev.device = readSysFile(device_path + "/devnum");
        dev.vendor_id = readSysFile(device_path + "/idVendor");
        dev.product_id = readSysFile(device_path + "/idProduct");
        dev.vendor_name = readSysFile(device_path + "/manufacturer");
        dev.product_name = readSysFile(device_path + "/product");
        dev.speed = readSysFile(device_path + "/speed");

        return dev;
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
