#include <homeshell/commands/GrepCommand.hpp>
#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <sstream>

using namespace homeshell;

class GrepCommandTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        test_dir_ = std::filesystem::temp_directory_path() / "grep_test";
        std::filesystem::create_directories(test_dir_);

        // Create test file 1
        test_file1_ = test_dir_ / "file1.txt";
        std::ofstream f1(test_file1_);
        f1 << "Hello World\n";
        f1 << "This is a test\n";
        f1 << "ERROR: Something went wrong\n";
        f1 << "Warning: Low memory\n";
        f1 << "hello again\n";
        f1.close();

        // Create test file 2
        test_file2_ = test_dir_ / "file2.txt";
        std::ofstream f2(test_file2_);
        f2 << "Another file\n";
        f2 << "ERROR: Connection failed\n";
        f2 << "All systems operational\n";
        f2.close();

        // Create subdirectory with file
        sub_dir_ = test_dir_ / "subdir";
        std::filesystem::create_directories(sub_dir_);
        test_file3_ = sub_dir_ / "file3.txt";
        std::ofstream f3(test_file3_);
        f3 << "Nested file content\n";
        f3 << "ERROR: Disk full\n";
        f3.close();
    }

    void TearDown() override
    {
        std::filesystem::remove_all(test_dir_);
    }

    std::filesystem::path test_dir_;
    std::filesystem::path test_file1_;
    std::filesystem::path test_file2_;
    std::filesystem::path test_file3_;
    std::filesystem::path sub_dir_;
};

TEST_F(GrepCommandTest, GetName)
{
    GrepCommand cmd;
    EXPECT_EQ(cmd.getName(), "grep");
}

TEST_F(GrepCommandTest, GetDescription)
{
    GrepCommand cmd;
    EXPECT_FALSE(cmd.getDescription().empty());
}

TEST_F(GrepCommandTest, GetType)
{
    GrepCommand cmd;
    EXPECT_EQ(cmd.getType(), CommandType::Synchronous);
}

TEST_F(GrepCommandTest, HelpOption)
{
    GrepCommand cmd;
    CommandContext context;
    context.args = {"--help"};

    testing::internal::CaptureStdout();
    auto status = cmd.execute(context);
    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_TRUE(status.isSuccess());
    EXPECT_TRUE(output.find("Usage") != std::string::npos);
    EXPECT_TRUE(output.find("grep") != std::string::npos);
}

TEST_F(GrepCommandTest, MissingPattern)
{
    GrepCommand cmd;
    CommandContext context;
    context.args = {};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();
    auto status = cmd.execute(context);
    testing::internal::GetCapturedStdout();
    testing::internal::GetCapturedStderr();

    EXPECT_FALSE(status.isSuccess());
}

TEST_F(GrepCommandTest, UnknownOption)
{
    GrepCommand cmd;
    CommandContext context;
    context.args = {"--unknown", "pattern"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();
    auto status = cmd.execute(context);
    testing::internal::GetCapturedStdout();
    testing::internal::GetCapturedStderr();

    EXPECT_FALSE(status.isSuccess());
}

TEST_F(GrepCommandTest, BasicSearch)
{
    GrepCommand cmd;
    CommandContext context;
    context.args = {"ERROR", test_file1_.string()};

    testing::internal::CaptureStdout();
    auto status = cmd.execute(context);
    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_TRUE(status.isSuccess());
    EXPECT_TRUE(output.find("ERROR: Something went wrong") != std::string::npos);
}

TEST_F(GrepCommandTest, CaseInsensitiveSearch)
{
    GrepCommand cmd;
    CommandContext context;
    context.args = {"-i", "hello", test_file1_.string()};

    testing::internal::CaptureStdout();
    auto status = cmd.execute(context);
    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_TRUE(status.isSuccess());
    EXPECT_TRUE(output.find("Hello World") != std::string::npos);
    EXPECT_TRUE(output.find("hello again") != std::string::npos);
}

TEST_F(GrepCommandTest, IgnoreCaseLongOption)
{
    GrepCommand cmd;
    CommandContext context;
    context.args = {"--ignore-case", "warning", test_file1_.string()};

    testing::internal::CaptureStdout();
    auto status = cmd.execute(context);
    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_TRUE(status.isSuccess());
    EXPECT_TRUE(output.find("Warning: Low memory") != std::string::npos);
}

TEST_F(GrepCommandTest, LineNumbers)
{
    GrepCommand cmd;
    CommandContext context;
    context.args = {"-n", "ERROR", test_file1_.string()};

    testing::internal::CaptureStdout();
    auto status = cmd.execute(context);
    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_TRUE(status.isSuccess());
    EXPECT_TRUE(output.find("3:") != std::string::npos); // Line 3
    EXPECT_TRUE(output.find("ERROR") != std::string::npos);
}

TEST_F(GrepCommandTest, MultipleFiles)
{
    GrepCommand cmd;
    CommandContext context;
    context.args = {"ERROR", test_file1_.string(), test_file2_.string()};

    testing::internal::CaptureStdout();
    auto status = cmd.execute(context);
    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_TRUE(status.isSuccess());
    // Should show filenames when multiple files
    EXPECT_TRUE(output.find("file1.txt") != std::string::npos);
    EXPECT_TRUE(output.find("file2.txt") != std::string::npos);
    EXPECT_TRUE(output.find("Something went wrong") != std::string::npos);
    EXPECT_TRUE(output.find("Connection failed") != std::string::npos);
}

TEST_F(GrepCommandTest, RecursiveSearch)
{
    GrepCommand cmd;
    CommandContext context;
    context.args = {"-r", "ERROR", test_dir_.string()};

    testing::internal::CaptureStdout();
    auto status = cmd.execute(context);
    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_TRUE(status.isSuccess());
    // Should find all three ERROR lines
    EXPECT_TRUE(output.find("Something went wrong") != std::string::npos);
    EXPECT_TRUE(output.find("Connection failed") != std::string::npos);
    EXPECT_TRUE(output.find("Disk full") != std::string::npos);
}

TEST_F(GrepCommandTest, RecursiveLongOption)
{
    GrepCommand cmd;
    CommandContext context;
    context.args = {"--recursive", "ERROR", test_dir_.string()};

    testing::internal::CaptureStdout();
    auto status = cmd.execute(context);
    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_TRUE(status.isSuccess());
    EXPECT_TRUE(output.find("ERROR") != std::string::npos);
}

TEST_F(GrepCommandTest, RecursiveWithLineNumbers)
{
    GrepCommand cmd;
    CommandContext context;
    context.args = {"-rn", "ERROR", test_dir_.string()};

    testing::internal::CaptureStdout();
    auto status = cmd.execute(context);
    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_TRUE(status.isSuccess());
    // Should have line numbers
    EXPECT_TRUE(output.find(":3:") != std::string::npos || output.find(":2:") != std::string::npos);
}

TEST_F(GrepCommandTest, NoMatchFound)
{
    GrepCommand cmd;
    CommandContext context;
    context.args = {"NOMATCH", test_file1_.string()};

    testing::internal::CaptureStdout();
    auto status = cmd.execute(context);
    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_FALSE(status.isSuccess()); // grep returns error when no match
    EXPECT_TRUE(output.empty());
}

TEST_F(GrepCommandTest, NonExistentFile)
{
    GrepCommand cmd;
    CommandContext context;
    context.args = {"ERROR", "/nonexistent/file.txt"};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();
    auto status = cmd.execute(context);
    testing::internal::GetCapturedStdout();
    testing::internal::GetCapturedStderr();

    EXPECT_FALSE(status.isSuccess());
}

TEST_F(GrepCommandTest, InvalidRegex)
{
    GrepCommand cmd;
    CommandContext context;
    context.args = {"[invalid", test_file1_.string()};

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();
    auto status = cmd.execute(context);
    testing::internal::GetCapturedStdout();
    testing::internal::GetCapturedStderr();

    EXPECT_FALSE(status.isSuccess());
}

TEST_F(GrepCommandTest, RegexPattern)
{
    GrepCommand cmd;
    CommandContext context;
    context.args = {"ERROR.*wrong", test_file1_.string()};

    testing::internal::CaptureStdout();
    auto status = cmd.execute(context);
    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_TRUE(status.isSuccess());
    EXPECT_TRUE(output.find("Something went wrong") != std::string::npos);
}

TEST_F(GrepCommandTest, WithFilenameOption)
{
    GrepCommand cmd;
    CommandContext context;
    context.args = {"-H", "ERROR", test_file1_.string()};

    testing::internal::CaptureStdout();
    auto status = cmd.execute(context);
    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_TRUE(status.isSuccess());
    EXPECT_TRUE(output.find("file1.txt") != std::string::npos);
}

TEST_F(GrepCommandTest, NoFilenameOption)
{
    GrepCommand cmd;
    CommandContext context;
    context.args = {"-h", "ERROR", test_file1_.string(), test_file2_.string()};

    testing::internal::CaptureStdout();
    auto status = cmd.execute(context);
    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_TRUE(status.isSuccess());
    // Should not show filenames
    EXPECT_TRUE(output.find("file1.txt") == std::string::npos);
    EXPECT_TRUE(output.find("file2.txt") == std::string::npos);
    EXPECT_TRUE(output.find("ERROR") != std::string::npos);
}

TEST_F(GrepCommandTest, ColorOption)
{
    GrepCommand cmd;
    CommandContext context;
    context.args = {"--color", "ERROR", test_file1_.string()};

    testing::internal::CaptureStdout();
    auto status = cmd.execute(context);
    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_TRUE(status.isSuccess());
    // Color codes are present (ANSI escape sequences)
    // We just check that output is generated
    EXPECT_FALSE(output.empty());
}

TEST_F(GrepCommandTest, NoColorOption)
{
    GrepCommand cmd;
    CommandContext context;
    context.args = {"--no-color", "ERROR", test_file1_.string()};

    testing::internal::CaptureStdout();
    auto status = cmd.execute(context);
    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_TRUE(status.isSuccess());
    EXPECT_TRUE(output.find("ERROR") != std::string::npos);
}

TEST_F(GrepCommandTest, EmptyFile)
{
    std::filesystem::path empty_file = test_dir_ / "empty.txt";
    std::ofstream f(empty_file);
    f.close();

    GrepCommand cmd;
    CommandContext context;
    context.args = {"ERROR", empty_file.string()};

    testing::internal::CaptureStdout();
    auto status = cmd.execute(context);
    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_FALSE(status.isSuccess()); // No match
    EXPECT_TRUE(output.empty());
}

TEST_F(GrepCommandTest, CombinedOptions)
{
    GrepCommand cmd;
    CommandContext context;
    context.args = {"-rni", "error", test_dir_.string()};

    testing::internal::CaptureStdout();
    auto status = cmd.execute(context);
    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_TRUE(status.isSuccess());
    // Should find all ERROR lines (case-insensitive, recursive, with line numbers)
    EXPECT_TRUE(output.find("ERROR") != std::string::npos);
}

