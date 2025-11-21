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

struct UsbDevice
{
    std::string bus;
    std::string device;
    std::string vendor_id;
    std::string product_id;
    std::string vendor_name;
    std::string product_name;
    std::string speed;
};

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
