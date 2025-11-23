#include <homeshell/commands/LessCommand.hpp>
#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

class LessCommandTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        test_dir_ = fs::temp_directory_path() / "homeshell_less_test";
        fs::create_directories(test_dir_);
        
        // Create test file with content
        test_file_ = test_dir_ / "test.txt";
        std::ofstream file(test_file_);
        for (int i = 1; i <= 100; ++i)
        {
            file << "This is line number " << i << "\n";
        }
        file.close();
        
        // Create small file
        small_file_ = test_dir_ / "small.txt";
        std::ofstream small(small_file_);
        small << "Line 1\nLine 2\nLine 3\n";
        small.close();
    }

    void TearDown() override
    {
        if (fs::exists(test_dir_))
        {
            fs::remove_all(test_dir_);
        }
    }

    fs::path test_dir_;
    fs::path test_file_;
    fs::path small_file_;
};

TEST_F(LessCommandTest, GetName)
{
    homeshell::LessCommand cmd;
    EXPECT_EQ(cmd.getName(), "less");
}

TEST_F(LessCommandTest, GetDescription)
{
    homeshell::LessCommand cmd;
    EXPECT_FALSE(cmd.getDescription().empty());
}

TEST_F(LessCommandTest, GetType)
{
    homeshell::LessCommand cmd;
    EXPECT_EQ(cmd.getType(), homeshell::CommandType::Synchronous);
}

TEST_F(LessCommandTest, SupportsCancellation)
{
    homeshell::LessCommand cmd;
    EXPECT_TRUE(cmd.supportsCancellation());
}

TEST_F(LessCommandTest, HelpOption)
{
    homeshell::LessCommand cmd;
    homeshell::CommandContext ctx;
    ctx.args = {"--help"};
    
    testing::internal::CaptureStdout();
    auto status = cmd.execute(ctx);
    std::string output = testing::internal::GetCapturedStdout();
    
    EXPECT_TRUE(status.isSuccess());
    EXPECT_TRUE(output.find("Usage:") != std::string::npos);
    EXPECT_TRUE(output.find("Keyboard Controls:") != std::string::npos);
}

TEST_F(LessCommandTest, UnknownOption)
{
    homeshell::LessCommand cmd;
    homeshell::CommandContext ctx;
    ctx.args = {"-x", test_file_.string()};
    
    testing::internal::CaptureStdout();
    auto status = cmd.execute(ctx);
    testing::internal::GetCapturedStdout();
    
    EXPECT_FALSE(status.isSuccess());
}

TEST_F(LessCommandTest, NonExistentFile)
{
    homeshell::LessCommand cmd;
    homeshell::CommandContext ctx;
    ctx.args = {(test_dir_ / "nonexistent.txt").string()};
    
    testing::internal::CaptureStdout();
    auto status = cmd.execute(ctx);
    testing::internal::GetCapturedStdout();
    
    EXPECT_FALSE(status.isSuccess());
}

TEST_F(LessCommandTest, EmptyFile)
{
    auto empty_file = test_dir_ / "empty.txt";
    std::ofstream(empty_file).close();
    
    homeshell::LessCommand cmd;
    homeshell::CommandContext ctx;
    ctx.args = {empty_file.string()};
    
    testing::internal::CaptureStdout();
    auto status = cmd.execute(ctx);
    std::string output = testing::internal::GetCapturedStdout();
    
    EXPECT_TRUE(status.isSuccess());
}

TEST_F(LessCommandTest, ReadSmallFile)
{
    homeshell::LessCommand cmd;
    homeshell::CommandContext ctx;
    ctx.args = {small_file_.string()};
    
    // Note: In non-TTY environment, less just prints the content
    // This test verifies it doesn't crash
    testing::internal::CaptureStdout();
    auto status = cmd.execute(ctx);
    std::string output = testing::internal::GetCapturedStdout();
    
    EXPECT_TRUE(status.isSuccess());
    // Should contain the lines when output is not a TTY
    EXPECT_TRUE(output.find("Line 1") != std::string::npos);
    EXPECT_TRUE(output.find("Line 2") != std::string::npos);
    EXPECT_TRUE(output.find("Line 3") != std::string::npos);
}

TEST_F(LessCommandTest, ReadLargeFile)
{
    homeshell::LessCommand cmd;
    homeshell::CommandContext ctx;
    ctx.args = {test_file_.string()};
    
    // In non-TTY, prints all content
    testing::internal::CaptureStdout();
    auto status = cmd.execute(ctx);
    std::string output = testing::internal::GetCapturedStdout();
    
    EXPECT_TRUE(status.isSuccess());
    // Should contain first and last lines
    EXPECT_TRUE(output.find("line number 1") != std::string::npos);
    EXPECT_TRUE(output.find("line number 100") != std::string::npos);
}

TEST_F(LessCommandTest, CancellationSupported)
{
    homeshell::LessCommand cmd;
    
    // Verify cancellation support exists
    EXPECT_TRUE(cmd.supportsCancellation());
    
    // Call cancel
    cmd.cancel();
    
    // Test passes if we got here
    EXPECT_TRUE(true);
}

TEST_F(LessCommandTest, FileWithLongLines)
{
    auto long_file = test_dir_ / "long.txt";
    std::ofstream file(long_file);
    
    // Create lines with varying lengths
    for (int i = 0; i < 10; ++i)
    {
        std::string line(i * 20, 'x');
        file << "Line " << i << ": " << line << "\n";
    }
    file.close();
    
    homeshell::LessCommand cmd;
    homeshell::CommandContext ctx;
    ctx.args = {long_file.string()};
    
    testing::internal::CaptureStdout();
    auto status = cmd.execute(ctx);
    std::string output = testing::internal::GetCapturedStdout();
    
    EXPECT_TRUE(status.isSuccess());
    EXPECT_TRUE(output.find("Line 0:") != std::string::npos);
}

TEST_F(LessCommandTest, FileWithSpecialCharacters)
{
    auto special_file = test_dir_ / "special.txt";
    std::ofstream file(special_file);
    file << "Tab:\t\n";
    file << "Special: !@#$%^&*()\n";
    file << "Unicode: ñ ü ö\n";
    file.close();
    
    homeshell::LessCommand cmd;
    homeshell::CommandContext ctx;
    ctx.args = {special_file.string()};
    
    testing::internal::CaptureStdout();
    auto status = cmd.execute(ctx);
    std::string output = testing::internal::GetCapturedStdout();
    
    EXPECT_TRUE(status.isSuccess());
    EXPECT_TRUE(output.find("Tab:") != std::string::npos);
}

// Note: We cannot test interactive pager mode in unit tests
// because it requires a real TTY. The tests above verify:
// 1. Command setup and argument parsing
// 2. File reading logic
// 3. Non-TTY fallback behavior (printing all content)
// 4. Error handling
//
// Interactive pager features would need integration tests
// with a pseudo-TTY or manual testing.

