#include <homeshell/commands/LsCommand.hpp>
#include <homeshell/FilesystemHelper.hpp>
#include <gtest/gtest.h>
#include <fstream>

using namespace homeshell;

class LsCommandTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Create a temporary test directory
        test_dir_ = "/tmp/homeshell_test_ls";
        std::filesystem::create_directory(test_dir_);
        
        // Create some test files
        std::ofstream(test_dir_ + "/file1.txt") << "test";
        std::ofstream(test_dir_ + "/file2.txt") << "test data";
        std::filesystem::create_directory(test_dir_ + "/subdir");
    }

    void TearDown() override
    {
        // Clean up test directory
        std::filesystem::remove_all(test_dir_);
    }

    std::string test_dir_;
};

TEST_F(LsCommandTest, GetName)
{
    LsCommand cmd;
    EXPECT_EQ(cmd.getName(), "ls");
}

TEST_F(LsCommandTest, GetDescription)
{
    LsCommand cmd;
    EXPECT_FALSE(cmd.getDescription().empty());
}

TEST_F(LsCommandTest, GetType)
{
    LsCommand cmd;
    EXPECT_EQ(cmd.getType(), CommandType::Synchronous);
}

TEST_F(LsCommandTest, BasicListing)
{
    LsCommand cmd;
    CommandContext ctx;
    ctx.args = {test_dir_};
    
    Status status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());
}

TEST_F(LsCommandTest, LongFormat)
{
    LsCommand cmd;
    CommandContext ctx;
    ctx.args = {"-l", test_dir_};
    
    Status status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());
}

TEST_F(LsCommandTest, HumanReadable)
{
    LsCommand cmd;
    CommandContext ctx;
    ctx.args = {"-lh", test_dir_};
    
    Status status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());
}

TEST_F(LsCommandTest, CombinedFlags)
{
    LsCommand cmd;
    CommandContext ctx;
    ctx.args = {"-hl", test_dir_};
    
    Status status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());
}

TEST_F(LsCommandTest, HelpFlag)
{
    LsCommand cmd;
    CommandContext ctx;
    ctx.args = {"-h"};
    
    Status status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());
}

TEST_F(LsCommandTest, HelpLongFlag)
{
    LsCommand cmd;
    CommandContext ctx;
    ctx.args = {"--help"};
    
    Status status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());
}

TEST_F(LsCommandTest, NonExistentPath)
{
    LsCommand cmd;
    CommandContext ctx;
    ctx.args = {"/nonexistent/path/12345"};
    
    Status status = cmd.execute(ctx);
    EXPECT_FALSE(status.isOk());
    EXPECT_NE(status.message.find("does not exist"), std::string::npos);
}

TEST_F(LsCommandTest, InvalidFlag)
{
    LsCommand cmd;
    CommandContext ctx;
    ctx.args = {"-x"};
    
    Status status = cmd.execute(ctx);
    EXPECT_FALSE(status.isOk());
    EXPECT_NE(status.message.find("Unknown flag"), std::string::npos);
}
