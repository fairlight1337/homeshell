/**
 * @file test_df_command.cpp
 * @brief Unit tests for DfCommand
 */

#include <homeshell/commands/DfCommand.hpp>

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>

namespace fs = std::filesystem;
using namespace homeshell;

class DfCommandTest : public ::testing::Test
{
protected:
    DfCommand cmd;
    std::stringstream output;
    std::stringstream error_output;
    std::streambuf* old_cout;
    std::streambuf* old_cerr;

    void SetUp() override
    {
        // Redirect cout and cerr to our stringstreams
        old_cout = std::cout.rdbuf(output.rdbuf());
        old_cerr = std::cerr.rdbuf(error_output.rdbuf());
    }

    void TearDown() override
    {
        // Restore cout and cerr
        std::cout.rdbuf(old_cout);
        std::cerr.rdbuf(old_cerr);
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

TEST_F(DfCommandTest, CommandName)
{
    EXPECT_EQ(cmd.getName(), "df");
}

TEST_F(DfCommandTest, CommandDescription)
{
    std::string desc = cmd.getDescription();
    EXPECT_FALSE(desc.empty());
    EXPECT_NE(desc.find("disk"), std::string::npos);
}

TEST_F(DfCommandTest, CommandType)
{
    EXPECT_EQ(cmd.getType(), homeshell::CommandType::Synchronous);
}

TEST_F(DfCommandTest, HelpOption)
{
    CommandContext ctx;
    ctx.args = {"--help"};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());

    std::string out = getOutput();
    EXPECT_NE(out.find("Usage:"), std::string::npos);
    EXPECT_NE(out.find("--all"), std::string::npos);
    EXPECT_NE(out.find("--human-readable"), std::string::npos);
}

TEST_F(DfCommandTest, NoArguments)
{
    // Should run without errors and display filesystem info
    CommandContext ctx;
    ctx.args = {};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());

    std::string out = getOutput();
    // Should contain at least the header
    EXPECT_NE(out.find("Filesystem"), std::string::npos);
    EXPECT_NE(out.find("Mounted on"), std::string::npos);
}

TEST_F(DfCommandTest, HumanReadableShortOption)
{
    CommandContext ctx;
    ctx.args = {"-h"};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());

    std::string out = getOutput();
    // Should show human-readable sizes (K, M, G)
    EXPECT_NE(out.find("Filesystem"), std::string::npos);
}

TEST_F(DfCommandTest, HumanReadableLongOption)
{
    CommandContext ctx;
    ctx.args = {"--human-readable"};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());

    std::string out = getOutput();
    EXPECT_NE(out.find("Filesystem"), std::string::npos);
}

TEST_F(DfCommandTest, AllOption)
{
    CommandContext ctx;
    ctx.args = {"-a"};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());

    std::string out = getOutput();
    EXPECT_NE(out.find("Filesystem"), std::string::npos);
    // With -a, should show more filesystems including pseudo ones
}

TEST_F(DfCommandTest, AllLongOption)
{
    CommandContext ctx;
    ctx.args = {"--all"};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());
}

TEST_F(DfCommandTest, CombinedOptions)
{
    CommandContext ctx;
    ctx.args = {"-ha"};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());

    std::string out = getOutput();
    EXPECT_NE(out.find("Filesystem"), std::string::npos);
}

TEST_F(DfCommandTest, CombinedOptionsReversed)
{
    CommandContext ctx;
    ctx.args = {"-ah"};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());
}

TEST_F(DfCommandTest, SpecificPath)
{
    // Test with root filesystem
    CommandContext ctx;
    ctx.args = {"/"};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());

    std::string out = getOutput();
    EXPECT_NE(out.find("Filesystem"), std::string::npos);
    EXPECT_NE(out.find("/"), std::string::npos);
}

TEST_F(DfCommandTest, MultiplePaths)
{
    CommandContext ctx;
    ctx.args = {"/", "/tmp"};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());
}

TEST_F(DfCommandTest, InvalidPath)
{
    CommandContext ctx;
    ctx.args = {"/nonexistent/path/that/does/not/exist"};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk()); // Command succeeds but shows error for path

    std::string err = getErrorOutput();
    EXPECT_NE(err.find("No such file or directory"), std::string::npos);
}

TEST_F(DfCommandTest, InvalidOption)
{
    CommandContext ctx;
    ctx.args = {"-x"};
    auto status = cmd.execute(ctx);
    EXPECT_FALSE(status.isOk());
    EXPECT_NE(status.message.find("Unknown option"), std::string::npos);
}

TEST_F(DfCommandTest, HumanReadableWithPath)
{
    CommandContext ctx;
    ctx.args = {"-h", "/"};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());

    std::string out = getOutput();
    EXPECT_NE(out.find("Filesystem"), std::string::npos);
}

TEST_F(DfCommandTest, AllWithHumanReadable)
{
    CommandContext ctx;
    ctx.args = {"-a", "--human-readable"};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());
}

// Integration test - checks if real filesystem info is available
TEST(DfIntegrationTest, RealSystemCheck)
{
    DfCommand cmd;

    std::stringstream output;
    std::streambuf* old_cout = std::cout.rdbuf(output.rdbuf());

    CommandContext ctx;
    ctx.args = {};
    auto status = cmd.execute(ctx);

    std::cout.rdbuf(old_cout);

    EXPECT_TRUE(status.isOk());

    std::string out = output.str();

    // Should have filesystem header
    EXPECT_NE(out.find("Filesystem"), std::string::npos);
    EXPECT_NE(out.find("Mounted on"), std::string::npos);

    // Should have at least root filesystem
    // (though output format may vary)
}

// Test output formatting
TEST(DfOutputTest, OutputContainsExpectedColumns)
{
    DfCommand cmd;

    std::stringstream output;
    std::streambuf* old_cout = std::cout.rdbuf(output.rdbuf());

    CommandContext ctx;
    ctx.args = {};
    cmd.execute(ctx);

    std::cout.rdbuf(old_cout);

    std::string out = output.str();

    // Should have all expected columns
    EXPECT_NE(out.find("Filesystem"), std::string::npos);
    EXPECT_NE(out.find("Use%"), std::string::npos);
    EXPECT_NE(out.find("Mounted on"), std::string::npos);
}

TEST(DfOutputTest, HumanReadableShowsUnits)
{
    DfCommand cmd;

    std::stringstream output;
    std::streambuf* old_cout = std::cout.rdbuf(output.rdbuf());

    CommandContext ctx;
    ctx.args = {"-h"};
    cmd.execute(ctx);

    std::cout.rdbuf(old_cout);

    std::string out = output.str();

    // Should have Size/Used/Avail columns
    EXPECT_NE(out.find("Size"), std::string::npos);
    EXPECT_NE(out.find("Used"), std::string::npos);
    EXPECT_NE(out.find("Avail"), std::string::npos);
}

TEST(DfOutputTest, NonHumanReadableShowsBlocks)
{
    DfCommand cmd;

    std::stringstream output;
    std::streambuf* old_cout = std::cout.rdbuf(output.rdbuf());

    CommandContext ctx;
    ctx.args = {};
    cmd.execute(ctx);

    std::cout.rdbuf(old_cout);

    std::string out = output.str();

    // Should have 1K-blocks column
    EXPECT_NE(out.find("1K-blocks"), std::string::npos);
}

// Test specific path filtering
TEST(DfFilterTest, RootPathShowsRootFilesystem)
{
    DfCommand cmd;

    std::stringstream output;
    std::streambuf* old_cout = std::cout.rdbuf(output.rdbuf());

    CommandContext ctx;
    ctx.args = {"/"};
    cmd.execute(ctx);

    std::cout.rdbuf(old_cout);

    std::string out = output.str();

    // Should show information about root filesystem
    EXPECT_NE(out.find("Filesystem"), std::string::npos);
}

// Test /proc/mounts parsing
TEST(DfParsingTest, ProcMountsExists)
{
    // /proc/mounts should exist on Linux systems
    std::ifstream file("/proc/mounts");
    EXPECT_TRUE(file.is_open());
}

// Test statvfs call
TEST(DfParsingTest, StatvfsRootWorks)
{
    // statvfs should work on root filesystem
    struct statvfs stat;
    int result = statvfs("/", &stat);
    EXPECT_EQ(result, 0);

    // Should have reasonable values
    EXPECT_GT(stat.f_blocks, 0);
}

// Test that both regular and human-readable formats work
TEST(DfFormatTest, BothFormatsProduceOutput)
{
    DfCommand cmd;

    // Test regular format
    {
        std::stringstream output;
        std::streambuf* old_cout = std::cout.rdbuf(output.rdbuf());

        CommandContext ctx;
        ctx.args = {};
        cmd.execute(ctx);

        std::cout.rdbuf(old_cout);

        std::string out = output.str();
        EXPECT_FALSE(out.empty());
        EXPECT_NE(out.find("1K-blocks"), std::string::npos);
    }

    // Test human-readable format
    {
        std::stringstream output;
        std::streambuf* old_cout = std::cout.rdbuf(output.rdbuf());

        CommandContext ctx;
        ctx.args = {"-h"};
        cmd.execute(ctx);

        std::cout.rdbuf(old_cout);

        std::string out = output.str();
        EXPECT_FALSE(out.empty());
        EXPECT_NE(out.find("Size"), std::string::npos);
    }
}

