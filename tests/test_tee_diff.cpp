/**
 * @file test_text_processing_batch3.cpp
 * @brief Unit tests for text processing commands batch 3 (tee, diff)
 */

#include <homeshell/commands/DiffCommand.hpp>
#include <homeshell/commands/TeeCommand.hpp>

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;
using namespace homeshell;

// ============================================================================
// TeeCommand Tests
// ============================================================================

class TeeCommandTest : public ::testing::Test
{
protected:
    TeeCommand cmd;
    std::stringstream output;
    std::stringstream input;
    std::streambuf* old_cout;
    std::streambuf* old_cin;
    std::string test_dir;

    void SetUp() override
    {
        old_cout = std::cout.rdbuf(output.rdbuf());
        old_cin = std::cin.rdbuf(input.rdbuf());
        test_dir = "/tmp/homeshell_tee_test_" + std::to_string(::getpid());
        fs::create_directory(test_dir);
    }

    void TearDown() override
    {
        std::cout.rdbuf(old_cout);
        std::cin.rdbuf(old_cin);
        if (fs::exists(test_dir))
        {
            fs::remove_all(test_dir);
        }
    }

    std::string getOutput()
    {
        return output.str();
    }

    void setInput(const std::string& text)
    {
        input.str(text);
        input.clear();
    }
};

TEST_F(TeeCommandTest, CommandName)
{
    EXPECT_EQ(cmd.getName(), "tee");
}

TEST_F(TeeCommandTest, CommandDescription)
{
    EXPECT_FALSE(cmd.getDescription().empty());
}

TEST_F(TeeCommandTest, BasicTee)
{
    std::string file = test_dir + "/output.txt";
    setInput("line1\nline2\nline3\n");

    CommandContext ctx;
    ctx.args = {file};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());

    // Check stdout
    std::string out = getOutput();
    EXPECT_NE(out.find("line1"), std::string::npos);
    EXPECT_NE(out.find("line2"), std::string::npos);
    EXPECT_NE(out.find("line3"), std::string::npos);

    // Check file
    std::ifstream in(file);
    std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    EXPECT_NE(content.find("line1"), std::string::npos);
    EXPECT_NE(content.find("line2"), std::string::npos);
    EXPECT_NE(content.find("line3"), std::string::npos);
}

TEST_F(TeeCommandTest, MultipleFiles)
{
    std::string file1 = test_dir + "/output1.txt";
    std::string file2 = test_dir + "/output2.txt";
    setInput("test content\n");

    CommandContext ctx;
    ctx.args = {file1, file2};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());

    // Check both files exist and have content
    EXPECT_TRUE(fs::exists(file1));
    EXPECT_TRUE(fs::exists(file2));

    std::ifstream in1(file1);
    std::string content1((std::istreambuf_iterator<char>(in1)), std::istreambuf_iterator<char>());
    EXPECT_NE(content1.find("test content"), std::string::npos);

    std::ifstream in2(file2);
    std::string content2((std::istreambuf_iterator<char>(in2)), std::istreambuf_iterator<char>());
    EXPECT_NE(content2.find("test content"), std::string::npos);
}

TEST_F(TeeCommandTest, AppendMode)
{
    std::string file = test_dir + "/append.txt";

    // Write initial content
    {
        std::ofstream out(file);
        out << "initial\n";
    }

    setInput("appended\n");

    CommandContext ctx;
    ctx.args = {"-a", file};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());

    // Check file has both initial and appended content
    std::ifstream in(file);
    std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    EXPECT_NE(content.find("initial"), std::string::npos);
    EXPECT_NE(content.find("appended"), std::string::npos);
}

TEST_F(TeeCommandTest, OverwriteMode)
{
    std::string file = test_dir + "/overwrite.txt";

    // Write initial content
    {
        std::ofstream out(file);
        out << "initial\n";
    }

    setInput("new content\n");

    CommandContext ctx;
    ctx.args = {file};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());

    // Check file has only new content
    std::ifstream in(file);
    std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    EXPECT_EQ(content.find("initial"), std::string::npos);
    EXPECT_NE(content.find("new content"), std::string::npos);
}

TEST_F(TeeCommandTest, EmptyInput)
{
    std::string file = test_dir + "/empty.txt";
    setInput("");

    CommandContext ctx;
    ctx.args = {file};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());

    // File should exist but be empty
    EXPECT_TRUE(fs::exists(file));
    EXPECT_EQ(fs::file_size(file), 0);
}

TEST_F(TeeCommandTest, HelpOption)
{
    CommandContext ctx;
    ctx.args = {"--help"};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());

    std::string out = getOutput();
    EXPECT_NE(out.find("Copy standard input"), std::string::npos);
}

// ============================================================================
// DiffCommand Tests
// ============================================================================

class DiffCommandTest : public ::testing::Test
{
protected:
    DiffCommand cmd;
    std::stringstream output;
    std::streambuf* old_cout;
    std::string test_dir;

    void SetUp() override
    {
        old_cout = std::cout.rdbuf(output.rdbuf());
        test_dir = "/tmp/homeshell_diff_test_" + std::to_string(::getpid());
        fs::create_directory(test_dir);
    }

    void TearDown() override
    {
        std::cout.rdbuf(old_cout);
        if (fs::exists(test_dir))
        {
            fs::remove_all(test_dir);
        }
    }

    std::string getOutput()
    {
        return output.str();
    }

    void createFile(const std::string& path, const std::string& content)
    {
        std::ofstream file(path);
        file << content;
    }
};

TEST_F(DiffCommandTest, CommandName)
{
    EXPECT_EQ(cmd.getName(), "diff");
}

TEST_F(DiffCommandTest, CommandDescription)
{
    EXPECT_FALSE(cmd.getDescription().empty());
}

TEST_F(DiffCommandTest, IdenticalFiles)
{
    std::string file1 = test_dir + "/file1.txt";
    std::string file2 = test_dir + "/file2.txt";
    createFile(file1, "line1\nline2\nline3\n");
    createFile(file2, "line1\nline2\nline3\n");

    CommandContext ctx;
    ctx.args = {file1, file2};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());

    // No output for identical files
    std::string out = getOutput();
    EXPECT_TRUE(out.empty() || out.find("differ") == std::string::npos);
}

TEST_F(DiffCommandTest, DifferentFiles)
{
    std::string file1 = test_dir + "/file1.txt";
    std::string file2 = test_dir + "/file2.txt";
    createFile(file1, "line1\nline2\nline3\n");
    createFile(file2, "line1\nmodified\nline3\n");

    CommandContext ctx;
    ctx.args = {file1, file2};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());

    std::string out = getOutput();
    EXPECT_FALSE(out.empty());
}

TEST_F(DiffCommandTest, UnifiedFormat)
{
    std::string file1 = test_dir + "/file1.txt";
    std::string file2 = test_dir + "/file2.txt";
    createFile(file1, "line1\nline2\nline3\n");
    createFile(file2, "line1\nmodified\nline3\n");

    CommandContext ctx;
    ctx.args = {"-u", file1, file2};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());

    std::string out = getOutput();
    EXPECT_NE(out.find("---"), std::string::npos);
    EXPECT_NE(out.find("+++"), std::string::npos);
    EXPECT_NE(out.find("@@"), std::string::npos);
}

TEST_F(DiffCommandTest, BriefMode)
{
    std::string file1 = test_dir + "/file1.txt";
    std::string file2 = test_dir + "/file2.txt";
    createFile(file1, "line1\nline2\n");
    createFile(file2, "line1\nmodified\n");

    CommandContext ctx;
    ctx.args = {"-q", file1, file2};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());

    std::string out = getOutput();
    EXPECT_NE(out.find("differ"), std::string::npos);
}

TEST_F(DiffCommandTest, BriefModeIdentical)
{
    std::string file1 = test_dir + "/file1.txt";
    std::string file2 = test_dir + "/file2.txt";
    createFile(file1, "same\n");
    createFile(file2, "same\n");

    CommandContext ctx;
    ctx.args = {"-q", file1, file2};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());

    std::string out = getOutput();
    EXPECT_TRUE(out.empty() || out.find("differ") == std::string::npos);
}

TEST_F(DiffCommandTest, IgnoreCase)
{
    std::string file1 = test_dir + "/file1.txt";
    std::string file2 = test_dir + "/file2.txt";
    createFile(file1, "Hello World\n");
    createFile(file2, "hello world\n");

    CommandContext ctx;
    ctx.args = {"-i", file1, file2};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());

    // Should be identical with -i
    std::string out = getOutput();
    EXPECT_TRUE(out.empty() || out.find("differ") == std::string::npos);
}

TEST_F(DiffCommandTest, NonExistentFile)
{
    std::string file1 = test_dir + "/exists.txt";
    std::string file2 = test_dir + "/nonexistent.txt";
    createFile(file1, "content\n");

    CommandContext ctx;
    ctx.args = {file1, file2};
    auto status = cmd.execute(ctx);
    EXPECT_FALSE(status.isOk());
}

TEST_F(DiffCommandTest, MissingArguments)
{
    CommandContext ctx;
    ctx.args = {};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk()); // Shows help
}

TEST_F(DiffCommandTest, HelpOption)
{
    CommandContext ctx;
    ctx.args = {"--help"};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());

    std::string out = getOutput();
    EXPECT_NE(out.find("Compare files"), std::string::npos);
}

TEST_F(DiffCommandTest, AddedLines)
{
    std::string file1 = test_dir + "/file1.txt";
    std::string file2 = test_dir + "/file2.txt";
    createFile(file1, "line1\nline2\n");
    createFile(file2, "line1\nline2\nline3\n");

    CommandContext ctx;
    ctx.args = {file1, file2};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());

    std::string out = getOutput();
    EXPECT_FALSE(out.empty());
}

TEST_F(DiffCommandTest, RemovedLines)
{
    std::string file1 = test_dir + "/file1.txt";
    std::string file2 = test_dir + "/file2.txt";
    createFile(file1, "line1\nline2\nline3\n");
    createFile(file2, "line1\nline2\n");

    CommandContext ctx;
    ctx.args = {file1, file2};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());

    std::string out = getOutput();
    EXPECT_FALSE(out.empty());
}

