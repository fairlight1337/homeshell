#include <homeshell/OutputRedirection.hpp>
#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <iostream>

using namespace homeshell;

class OutputRedirectionTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Create a temp directory for test files
        test_dir_ = std::filesystem::temp_directory_path() / "homeshell_redir_test";
        std::filesystem::create_directories(test_dir_);
    }

    void TearDown() override
    {
        // Clean up test files
        std::filesystem::remove_all(test_dir_);
    }

    std::string readFile(const std::string& filename)
    {
        std::ifstream file(filename);
        if (!file)
            return "";
        
        std::string content((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
        return content;
    }

    std::filesystem::path test_dir_;
};

// Test parsing of > operator
TEST_F(OutputRedirectionTest, ParseStdoutTruncate)
{
    RedirectInfo info = OutputRedirection::parse("echo hello > output.txt");
    
    EXPECT_EQ(info.type, RedirectType::Stdout);
    EXPECT_EQ(info.mode, RedirectMode::Truncate);
    EXPECT_EQ(info.command, "echo hello");
    EXPECT_EQ(info.filename, "output.txt");
}

// Test parsing of >> operator
TEST_F(OutputRedirectionTest, ParseStdoutAppend)
{
    RedirectInfo info = OutputRedirection::parse("echo hello >> output.txt");
    
    EXPECT_EQ(info.type, RedirectType::Stdout);
    EXPECT_EQ(info.mode, RedirectMode::Append);
    EXPECT_EQ(info.command, "echo hello");
    EXPECT_EQ(info.filename, "output.txt");
}

// Test parsing of 2> operator
TEST_F(OutputRedirectionTest, ParseStderrTruncate)
{
    RedirectInfo info = OutputRedirection::parse("command 2> error.txt");
    
    EXPECT_EQ(info.type, RedirectType::Stderr);
    EXPECT_EQ(info.mode, RedirectMode::Truncate);
    EXPECT_EQ(info.command, "command");
    EXPECT_EQ(info.filename, "error.txt");
}

// Test parsing of 2>> operator
TEST_F(OutputRedirectionTest, ParseStderrAppend)
{
    RedirectInfo info = OutputRedirection::parse("command 2>> error.txt");
    
    EXPECT_EQ(info.type, RedirectType::Stderr);
    EXPECT_EQ(info.mode, RedirectMode::Append);
    EXPECT_EQ(info.command, "command");
    EXPECT_EQ(info.filename, "error.txt");
}

// Test parsing of &> operator
TEST_F(OutputRedirectionTest, ParseBothTruncate)
{
    RedirectInfo info = OutputRedirection::parse("command &> output.txt");
    
    EXPECT_EQ(info.type, RedirectType::Both);
    EXPECT_EQ(info.mode, RedirectMode::Truncate);
    EXPECT_EQ(info.command, "command");
    EXPECT_EQ(info.filename, "output.txt");
}

// Test parsing of &>> operator
TEST_F(OutputRedirectionTest, ParseBothAppend)
{
    RedirectInfo info = OutputRedirection::parse("command &>> output.txt");
    
    EXPECT_EQ(info.type, RedirectType::Both);
    EXPECT_EQ(info.mode, RedirectMode::Append);
    EXPECT_EQ(info.command, "command");
    EXPECT_EQ(info.filename, "output.txt");
}

// Test no redirection
TEST_F(OutputRedirectionTest, ParseNoRedirection)
{
    RedirectInfo info = OutputRedirection::parse("echo hello");
    
    EXPECT_EQ(info.type, RedirectType::None);
    EXPECT_EQ(info.command, "echo hello");
}

// Test command with spaces around operator
TEST_F(OutputRedirectionTest, ParseWithSpaces)
{
    RedirectInfo info = OutputRedirection::parse("echo hello   >   output.txt");
    
    EXPECT_EQ(info.type, RedirectType::Stdout);
    EXPECT_EQ(info.mode, RedirectMode::Truncate);
    EXPECT_EQ(info.command, "echo hello");
    EXPECT_EQ(info.filename, "output.txt");
}

// Test actual stdout redirection (truncate)
TEST_F(OutputRedirectionTest, ActualRedirectStdoutTruncate)
{
    std::string output_file = (test_dir_ / "stdout_test.txt").string();
    
    RedirectInfo info;
    info.type = RedirectType::Stdout;
    info.mode = RedirectMode::Truncate;
    info.filename = output_file;
    info.command = "test";
    
    OutputRedirection redirector;
    ASSERT_TRUE(redirector.redirect(info));
    
    // Write to stdout (should go to file)
    std::cout << "Hello, World!" << std::endl;
    
    // Restore output
    redirector.restore();
    
    // Check file contents
    std::string content = readFile(output_file);
    EXPECT_EQ(content, "Hello, World!\n");
}

// Test actual stdout redirection (append)
TEST_F(OutputRedirectionTest, ActualRedirectStdoutAppend)
{
    std::string output_file = (test_dir_ / "stdout_append_test.txt").string();
    
    // Write initial content
    {
        std::ofstream file(output_file);
        file << "Initial\n";
    }
    
    RedirectInfo info;
    info.type = RedirectType::Stdout;
    info.mode = RedirectMode::Append;
    info.filename = output_file;
    info.command = "test";
    
    OutputRedirection redirector;
    ASSERT_TRUE(redirector.redirect(info));
    
    // Write to stdout (should append to file)
    std::cout << "Appended" << std::endl;
    
    // Restore output
    redirector.restore();
    
    // Check file contents
    std::string content = readFile(output_file);
    EXPECT_EQ(content, "Initial\nAppended\n");
}

// Test actual stderr redirection
TEST_F(OutputRedirectionTest, ActualRedirectStderr)
{
    std::string error_file = (test_dir_ / "stderr_test.txt").string();
    
    RedirectInfo info;
    info.type = RedirectType::Stderr;
    info.mode = RedirectMode::Truncate;
    info.filename = error_file;
    info.command = "test";
    
    OutputRedirection redirector;
    ASSERT_TRUE(redirector.redirect(info));
    
    // Write to stderr (should go to file)
    std::cerr << "Error message" << std::endl;
    
    // Restore output
    redirector.restore();
    
    // Check file contents
    std::string content = readFile(error_file);
    EXPECT_EQ(content, "Error message\n");
}

// Test invalid file path
TEST_F(OutputRedirectionTest, InvalidFilePath)
{
    RedirectInfo info;
    info.type = RedirectType::Stdout;
    info.mode = RedirectMode::Truncate;
    info.filename = "/nonexistent/directory/file.txt";
    info.command = "test";
    
    OutputRedirection redirector;
    EXPECT_FALSE(redirector.redirect(info));
}

