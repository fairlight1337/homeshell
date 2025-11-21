#include <homeshell/commands/CatCommand.hpp>
#include <gtest/gtest.h>
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

class CatCommandTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Create temporary directory for test files
        test_dir_ = fs::temp_directory_path() / "homeshell_cat_test";
        fs::create_directories(test_dir_);

        // Create test files
        test_file_ = test_dir_ / "test.txt";
        std::ofstream file(test_file_);
        file << "Hello World\nLine 2\nLine 3";
        file.close();

        empty_file_ = test_dir_ / "empty.txt";
        std::ofstream(empty_file_).close();

        test_dir_path_ = test_dir_ / "subdir";
        fs::create_directory(test_dir_path_);
    }

    void TearDown() override
    {
        // Clean up test directory
        if (fs::exists(test_dir_))
        {
            fs::remove_all(test_dir_);
        }
    }

    fs::path test_dir_;
    fs::path test_file_;
    fs::path empty_file_;
    fs::path test_dir_path_;
};

TEST_F(CatCommandTest, GetName)
{
    homeshell::CatCommand cmd;
    EXPECT_EQ(cmd.getName(), "cat");
}

TEST_F(CatCommandTest, GetType)
{
    homeshell::CatCommand cmd;
    EXPECT_EQ(cmd.getType(), homeshell::CommandType::Synchronous);
}

TEST_F(CatCommandTest, NoArguments)
{
    homeshell::CatCommand cmd;
    homeshell::CommandContext ctx;
    
    auto status = cmd.execute(ctx);
    EXPECT_FALSE(status.isSuccess());
}

TEST_F(CatCommandTest, ReadFile)
{
    homeshell::CatCommand cmd;
    homeshell::CommandContext ctx;
    ctx.args = {test_file_.string()};
    
    // Redirect stdout to capture output
    testing::internal::CaptureStdout();
    auto status = cmd.execute(ctx);
    std::string output = testing::internal::GetCapturedStdout();
    
    EXPECT_TRUE(status.isSuccess());
    EXPECT_TRUE(output.find("Hello World") != std::string::npos);
    EXPECT_TRUE(output.find("Line 2") != std::string::npos);
    EXPECT_TRUE(output.find("Line 3") != std::string::npos);
}

TEST_F(CatCommandTest, ReadEmptyFile)
{
    homeshell::CatCommand cmd;
    homeshell::CommandContext ctx;
    ctx.args = {empty_file_.string()};
    
    testing::internal::CaptureStdout();
    auto status = cmd.execute(ctx);
    std::string output = testing::internal::GetCapturedStdout();
    
    EXPECT_TRUE(status.isSuccess());
    EXPECT_TRUE(output.empty() || output == "\n");
}

TEST_F(CatCommandTest, NonExistentFile)
{
    homeshell::CatCommand cmd;
    homeshell::CommandContext ctx;
    ctx.args = {(test_dir_ / "nonexistent.txt").string()};
    
    testing::internal::CaptureStdout();
    auto status = cmd.execute(ctx);
    testing::internal::GetCapturedStdout();
    
    EXPECT_FALSE(status.isSuccess());
}

TEST_F(CatCommandTest, DirectoryError)
{
    homeshell::CatCommand cmd;
    homeshell::CommandContext ctx;
    ctx.args = {test_dir_path_.string()};
    
    testing::internal::CaptureStdout();
    auto status = cmd.execute(ctx);
    testing::internal::GetCapturedStdout();
    
    EXPECT_FALSE(status.isSuccess());
}

