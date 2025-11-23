#include <gtest/gtest.h>
#include <homeshell/commands/ZipCommand.hpp>
#include <homeshell/commands/UnzipCommand.hpp>
#include <homeshell/commands/ZipInfoCommand.hpp>
#include <homeshell/VirtualFilesystem.hpp>
#include <filesystem>
#include <fstream>

using namespace homeshell;

class ArchiveCommandsTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Save original directory
        original_dir_ = std::filesystem::current_path();
        
        // Create a temporary test directory
        test_dir_ = std::filesystem::temp_directory_path() / "homeshell_archive_test";
        std::filesystem::remove_all(test_dir_);
        std::filesystem::create_directories(test_dir_);
        
        // Change to test directory
        std::filesystem::current_path(test_dir_);
        
        // Reset VirtualFilesystem to clean state
        auto& vfs = VirtualFilesystem::getInstance();
        std::string result;
        vfs.changeDirectory(test_dir_.string(), result);
        
        // Create test files and directories
        createTestFiles();
    }

    void TearDown() override
    {
        // Restore original directory
        std::filesystem::current_path(original_dir_);
        
        // Clean up test directory
        std::filesystem::remove_all(test_dir_);
    }

    void createTestFiles()
    {
        // Create some test files
        writeFile("file1.txt", "Hello World");
        writeFile("file2.txt", "Test Data");
        
        // Create a subdirectory with files
        std::filesystem::create_directories(test_dir_ / "subdir");
        writeFile("subdir/file3.txt", "Nested File");
        writeFile("subdir/file4.txt", "Another Nested File");
        
        // Create a deeper structure
        std::filesystem::create_directories(test_dir_ / "subdir" / "deeper");
        writeFile("subdir/deeper/file5.txt", "Deep File");
    }

    void writeFile(const std::string& path, const std::string& content)
    {
        std::ofstream file(test_dir_ / path);
        file << content;
    }

    std::string readFile(const std::string& path)
    {
        std::ifstream file(path);
        return std::string((std::istreambuf_iterator<char>(file)),
                          std::istreambuf_iterator<char>());
    }

    std::filesystem::path test_dir_;
    std::filesystem::path original_dir_;
};

// ZipCommand Tests
TEST_F(ArchiveCommandsTest, ZipCommandGetName)
{
    ZipCommand cmd;
    EXPECT_EQ(cmd.getName(), "zip");
}

TEST_F(ArchiveCommandsTest, ZipCommandGetDescription)
{
    ZipCommand cmd;
    EXPECT_FALSE(cmd.getDescription().empty());
}

TEST_F(ArchiveCommandsTest, ZipCommandGetType)
{
    ZipCommand cmd;
    EXPECT_EQ(cmd.getType(), CommandType::Synchronous);
}

TEST_F(ArchiveCommandsTest, ZipCreateArchiveSingleFile)
{
    ZipCommand cmd;
    CommandContext ctx;
    ctx.args = {"test.zip", "file1.txt"};
    
    Status status = cmd.execute(ctx);
    EXPECT_TRUE(status.isSuccess());
    EXPECT_TRUE(std::filesystem::exists("test.zip"));
}

TEST_F(ArchiveCommandsTest, ZipCreateArchiveMultipleFiles)
{
    ZipCommand cmd;
    CommandContext ctx;
    ctx.args = {"test.zip", "file1.txt", "file2.txt"};
    
    Status status = cmd.execute(ctx);
    EXPECT_TRUE(status.isSuccess());
    EXPECT_TRUE(std::filesystem::exists("test.zip"));
}

TEST_F(ArchiveCommandsTest, ZipCreateArchiveWithDirectory)
{
    ZipCommand cmd;
    CommandContext ctx;
    ctx.args = {"test.zip", "subdir"};
    
    Status status = cmd.execute(ctx);
    EXPECT_TRUE(status.isSuccess());
    EXPECT_TRUE(std::filesystem::exists("test.zip"));
}

TEST_F(ArchiveCommandsTest, ZipInsufficientArguments)
{
    ZipCommand cmd;
    CommandContext ctx;
    ctx.args = {"test.zip"};  // Missing files
    
    Status status = cmd.execute(ctx);
    EXPECT_TRUE(status.isSuccess() == false);
}

TEST_F(ArchiveCommandsTest, ZipNonexistentFile)
{
    ZipCommand cmd;
    CommandContext ctx;
    ctx.args = {"test.zip", "nonexistent.txt"};
    
    Status status = cmd.execute(ctx);
    EXPECT_TRUE(status.isSuccess() == false);
}

// UnzipCommand Tests
TEST_F(ArchiveCommandsTest, UnzipCommandGetName)
{
    UnzipCommand cmd;
    EXPECT_EQ(cmd.getName(), "unzip");
}

TEST_F(ArchiveCommandsTest, UnzipCommandGetDescription)
{
    UnzipCommand cmd;
    EXPECT_FALSE(cmd.getDescription().empty());
}

TEST_F(ArchiveCommandsTest, UnzipCommandGetType)
{
    UnzipCommand cmd;
    EXPECT_EQ(cmd.getType(), CommandType::Synchronous);
}

TEST_F(ArchiveCommandsTest, UnzipExtractArchive)
{
    // First create an archive
    ZipCommand zip_cmd;
    CommandContext zip_ctx;
    zip_ctx.args = {"test.zip", "file1.txt", "file2.txt"};
    zip_cmd.execute(zip_ctx);
    
    // Create extraction directory
    std::filesystem::create_directories("extract");
    
    // Extract the archive
    UnzipCommand unzip_cmd;
    CommandContext unzip_ctx;
    unzip_ctx.args = {"test.zip", "-o", "extract"};
    
    Status status = unzip_cmd.execute(unzip_ctx);
    EXPECT_TRUE(status.isSuccess());
    EXPECT_TRUE(std::filesystem::exists("extract/file1.txt"));
    EXPECT_TRUE(std::filesystem::exists("extract/file2.txt"));
}

TEST_F(ArchiveCommandsTest, UnzipPreserveDirectoryStructure)
{
    // Create an archive with directory structure
    ZipCommand zip_cmd;
    CommandContext zip_ctx;
    zip_ctx.args = {"test.zip", "subdir"};
    zip_cmd.execute(zip_ctx);
    
    // Extract
    std::filesystem::create_directories("extract");
    UnzipCommand unzip_cmd;
    CommandContext unzip_ctx;
    unzip_ctx.args = {"test.zip", "-o", "extract"};
    
    Status status = unzip_cmd.execute(unzip_ctx);
    EXPECT_TRUE(status.isSuccess());
    EXPECT_TRUE(std::filesystem::exists("extract/subdir/file3.txt"));
    EXPECT_TRUE(std::filesystem::exists("extract/subdir/deeper/file5.txt"));
}

TEST_F(ArchiveCommandsTest, UnzipJunkPaths)
{
    // Create an archive with directory structure
    ZipCommand zip_cmd;
    CommandContext zip_ctx;
    zip_ctx.args = {"test.zip", "subdir"};
    zip_cmd.execute(zip_ctx);
    
    // Extract with -j flag (junk paths)
    std::filesystem::create_directories("extract_flat");
    UnzipCommand unzip_cmd;
    CommandContext unzip_ctx;
    unzip_ctx.args = {"test.zip", "-o", "extract_flat", "-j"};
    
    Status status = unzip_cmd.execute(unzip_ctx);
    EXPECT_TRUE(status.isSuccess());
    
    // Files should be extracted flat (no subdirectories)
    EXPECT_TRUE(std::filesystem::exists("extract_flat/file3.txt"));
    EXPECT_TRUE(std::filesystem::exists("extract_flat/file4.txt"));
    EXPECT_TRUE(std::filesystem::exists("extract_flat/file5.txt"));
}

TEST_F(ArchiveCommandsTest, UnzipNonexistentArchive)
{
    UnzipCommand cmd;
    CommandContext ctx;
    ctx.args = {"nonexistent.zip"};
    
    Status status = cmd.execute(ctx);
    EXPECT_TRUE(status.isSuccess() == false);
}

TEST_F(ArchiveCommandsTest, UnzipNoArguments)
{
    UnzipCommand cmd;
    CommandContext ctx;
    ctx.args = {};
    
    Status status = cmd.execute(ctx);
    EXPECT_TRUE(status.isSuccess() == false);
}

// ZipInfoCommand Tests
TEST_F(ArchiveCommandsTest, ZipInfoCommandGetName)
{
    ZipInfoCommand cmd;
    EXPECT_EQ(cmd.getName(), "zipinfo");
}

TEST_F(ArchiveCommandsTest, ZipInfoCommandGetDescription)
{
    ZipInfoCommand cmd;
    EXPECT_FALSE(cmd.getDescription().empty());
}

TEST_F(ArchiveCommandsTest, ZipInfoCommandGetType)
{
    ZipInfoCommand cmd;
    EXPECT_EQ(cmd.getType(), CommandType::Synchronous);
}

TEST_F(ArchiveCommandsTest, ZipInfoListContents)
{
    // Create an archive
    ZipCommand zip_cmd;
    CommandContext zip_ctx;
    zip_ctx.args = {"test.zip", "file1.txt", "file2.txt", "subdir"};
    zip_cmd.execute(zip_ctx);
    
    // List contents
    ZipInfoCommand zipinfo_cmd;
    CommandContext zipinfo_ctx;
    zipinfo_ctx.args = {"test.zip"};
    
    Status status = zipinfo_cmd.execute(zipinfo_ctx);
    EXPECT_TRUE(status.isSuccess());
}

TEST_F(ArchiveCommandsTest, ZipInfoNonexistentArchive)
{
    ZipInfoCommand cmd;
    CommandContext ctx;
    ctx.args = {"nonexistent.zip"};
    
    Status status = cmd.execute(ctx);
    EXPECT_TRUE(status.isSuccess() == false);
}

TEST_F(ArchiveCommandsTest, ZipInfoNoArguments)
{
    ZipInfoCommand cmd;
    CommandContext ctx;
    ctx.args = {};
    
    Status status = cmd.execute(ctx);
    EXPECT_TRUE(status.isSuccess() == false);
}

// Integration test: Zip -> ZipInfo -> Unzip -> Verify
TEST_F(ArchiveCommandsTest, FullWorkflow)
{
    // Step 1: Create archive
    ZipCommand zip_cmd;
    CommandContext zip_ctx;
    zip_ctx.args = {"workflow.zip", "file1.txt", "subdir"};
    Status zip_status = zip_cmd.execute(zip_ctx);
    EXPECT_TRUE(zip_status.isSuccess());
    
    // Step 2: List contents
    ZipInfoCommand zipinfo_cmd;
    CommandContext zipinfo_ctx;
    zipinfo_ctx.args = {"workflow.zip"};
    Status zipinfo_status = zipinfo_cmd.execute(zipinfo_ctx);
    EXPECT_TRUE(zipinfo_status.isSuccess());
    
    // Step 3: Extract
    std::filesystem::create_directories("verify");
    UnzipCommand unzip_cmd;
    CommandContext unzip_ctx;
    unzip_ctx.args = {"workflow.zip", "-o", "verify"};
    Status unzip_status = unzip_cmd.execute(unzip_ctx);
    EXPECT_TRUE(unzip_status.isSuccess());
    
    // Step 4: Verify extracted files
    EXPECT_TRUE(std::filesystem::exists("verify/file1.txt"));
    EXPECT_TRUE(std::filesystem::exists("verify/subdir/file3.txt"));
    
    // Verify content
    std::string content = readFile("verify/file1.txt");
    EXPECT_EQ(content, "Hello World");
}

