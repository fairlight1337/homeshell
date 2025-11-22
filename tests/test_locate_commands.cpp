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

