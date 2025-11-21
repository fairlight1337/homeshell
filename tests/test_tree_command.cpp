#include <gtest/gtest.h>
#include <homeshell/commands/TreeCommand.hpp>
#include <homeshell/VirtualFilesystem.hpp>
#include <filesystem>
#include <fstream>

using namespace homeshell;

class TreeCommandTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Save original directory first
        original_dir_ = std::filesystem::current_path();
        
        // Create a temporary test directory
        test_dir_ = std::filesystem::temp_directory_path() / "homeshell_tree_test";
        std::filesystem::remove_all(test_dir_);
        std::filesystem::create_directories(test_dir_);
        
        // Reset VirtualFilesystem to clean state
        auto& vfs = VirtualFilesystem::getInstance();
        std::string result;
        vfs.changeDirectory(original_dir_.string(), result);
        
        // Create test directory structure
        createTestStructure();
    }

    void TearDown() override
    {
        // Restore original directory
        std::filesystem::current_path(original_dir_);
        
        // Clean up test directory
        std::filesystem::remove_all(test_dir_);
    }

    void createTestStructure()
    {
        // Create a multi-level directory structure
        std::filesystem::create_directories(test_dir_ / "dir1" / "subdir1");
        std::filesystem::create_directories(test_dir_ / "dir1" / "subdir2");
        std::filesystem::create_directories(test_dir_ / "dir2");
        std::filesystem::create_directories(test_dir_ / "dir3" / "deep" / "deeper");
        
        // Create files
        writeFile("file1.txt", "content1");
        writeFile("file2.txt", "content2");
        writeFile("dir1/file3.txt", "content3");
        writeFile("dir1/subdir1/file4.txt", "content4");
        writeFile("dir1/subdir2/file5.txt", "content5");
        writeFile("dir2/file6.txt", "content6");
        writeFile("dir3/deep/deeper/file7.txt", "content7");
    }

    void writeFile(const std::string& path, const std::string& content)
    {
        std::ofstream file(test_dir_ / path);
        file << content;
    }

    std::filesystem::path test_dir_;
    std::filesystem::path original_dir_;
};

TEST_F(TreeCommandTest, GetName)
{
    TreeCommand cmd;
    EXPECT_EQ(cmd.getName(), "tree");
}

TEST_F(TreeCommandTest, GetDescription)
{
    TreeCommand cmd;
    EXPECT_FALSE(cmd.getDescription().empty());
}

TEST_F(TreeCommandTest, GetType)
{
    TreeCommand cmd;
    EXPECT_EQ(cmd.getType(), CommandType::Synchronous);
}

TEST_F(TreeCommandTest, TreeCurrentDirectory)
{
    // Change to test directory
    std::filesystem::current_path(test_dir_);
    
    TreeCommand cmd;
    CommandContext ctx;
    ctx.args = {};  // No arguments, should use current directory
    
    Status status = cmd.execute(ctx);
    EXPECT_TRUE(status.isSuccess());
}

TEST_F(TreeCommandTest, TreeSpecificDirectory)
{
    TreeCommand cmd;
    CommandContext ctx;
    ctx.args = {test_dir_.string()};
    
    Status status = cmd.execute(ctx);
    EXPECT_TRUE(status.isSuccess());
}

TEST_F(TreeCommandTest, TreeSubdirectory)
{
    TreeCommand cmd;
    CommandContext ctx;
    ctx.args = {(test_dir_ / "dir1").string()};
    
    Status status = cmd.execute(ctx);
    EXPECT_TRUE(status.isSuccess());
}

TEST_F(TreeCommandTest, TreeNonexistentDirectory)
{
    TreeCommand cmd;
    CommandContext ctx;
    ctx.args = {(test_dir_ / "nonexistent").string()};
    
    Status status = cmd.execute(ctx);
    EXPECT_TRUE(status.isSuccess() == false);
}

TEST_F(TreeCommandTest, TreeFile)
{
    TreeCommand cmd;
    CommandContext ctx;
    ctx.args = {(test_dir_ / "file1.txt").string()};
    
    Status status = cmd.execute(ctx);
    EXPECT_TRUE(status.isSuccess() == false);
}

TEST_F(TreeCommandTest, TreeEmptyDirectory)
{
    std::filesystem::path empty_dir = test_dir_ / "empty";
    std::filesystem::create_directories(empty_dir);
    
    TreeCommand cmd;
    CommandContext ctx;
    ctx.args = {empty_dir.string()};
    
    Status status = cmd.execute(ctx);
    EXPECT_TRUE(status.isSuccess());
}

TEST_F(TreeCommandTest, TreeDeepStructure)
{
    TreeCommand cmd;
    CommandContext ctx;
    ctx.args = {(test_dir_ / "dir3").string()};
    
    Status status = cmd.execute(ctx);
    EXPECT_TRUE(status.isSuccess());
}

TEST_F(TreeCommandTest, SupportsCancellation)
{
    TreeCommand cmd;
    EXPECT_FALSE(cmd.supportsCancellation());
}

