#pragma once

#include <homeshell/Command.hpp>
#include <homeshell/Status.hpp>
#include <homeshell/VirtualFilesystem.hpp>

#include <fmt/color.h>
#include <fmt/core.h>

#include <string>
#include <vector>

namespace homeshell
{

class VfsCommand : public ICommand
{
public:
    std::string getName() const override
    {
        return "vfs";
    }

    std::string getDescription() const override
    {
        return "Show virtual filesystem information";
    }

    CommandType getType() const override
    {
        return CommandType::Synchronous;
    }

    Status execute(const CommandContext& context) override
    {
        auto& vfs = VirtualFilesystem::getInstance();
        auto mount_names = vfs.getMountNames();

        if (mount_names.empty())
        {
            fmt::print("No virtual filesystems mounted.\n");
            return Status::ok();
        }

        fmt::print(fg(fmt::color::yellow) | fmt::emphasis::bold, "Virtual Filesystems:\n\n");

        for (const auto& name : mount_names)
        {
            auto* mount = vfs.getMount(name);
            if (mount && mount->is_mounted())
            {
                int64_t used = mount->getUsedSpace();
                int64_t max = mount->getMaxSpace();
                double usage_pct = max > 0 ? (100.0 * used / max) : 0.0;

                fmt::print(fg(fmt::color::cyan) | fmt::emphasis::bold, "{}\n", name);
                fmt::print("  Mount Point: {}\n", mount->getMountPoint());
                fmt::print("  Database:    {}\n", mount->getDbPath());
                fmt::print("  Used:        {} / {} ({:.1f}%)\n", formatBytes(used),
                           formatBytes(max), usage_pct);
                fmt::print("\n");
            }
        }

        return Status::ok();
    }

private:
    std::string formatBytes(int64_t bytes) const
    {
        const char* units[] = {"B", "KB", "MB", "GB", "TB"};
        int unit_idx = 0;
        double size = static_cast<double>(bytes);

        while (size >= 1024.0 && unit_idx < 4)
        {
            size /= 1024.0;
            unit_idx++;
        }

        if (unit_idx == 0)
        {
            return fmt::format("{} {}", static_cast<int64_t>(size), units[unit_idx]);
        }
        else
        {
            return fmt::format("{:.2f} {}", size, units[unit_idx]);
        }
    }
};

} // namespace homeshell
