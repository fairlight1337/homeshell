/**
 * @file test_utility_commands_batch1.cpp
 * @brief Unit tests for utility commands batch 1 (hostname, uptime, free, head, wc)
 */

#include <homeshell/commands/FreeCommand.hpp>
#include <homeshell/commands/HeadCommand.hpp>
#include <homeshell/commands/HostnameCommand.hpp>
#include <homeshell/commands/UptimeCommand.hpp>
#include <homeshell/commands/WcCommand.hpp>

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;
using namespace homeshell;

// ============================================================================
// HostnameCommand Tests
// ============================================================================

class HostnameCommandTest : public ::testing::Test
{
protected:
    HostnameCommand cmd;
    std::stringstream output;
    std::streambuf* old_cout;

    void SetUp() override
    {
        old_cout = std::cout.rdbuf(output.rdbuf());
    }

    void TearDown() override
    {
        std::cout.rdbuf(old_cout);
    }

    std::string getOutput()
    {
        return output.str();
    }
};

TEST_F(HostnameCommandTest, CommandName)
{
    EXPECT_EQ(cmd.getName(), "hostname");
}

TEST_F(HostnameCommandTest, CommandDescription)
{
    EXPECT_FALSE(cmd.getDescription().empty());
}

TEST_F(HostnameCommandTest, BasicExecution)
{
    CommandContext ctx;
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());

    std::string out = getOutput();
    EXPECT_FALSE(out.empty());
    EXPECT_NE(out.find("\n"), std::string::npos);
}

TEST_F(HostnameCommandTest, HelpOption)
{
    CommandContext ctx;
    ctx.args = {"--help"};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());

    std::string out = getOutput();
    EXPECT_NE(out.find("Usage:"), std::string::npos);
}

TEST_F(HostnameCommandTest, InvalidOption)
{
    CommandContext ctx;
    ctx.args = {"--invalid"};
    auto status = cmd.execute(ctx);
    EXPECT_FALSE(status.isOk());
}

// ============================================================================
// UptimeCommand Tests
// ============================================================================

class UptimeCommandTest : public ::testing::Test
{
protected:
    UptimeCommand cmd;
    std::stringstream output;
    std::streambuf* old_cout;

    void SetUp() override
    {
        old_cout = std::cout.rdbuf(output.rdbuf());
    }

    void TearDown() override
    {
        std::cout.rdbuf(old_cout);
    }

    std::string getOutput()
    {
        return output.str();
    }
};

TEST_F(UptimeCommandTest, CommandName)
{
    EXPECT_EQ(cmd.getName(), "uptime");
}

TEST_F(UptimeCommandTest, BasicExecution)
{
    CommandContext ctx;
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());

    std::string out = getOutput();
    EXPECT_FALSE(out.empty());
    EXPECT_NE(out.find("up"), std::string::npos);
}

TEST_F(UptimeCommandTest, PrettyFormat)
{
    CommandContext ctx;
    ctx.args = {"-p"};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());

    std::string out = getOutput();
    EXPECT_NE(out.find("up"), std::string::npos);
}

TEST_F(UptimeCommandTest, SinceOption)
{
    CommandContext ctx;
    ctx.args = {"-s"};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());

    std::string out = getOutput();
    EXPECT_FALSE(out.empty());
    // Should contain date/time format
}

TEST_F(UptimeCommandTest, HelpOption)
{
    CommandContext ctx;
    ctx.args = {"--help"};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());

    std::string out = getOutput();
    EXPECT_NE(out.find("Usage:"), std::string::npos);
}

// ============================================================================
// FreeCommand Tests
// ============================================================================

class FreeCommandTest : public ::testing::Test
{
protected:
    FreeCommand cmd;
    std::stringstream output;
    std::streambuf* old_cout;

    void SetUp() override
    {
        old_cout = std::cout.rdbuf(output.rdbuf());
    }

    void TearDown() override
    {
        std::cout.rdbuf(old_cout);
    }

    std::string getOutput()
    {
        return output.str();
    }
};

TEST_F(FreeCommandTest, CommandName)
{
    EXPECT_EQ(cmd.getName(), "free");
}

TEST_F(FreeCommandTest, BasicExecution)
{
    CommandContext ctx;
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());

    std::string out = getOutput();
    EXPECT_NE(out.find("Mem:"), std::string::npos);
    EXPECT_NE(out.find("Swap:"), std::string::npos);
}

TEST_F(FreeCommandTest, HumanReadable)
{
    CommandContext ctx;
    ctx.args = {"-h"};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());

    std::string out = getOutput();
    EXPECT_TRUE(out.find("Ki") != std::string::npos || out.find("Mi") != std::string::npos ||
                out.find("Gi") != std::string::npos);
}

TEST_F(FreeCommandTest, MegabytesOption)
{
    CommandContext ctx;
    ctx.args = {"-m"};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());
}

TEST_F(FreeCommandTest, GigabytesOption)
{
    CommandContext ctx;
    ctx.args = {"-g"};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());
}

TEST_F(FreeCommandTest, HelpOption)
{
    CommandContext ctx;
    ctx.args = {"--help"};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());

    std::string out = getOutput();
    EXPECT_NE(out.find("Usage:"), std::string::npos);
}

// ============================================================================
// HeadCommand Tests
// ============================================================================

class HeadCommandTest : public ::testing::Test
{
protected:
    HeadCommand cmd;
    std::stringstream output;
    std::streambuf* old_cout;
    std::string test_file;

    void SetUp() override
    {
        old_cout = std::cout.rdbuf(output.rdbuf());

        // Create test file
        test_file = "/tmp/homeshell_head_test.txt";
        std::ofstream file(test_file);
        for (int i = 1; i <= 20; ++i)
        {
            file << "Line " << i << "\n";
        }
        file.close();
    }

    void TearDown() override
    {
        std::cout.rdbuf(old_cout);
        if (fs::exists(test_file))
        {
            fs::remove(test_file);
        }
    }

    std::string getOutput()
    {
        return output.str();
    }

    void clearOutput()
    {
        output.str("");
        output.clear();
    }
};

TEST_F(HeadCommandTest, CommandName)
{
    EXPECT_EQ(cmd.getName(), "head");
}

TEST_F(HeadCommandTest, DefaultLines)
{
    CommandContext ctx;
    ctx.args = {test_file};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());

    std::string out = getOutput();
    int line_count = std::count(out.begin(), out.end(), '\n');
    EXPECT_EQ(line_count, 10); // Default is 10 lines
}

TEST_F(HeadCommandTest, CustomLineCount)
{
    CommandContext ctx;
    ctx.args = {"-n", "5", test_file};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());

    std::string out = getOutput();
    int line_count = std::count(out.begin(), out.end(), '\n');
    EXPECT_EQ(line_count, 5);
}

TEST_F(HeadCommandTest, ShortFormLineCount)
{
    CommandContext ctx;
    ctx.args = {"-3", test_file};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());

    std::string out = getOutput();
    int line_count = std::count(out.begin(), out.end(), '\n');
    EXPECT_EQ(line_count, 3);
}

TEST_F(HeadCommandTest, NonExistentFile)
{
    CommandContext ctx;
    ctx.args = {"/nonexistent/file.txt"};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk()); // Command succeeds but shows error for file
}

TEST_F(HeadCommandTest, MultipleFiles)
{
    CommandContext ctx;
    ctx.args = {"-n", "3", test_file, test_file};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());

    std::string out = getOutput();
    EXPECT_NE(out.find("==>"), std::string::npos); // Should show headers
}

TEST_F(HeadCommandTest, HelpOption)
{
    CommandContext ctx;
    ctx.args = {"--help"};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());

    std::string out = getOutput();
    EXPECT_NE(out.find("Usage:"), std::string::npos);
}

// ============================================================================
// WcCommand Tests
// ============================================================================

class WcCommandTest : public ::testing::Test
{
protected:
    WcCommand cmd;
    std::stringstream output;
    std::streambuf* old_cout;
    std::string test_file;

    void SetUp() override
    {
        old_cout = std::cout.rdbuf(output.rdbuf());

        // Create test file with known content
        test_file = "/tmp/homeshell_wc_test.txt";
        std::ofstream file(test_file);
        file << "Hello world\n";
        file << "This is a test\n";
        file << "Line three\n";
        file.close();
    }

    void TearDown() override
    {
        std::cout.rdbuf(old_cout);
        if (fs::exists(test_file))
        {
            fs::remove(test_file);
        }
    }

    std::string getOutput()
    {
        return output.str();
    }

    void clearOutput()
    {
        output.str("");
        output.clear();
    }
};

TEST_F(WcCommandTest, CommandName)
{
    EXPECT_EQ(cmd.getName(), "wc");
}

TEST_F(WcCommandTest, DefaultCounts)
{
    CommandContext ctx;
    ctx.args = {test_file};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());

    std::string out = getOutput();
    // Should contain lines, words, bytes
    EXPECT_NE(out.find("3"), std::string::npos);  // 3 lines
    EXPECT_NE(out.find("8"), std::string::npos);  // 8 words
    EXPECT_NE(out.find(test_file), std::string::npos);
}

TEST_F(WcCommandTest, LinesOnly)
{
    CommandContext ctx;
    ctx.args = {"-l", test_file};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());

    std::string out = getOutput();
    EXPECT_NE(out.find("3"), std::string::npos); // 3 lines
}

TEST_F(WcCommandTest, WordsOnly)
{
    CommandContext ctx;
    ctx.args = {"-w", test_file};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());

    std::string out = getOutput();
    EXPECT_NE(out.find("8"), std::string::npos); // 8 words
}

TEST_F(WcCommandTest, BytesOnly)
{
    CommandContext ctx;
    ctx.args = {"-c", test_file};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());
}

TEST_F(WcCommandTest, CombinedOptions)
{
    CommandContext ctx;
    ctx.args = {"-lw", test_file};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());
}

TEST_F(WcCommandTest, MultipleFiles)
{
    CommandContext ctx;
    ctx.args = {test_file, test_file};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());

    std::string out = getOutput();
    EXPECT_NE(out.find("total"), std::string::npos);
}

TEST_F(WcCommandTest, HelpOption)
{
    CommandContext ctx;
    ctx.args = {"--help"};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());

    std::string out = getOutput();
    EXPECT_NE(out.find("Usage:"), std::string::npos);
}

