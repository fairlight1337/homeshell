#include <homeshell/PromptFormatter.hpp>
#include <gtest/gtest.h>

using namespace homeshell;

TEST(PromptFormatterTest, NoTokens)
{
    std::string result = PromptFormatter::format("simple> ");
    EXPECT_EQ(result, "simple> ");
}

TEST(PromptFormatterTest, UserToken)
{
    std::string result = PromptFormatter::format("%user%> ");
    EXPECT_NE(result, "%user%> ");
    EXPECT_TRUE(result.find("%user%") == std::string::npos);
}

TEST(PromptFormatterTest, PathToken)
{
    std::string result = PromptFormatter::format("[%path%]> ");
    EXPECT_NE(result, "[%path%]> ");
    EXPECT_TRUE(result.find("%path%") == std::string::npos);
    EXPECT_TRUE(result.find("[") != std::string::npos);
    EXPECT_TRUE(result.find("]") != std::string::npos);
}

TEST(PromptFormatterTest, FolderToken)
{
    std::string result = PromptFormatter::format("%folder%> ");
    EXPECT_NE(result, "%folder%> ");
    EXPECT_TRUE(result.find("%folder%") == std::string::npos);
}

TEST(PromptFormatterTest, TimeToken)
{
    std::string result = PromptFormatter::format("[%time%]> ");
    EXPECT_NE(result, "[%time%]> ");
    EXPECT_TRUE(result.find("%time%") == std::string::npos);
    // Check time format (HH:MM:SS)
    EXPECT_TRUE(result.find(":") != std::string::npos);
}

TEST(PromptFormatterTest, DateToken)
{
    std::string result = PromptFormatter::format("[%date%]> ");
    EXPECT_NE(result, "[%date%]> ");
    EXPECT_TRUE(result.find("%date%") == std::string::npos);
    // Check date format (YYYY-MM-DD)
    EXPECT_TRUE(result.find("-") != std::string::npos);
}

TEST(PromptFormatterTest, MultipleTokens)
{
    std::string result = PromptFormatter::format("[%user%@%folder% %time%]> ");
    EXPECT_TRUE(result.find("%user%") == std::string::npos);
    EXPECT_TRUE(result.find("%folder%") == std::string::npos);
    EXPECT_TRUE(result.find("%time%") == std::string::npos);
    EXPECT_TRUE(result.find("@") != std::string::npos);
    EXPECT_TRUE(result.find("[") != std::string::npos);
    EXPECT_TRUE(result.find("]") != std::string::npos);
}

