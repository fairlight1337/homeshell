#pragma once

#include <fstream>
#include <nlohmann/json.hpp>
#include <string>

namespace homeshell
{

struct Config
{
    std::string prompt_format = "$ ";
    std::string history_file = "~/.homeshell_history";
    std::string motd = "Welcome to Homeshell!\nType 'help' for available commands.";

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
