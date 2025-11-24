/**
 * @file test_security_batch4.cpp
 * @brief Unit tests for security commands batch 4 (sha256sum, md5sum, base64)
 */

#include <homeshell/commands/Base64Command.hpp>
#include <homeshell/commands/Md5sumCommand.hpp>
#include <homeshell/commands/Sha256sumCommand.hpp>

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;
using namespace homeshell;

// ============================================================================
// Sha256sumCommand Tests
// ============================================================================

class Sha256sumCommandTest : public ::testing::Test
{
protected:
    Sha256sumCommand cmd;
    std::stringstream output;
    std::stringstream input;
    std::streambuf* old_cout;
    std::streambuf* old_cin;
    std::string test_dir;

    void SetUp() override
    {
        old_cout = std::cout.rdbuf(output.rdbuf());
        old_cin = std::cin.rdbuf(input.rdbuf());
        test_dir = "/tmp/homeshell_sha256_test_" + std::to_string(::getpid());
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

    void createFile(const std::string& path, const std::string& content)
    {
        std::ofstream file(path);
        file << content;
    }
};

TEST_F(Sha256sumCommandTest, CommandName)
{
    EXPECT_EQ(cmd.getName(), "sha256sum");
}

TEST_F(Sha256sumCommandTest, BasicHash)
{
    std::string file = test_dir + "/test.txt";
    createFile(file, "hello\n");

    CommandContext ctx;
    ctx.args = {file};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());

    std::string out = getOutput();
    EXPECT_FALSE(out.empty());
    EXPECT_NE(out.find(file), std::string::npos);
}

TEST_F(Sha256sumCommandTest, StdinHash)
{
    setInput("test data\n");

    CommandContext ctx;
    ctx.args = {};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());

    std::string out = getOutput();
    EXPECT_FALSE(out.empty());
    EXPECT_NE(out.find("-"), std::string::npos);
}

TEST_F(Sha256sumCommandTest, MultipleFiles)
{
    std::string file1 = test_dir + "/file1.txt";
    std::string file2 = test_dir + "/file2.txt";
    createFile(file1, "content1");
    createFile(file2, "content2");

    CommandContext ctx;
    ctx.args = {file1, file2};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());

    std::string out = getOutput();
    EXPECT_NE(out.find(file1), std::string::npos);
    EXPECT_NE(out.find(file2), std::string::npos);
}

TEST_F(Sha256sumCommandTest, NonExistentFile)
{
    CommandContext ctx;
    ctx.args = {"/nonexistent/file.txt"};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk()); // Command succeeds but prints error
}

TEST_F(Sha256sumCommandTest, HelpOption)
{
    CommandContext ctx;
    ctx.args = {"--help"};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());

    std::string out = getOutput();
    EXPECT_NE(out.find("SHA-256"), std::string::npos);
}

// ============================================================================
// Md5sumCommand Tests
// ============================================================================

class Md5sumCommandTest : public ::testing::Test
{
protected:
    Md5sumCommand cmd;
    std::stringstream output;
    std::stringstream input;
    std::streambuf* old_cout;
    std::streambuf* old_cin;
    std::string test_dir;

    void SetUp() override
    {
        old_cout = std::cout.rdbuf(output.rdbuf());
        old_cin = std::cin.rdbuf(input.rdbuf());
        test_dir = "/tmp/homeshell_md5_test_" + std::to_string(::getpid());
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

    void createFile(const std::string& path, const std::string& content)
    {
        std::ofstream file(path);
        file << content;
    }
};

TEST_F(Md5sumCommandTest, CommandName)
{
    EXPECT_EQ(cmd.getName(), "md5sum");
}

TEST_F(Md5sumCommandTest, BasicHash)
{
    std::string file = test_dir + "/test.txt";
    createFile(file, "hello\n");

    CommandContext ctx;
    ctx.args = {file};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());

    std::string out = getOutput();
    EXPECT_FALSE(out.empty());
    EXPECT_NE(out.find(file), std::string::npos);
}

TEST_F(Md5sumCommandTest, StdinHash)
{
    setInput("test data\n");

    CommandContext ctx;
    ctx.args = {};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());

    std::string out = getOutput();
    EXPECT_FALSE(out.empty());
    EXPECT_NE(out.find("-"), std::string::npos);
}

TEST_F(Md5sumCommandTest, KnownHash)
{
    std::string file = test_dir + "/test.txt";
    createFile(file, "hello\n");

    CommandContext ctx;
    ctx.args = {file};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());

    std::string out = getOutput();
    // "hello\n" has MD5: b1946ac92492d2347c6235b4d2611184
    EXPECT_NE(out.find("b1946ac92492d2347c6235b4d2611184"), std::string::npos);
}

TEST_F(Md5sumCommandTest, MultipleFiles)
{
    std::string file1 = test_dir + "/file1.txt";
    std::string file2 = test_dir + "/file2.txt";
    createFile(file1, "content1");
    createFile(file2, "content2");

    CommandContext ctx;
    ctx.args = {file1, file2};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());

    std::string out = getOutput();
    EXPECT_NE(out.find(file1), std::string::npos);
    EXPECT_NE(out.find(file2), std::string::npos);
}

TEST_F(Md5sumCommandTest, HelpOption)
{
    CommandContext ctx;
    ctx.args = {"--help"};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());

    std::string out = getOutput();
    EXPECT_NE(out.find("MD5"), std::string::npos);
}

// ============================================================================
// Base64Command Tests
// ============================================================================

class Base64CommandTest : public ::testing::Test
{
protected:
    Base64Command cmd;
    std::stringstream output;
    std::stringstream input;
    std::streambuf* old_cout;
    std::streambuf* old_cin;
    std::string test_dir;

    void SetUp() override
    {
        old_cout = std::cout.rdbuf(output.rdbuf());
        old_cin = std::cin.rdbuf(input.rdbuf());
        test_dir = "/tmp/homeshell_base64_test_" + std::to_string(::getpid());
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

    void createFile(const std::string& path, const std::string& content)
    {
        std::ofstream file(path);
        file << content;
    }
};

TEST_F(Base64CommandTest, CommandName)
{
    EXPECT_EQ(cmd.getName(), "base64");
}

TEST_F(Base64CommandTest, BasicEncode)
{
    setInput("hello\n");

    CommandContext ctx;
    ctx.args = {};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());

    std::string out = getOutput();
    EXPECT_NE(out.find("aGVsbG8K"), std::string::npos);
}

TEST_F(Base64CommandTest, BasicDecode)
{
    setInput("aGVsbG8K\n");

    CommandContext ctx;
    ctx.args = {"-d"};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());

    std::string out = getOutput();
    EXPECT_NE(out.find("hello"), std::string::npos);
}

TEST_F(Base64CommandTest, EncodeFile)
{
    std::string file = test_dir + "/test.txt";
    createFile(file, "test data\n");

    CommandContext ctx;
    ctx.args = {file};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());

    std::string out = getOutput();
    EXPECT_FALSE(out.empty());
}

TEST_F(Base64CommandTest, DecodeFile)
{
    std::string file = test_dir + "/encoded.txt";
    createFile(file, "dGVzdCBkYXRhCg==");

    CommandContext ctx;
    ctx.args = {"-d", file};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());

    std::string out = getOutput();
    EXPECT_NE(out.find("test data"), std::string::npos);
}

TEST_F(Base64CommandTest, RoundTrip)
{
    // Test encode then decode gets back original
    std::string file = test_dir + "/test.txt";
    createFile(file, "The quick brown fox jumps over the lazy dog\n");

    // Encode to file
    CommandContext ctx1;
    ctx1.args = {file};
    cmd.execute(ctx1);
    std::string encoded = getOutput();

    // Verify it's base64
    EXPECT_FALSE(encoded.empty());
    
    // Clear and create encoded file
    output.str("");
    output.clear();
    
    std::string encoded_file = test_dir + "/encoded.txt";
    createFile(encoded_file, encoded);

    // Decode
    CommandContext ctx2;
    ctx2.args = {"-d", encoded_file};
    cmd.execute(ctx2);
    std::string decoded = getOutput();

    // Should contain the original text
    EXPECT_NE(decoded.find("The quick brown fox"), std::string::npos);
}

TEST_F(Base64CommandTest, EmptyInput)
{
    setInput("");

    CommandContext ctx;
    ctx.args = {};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());
}

TEST_F(Base64CommandTest, HelpOption)
{
    CommandContext ctx;
    ctx.args = {"--help"};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());

    std::string out = getOutput();
    EXPECT_NE(out.find("base64"), std::string::npos);
}

