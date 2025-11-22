#pragma once

#include <fstream>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace homeshell
{

/**
 * @brief Configuration for an encrypted mount
 *
 * Defines the parameters for an encrypted virtual filesystem mount.
 * Used when loading mount configurations from JSON config files.
 */
struct MountConfig
{
    std::string name;          ///< Unique mount identifier
    std::string db_path;       ///< Path to SQLCipher database file
    std::string mount_point;   ///< Virtual mount point (e.g., "/secure")
    std::string password;      ///< Mount password (optional, will prompt if empty)
    int64_t max_size_mb = 100; ///< Maximum storage size in megabytes
    bool auto_mount = true;    ///< Whether to mount automatically on shell startup
};

/**
 * @brief Shell configuration
 *
 * Contains all configuration options for the shell, including prompt format,
 * history file location, MOTD, and encrypted mount definitions.
 *
 * @details Configuration can be loaded from a JSON file or use sensible defaults.
 *
 *          JSON format example:
 *          ```json
 *          {
 *            "prompt_format": "homeshell> ",
 *            "history_file": "~/.homeshell_history",
 *            "motd": "Welcome to Homeshell!",
 *            "encrypted_mounts": [
 *              {
 *                "name": "secure",
 *                "db_path": "~/secure.db",
 *                "mount_point": "/secure",
 *                "password": "",
 *                "max_size_mb": 100,
 *                "auto_mount": true
 *              }
 *            ]
 *          }
 *          ```
 *
 *          Paths support tilde expansion (~/ for home directory).
 */
struct Config
{
    std::string prompt_format = "$ ";                  ///< Shell prompt format string
    std::string history_file = "~/.homeshell_history"; ///< Path to history file
    std::string motd =
        "Welcome to Homeshell!\nType 'help' for available commands."; ///< Message of the day
    std::vector<MountConfig> encrypted_mounts; ///< Encrypted mount configurations

    /**
     * @brief Load configuration from JSON file
     * @param filepath Path to configuration file
     * @return Parsed configuration
     * @throws std::runtime_error if file cannot be opened or parsed
     */
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

    /**
     * @brief Load default configuration
     * @return Configuration with default values
     */
    static Config loadDefault()
    {
        return Config{};
    }

    /**
     * @brief Expand tilde (~) in file paths to home directory
     * @param path Path that may contain ~ prefix
     * @return Expanded path with home directory substituted
     *
     * @details Supports both Unix ($HOME) and Windows (%USERPROFILE%) environments.
     *          Paths not starting with ~ are returned unchanged.
     */
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
