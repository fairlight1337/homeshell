#include <homeshell/EncryptedMount.hpp>
#include <gtest/gtest.h>
#include <filesystem>

namespace fs = std::filesystem;

class EncryptedMountTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        test_dir_ = fs::temp_directory_path() / "homeshell_encrypt_test";
        fs::create_directories(test_dir_);
        
        db_path_ = test_dir_ / "test.db";
        password_ = "test_password_123";
    }

    void TearDown() override
    {
        if (fs::exists(test_dir_))
        {
            fs::remove_all(test_dir_);
        }
    }

    fs::path test_dir_;
    fs::path db_path_;
    std::string password_;
};

TEST_F(EncryptedMountTest, CreateAndMount)
{
    homeshell::EncryptedMount mount("test", db_path_.string(), "/test", 10);
    
    EXPECT_FALSE(mount.is_mounted());
    EXPECT_EQ(mount.getName(), "test");
    EXPECT_EQ(mount.getMountPoint(), "/test");
    
    EXPECT_TRUE(mount.mount(password_));
    EXPECT_TRUE(mount.is_mounted());
}

TEST_F(EncryptedMountTest, MountWithWrongPassword)
{
    // Create and mount with correct password
    {
        homeshell::EncryptedMount mount("test", db_path_.string(), "/test", 10);
        EXPECT_TRUE(mount.mount(password_));
        mount.unmount();
    }
    
    // Try to mount with wrong password
    homeshell::EncryptedMount mount("test", db_path_.string(), "/test", 10);
    EXPECT_FALSE(mount.mount("wrong_password"));
}

TEST_F(EncryptedMountTest, CreateDirectory)
{
    homeshell::EncryptedMount mount("test", db_path_.string(), "/test", 10);
    ASSERT_TRUE(mount.mount(password_));
    
    EXPECT_TRUE(mount.createDirectory("/mydir"));
    EXPECT_TRUE(mount.exists("/mydir"));
    EXPECT_TRUE(mount.isDirectory("/mydir"));
}

TEST_F(EncryptedMountTest, CreateNestedDirectories)
{
    homeshell::EncryptedMount mount("test", db_path_.string(), "/test", 10);
    ASSERT_TRUE(mount.mount(password_));
    
    EXPECT_TRUE(mount.createDirectory("/parent/child/grandchild"));
    EXPECT_TRUE(mount.exists("/parent"));
    EXPECT_TRUE(mount.exists("/parent/child"));
    EXPECT_TRUE(mount.exists("/parent/child/grandchild"));
}

TEST_F(EncryptedMountTest, WriteAndReadFile)
{
    homeshell::EncryptedMount mount("test", db_path_.string(), "/test", 10);
    ASSERT_TRUE(mount.mount(password_));
    
    std::string content = "Hello, Encrypted World!";
    EXPECT_TRUE(mount.writeFile("/test.txt", content));
    
    std::string read_content;
    EXPECT_TRUE(mount.readFile("/test.txt", read_content));
    EXPECT_EQ(content, read_content);
}

TEST_F(EncryptedMountTest, OverwriteFile)
{
    homeshell::EncryptedMount mount("test", db_path_.string(), "/test", 10);
    ASSERT_TRUE(mount.mount(password_));
    
    EXPECT_TRUE(mount.writeFile("/test.txt", "Original"));
    EXPECT_TRUE(mount.writeFile("/test.txt", "Updated"));
    
    std::string content;
    EXPECT_TRUE(mount.readFile("/test.txt", content));
    EXPECT_EQ(content, "Updated");
}

TEST_F(EncryptedMountTest, ListDirectory)
{
    homeshell::EncryptedMount mount("test", db_path_.string(), "/test", 10);
    ASSERT_TRUE(mount.mount(password_));
    
    mount.createDirectory("/dir1");
    mount.createDirectory("/dir2");
    mount.writeFile("/file1.txt", "content1");
    mount.writeFile("/file2.txt", "content2");
    
    auto entries = mount.listDirectory("/");
    EXPECT_EQ(entries.size(), 4);
    
    // Check that all entries are present
    std::vector<std::string> names;
    for (const auto& entry : entries)
    {
        names.push_back(entry.name);
    }
    
    EXPECT_TRUE(std::find(names.begin(), names.end(), "dir1") != names.end());
    EXPECT_TRUE(std::find(names.begin(), names.end(), "dir2") != names.end());
    EXPECT_TRUE(std::find(names.begin(), names.end(), "file1.txt") != names.end());
    EXPECT_TRUE(std::find(names.begin(), names.end(), "file2.txt") != names.end());
}

TEST_F(EncryptedMountTest, RemoveFile)
{
    homeshell::EncryptedMount mount("test", db_path_.string(), "/test", 10);
    ASSERT_TRUE(mount.mount(password_));
    
    mount.writeFile("/todelete.txt", "content");
    EXPECT_TRUE(mount.exists("/todelete.txt"));
    
    EXPECT_TRUE(mount.remove("/todelete.txt"));
    EXPECT_FALSE(mount.exists("/todelete.txt"));
}

TEST_F(EncryptedMountTest, RemoveDirectory)
{
    homeshell::EncryptedMount mount("test", db_path_.string(), "/test", 10);
    ASSERT_TRUE(mount.mount(password_));
    
    mount.createDirectory("/dir");
    mount.writeFile("/dir/file.txt", "content");
    
    EXPECT_TRUE(mount.remove("/dir"));
    EXPECT_FALSE(mount.exists("/dir"));
    EXPECT_FALSE(mount.exists("/dir/file.txt"));
}

TEST_F(EncryptedMountTest, GetUsedSpace)
{
    homeshell::EncryptedMount mount("test", db_path_.string(), "/test", 10);
    ASSERT_TRUE(mount.mount(password_));
    
    int64_t initial = mount.getUsedSpace();
    
    std::string content(1000, 'x');  // 1000 bytes
    mount.writeFile("/large.txt", content);
    
    int64_t after = mount.getUsedSpace();
    EXPECT_GT(after, initial);
    EXPECT_GE(after, 1000);
}

TEST_F(EncryptedMountTest, PersistenceAcrossSessions)
{
    // Create and write in first session
    {
        homeshell::EncryptedMount mount("test", db_path_.string(), "/test", 10);
        ASSERT_TRUE(mount.mount(password_));
        mount.writeFile("/persistent.txt", "This persists");
        mount.unmount();
    }
    
    // Read in second session
    {
        homeshell::EncryptedMount mount("test", db_path_.string(), "/test", 10);
        ASSERT_TRUE(mount.mount(password_));
        
        std::string content;
        EXPECT_TRUE(mount.readFile("/persistent.txt", content));
        EXPECT_EQ(content, "This persists");
    }
}

TEST_F(EncryptedMountTest, RootDirectoryExists)
{
    homeshell::EncryptedMount mount("test", db_path_.string(), "/test", 10);
    ASSERT_TRUE(mount.mount(password_));
    
    EXPECT_TRUE(mount.exists("/"));
    EXPECT_TRUE(mount.isDirectory("/"));
}

