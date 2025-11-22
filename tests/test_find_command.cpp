#include <gtest/gtest.h>
#include <homeshell/commands/FindCommand.hpp>
#include <homeshell/VirtualFilesystem.hpp>

#include <filesystem>
#include <fstream>

namespace homeshell
{

class FindCommandTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        command_ = std::make_shared<FindCommand>();
        test_dir_ = "/tmp/test_find_cmd";
        
        // Create test directory structure
        std::filesystem::create_directories(test_dir_ + "/dir1/subdir");
        std::filesystem::create_directories(test_dir_ + "/dir2");
        
        // Create test files
        createFile(test_dir_ + "/file1.txt");
        createFile(test_dir_ + "/file2.cpp");
        createFile(test_dir_ + "/README.md");
        createFile(test_dir_ + "/Test.TXT");
        createFile(test_dir_ + "/dir1/test.txt");
        createFile(test_dir_ + "/dir1/data.json");
        createFile(test_dir_ + "/dir1/subdir/deep.txt");
        createFile(test_dir_ + "/dir2/config.yml");
    }

    void TearDown() override
    {
        // Clean up test directory
        std::filesystem::remove_all(test_dir_);
    }

    void createFile(const std::string& path)
    {
        std::ofstream file(path);
        file << "test content\n";
        file.close();
    }

    std::shared_ptr<FindCommand> command_;
    std::string test_dir_;
};

TEST_F(FindCommandTest, BasicInfo)
{
    EXPECT_EQ(command_->getName(), "find");
    EXPECT_FALSE(command_->getDescription().empty());
    EXPECT_EQ(command_->getType(), CommandType::Synchronous);
}

TEST_F(FindCommandTest, FindAll)
{
    CommandContext context;
    context.args = {test_dir_};
    context.use_colors = false;
    
    testing::internal::CaptureStdout();
    Status status = command_->execute(context);
    std::string output = testing::internal::GetCapturedStdout();
    
    EXPECT_TRUE(status.isSuccess());
    EXPECT_NE(output.find("file1.txt"), std::string::npos);
    EXPECT_NE(output.find("file2.cpp"), std::string::npos);
    EXPECT_NE(output.find("deep.txt"), std::string::npos);
}

TEST_F(FindCommandTest, FindByNameLiteral)
{
    CommandContext context;
    context.args = {test_dir_, "-name", "test.txt"};
    context.use_colors = false;
    
    testing::internal::CaptureStdout();
    Status status = command_->execute(context);
    std::string output = testing::internal::GetCapturedStdout();
    
    EXPECT_TRUE(status.isSuccess());
    EXPECT_NE(output.find("dir1/test.txt"), std::string::npos);
    EXPECT_EQ(output.find("Test.TXT"), std::string::npos); // Case sensitive
}

TEST_F(FindCommandTest, FindByNameCaseInsensitive)
{
    CommandContext context;
    context.args = {test_dir_, "-iname", "test.txt"};
    context.use_colors = false;
    
    testing::internal::CaptureStdout();
    Status status = command_->execute(context);
    std::string output = testing::internal::GetCapturedStdout();
    
    EXPECT_TRUE(status.isSuccess());
    EXPECT_NE(output.find("dir1/test.txt"), std::string::npos);
    EXPECT_NE(output.find("Test.TXT"), std::string::npos); // Should match
}

TEST_F(FindCommandTest, FindByTypeFile)
{
    CommandContext context;
    context.args = {test_dir_, "-type", "f"};
    context.use_colors = false;
    
    testing::internal::CaptureStdout();
    Status status = command_->execute(context);
    std::string output = testing::internal::GetCapturedStdout();
    
    EXPECT_TRUE(status.isSuccess());
    EXPECT_NE(output.find("file1.txt"), std::string::npos);
    EXPECT_EQ(output.find("/dir1\n"), std::string::npos); // No directories
    EXPECT_EQ(output.find("/dir2\n"), std::string::npos);
}

TEST_F(FindCommandTest, FindByTypeDirectory)
{
    CommandContext context;
    context.args = {test_dir_, "-type", "d"};
    context.use_colors = false;
    
    testing::internal::CaptureStdout();
    Status status = command_->execute(context);
    std::string output = testing::internal::GetCapturedStdout();
    
    EXPECT_TRUE(status.isSuccess());
    EXPECT_NE(output.find("/dir1"), std::string::npos);
    EXPECT_NE(output.find("/dir2"), std::string::npos);
    EXPECT_NE(output.find("/subdir"), std::string::npos);
    EXPECT_EQ(output.find("file1.txt"), std::string::npos); // No files
}

TEST_F(FindCommandTest, FindWithMaxDepth1)
{
    CommandContext context;
    context.args = {test_dir_, "-maxdepth", "1"};
    context.use_colors = false;
    
    testing::internal::CaptureStdout();
    Status status = command_->execute(context);
    std::string output = testing::internal::GetCapturedStdout();
    
    EXPECT_TRUE(status.isSuccess());
    // Should find direct children
    EXPECT_NE(output.find("file1.txt"), std::string::npos);
    EXPECT_NE(output.find("/dir1"), std::string::npos);
    // Should NOT find grandchildren
    EXPECT_EQ(output.find("dir1/test.txt"), std::string::npos);
    EXPECT_EQ(output.find("deep.txt"), std::string::npos);
}

TEST_F(FindCommandTest, FindWithMaxDepth2)
{
    CommandContext context;
    context.args = {test_dir_, "-maxdepth", "2"};
    context.use_colors = false;
    
    testing::internal::CaptureStdout();
    Status status = command_->execute(context);
    std::string output = testing::internal::GetCapturedStdout();
    
    EXPECT_TRUE(status.isSuccess());
    // Should find up to grandchildren
    EXPECT_NE(output.find("file1.txt"), std::string::npos);
    EXPECT_NE(output.find("dir1/test.txt"), std::string::npos);
    EXPECT_NE(output.find("/subdir"), std::string::npos);
    // Should NOT find great-grandchildren
    EXPECT_EQ(output.find("deep.txt"), std::string::npos);
}

TEST_F(FindCommandTest, CombineNameAndType)
{
    CommandContext context;
    context.args = {test_dir_, "-name", "test.txt", "-type", "f"};
    context.use_colors = false;
    
    testing::internal::CaptureStdout();
    Status status = command_->execute(context);
    std::string output = testing::internal::GetCapturedStdout();
    
    EXPECT_TRUE(status.isSuccess());
    EXPECT_NE(output.find("dir1/test.txt"), std::string::npos);
}

TEST_F(FindCommandTest, NonexistentPath)
{
    CommandContext context;
    context.args = {"/nonexistent/path"};
    
    Status status = command_->execute(context);
    
    EXPECT_FALSE(status.isSuccess());
    EXPECT_NE(status.message.find("not found"), std::string::npos);
}

TEST_F(FindCommandTest, InvalidTypeArgument)
{
    CommandContext context;
    context.args = {test_dir_, "-type", "x"};
    
    Status status = command_->execute(context);
    
    EXPECT_FALSE(status.isSuccess());
    EXPECT_NE(status.message.find("Invalid type"), std::string::npos);
}

TEST_F(FindCommandTest, HelpOption)
{
    CommandContext context;
    context.args = {"--help"};
    
    testing::internal::CaptureStdout();
    Status status = command_->execute(context);
    std::string output = testing::internal::GetCapturedStdout();
    
    EXPECT_TRUE(status.isSuccess());
    EXPECT_NE(output.find("Usage"), std::string::npos);
    EXPECT_NE(output.find("-name"), std::string::npos);
    EXPECT_NE(output.find("-type"), std::string::npos);
}

} // namespace homeshell

