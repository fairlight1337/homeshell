#include <homeshell/commands/MkdirCommand.hpp>
#include <homeshell/commands/TouchCommand.hpp>
#include <homeshell/commands/RmCommand.hpp>
#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

class FileCommandsTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        test_dir_ = fs::temp_directory_path() / "homeshell_file_test";
        fs::create_directories(test_dir_);
        
        // Change to test directory
        original_dir_ = fs::current_path();
        fs::current_path(test_dir_);
    }

    void TearDown() override
    {
        // Restore original directory
        fs::current_path(original_dir_);
        
        // Clean up
        if (fs::exists(test_dir_))
        {
            fs::remove_all(test_dir_);
        }
    }

    fs::path test_dir_;
    fs::path original_dir_;
};

// Mkdir Tests
TEST_F(FileCommandsTest, MkdirGetName)
{
    homeshell::MkdirCommand cmd;
    EXPECT_EQ(cmd.getName(), "mkdir");
}

TEST_F(FileCommandsTest, MkdirCreateDirectory)
{
    homeshell::MkdirCommand cmd;
    homeshell::CommandContext ctx;
    ctx.args = {(test_dir_ / "testdir").string()};
    
    testing::internal::CaptureStdout();
    auto status = cmd.execute(ctx);
    testing::internal::GetCapturedStdout();
    
    EXPECT_TRUE(status.isSuccess());
    EXPECT_TRUE(fs::exists(test_dir_ / "testdir"));
    EXPECT_TRUE(fs::is_directory(test_dir_ / "testdir"));
}

TEST_F(FileCommandsTest, MkdirNoArguments)
{
    homeshell::MkdirCommand cmd;
    homeshell::CommandContext ctx;
    
    testing::internal::CaptureStdout();
    auto status = cmd.execute(ctx);
    testing::internal::GetCapturedStdout();
    
    EXPECT_FALSE(status.isSuccess());
}

TEST_F(FileCommandsTest, MkdirAlreadyExists)
{
    fs::create_directory(test_dir_ / "existing");
    
    homeshell::MkdirCommand cmd;
    homeshell::CommandContext ctx;
    ctx.args = {(test_dir_ / "existing").string()};
    
    testing::internal::CaptureStdout();
    auto status = cmd.execute(ctx);
    testing::internal::GetCapturedStdout();
    
    EXPECT_FALSE(status.isSuccess());
}

// Touch Tests
TEST_F(FileCommandsTest, TouchGetName)
{
    homeshell::TouchCommand cmd;
    EXPECT_EQ(cmd.getName(), "touch");
}

TEST_F(FileCommandsTest, TouchCreateFile)
{
    homeshell::TouchCommand cmd;
    homeshell::CommandContext ctx;
    ctx.args = {(test_dir_ / "newfile.txt").string()};
    
    testing::internal::CaptureStdout();
    auto status = cmd.execute(ctx);
    testing::internal::GetCapturedStdout();
    
    EXPECT_TRUE(status.isSuccess());
    EXPECT_TRUE(fs::exists(test_dir_ / "newfile.txt"));
    EXPECT_TRUE(fs::is_regular_file(test_dir_ / "newfile.txt"));
}

TEST_F(FileCommandsTest, TouchExistingFile)
{
    // Create file
    auto existing = test_dir_ / "existing.txt";
    std::ofstream(existing) << "content";
    
    homeshell::TouchCommand cmd;
    homeshell::CommandContext ctx;
    ctx.args = {existing.string()};
    
    testing::internal::CaptureStdout();
    auto status = cmd.execute(ctx);
    testing::internal::GetCapturedStdout();
    
    EXPECT_TRUE(status.isSuccess());
    EXPECT_TRUE(fs::exists(existing));
}

TEST_F(FileCommandsTest, TouchNoArguments)
{
    homeshell::TouchCommand cmd;
    homeshell::CommandContext ctx;
    
    testing::internal::CaptureStdout();
    auto status = cmd.execute(ctx);
    testing::internal::GetCapturedStdout();
    
    EXPECT_FALSE(status.isSuccess());
}

TEST_F(FileCommandsTest, TouchDirectoryError)
{
    fs::create_directory(test_dir_ / "dir");
    
    homeshell::TouchCommand cmd;
    homeshell::CommandContext ctx;
    ctx.args = {(test_dir_ / "dir").string()};
    
    testing::internal::CaptureStdout();
    auto status = cmd.execute(ctx);
    testing::internal::GetCapturedStdout();
    
    EXPECT_FALSE(status.isSuccess());
}

// Rm Tests
TEST_F(FileCommandsTest, RmGetName)
{
    homeshell::RmCommand cmd;
    EXPECT_EQ(cmd.getName(), "rm");
}

TEST_F(FileCommandsTest, RmRemoveFile)
{
    auto todelete = test_dir_ / "todelete.txt";
    std::ofstream(todelete) << "content";
    
    homeshell::RmCommand cmd;
    homeshell::CommandContext ctx;
    ctx.args = {todelete.string()};
    
    testing::internal::CaptureStdout();
    auto status = cmd.execute(ctx);
    testing::internal::GetCapturedStdout();
    
    EXPECT_TRUE(status.isSuccess());
    EXPECT_FALSE(fs::exists(todelete));
}

TEST_F(FileCommandsTest, RmRemoveDirectory)
{
    auto dirtoremove = test_dir_ / "dirtoremove";
    fs::create_directory(dirtoremove);
    std::ofstream(dirtoremove / "file.txt") << "content";
    
    homeshell::RmCommand cmd;
    homeshell::CommandContext ctx;
    ctx.args = {dirtoremove.string()};
    
    testing::internal::CaptureStdout();
    auto status = cmd.execute(ctx);
    testing::internal::GetCapturedStdout();
    
    EXPECT_TRUE(status.isSuccess());
    EXPECT_FALSE(fs::exists(dirtoremove));
}

TEST_F(FileCommandsTest, RmNoArguments)
{
    homeshell::RmCommand cmd;
    homeshell::CommandContext ctx;
    
    testing::internal::CaptureStdout();
    auto status = cmd.execute(ctx);
    testing::internal::GetCapturedStdout();
    
    EXPECT_FALSE(status.isSuccess());
}

TEST_F(FileCommandsTest, RmNonExistent)
{
    homeshell::RmCommand cmd;
    homeshell::CommandContext ctx;
    ctx.args = {(test_dir_ / "nonexistent.txt").string()};
    
    testing::internal::CaptureStdout();
    auto status = cmd.execute(ctx);
    testing::internal::GetCapturedStdout();
    
    EXPECT_FALSE(status.isSuccess());
}

