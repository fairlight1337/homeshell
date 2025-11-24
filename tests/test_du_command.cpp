/**
 * @file test_du_command.cpp
 * @brief Unit tests for DuCommand
 */

#include <homeshell/commands/DuCommand.hpp>
#include <homeshell/VirtualFilesystem.hpp>

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>

namespace fs = std::filesystem;
using namespace homeshell;

class DuCommandTest : public ::testing::Test
{
protected:
    DuCommand cmd;
    std::stringstream output;
    std::stringstream error_output;
    std::streambuf* old_cout;
    std::streambuf* old_cerr;
    std::string test_dir;

    void SetUp() override
    {
        // Redirect cout and cerr to our stringstreams
        old_cout = std::cout.rdbuf(output.rdbuf());
        old_cerr = std::cerr.rdbuf(error_output.rdbuf());

        // Create test directory structure
        test_dir = "/tmp/homeshell_du_test_" + std::to_string(getpid());
        fs::create_directories(test_dir);
        fs::create_directories(test_dir + "/subdir1");
        fs::create_directories(test_dir + "/subdir2");
        fs::create_directories(test_dir + "/subdir1/nested");

        // Create test files with known sizes
        createFile(test_dir + "/file1.txt", 1024);           // 1 KB
        createFile(test_dir + "/file2.txt", 2048);           // 2 KB
        createFile(test_dir + "/subdir1/file3.txt", 4096);   // 4 KB
        createFile(test_dir + "/subdir2/file4.txt", 8192);   // 8 KB
        createFile(test_dir + "/subdir1/nested/file5.txt", 512); // 0.5 KB
    }

    void TearDown() override
    {
        // Restore cout and cerr
        std::cout.rdbuf(old_cout);
        std::cerr.rdbuf(old_cerr);

        // Clean up test directory
        if (fs::exists(test_dir))
        {
            fs::remove_all(test_dir);
        }
    }

    void createFile(const std::string& path, size_t size)
    {
        std::ofstream file(path, std::ios::binary);
        std::vector<char> data(size, 'x');
        file.write(data.data(), size);
    }

    std::string getOutput()
    {
        return output.str();
    }

    std::string getErrorOutput()
    {
        return error_output.str();
    }

    void clearOutput()
    {
        output.str("");
        output.clear();
        error_output.str("");
        error_output.clear();
    }
};

TEST_F(DuCommandTest, CommandName)
{
    EXPECT_EQ(cmd.getName(), "du");
}

TEST_F(DuCommandTest, CommandDescription)
{
    std::string desc = cmd.getDescription();
    EXPECT_FALSE(desc.empty());
    EXPECT_NE(desc.find("disk"), std::string::npos);
}

TEST_F(DuCommandTest, CommandType)
{
    EXPECT_EQ(cmd.getType(), homeshell::CommandType::Synchronous);
}

TEST_F(DuCommandTest, HelpOption)
{
    CommandContext ctx;
    ctx.args = {"--help"};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());

    std::string out = getOutput();
    EXPECT_NE(out.find("Usage:"), std::string::npos);
    EXPECT_NE(out.find("--human-readable"), std::string::npos);
    EXPECT_NE(out.find("--max-depth"), std::string::npos);
}

TEST_F(DuCommandTest, BasicUsage)
{
    CommandContext ctx;
    ctx.args = {test_dir};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());

    std::string out = getOutput();
    // Should show directory structure
    EXPECT_NE(out.find(test_dir), std::string::npos);
}

TEST_F(DuCommandTest, HumanReadableShortOption)
{
    CommandContext ctx;
    ctx.args = {"-h", test_dir};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());

    std::string out = getOutput();
    // Should contain human-readable units (K, M, G)
    EXPECT_TRUE(out.find("K") != std::string::npos || out.find("M") != std::string::npos ||
                out.find("B") != std::string::npos);
}

TEST_F(DuCommandTest, HumanReadableLongOption)
{
    CommandContext ctx;
    ctx.args = {"--human-readable", test_dir};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());
}

TEST_F(DuCommandTest, SummaryOption)
{
    CommandContext ctx;
    ctx.args = {"-s", test_dir};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());

    std::string out = getOutput();
    // Should show only one line for the directory
    int line_count = std::count(out.begin(), out.end(), '\n');
    EXPECT_EQ(line_count, 1);
}

TEST_F(DuCommandTest, SummaryLongOption)
{
    CommandContext ctx;
    ctx.args = {"--summarize", test_dir};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());

    std::string out = getOutput();
    int line_count = std::count(out.begin(), out.end(), '\n');
    EXPECT_EQ(line_count, 1);
}

TEST_F(DuCommandTest, MaxDepthOption)
{
    CommandContext ctx;
    ctx.args = {"--max-depth=0", test_dir};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());

    std::string out = getOutput();
    // Should show only the top directory
    int line_count = std::count(out.begin(), out.end(), '\n');
    EXPECT_EQ(line_count, 1);
}

TEST_F(DuCommandTest, MaxDepthShortOption)
{
    CommandContext ctx;
    ctx.args = {"-d", "1", test_dir};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());

    std::string out = getOutput();
    // Should show top directory and immediate subdirectories
    EXPECT_NE(out.find("subdir1"), std::string::npos);
    EXPECT_NE(out.find("subdir2"), std::string::npos);
    // Should NOT show nested directory
    EXPECT_EQ(out.find("nested"), std::string::npos);
}

TEST_F(DuCommandTest, MaxDepthCombined)
{
    CommandContext ctx;
    ctx.args = {"-d1", test_dir};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());
}

TEST_F(DuCommandTest, AllFilesOption)
{
    CommandContext ctx;
    ctx.args = {"-a", test_dir};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());

    std::string out = getOutput();
    // Should show individual files
    EXPECT_NE(out.find("file1.txt"), std::string::npos);
    EXPECT_NE(out.find("file2.txt"), std::string::npos);
}

TEST_F(DuCommandTest, AllFilesLongOption)
{
    CommandContext ctx;
    ctx.args = {"--all", test_dir};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());

    std::string out = getOutput();
    EXPECT_NE(out.find("file1.txt"), std::string::npos);
}

TEST_F(DuCommandTest, TotalOption)
{
    CommandContext ctx;
    ctx.args = {"-c", test_dir + "/subdir1", test_dir + "/subdir2"};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());

    std::string out = getOutput();
    // Should show "total" at the end
    EXPECT_NE(out.find("total"), std::string::npos);
}

TEST_F(DuCommandTest, TotalLongOption)
{
    CommandContext ctx;
    ctx.args = {"--total", test_dir + "/subdir1", test_dir + "/subdir2"};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());

    std::string out = getOutput();
    EXPECT_NE(out.find("total"), std::string::npos);
}

TEST_F(DuCommandTest, CombinedOptions)
{
    CommandContext ctx;
    ctx.args = {"-hsc", test_dir + "/subdir1", test_dir + "/subdir2"};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());

    std::string out = getOutput();
    // Should be human-readable, summary, with total
    EXPECT_TRUE(out.find("K") != std::string::npos || out.find("M") != std::string::npos);
    EXPECT_NE(out.find("total"), std::string::npos);
}

TEST_F(DuCommandTest, InvalidPath)
{
    CommandContext ctx;
    ctx.args = {"/nonexistent/path/that/does/not/exist"};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk()); // Command succeeds but shows error for path

    std::string err = getErrorOutput();
    EXPECT_NE(err.find("cannot access"), std::string::npos);
}

TEST_F(DuCommandTest, InvalidMaxDepth)
{
    CommandContext ctx;
    ctx.args = {"--max-depth=invalid", test_dir};
    auto status = cmd.execute(ctx);
    EXPECT_FALSE(status.isOk());
    EXPECT_NE(status.message.find("Invalid"), std::string::npos);
}

TEST_F(DuCommandTest, NegativeMaxDepth)
{
    CommandContext ctx;
    ctx.args = {"--max-depth=-1", test_dir};
    auto status = cmd.execute(ctx);
    EXPECT_FALSE(status.isOk());
}

TEST_F(DuCommandTest, InvalidOption)
{
    CommandContext ctx;
    ctx.args = {"-x", test_dir};
    auto status = cmd.execute(ctx);
    EXPECT_FALSE(status.isOk());
    EXPECT_NE(status.message.find("Unknown option"), std::string::npos);
}

TEST_F(DuCommandTest, CurrentDirectory)
{
    // Change to test directory
    char old_cwd[PATH_MAX];
    getcwd(old_cwd, PATH_MAX);
    chdir(test_dir.c_str());

    CommandContext ctx;
    ctx.args = {}; // No arguments means current directory
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());

    std::string out = getOutput();
    EXPECT_FALSE(out.empty());

    // Restore directory
    chdir(old_cwd);
}

TEST_F(DuCommandTest, MultiplePaths)
{
    CommandContext ctx;
    ctx.args = {test_dir + "/subdir1", test_dir + "/subdir2"};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());

    std::string out = getOutput();
    EXPECT_NE(out.find("subdir1"), std::string::npos);
    EXPECT_NE(out.find("subdir2"), std::string::npos);
}

TEST_F(DuCommandTest, SingleFile)
{
    CommandContext ctx;
    ctx.args = {test_dir + "/file1.txt"};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());

    std::string out = getOutput();
    EXPECT_NE(out.find("file1.txt"), std::string::npos);
}

// Integration test
TEST(DuIntegrationTest, RealDirectoryCheck)
{
    DuCommand cmd;

    std::stringstream output;
    std::streambuf* old_cout = std::cout.rdbuf(output.rdbuf());

    CommandContext ctx;
    ctx.args = {"-s", "/tmp"}; // Summary of /tmp
    auto status = cmd.execute(ctx);

    std::cout.rdbuf(old_cout);

    EXPECT_TRUE(status.isOk());

    std::string out = output.str();
    EXPECT_NE(out.find("/tmp"), std::string::npos);
}

// Test output formatting
TEST(DuOutputTest, ContainsExpectedFormat)
{
    DuCommand cmd;

    // Create temporary test directory
    std::string test_dir = "/tmp/homeshell_du_format_test";
    fs::create_directories(test_dir);

    std::ofstream file(test_dir + "/testfile.txt");
    file << "test content";
    file.close();

    std::stringstream output;
    std::streambuf* old_cout = std::cout.rdbuf(output.rdbuf());

    CommandContext ctx;
    ctx.args = {"-s", test_dir};
    cmd.execute(ctx);

    std::cout.rdbuf(old_cout);

    std::string out = output.str();

    // Should have size followed by tab and path
    EXPECT_TRUE(out.find("\t") != std::string::npos);
    EXPECT_NE(out.find(test_dir), std::string::npos);

    // Clean up
    fs::remove_all(test_dir);
}

// Test human-readable formatting
TEST(DuFormatTest, HumanReadableFormat)
{
    DuCommand cmd;

    std::string test_dir = "/tmp/homeshell_du_hr_test";
    fs::create_directories(test_dir);

    // Create a file with known size
    std::ofstream file(test_dir + "/largefile.txt", std::ios::binary);
    std::vector<char> data(5 * 1024 * 1024, 'x'); // 5 MB
    file.write(data.data(), data.size());
    file.close();

    std::stringstream output;
    std::streambuf* old_cout = std::cout.rdbuf(output.rdbuf());

    CommandContext ctx;
    ctx.args = {"-h", "-s", test_dir};
    cmd.execute(ctx);

    std::cout.rdbuf(old_cout);

    std::string out = output.str();

    // Should contain human-readable unit
    EXPECT_TRUE(out.find("M") != std::string::npos || out.find("K") != std::string::npos);

    // Clean up
    fs::remove_all(test_dir);
}

