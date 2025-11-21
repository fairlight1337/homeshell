#pragma once

#include <homeshell/Command.hpp>
#include <homeshell/EncryptedMount.hpp>
#include <homeshell/PasswordInput.hpp>
#include <homeshell/Status.hpp>
#include <homeshell/VirtualFilesystem.hpp>

#include <fmt/color.h>

#include <memory>
#include <string>
#include <vector>

namespace homeshell
{

class MountCommand : public ICommand
{
public:
    std::string getName() const override
    {
        return "mount";
    }

    std::string getDescription() const override
    {
        return "Mount an encrypted storage file";
    }

    CommandType getType() const override
    {
        return CommandType::Synchronous;
    }

    Status execute(const CommandContext& context) override
    {
        if (context.args.size() < 3)
        {
            fmt::print(fg(fmt::color::red), "Error: Insufficient arguments\n");
            fmt::print("Usage: mount <name> <db_path> <mount_point> [password] [max_size_mb]\n");
            fmt::print("  If password is omitted, you will be prompted to enter it.\n");
            return Status::error("Insufficient arguments");
        }

        std::string name = context.args[0];
        std::string db_path = context.args[1];
        std::string mount_point = context.args[2];
        std::string password;
        int64_t max_size_mb = 100;

        // Password is optional - prompt if not provided
        if (context.args.size() >= 4)
        {
            password = context.args[3];
        }
        else
        {
            // Prompt for password
            std::string prompt = fmt::format("Enter password for mount '{}': ", name);
            password = PasswordInput::readPassword(prompt);

            if (password.empty())
            {
                fmt::print(fg(fmt::color::red), "Error: Password cannot be empty\n");
                return Status::error("Empty password");
            }
        }

        // Max size is optional
        if (context.args.size() >= 5)
        {
            try
            {
                max_size_mb = std::stoll(context.args[4]);
            }
            catch (...)
            {
                fmt::print(fg(fmt::color::red), "Error: Invalid max_size_mb value\n");
                return Status::error("Invalid max_size_mb");
            }
        }

        // Create mount
        auto mount = std::make_shared<EncryptedMount>(name, db_path, mount_point, max_size_mb);

        if (!mount->mount(password))
        {
            fmt::print(fg(fmt::color::red), "Error: Failed to mount '{}'\n", name);
            fmt::print("Check that the password is correct and the file is accessible.\n");
            return Status::error("Mount failed");
        }

        // Add to VFS
        auto& vfs = VirtualFilesystem::getInstance();
        if (!vfs.addMount(mount))
        {
            mount->unmount();
            fmt::print(fg(fmt::color::red), "Error: Failed to add mount to VFS\n");
            return Status::error("VFS add failed");
        }

        fmt::print(fg(fmt::color::green), "Successfully mounted '{}' at '{}'\n", name, mount_point);
        return Status::ok();
    }
};

} // namespace homeshell
