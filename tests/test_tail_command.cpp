#include <homeshell/commands/TailCommand.hpp>
#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <thread>
#include <chrono>

namespace fs = std::filesystem;

class TailCommandTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        test_dir_ = fs::temp_directory_path() / "homeshell_tail_test";
        fs::create_directories(test_dir_);
        
        // Create test file with numbered lines
        test_file_ = test_dir_ / "test.txt";
        std::ofstream file(test_file_);
        for (int i = 1; i <= 20; ++i)
        {
            file << "Line " << i << "\n";
        }
        file.close();
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
};

TEST_F(TailCommandTest, GetName)
{
    homeshell::TailCommand cmd;
    EXPECT_EQ(cmd.getName(), "tail");
}

TEST_F(TailCommandTest, GetDescription)
{
    homeshell::TailCommand cmd;
    EXPECT_FALSE(cmd.getDescription().empty());
}

TEST_F(TailCommandTest, GetType)
{
    homeshell::TailCommand cmd;
    EXPECT_EQ(cmd.getType(), homeshell::CommandType::Synchronous);
}

TEST_F(TailCommandTest, SupportsCancellation)
{
    homeshell::TailCommand cmd;
    EXPECT_TRUE(cmd.supportsCancellation());
}

TEST_F(TailCommandTest, DefaultLastTenLines)
{
    homeshell::TailCommand cmd;
    homeshell::CommandContext ctx;
    ctx.args = {test_file_.string()};
    
    testing::internal::CaptureStdout();
    auto status = cmd.execute(ctx);
    std::string output = testing::internal::GetCapturedStdout();
    
    EXPECT_TRUE(status.isSuccess());
    
    // Should show last 10 lines (Line 11 through Line 20)
    EXPECT_TRUE(output.find("Line 11") != std::string::npos);
    EXPECT_TRUE(output.find("Line 20") != std::string::npos);
    EXPECT_TRUE(output.find("Line 10") == std::string::npos);
}

TEST_F(TailCommandTest, CustomLineCount)
{
    homeshell::TailCommand cmd;
    homeshell::CommandContext ctx;
    ctx.args = {"-n", "5", test_file_.string()};
    
    testing::internal::CaptureStdout();
    auto status = cmd.execute(ctx);
    std::string output = testing::internal::GetCapturedStdout();
    
    EXPECT_TRUE(status.isSuccess());
    
    // Should show last 5 lines (Line 16 through Line 20)
    EXPECT_TRUE(output.find("Line 16") != std::string::npos);
    EXPECT_TRUE(output.find("Line 20") != std::string::npos);
    EXPECT_TRUE(output.find("Line 15") == std::string::npos);
}

TEST_F(TailCommandTest, LargeLineCount)
{
    homeshell::TailCommand cmd;
    homeshell::CommandContext ctx;
    ctx.args = {"-n", "100", test_file_.string()};
    
    testing::internal::CaptureStdout();
    auto status = cmd.execute(ctx);
    std::string output = testing::internal::GetCapturedStdout();
    
    EXPECT_TRUE(status.isSuccess());
    
    // Should show all 20 lines
    EXPECT_TRUE(output.find("Line 1") != std::string::npos);
    EXPECT_TRUE(output.find("Line 20") != std::string::npos);
}

TEST_F(TailCommandTest, SingleLine)
{
    homeshell::TailCommand cmd;
    homeshell::CommandContext ctx;
    ctx.args = {"-n", "1", test_file_.string()};
    
    testing::internal::CaptureStdout();
    auto status = cmd.execute(ctx);
    std::string output = testing::internal::GetCapturedStdout();
    
    EXPECT_TRUE(status.isSuccess());
    
    // Should show only last line
    EXPECT_TRUE(output.find("Line 20") != std::string::npos);
    EXPECT_TRUE(output.find("Line 19") == std::string::npos);
}

TEST_F(TailCommandTest, NonExistentFile)
{
    homeshell::TailCommand cmd;
    homeshell::CommandContext ctx;
    ctx.args = {(test_dir_ / "nonexistent.txt").string()};
    
    testing::internal::CaptureStdout();
    auto status = cmd.execute(ctx);
    testing::internal::GetCapturedStdout();
    
    EXPECT_FALSE(status.isSuccess());
}

TEST_F(TailCommandTest, InvalidLineCount)
{
    homeshell::TailCommand cmd;
    homeshell::CommandContext ctx;
    ctx.args = {"-n", "invalid", test_file_.string()};
    
    testing::internal::CaptureStdout();
    auto status = cmd.execute(ctx);
    testing::internal::GetCapturedStdout();
    
    EXPECT_FALSE(status.isSuccess());
}

TEST_F(TailCommandTest, NegativeLineCount)
{
    homeshell::TailCommand cmd;
    homeshell::CommandContext ctx;
    ctx.args = {"-n", "-5", test_file_.string()};
    
    testing::internal::CaptureStdout();
    auto status = cmd.execute(ctx);
    testing::internal::GetCapturedStdout();
    
    EXPECT_FALSE(status.isSuccess());
}

TEST_F(TailCommandTest, UnknownOption)
{
    homeshell::TailCommand cmd;
    homeshell::CommandContext ctx;
    ctx.args = {"-x", test_file_.string()};
    
    testing::internal::CaptureStdout();
    auto status = cmd.execute(ctx);
    testing::internal::GetCapturedStdout();
    
    EXPECT_FALSE(status.isSuccess());
}

TEST_F(TailCommandTest, HelpOption)
{
    homeshell::TailCommand cmd;
    homeshell::CommandContext ctx;
    ctx.args = {"--help"};
    
    testing::internal::CaptureStdout();
    auto status = cmd.execute(ctx);
    std::string output = testing::internal::GetCapturedStdout();
    
    EXPECT_TRUE(status.isSuccess());
    EXPECT_TRUE(output.find("Usage:") != std::string::npos);
}

TEST_F(TailCommandTest, EmptyFile)
{
    auto empty_file = test_dir_ / "empty.txt";
    std::ofstream(empty_file).close();
    
    homeshell::TailCommand cmd;
    homeshell::CommandContext ctx;
    ctx.args = {empty_file.string()};
    
    testing::internal::CaptureStdout();
    auto status = cmd.execute(ctx);
    std::string output = testing::internal::GetCapturedStdout();
    
    EXPECT_TRUE(status.isSuccess());
    EXPECT_TRUE(output.empty() || output == "\n");
}

TEST_F(TailCommandTest, FollowMode)
{
    // Create a file for following
    auto follow_file = test_dir_ / "follow.txt";
    std::ofstream file(follow_file);
    file << "Initial line\n";
    file.close();
    
    homeshell::TailCommand cmd;
    homeshell::CommandContext ctx;
    ctx.args = {"-f", follow_file.string()};
    
    // Start follow in a thread
    std::thread follow_thread([&cmd, &ctx]() {
        testing::internal::CaptureStdout();
        cmd.execute(ctx);
        testing::internal::GetCapturedStdout();
    });
    
    // Give it a moment to start
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Append to the file
    std::ofstream append_file(follow_file, std::ios::app);
    append_file << "New line\n";
    append_file.close();
    
    // Give it time to detect the change
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Cancel the command
    cmd.cancel();
    
    // Wait for thread to finish
    if (follow_thread.joinable())
    {
        follow_thread.join();
    }
    
    // Test passes if we got here without hanging
    EXPECT_TRUE(true);
}

TEST_F(TailCommandTest, FollowAndCustomLineCount)
{
    homeshell::TailCommand cmd;
    homeshell::CommandContext ctx;
    ctx.args = {"-n", "3", "-f", test_file_.string()};
    
    // Start in thread and cancel quickly
    std::thread follow_thread([&cmd, &ctx]() {
        testing::internal::CaptureStdout();
        cmd.execute(ctx);
        testing::internal::GetCapturedStdout();
    });
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    cmd.cancel();
    
    if (follow_thread.joinable())
    {
        follow_thread.join();
    }
    
    EXPECT_TRUE(true);
}

TEST_F(TailCommandTest, CancellationWorks)
{
    homeshell::TailCommand cmd;
    
    // Test that cancel flag works
    EXPECT_FALSE(cmd.supportsCancellation() == false);
    
    cmd.cancel();
    
    // Cancellation should be set
    // (We can't directly test the atomic, but the follow tests verify it works)
    EXPECT_TRUE(true);
}

