#pragma once

#include <fstream>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace homeshell
{

struct MountConfig
{
    std::string name;
    std::string db_path;
    std::string mount_point;
    std::string password;
    int64_t max_size_mb = 100; // Default 100MB
    bool auto_mount = true;
};

struct Config
{
    std::string prompt_format = "$ ";
    std::string history_file = "~/.homeshell_history";
    std::string motd = "Welcome to Homeshell!\nType 'help' for available commands.";
    std::vector<MountConfig> encrypted_mounts;

    static Config loadFromFile(const std::string& filepath)
    {
        Config config;

        std::ifstream file(filepath);
        if (!file.is_open())
        {
            throw std::runtime_error("Could not open config file: " + filepath);
        }

        nlohmann::json j;
        file >> j;

        if (j.contains("prompt_format"))
        {
            config.prompt_format = j["prompt_format"].get<std::string>();
        }

        if (j.contains("history_file"))
        {
            config.history_file = j["history_file"].get<std::string>();
        }

        if (j.contains("motd"))
        {
            config.motd = j["motd"].get<std::string>();
        }

        if (j.contains("encrypted_mounts") && j["encrypted_mounts"].is_array())
        {
            for (const auto& mount_json : j["encrypted_mounts"])
            {
                MountConfig mount;
                if (mount_json.contains("name"))
                {
                    mount.name = mount_json["name"].get<std::string>();
                }
                if (mount_json.contains("db_path"))
                {
                    mount.db_path = mount_json["db_path"].get<std::string>();
                }
                if (mount_json.contains("mount_point"))
                {
                    mount.mount_point = mount_json["mount_point"].get<std::string>();
                }
                if (mount_json.contains("password"))
                {
                    mount.password = mount_json["password"].get<std::string>();
                }
                if (mount_json.contains("max_size_mb"))
                {
                    mount.max_size_mb = mount_json["max_size_mb"].get<int64_t>();
                }
                if (mount_json.contains("auto_mount"))
                {
                    mount.auto_mount = mount_json["auto_mount"].get<bool>();
                }

                config.encrypted_mounts.push_back(mount);
            }
        }

        return config;
    }

    static Config loadDefault()
    {
        return Config{};
    }

    std::string expandPath(const std::string& path) const
    {
        if (path.empty() || path[0] != '~')
        {
            return path;
        }

        const char* home = std::getenv("HOME");
#ifdef _WIN32
        if (!home)
        {
            home = std::getenv("USERPROFILE");
        }
#endif
        if (!home)
        {
            return path;
        }

        return std::string(home) + path.substr(1);
    }
};

} // namespace homeshell
