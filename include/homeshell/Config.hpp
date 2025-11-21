#pragma once

#include <fstream>
#include <nlohmann/json.hpp>
#include <string>

namespace homeshell
{

struct Config
{
    std::string prompt_format = "$ ";

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

        return config;
    }

    static Config loadDefault()
    {
        return Config{};
    }
};

} // namespace homeshell
