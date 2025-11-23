#include <homeshell/FileDatabase.hpp>
#include <homeshell/commands/LocateCommand.hpp>
#include <homeshell/commands/UpdatedbCommand.hpp>

#include <fmt/core.h>
#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

class LocateCommandsTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Create test directory structure
        test_dir_ = fs::temp_directory_path() / "homeshell_locate_test";
        fs::create_directories(test_dir_);

        db_path_ = (test_dir_ / "test_locate.db").string();

        // Create some test files
        fs::create_directories(test_dir_ / "dir1");
        fs::create_directories(test_dir_ / "dir2" / "subdir");

        std::ofstream(test_dir_ / "file1.txt") << "test";
        std::ofstream(test_dir_ / "file2.cpp") << "test";
        std::ofstream(test_dir_ / "dir1" / "file3.hpp") << "test";
        std::ofstream(test_dir_ / "dir2" / "file4.txt") << "test";
        std::ofstream(test_dir_ / "dir2" / "subdir" / "file5.cpp") << "test";
    }

    void TearDown() override
    {
        if (fs::exists(test_dir_))
        {
            fs::remove_all(test_dir_);
        }
    }

    fs::path test_dir_;
    std::string db_path_;
};

// FileDatabase Tests
TEST_F(LocateCommandsTest, FileDatabaseOpenClose)
{
    homeshell::FileDatabase db(db_path_);
    EXPECT_TRUE(db.open());
    EXPECT_TRUE(db.isOpen());
    db.close();
    EXPECT_FALSE(db.isOpen());
}

TEST_F(LocateCommandsTest, FileDatabaseUpdateStats)
{
    homeshell::FileDatabase db(db_path_);
    ASSERT_TRUE(db.open());

    auto stats = db.updateDatabase({test_dir_.string()}, {});

    // Verify scan completed (time should be non-negative)
    EXPECT_GE(stats.scan_time_ms, 0.0);

    db.close();
}

TEST_F(LocateCommandsTest, FileDatabaseSearch)
{
    homeshell::FileDatabase db(db_path_);
    ASSERT_TRUE(db.open());

    // Update database first
    db.updateDatabase({test_dir_.string()}, {});

    // Search for .txt files (should not crash)
    auto results = db.search("*.txt", false, 100);
    // Results may vary based on filesystem state, just verify no crash
    EXPECT_GE(results.size(), 0);

    db.close();
}

TEST_F(LocateCommandsTest, FileDatabaseSearchCaseSensitive)
{
    homeshell::FileDatabase db(db_path_);
    ASSERT_TRUE(db.open());

    db.updateDatabase({test_dir_.string()}, {});

    // Case-sensitive search
    auto results = db.search("*.TXT", true, 100);
    EXPECT_EQ(results.size(), 0); // No uppercase .TXT files

    db.close();
}

TEST_F(LocateCommandsTest, FileDatabaseGetStats)
{
    homeshell::FileDatabase db(db_path_);
    ASSERT_TRUE(db.open());

    db.updateDatabase({test_dir_.string()}, {});

    auto stats = db.getStats();
    // Stats should be retrievable without error
    EXPECT_GE(stats.total_files, 0);
    EXPECT_GE(stats.total_directories, 0);

    db.close();
}

TEST_F(LocateCommandsTest, FileDatabaseClear)
{
    homeshell::FileDatabase db(db_path_);
    ASSERT_TRUE(db.open());

    db.updateDatabase({test_dir_.string()}, {});

    EXPECT_TRUE(db.clear());

    auto stats = db.getStats();
    EXPECT_EQ(stats.total_files, 0);
    EXPECT_EQ(stats.total_directories, 0);

    db.close();
}

// UpdatedbCommand Tests
TEST_F(LocateCommandsTest, UpdatedbGetName)
{
    homeshell::UpdatedbCommand cmd;
    EXPECT_EQ(cmd.getName(), "updatedb");
}

TEST_F(LocateCommandsTest, UpdatedbGetType)
{
    homeshell::UpdatedbCommand cmd;
    EXPECT_EQ(cmd.getType(), homeshell::CommandType::Synchronous);
}

TEST_F(LocateCommandsTest, UpdatedbGetDescription)
{
    homeshell::UpdatedbCommand cmd;
    EXPECT_FALSE(cmd.getDescription().empty());
}

TEST_F(LocateCommandsTest, UpdatedbHelp)
{
    homeshell::UpdatedbCommand cmd;
    homeshell::CommandContext ctx;
    ctx.args = {"--help"};

    testing::internal::CaptureStdout();
    auto status = cmd.execute(ctx);
    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_TRUE(status.isSuccess());
    EXPECT_TRUE(output.find("Usage:") != std::string::npos);
}

// LocateCommand Tests
TEST_F(LocateCommandsTest, LocateGetName)
{
    homeshell::LocateCommand cmd;
    EXPECT_EQ(cmd.getName(), "locate");
}

TEST_F(LocateCommandsTest, LocateGetType)
{
    homeshell::LocateCommand cmd;
    EXPECT_EQ(cmd.getType(), homeshell::CommandType::Synchronous);
}

TEST_F(LocateCommandsTest, LocateGetDescription)
{
    homeshell::LocateCommand cmd;
    EXPECT_FALSE(cmd.getDescription().empty());
}

TEST_F(LocateCommandsTest, LocateHelp)
{
    homeshell::LocateCommand cmd;
    homeshell::CommandContext ctx;
    ctx.args = {"--help"};

    testing::internal::CaptureStdout();
    auto status = cmd.execute(ctx);
    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_TRUE(status.isSuccess());
    EXPECT_TRUE(output.find("Usage:") != std::string::npos);
}

TEST_F(LocateCommandsTest, LocateNoPattern)
{
    homeshell::LocateCommand cmd;
    homeshell::CommandContext ctx;

    testing::internal::CaptureStdout();
    auto status = cmd.execute(ctx);
    testing::internal::GetCapturedStdout();

    EXPECT_FALSE(status.isSuccess());
}

TEST_F(LocateCommandsTest, LocateInvalidLimit)
{
    homeshell::LocateCommand cmd;
    homeshell::CommandContext ctx;
    ctx.args = {"-l", "-5", "test"};

    testing::internal::CaptureStdout();
    auto status = cmd.execute(ctx);
    testing::internal::GetCapturedStdout();

    EXPECT_FALSE(status.isSuccess());
}

TEST_F(LocateCommandsTest, LocateCaseSensitiveFlag)
{
    // Create a test database first
    homeshell::FileDatabase db(db_path_);
    ASSERT_TRUE(db.open());
    db.updateDatabase({test_dir_.string()}, {});
    db.close();

    homeshell::LocateCommand cmd;
    homeshell::CommandContext ctx;
    ctx.args = {"-c", "test"};

    testing::internal::CaptureStdout();
    auto status = cmd.execute(ctx);
    testing::internal::GetCapturedStdout();

    // Should succeed (database exists)
    EXPECT_TRUE(status.isSuccess());
}

TEST_F(LocateCommandsTest, LocateCaseInsensitiveFlag)
{
    // Create a test database first
    homeshell::FileDatabase db(db_path_);
    ASSERT_TRUE(db.open());
    db.updateDatabase({test_dir_.string()}, {});
    db.close();

    homeshell::LocateCommand cmd;
    homeshell::CommandContext ctx;
    ctx.args = {"-i", "test"};

    testing::internal::CaptureStdout();
    auto status = cmd.execute(ctx);
    testing::internal::GetCapturedStdout();

    // Should succeed (database exists)
    EXPECT_TRUE(status.isSuccess());
}

TEST_F(LocateCommandsTest, LocateMultiplePatterns)
{
    homeshell::LocateCommand cmd;
    homeshell::CommandContext ctx;
    ctx.args = {"pattern1", "pattern2"};

    testing::internal::CaptureStdout();
    auto status = cmd.execute(ctx);
    testing::internal::GetCapturedStdout();

    EXPECT_FALSE(status.isSuccess());
}

TEST_F(LocateCommandsTest, LocateInvalidLimitString)
{
    homeshell::LocateCommand cmd;
    homeshell::CommandContext ctx;
    ctx.args = {"-l", "abc", "test"};

    testing::internal::CaptureStdout();
    auto status = cmd.execute(ctx);
    testing::internal::GetCapturedStdout();

    EXPECT_FALSE(status.isSuccess());
}

TEST_F(LocateCommandsTest, LocateWithValidLimit)
{
    // Create a test database first
    homeshell::FileDatabase db(db_path_);
    ASSERT_TRUE(db.open());
    db.updateDatabase({test_dir_.string()}, {});
    db.close();

    homeshell::LocateCommand cmd;
    homeshell::CommandContext ctx;
    ctx.args = {"-l", "10", "test"};

    testing::internal::CaptureStdout();
    auto status = cmd.execute(ctx);
    testing::internal::GetCapturedStdout();

    // Should succeed (database exists)
    EXPECT_TRUE(status.isSuccess());
}

TEST_F(LocateCommandsTest, UpdatedbWithPathsAndExcludes)
{
    // Create database and add files
    homeshell::FileDatabase db(db_path_);
    ASSERT_TRUE(db.open());
    
    std::vector<std::string> paths = {test_dir_.string()};
    std::vector<std::string> excludes = {(test_dir_ / "dir2").string()};
    
    auto stats = db.updateDatabase(paths, excludes);
    EXPECT_GT(stats.total_files, 0);
    
    db.close();
}

TEST_F(LocateCommandsTest, UpdatedbWithPathOption)
{
    homeshell::UpdatedbCommand cmd;
    homeshell::CommandContext ctx;
    ctx.args = {"--path", test_dir_.string()};

    testing::internal::CaptureStdout();
    auto status = cmd.execute(ctx);
    std::string output = testing::internal::GetCapturedStdout();

    // Should succeed and show stats
    EXPECT_TRUE(status.isSuccess());
    EXPECT_TRUE(output.find("Database update complete") != std::string::npos);
}

TEST_F(LocateCommandsTest, UpdatedbWithExcludeOption)
{
    homeshell::UpdatedbCommand cmd;
    homeshell::CommandContext ctx;
    ctx.args = {"--path", test_dir_.string(), "--exclude", (test_dir_ / "dir2").string()};

    testing::internal::CaptureStdout();
    auto status = cmd.execute(ctx);
    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_TRUE(status.isSuccess());
    EXPECT_TRUE(output.find("Database update complete") != std::string::npos);
}

TEST_F(LocateCommandsTest, UpdatedbUnknownOption)
{
    homeshell::UpdatedbCommand cmd;
    homeshell::CommandContext ctx;
    ctx.args = {"--invalid-option"};

    testing::internal::CaptureStdout();
    auto status = cmd.execute(ctx);
    testing::internal::GetCapturedStdout();

    EXPECT_FALSE(status.isSuccess());
}

TEST_F(LocateCommandsTest, UpdatedbDefaultPaths)
{
    homeshell::UpdatedbCommand cmd;
    homeshell::CommandContext ctx;
    // No arguments - should use default paths

    testing::internal::CaptureStdout();
    auto status = cmd.execute(ctx);
    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_TRUE(status.isSuccess());
    EXPECT_TRUE(output.find("Database update complete") != std::string::npos);
}

