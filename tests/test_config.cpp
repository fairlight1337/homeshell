#include <homeshell/Config.hpp>
#include <gtest/gtest.h>
#include <fstream>

using namespace homeshell;

TEST(ConfigTest, DefaultConfig)
{
    Config config = Config::loadDefault();
    EXPECT_EQ(config.prompt_format, "$ ");
}

TEST(ConfigTest, LoadFromValidFile)
{
    // Create a temporary config file
    std::string temp_file = "/tmp/test_config.json";
    {
        std::ofstream file(temp_file);
        file << R"({"prompt_format": "test> "})";
    }

    Config config = Config::loadFromFile(temp_file);
    EXPECT_EQ(config.prompt_format, "test> ");

    // Clean up
    std::remove(temp_file.c_str());
}

TEST(ConfigTest, LoadFromMissingFile)
{
    EXPECT_THROW(Config::loadFromFile("/nonexistent/file.json"), std::runtime_error);
}

TEST(ConfigTest, LoadFromFileWithMissingField)
{
    // Create a temporary config file without prompt_format
    std::string temp_file = "/tmp/test_config_empty.json";
    {
        std::ofstream file(temp_file);
        file << R"({})";
    }

    Config config = Config::loadFromFile(temp_file);
    // Should use default value
    EXPECT_EQ(config.prompt_format, "$ ");

    // Clean up
    std::remove(temp_file.c_str());
}

