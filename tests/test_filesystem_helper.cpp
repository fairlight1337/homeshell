#include <homeshell/FilesystemHelper.hpp>
#include <gtest/gtest.h>
#include <fstream>

using namespace homeshell;

TEST(FilesystemHelperTest, GetCurrentDirectory)
{
    auto cwd = FilesystemHelper::getCurrentDirectory();
    EXPECT_FALSE(cwd.empty());
    EXPECT_TRUE(cwd.is_absolute());
}

TEST(FilesystemHelperTest, ChangeDirectory)
{
    auto original_cwd = FilesystemHelper::getCurrentDirectory();

    // Change to parent directory
    bool success = FilesystemHelper::changeDirectory("..");
    EXPECT_TRUE(success);

    auto new_cwd = FilesystemHelper::getCurrentDirectory();
    EXPECT_NE(original_cwd, new_cwd);

    // Change back
    FilesystemHelper::changeDirectory(original_cwd.string());
    auto restored_cwd = FilesystemHelper::getCurrentDirectory();
    EXPECT_EQ(original_cwd, restored_cwd);
}

TEST(FilesystemHelperTest, Exists)
{
    EXPECT_TRUE(FilesystemHelper::exists("."));
    EXPECT_TRUE(FilesystemHelper::exists(".."));
    EXPECT_FALSE(FilesystemHelper::exists("/nonexistent/path/12345"));
}

TEST(FilesystemHelperTest, IsDirectory)
{
    EXPECT_TRUE(FilesystemHelper::isDirectory("."));
    EXPECT_TRUE(FilesystemHelper::isDirectory(".."));

    // Create a temporary file
    std::string temp_file = "/tmp/test_file.txt";
    {
        std::ofstream file(temp_file);
        file << "test";
    }

    EXPECT_FALSE(FilesystemHelper::isDirectory(temp_file));
    EXPECT_TRUE(FilesystemHelper::isRegularFile(temp_file));

    // Clean up
    std::remove(temp_file.c_str());
}

TEST(FilesystemHelperTest, FormatSize)
{
    EXPECT_EQ(FilesystemHelper::formatSize(0), "0 B");
    EXPECT_EQ(FilesystemHelper::formatSize(500), "500 B");
    EXPECT_EQ(FilesystemHelper::formatSize(1024), "1.0 KB");
    EXPECT_EQ(FilesystemHelper::formatSize(1024 * 1024), "1.0 MB");
    EXPECT_EQ(FilesystemHelper::formatSize(1536 * 1024), "1.5 MB");
    EXPECT_EQ(FilesystemHelper::formatSize(1024ULL * 1024 * 1024), "1.0 GB");
}

TEST(FilesystemHelperTest, ListDirectory)
{
    // List current directory
    auto entries = FilesystemHelper::listDirectory(".");
    EXPECT_FALSE(entries.empty());

    // Verify entries have valid properties
    for (const auto& entry : entries)
    {
        EXPECT_FALSE(entry.name.empty());
        EXPECT_FALSE(entry.permissions.empty());
    }
}

