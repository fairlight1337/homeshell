#pragma once

#include <homeshell/Command.hpp>
#include <homeshell/FilesystemHelper.hpp>
#include <homeshell/Status.hpp>

#include <fmt/color.h>
#include <fmt/core.h>

#include <ctime>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#ifdef __linux__
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <sys/statvfs.h>
#include <sys/sysinfo.h>
#include <sys/utsname.h>
#endif

namespace homeshell
{

class SysinfoCommand : public ICommand
{
public:
    std::string getName() const override
    {
        return "sysinfo";
    }

    std::string getDescription() const override
    {
        return "Show comprehensive system information";
    }

    CommandType getType() const override
    {
        return CommandType::Synchronous;
    }

    Status execute(const CommandContext& context) override
    {
#ifdef __linux__
        use_colors_ = context.use_colors;

        printHeader("SYSTEM INFORMATION");

        printOSInfo();
        printCPUInfo();
        printMemoryInfo();
        printLoadInfo();
        printGPUInfo();
        printDiskInfo();
        printNetworkInfo();
        printDisplayInfo();

        return Status::ok();
#else
        if (context.use_colors)
            fmt::print(fg(fmt::color::red), "Error: sysinfo is only supported on Linux\n");
        else
            fmt::print("Error: sysinfo is only supported on Linux\n");
        return Status::error("Unsupported platform");
#endif
    }

private:
#ifdef __linux__
    bool use_colors_ = true;

    void printHeader(const std::string& title)
    {
        fmt::print("\n");
        if (use_colors_)
        {
            fmt::print(fg(fmt::color::yellow) | fmt::emphasis::bold, "{}\n", title);
            fmt::print(fg(fmt::color::yellow), "{}\n\n", std::string(title.length(), '='));
        }
        else
        {
            fmt::print("{}\n", title);
            fmt::print("{}\n\n", std::string(title.length(), '='));
        }
    }

    void printSection(const std::string& title)
    {
        if (use_colors_)
        {
            fmt::print(fg(fmt::color::cyan) | fmt::emphasis::bold, "\n{}:\n", title);
            fmt::print(fg(fmt::color::cyan), "{}\n", std::string(title.length() + 1, '-'));
        }
        else
        {
            fmt::print("\n{}:\n", title);
            fmt::print("{}\n", std::string(title.length() + 1, '-'));
        }
    }

    void printField(const std::string& name, const std::string& value,
                    fmt::color color = fmt::color::white)
    {
        fmt::print("  {:<20}: ", name);
        if (use_colors_)
            fmt::print(fg(color), "{}\n", value);
        else
            fmt::print("{}\n", value);
    }

    void printOSInfo()
    {
        printSection("Operating System");

        struct utsname uts;
        if (uname(&uts) == 0)
        {
            printField("OS", uts.sysname);
            printField("Hostname", uts.nodename);
            printField("Kernel", uts.release);
            printField("Architecture", uts.machine);
        }

        // Try to read OS release info
        std::ifstream os_release("/etc/os-release");
        if (os_release)
        {
            std::string line;
            std::string pretty_name;
            while (std::getline(os_release, line))
            {
                if (line.find("PRETTY_NAME=") == 0)
                {
                    pretty_name = line.substr(13);
                    // Remove quotes
                    if (!pretty_name.empty() && pretty_name.front() == '"')
                        pretty_name = pretty_name.substr(1, pretty_name.length() - 2);
                    printField("Distribution", pretty_name);
                    break;
                }
            }
        }

        // Uptime
        struct sysinfo si;
        if (sysinfo(&si) == 0)
        {
            long uptime = si.uptime;
            int days = uptime / 86400;
            int hours = (uptime % 86400) / 3600;
            int minutes = (uptime % 3600) / 60;

            std::string uptime_str;
            if (days > 0)
                uptime_str = fmt::format("{} days, {} hours, {} minutes", days, hours, minutes);
            else if (hours > 0)
                uptime_str = fmt::format("{} hours, {} minutes", hours, minutes);
            else
                uptime_str = fmt::format("{} minutes", minutes);

            printField("Uptime", uptime_str, fmt::color::green);
        }
    }

    void printCPUInfo()
    {
        printSection("CPU");

        std::ifstream cpuinfo("/proc/cpuinfo");
        if (!cpuinfo)
            return;

        std::string line;
        std::string model_name;
        int cpu_count = 0;
        double cpu_mhz = 0.0;
        int cores = 0;

        while (std::getline(cpuinfo, line))
        {
            if (line.find("model name") != std::string::npos)
            {
                auto pos = line.find(':');
                if (pos != std::string::npos)
                {
                    model_name = line.substr(pos + 2);
                }
            }
            else if (line.find("processor") != std::string::npos)
            {
                cpu_count++;
            }
            else if (line.find("cpu MHz") != std::string::npos)
            {
                auto pos = line.find(':');
                if (pos != std::string::npos)
                {
                    try
                    {
                        cpu_mhz = std::stod(line.substr(pos + 2));
                    }
                    catch (...)
                    {
                    }
                }
            }
            else if (line.find("cpu cores") != std::string::npos)
            {
                auto pos = line.find(':');
                if (pos != std::string::npos)
                {
                    try
                    {
                        cores = std::stoi(line.substr(pos + 2));
                    }
                    catch (...)
                    {
                    }
                }
            }
        }

        if (!model_name.empty())
        {
            printField("Model", model_name);
        }

        if (cpu_count > 0)
        {
            printField("Processors", std::to_string(cpu_count));
        }

        if (cores > 0)
        {
            printField("Cores per CPU", std::to_string(cores));
        }

        if (cpu_mhz > 0)
        {
            printField("Current Speed", fmt::format("{:.2f} MHz", cpu_mhz));
        }
    }

    void printMemoryInfo()
    {
        printSection("Memory");

        struct sysinfo si;
        if (sysinfo(&si) == 0)
        {
            uint64_t total_ram = si.totalram * si.mem_unit;
            uint64_t free_ram = si.freeram * si.mem_unit;
            uint64_t used_ram = total_ram - free_ram;
            uint64_t total_swap = si.totalswap * si.mem_unit;
            uint64_t free_swap = si.freeswap * si.mem_unit;

            double used_percent = (double)used_ram / total_ram * 100.0;

            printField("Total RAM", FilesystemHelper::formatSize(total_ram));
            printField("Used RAM",
                       FilesystemHelper::formatSize(used_ram) +
                           fmt::format(" ({:.1f}%)", used_percent),
                       used_percent > 90   ? fmt::color::red
                       : used_percent > 70 ? fmt::color::yellow
                                           : fmt::color::green);
            printField("Free RAM", FilesystemHelper::formatSize(free_ram));

            if (total_swap > 0)
            {
                uint64_t used_swap = total_swap - free_swap;
                double swap_percent = (double)used_swap / total_swap * 100.0;
                printField("Total Swap", FilesystemHelper::formatSize(total_swap));
                printField("Used Swap", FilesystemHelper::formatSize(used_swap) +
                                            fmt::format(" ({:.1f}%)", swap_percent));
            }
        }
    }

    void printLoadInfo()
    {
        printSection("System Load");

        std::ifstream loadavg("/proc/loadavg");
        if (loadavg)
        {
            double load1, load5, load15;
            if (loadavg >> load1 >> load5 >> load15)
            {
                printField("Load Average (1m)", fmt::format("{:.2f}", load1));
                printField("Load Average (5m)", fmt::format("{:.2f}", load5));
                printField("Load Average (15m)", fmt::format("{:.2f}", load15));
            }
        }
    }

    void printGPUInfo()
    {
        printSection("Graphics");

        std::error_code ec;
        bool found_gpu = false;

        // Check for NVIDIA GPU
        if (std::filesystem::exists("/proc/driver/nvidia/version", ec))
        {
            std::ifstream nvidia("/proc/driver/nvidia/version");
            if (nvidia)
            {
                std::string line;
                std::getline(nvidia, line);
                printField("NVIDIA Driver", line);
                found_gpu = true;
            }
        }

        // Check for AMD GPU
        if (std::filesystem::exists("/sys/class/drm", ec))
        {
            for (const auto& entry : std::filesystem::directory_iterator("/sys/class/drm", ec))
            {
                std::string name = entry.path().filename().string();
                if (name.find("card") == 0 && name.find("-") == std::string::npos)
                {
                    std::string device_path = entry.path().string() + "/device";

                    std::string vendor = readSysFile(device_path + "/vendor");
                    std::string device = readSysFile(device_path + "/device");

                    if (!vendor.empty())
                    {
                        printField("GPU " + name, vendor + " " + device);
                        found_gpu = true;
                    }
                }
            }
        }

        if (!found_gpu)
        {
            printField("GPU", "Not detected", fmt::color::yellow);
        }
    }

    void printDiskInfo()
    {
        printSection("Disks");

        std::ifstream mounts("/proc/mounts");
        if (!mounts)
            return;

        std::vector<std::string> checked;
        std::string line;

        while (std::getline(mounts, line))
        {
            std::istringstream iss(line);
            std::string device, mountpoint, fstype;

            if (!(iss >> device >> mountpoint >> fstype))
                continue;

            // Skip virtual filesystems
            if (device[0] != '/' || fstype == "tmpfs" || fstype == "devtmpfs" ||
                fstype == "sysfs" || fstype == "proc" || fstype == "devpts" || fstype == "cgroup" ||
                fstype == "cgroup2" || fstype == "pstore" || fstype == "bpf" ||
                fstype == "tracefs" || fstype == "debugfs" || fstype == "securityfs" ||
                fstype == "hugetlbfs" || fstype == "mqueue" || fstype == "configfs" ||
                fstype == "fusectl" || fstype == "fuse.gvfsd-fuse")
                continue;

            // Skip if we've already checked this mountpoint
            if (std::find(checked.begin(), checked.end(), mountpoint) != checked.end())
                continue;
            checked.push_back(mountpoint);

            struct statvfs stat;
            if (statvfs(mountpoint.c_str(), &stat) == 0)
            {
                uint64_t total = stat.f_blocks * stat.f_frsize;
                uint64_t free = stat.f_bfree * stat.f_frsize;
                uint64_t used = total - free;
                double used_percent = total > 0 ? (double)used / total * 100.0 : 0.0;

                std::string disk_info =
                    fmt::format("{} ({}) - {} / {} used ({:.1f}%)", mountpoint, fstype,
                                FilesystemHelper::formatSize(used),
                                FilesystemHelper::formatSize(total), used_percent);

                auto color = used_percent > 90   ? fmt::color::red
                             : used_percent > 70 ? fmt::color::yellow
                                                 : fmt::color::green;

                printField(device, disk_info, color);
            }
        }
    }

    void printNetworkInfo()
    {
        printSection("Network");

        struct ifaddrs* ifaddr;
        if (getifaddrs(&ifaddr) == -1)
            return;

        std::map<std::string, std::vector<std::string>> interface_ips;

        // Collect all IP addresses for each interface
        for (struct ifaddrs* ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next)
        {
            if (ifa->ifa_addr == nullptr)
                continue;

            std::string ifname = ifa->ifa_name;

            // Skip loopback
            if (ifname == "lo")
                continue;

            // Handle IPv4
            if (ifa->ifa_addr->sa_family == AF_INET)
            {
                char addr_buf[INET_ADDRSTRLEN];
                char mask_buf[INET_ADDRSTRLEN];

                struct sockaddr_in* addr = (struct sockaddr_in*)ifa->ifa_addr;
                struct sockaddr_in* netmask = (struct sockaddr_in*)ifa->ifa_netmask;

                inet_ntop(AF_INET, &addr->sin_addr, addr_buf, INET_ADDRSTRLEN);
                inet_ntop(AF_INET, &netmask->sin_addr, mask_buf, INET_ADDRSTRLEN);

                // Calculate CIDR prefix length
                uint32_t mask = ntohl(netmask->sin_addr.s_addr);
                int prefix = 0;
                while (mask & (1u << 31))
                {
                    prefix++;
                    mask <<= 1;
                }

                interface_ips[ifname].push_back(fmt::format("IPv4: {}/{}", addr_buf, prefix));
            }
            // Handle IPv6
            else if (ifa->ifa_addr->sa_family == AF_INET6)
            {
                char addr_buf[INET6_ADDRSTRLEN];
                struct sockaddr_in6* addr = (struct sockaddr_in6*)ifa->ifa_addr;
                struct sockaddr_in6* netmask = (struct sockaddr_in6*)ifa->ifa_netmask;

                inet_ntop(AF_INET6, &addr->sin6_addr, addr_buf, INET6_ADDRSTRLEN);

                // Calculate IPv6 prefix length
                int prefix = 0;
                uint8_t* mask_bytes = (uint8_t*)&netmask->sin6_addr;
                for (int i = 0; i < 16; i++)
                {
                    uint8_t byte = mask_bytes[i];
                    while (byte & 0x80)
                    {
                        prefix++;
                        byte <<= 1;
                    }
                }

                interface_ips[ifname].push_back(fmt::format("IPv6: {}/{}", addr_buf, prefix));
            }
        }

        freeifaddrs(ifaddr);

        // Now display interfaces with their IPs
        std::error_code ec;
        if (!std::filesystem::exists("/sys/class/net", ec))
            return;

        for (const auto& entry : std::filesystem::directory_iterator("/sys/class/net", ec))
        {
            std::string interface = entry.path().filename().string();

            // Skip loopback
            if (interface == "lo")
                continue;

            std::string operstate = readSysFile(entry.path().string() + "/operstate");
            std::string mac_address = readSysFile(entry.path().string() + "/address");

            auto color = (operstate == "up") ? fmt::color::green : fmt::color::red;

            std::string status = operstate;
            if (!mac_address.empty())
            {
                status += " (MAC: " + mac_address + ")";
            }

            printField(interface, status, color);

            // Print IP addresses with indentation
            if (interface_ips.find(interface) != interface_ips.end())
            {
                for (const auto& ip_info : interface_ips[interface])
                {
                    if (use_colors_)
                    {
                        fmt::print("    {}\n", ip_info);
                    }
                    else
                    {
                        fmt::print("    {}\n", ip_info);
                    }
                }
            }
        }
    }

    void printDisplayInfo()
    {
        printSection("Display");

        // Check for X11 display
        const char* display_env = std::getenv("DISPLAY");
        if (display_env)
        {
            printField("DISPLAY", display_env);
        }

        // Check for Wayland
        const char* wayland_env = std::getenv("WAYLAND_DISPLAY");
        if (wayland_env)
        {
            printField("WAYLAND_DISPLAY", wayland_env);
        }

        if (!display_env && !wayland_env)
        {
            printField("Display Server", "Not detected", fmt::color::yellow);
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
