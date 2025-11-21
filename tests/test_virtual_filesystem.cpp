#include <homeshell/VirtualFilesystem.hpp>
#include <homeshell/EncryptedMount.hpp>
#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

class VirtualFilesystemTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        test_dir_ = fs::temp_directory_path() / "homeshell_vfs_test";
        fs::create_directories(test_dir_);
        
        db_path_ = test_dir_ / "test.db";
        password_ = "test_password";
        
        // Create a real file for testing
        real_file_ = test_dir_ / "real_file.txt";
        std::ofstream(real_file_) << "Real content";
    }

    void TearDown() override
    {
        // Remove all mounts
        auto& vfs = homeshell::VirtualFilesystem::getInstance();
        auto names = vfs.getMountNames();
        for (const auto& name : names)
        {
            vfs.removeMount(name);
        }
        
        if (fs::exists(test_dir_))
        {
            fs::remove_all(test_dir_);
        }
    }

    fs::path test_dir_;
    fs::path db_path_;
    fs::path real_file_;
    std::string password_;
};

TEST_F(VirtualFilesystemTest, AddAndRemoveMount)
{
    auto& vfs = homeshell::VirtualFilesystem::getInstance();
    
    auto mount = std::make_shared<homeshell::EncryptedMount>(
        "test", db_path_.string(), "/virtual", 10);
    ASSERT_TRUE(mount->mount(password_));
    
    EXPECT_TRUE(vfs.addMount(mount));
    EXPECT_EQ(vfs.getMountNames().size(), 1);
    EXPECT_EQ(vfs.getMountNames()[0], "test");
    
    EXPECT_TRUE(vfs.removeMount("test"));
    EXPECT_EQ(vfs.getMountNames().size(), 0);
}

TEST_F(VirtualFilesystemTest, GetMount)
{
    auto& vfs = homeshell::VirtualFilesystem::getInstance();
    
    auto mount = std::make_shared<homeshell::EncryptedMount>(
        "test", db_path_.string(), "/virtual", 10);
    ASSERT_TRUE(mount->mount(password_));
    vfs.addMount(mount);
    
    auto* retrieved = vfs.getMount("test");
    EXPECT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved->getName(), "test");
    
    auto* nonexistent = vfs.getMount("nonexistent");
    EXPECT_EQ(nonexistent, nullptr);
}

TEST_F(VirtualFilesystemTest, PathResolutionReal)
{
    auto& vfs = homeshell::VirtualFilesystem::getInstance();
    
    auto resolved = vfs.resolvePath(real_file_.string());
    EXPECT_EQ(resolved.type, homeshell::PathType::Real);
    EXPECT_EQ(resolved.full_path, real_file_.string());
}

TEST_F(VirtualFilesystemTest, PathResolutionVirtual)
{
    auto& vfs = homeshell::VirtualFilesystem::getInstance();
    
    auto mount = std::make_shared<homeshell::EncryptedMount>(
        "test", db_path_.string(), "/virtual", 10);
    ASSERT_TRUE(mount->mount(password_));
    vfs.addMount(mount);
    
    auto resolved = vfs.resolvePath("/virtual/file.txt");
    EXPECT_EQ(resolved.type, homeshell::PathType::Virtual);
    EXPECT_EQ(resolved.mount_point, "/virtual");
    EXPECT_EQ(resolved.relative_path, "/file.txt");
    EXPECT_NE(resolved.mount, nullptr);
}

TEST_F(VirtualFilesystemTest, ExistsReal)
{
    auto& vfs = homeshell::VirtualFilesystem::getInstance();
    
    EXPECT_TRUE(vfs.exists(real_file_.string()));
    EXPECT_FALSE(vfs.exists((test_dir_ / "nonexistent.txt").string()));
}

TEST_F(VirtualFilesystemTest, ExistsVirtual)
{
    auto& vfs = homeshell::VirtualFilesystem::getInstance();
    
    auto mount = std::make_shared<homeshell::EncryptedMount>(
        "test", db_path_.string(), "/virtual", 10);
    ASSERT_TRUE(mount->mount(password_));
    mount->writeFile("/test.txt", "content");
    vfs.addMount(mount);
    
    EXPECT_TRUE(vfs.exists("/virtual/test.txt"));
    EXPECT_FALSE(vfs.exists("/virtual/nonexistent.txt"));
}

TEST_F(VirtualFilesystemTest, WriteAndReadFileReal)
{
    auto& vfs = homeshell::VirtualFilesystem::getInstance();
    
    auto path = (test_dir_ / "new_file.txt").string();
    EXPECT_TRUE(vfs.writeFile(path, "New content"));
    
    std::string content;
    EXPECT_TRUE(vfs.readFile(path, content));
    EXPECT_EQ(content, "New content");
}

TEST_F(VirtualFilesystemTest, WriteAndReadFileVirtual)
{
    auto& vfs = homeshell::VirtualFilesystem::getInstance();
    
    auto mount = std::make_shared<homeshell::EncryptedMount>(
        "test", db_path_.string(), "/virtual", 10);
    ASSERT_TRUE(mount->mount(password_));
    vfs.addMount(mount);
    
    EXPECT_TRUE(vfs.writeFile("/virtual/test.txt", "Virtual content"));
    
    std::string content;
    EXPECT_TRUE(vfs.readFile("/virtual/test.txt", content));
    EXPECT_EQ(content, "Virtual content");
}

TEST_F(VirtualFilesystemTest, CreateDirectoryReal)
{
    auto& vfs = homeshell::VirtualFilesystem::getInstance();
    
    auto path = (test_dir_ / "new_dir").string();
    EXPECT_TRUE(vfs.createDirectory(path));
    EXPECT_TRUE(vfs.exists(path));
    EXPECT_TRUE(vfs.isDirectory(path));
}

TEST_F(VirtualFilesystemTest, CreateDirectoryVirtual)
{
    auto& vfs = homeshell::VirtualFilesystem::getInstance();
    
    auto mount = std::make_shared<homeshell::EncryptedMount>(
        "test", db_path_.string(), "/virtual", 10);
    ASSERT_TRUE(mount->mount(password_));
    vfs.addMount(mount);
    
    EXPECT_TRUE(vfs.createDirectory("/virtual/dir"));
    EXPECT_TRUE(vfs.exists("/virtual/dir"));
    EXPECT_TRUE(vfs.isDirectory("/virtual/dir"));
}

TEST_F(VirtualFilesystemTest, RemoveReal)
{
    auto& vfs = homeshell::VirtualFilesystem::getInstance();
    
    auto path = (test_dir_ / "to_remove.txt").string();
    std::ofstream(path) << "content";
    
    EXPECT_TRUE(vfs.exists(path));
    EXPECT_TRUE(vfs.remove(path));
    EXPECT_FALSE(vfs.exists(path));
}

TEST_F(VirtualFilesystemTest, RemoveVirtual)
{
    auto& vfs = homeshell::VirtualFilesystem::getInstance();
    
    auto mount = std::make_shared<homeshell::EncryptedMount>(
        "test", db_path_.string(), "/virtual", 10);
    ASSERT_TRUE(mount->mount(password_));
    mount->writeFile("/remove.txt", "content");
    vfs.addMount(mount);
    
    EXPECT_TRUE(vfs.exists("/virtual/remove.txt"));
    EXPECT_TRUE(vfs.remove("/virtual/remove.txt"));
    EXPECT_FALSE(vfs.exists("/virtual/remove.txt"));
}

TEST_F(VirtualFilesystemTest, ListDirectoryReal)
{
    auto& vfs = homeshell::VirtualFilesystem::getInstance();
    
    // Create some test files
    std::ofstream(test_dir_ / "file1.txt") << "content1";
    std::ofstream(test_dir_ / "file2.txt") << "content2";
    fs::create_directory(test_dir_ / "subdir");
    
    auto entries = vfs.listDirectory(test_dir_.string());
    EXPECT_GE(entries.size(), 4);  // real_file.txt, file1.txt, file2.txt, subdir, test.db
}

TEST_F(VirtualFilesystemTest, ListDirectoryVirtual)
{
    auto& vfs = homeshell::VirtualFilesystem::getInstance();
    
    auto mount = std::make_shared<homeshell::EncryptedMount>(
        "test", db_path_.string(), "/virtual", 10);
    ASSERT_TRUE(mount->mount(password_));
    mount->writeFile("/file1.txt", "content1");
    mount->writeFile("/file2.txt", "content2");
    mount->createDirectory("/dir");
    vfs.addMount(mount);
    
    auto entries = vfs.listDirectory("/virtual");
    EXPECT_EQ(entries.size(), 3);
}

TEST_F(VirtualFilesystemTest, MultipleMounts)
{
    auto& vfs = homeshell::VirtualFilesystem::getInstance();
    
    auto mount1 = std::make_shared<homeshell::EncryptedMount>(
        "mount1", (test_dir_ / "db1.db").string(), "/virtual1", 10);
    auto mount2 = std::make_shared<homeshell::EncryptedMount>(
        "mount2", (test_dir_ / "db2.db").string(), "/virtual2", 10);
    
    ASSERT_TRUE(mount1->mount(password_));
    ASSERT_TRUE(mount2->mount(password_));
    
    vfs.addMount(mount1);
    vfs.addMount(mount2);
    
    EXPECT_EQ(vfs.getMountNames().size(), 2);
    
    mount1->writeFile("/file1.txt", "mount1 content");
    mount2->writeFile("/file2.txt", "mount2 content");
    
    std::string content1, content2;
    EXPECT_TRUE(vfs.readFile("/virtual1/file1.txt", content1));
    EXPECT_TRUE(vfs.readFile("/virtual2/file2.txt", content2));
    
    EXPECT_EQ(content1, "mount1 content");
    EXPECT_EQ(content2, "mount2 content");
}

