/**
 * @file test_final_batch5.cpp
 * @brief Tests for final batch of commands: ps, tar, curl
 */

#include <homeshell/commands/CurlCommand.hpp>
#include <homeshell/commands/PsCommand.hpp>
#include <homeshell/commands/TarCommand.hpp>

#include <gtest/gtest.h>

#include <fstream>
#include <sstream>

// Helper to capture stdout/stderr
class OutputCapture
{
public:
    OutputCapture()
    {
        old_cout = std::cout.rdbuf(cout_buffer.rdbuf());
        old_cerr = std::cerr.rdbuf(cerr_buffer.rdbuf());
    }

    ~OutputCapture()
    {
        std::cout.rdbuf(old_cout);
        std::cerr.rdbuf(old_cerr);
    }

    std::string getCout() const
    {
        return cout_buffer.str();
    }

    std::string getCerr() const
    {
        return cerr_buffer.str();
    }

private:
    std::stringstream cout_buffer;
    std::stringstream cerr_buffer;
    std::streambuf* old_cout;
    std::streambuf* old_cerr;
};

// ============================================================================
// PS COMMAND TESTS
// ============================================================================

TEST(PsCommandTest, BasicExecution)
{
    homeshell::PsCommand cmd;
    homeshell::CommandContext ctx;

    OutputCapture capture;
    auto status = cmd.execute(ctx);

    EXPECT_TRUE(status.isOk());
    std::string output = capture.getCout();
    EXPECT_NE(output.find("PID"), std::string::npos);
    EXPECT_NE(output.find("STATE"), std::string::npos);
}

TEST(PsCommandTest, HelpOption)
{
    homeshell::PsCommand cmd;
    homeshell::CommandContext ctx;
    ctx.args = {"--help"};

    OutputCapture capture;
    auto status = cmd.execute(ctx);

    EXPECT_TRUE(status.isOk());
    std::string output = capture.getCout();
    EXPECT_NE(output.find("Usage:"), std::string::npos);
    EXPECT_NE(output.find("Display process status"), std::string::npos);
}

TEST(PsCommandTest, AllProcesses)
{
    homeshell::PsCommand cmd;
    homeshell::CommandContext ctx;
    ctx.args = {"-a"};

    OutputCapture capture;
    auto status = cmd.execute(ctx);

    EXPECT_TRUE(status.isOk());
    std::string output = capture.getCout();
    EXPECT_NE(output.find("PID"), std::string::npos);
}

TEST(PsCommandTest, AllProcessesLongOption)
{
    homeshell::PsCommand cmd;
    homeshell::CommandContext ctx;
    ctx.args = {"--all"};

    OutputCapture capture;
    auto status = cmd.execute(ctx);

    EXPECT_TRUE(status.isOk());
}

TEST(PsCommandTest, FullFormat)
{
    homeshell::PsCommand cmd;
    homeshell::CommandContext ctx;
    ctx.args = {"-f"};

    OutputCapture capture;
    auto status = cmd.execute(ctx);

    EXPECT_TRUE(status.isOk());
    std::string output = capture.getCout();
    EXPECT_NE(output.find("PID"), std::string::npos);
    EXPECT_NE(output.find("PPID"), std::string::npos);
    EXPECT_NE(output.find("USER"), std::string::npos);
}

TEST(PsCommandTest, AuxFormat)
{
    homeshell::PsCommand cmd;
    homeshell::CommandContext ctx;
    ctx.args = {"aux"};

    OutputCapture capture;
    auto status = cmd.execute(ctx);

    EXPECT_TRUE(status.isOk());
}

TEST(PsCommandTest, CombinedOptions)
{
    homeshell::PsCommand cmd;
    homeshell::CommandContext ctx;
    ctx.args = {"-a", "-f"};

    OutputCapture capture;
    auto status = cmd.execute(ctx);

    EXPECT_TRUE(status.isOk());
    std::string output = capture.getCout();
    EXPECT_NE(output.find("PPID"), std::string::npos);
}

// ============================================================================
// TAR COMMAND TESTS
// ============================================================================

TEST(TarCommandTest, HelpOption)
{
    homeshell::TarCommand cmd;
    homeshell::CommandContext ctx;
    ctx.args = {"--help"};

    OutputCapture capture;
    auto status = cmd.execute(ctx);

    EXPECT_TRUE(status.isOk());
    std::string output = capture.getCout();
    EXPECT_NE(output.find("Usage:"), std::string::npos);
    EXPECT_NE(output.find("tar"), std::string::npos);
}

TEST(TarCommandTest, CreateSimpleArchive)
{
    // Create test files
    std::ofstream test1("test_tar1.txt");
    test1 << "test content 1\n";
    test1.close();

    std::ofstream test2("test_tar2.txt");
    test2 << "test content 2\n";
    test2.close();

    homeshell::TarCommand cmd;
    homeshell::CommandContext ctx;
    ctx.args = {"-cf", "test.tar", "test_tar1.txt", "test_tar2.txt"};

    OutputCapture capture;
    auto status = cmd.execute(ctx);

    EXPECT_TRUE(status.isOk());
    EXPECT_TRUE(std::filesystem::exists("test.tar"));

    // Cleanup
    std::filesystem::remove("test_tar1.txt");
    std::filesystem::remove("test_tar2.txt");
    std::filesystem::remove("test.tar");
}

TEST(TarCommandTest, CreateWithVerbose)
{
    std::ofstream test("test_tar_verbose.txt");
    test << "verbose test\n";
    test.close();

    homeshell::TarCommand cmd;
    homeshell::CommandContext ctx;
    ctx.args = {"-cvf", "test_verbose.tar", "test_tar_verbose.txt"};

    OutputCapture capture;
    auto status = cmd.execute(ctx);

    EXPECT_TRUE(status.isOk());
    std::string output = capture.getCout();
    EXPECT_NE(output.find("test_tar_verbose.txt"), std::string::npos);

    // Cleanup
    std::filesystem::remove("test_tar_verbose.txt");
    std::filesystem::remove("test_verbose.tar");
}

TEST(TarCommandTest, ExtractArchive)
{
    // Create and extract
    std::ofstream test("test_tar_extract.txt");
    test << "extract test\n";
    test.close();

    homeshell::TarCommand cmd;
    homeshell::CommandContext ctx;
    ctx.args = {"-cf", "test_extract.tar", "test_tar_extract.txt"};

    OutputCapture capture1;
    auto status1 = cmd.execute(ctx);
    EXPECT_TRUE(status1.isOk());

    // Remove original
    std::filesystem::remove("test_tar_extract.txt");

    // Extract
    ctx.args = {"-xf", "test_extract.tar"};
    OutputCapture capture2;
    auto status2 = cmd.execute(ctx);

    EXPECT_TRUE(status2.isOk());
    EXPECT_TRUE(std::filesystem::exists("test_tar_extract.txt"));

    // Cleanup
    std::filesystem::remove("test_tar_extract.txt");
    std::filesystem::remove("test_extract.tar");
}

TEST(TarCommandTest, ListArchive)
{
    std::ofstream test("test_tar_list.txt");
    test << "list test\n";
    test.close();

    homeshell::TarCommand cmd;
    homeshell::CommandContext ctx;
    ctx.args = {"-cf", "test_list.tar", "test_tar_list.txt"};

    OutputCapture capture1;
    auto status1 = cmd.execute(ctx);
    EXPECT_TRUE(status1.isOk());

    // List contents
    ctx.args = {"-tf", "test_list.tar"};
    OutputCapture capture2;
    auto status2 = cmd.execute(ctx);

    EXPECT_TRUE(status2.isOk());
    std::string output = capture2.getCout();
    EXPECT_NE(output.find("test_tar_list.txt"), std::string::npos);

    // Cleanup
    std::filesystem::remove("test_tar_list.txt");
    std::filesystem::remove("test_list.tar");
}

TEST(TarCommandTest, MissingArchiveFile)
{
    homeshell::TarCommand cmd;
    homeshell::CommandContext ctx;
    ctx.args = {"-c"};

    OutputCapture capture;
    auto status = cmd.execute(ctx);

    EXPECT_FALSE(status.isOk());
}

TEST(TarCommandTest, MissingOperation)
{
    homeshell::TarCommand cmd;
    homeshell::CommandContext ctx;
    ctx.args = {"-f", "test.tar"};

    OutputCapture capture;
    auto status = cmd.execute(ctx);

    EXPECT_FALSE(status.isOk());
}

TEST(TarCommandTest, NonExistentFile)
{
    homeshell::TarCommand cmd;
    homeshell::CommandContext ctx;
    ctx.args = {"-cf", "test_nonexistent.tar", "file_that_does_not_exist.txt"};

    OutputCapture capture;
    auto status = cmd.execute(ctx);

    // Should succeed but skip non-existent file
    EXPECT_TRUE(status.isOk());
    std::string output = capture.getCerr();
    EXPECT_NE(output.find("No such file"), std::string::npos);

    // Cleanup if created
    std::filesystem::remove("test_nonexistent.tar");
}

// ============================================================================
// CURL COMMAND TESTS
// ============================================================================

TEST(CurlCommandTest, HelpOption)
{
    homeshell::CurlCommand cmd;
    homeshell::CommandContext ctx;
    ctx.args = {"--help"};

    OutputCapture capture;
    auto status = cmd.execute(ctx);

    EXPECT_TRUE(status.isOk());
    std::string output = capture.getCout();
    EXPECT_NE(output.find("Usage:"), std::string::npos);
    EXPECT_NE(output.find("curl"), std::string::npos);
}

TEST(CurlCommandTest, BasicUrl)
{
    homeshell::CurlCommand cmd;
    homeshell::CommandContext ctx;
    ctx.args = {"http://example.com"};

    OutputCapture capture;
    auto status = cmd.execute(ctx);

    EXPECT_TRUE(status.isOk());
    std::string output = capture.getCout();
    EXPECT_NE(output.find("Mock"), std::string::npos);
}

TEST(CurlCommandTest, SilentMode)
{
    homeshell::CurlCommand cmd;
    homeshell::CommandContext ctx;
    ctx.args = {"-s", "http://example.com"};

    OutputCapture capture;
    auto status = cmd.execute(ctx);

    EXPECT_TRUE(status.isOk());
    std::string output = capture.getCerr();
    EXPECT_EQ(output.find("would fetch"), std::string::npos);
}

TEST(CurlCommandTest, OutputToFile)
{
    homeshell::CurlCommand cmd;
    homeshell::CommandContext ctx;
    ctx.args = {"-o", "test_curl_output.html", "http://example.com"};

    OutputCapture capture;
    auto status = cmd.execute(ctx);

    EXPECT_TRUE(status.isOk());
    EXPECT_TRUE(std::filesystem::exists("test_curl_output.html"));

    // Verify content
    std::ifstream in("test_curl_output.html");
    std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    EXPECT_NE(content.find("Mock"), std::string::npos);

    // Cleanup
    std::filesystem::remove("test_curl_output.html");
}

TEST(CurlCommandTest, OutputToFileLongOption)
{
    homeshell::CurlCommand cmd;
    homeshell::CommandContext ctx;
    ctx.args = {"--output", "test_curl_long.html", "http://example.com"};

    OutputCapture capture;
    auto status = cmd.execute(ctx);

    EXPECT_TRUE(status.isOk());
    EXPECT_TRUE(std::filesystem::exists("test_curl_long.html"));

    // Cleanup
    std::filesystem::remove("test_curl_long.html");
}

TEST(CurlCommandTest, SilentWithOutput)
{
    homeshell::CurlCommand cmd;
    homeshell::CommandContext ctx;
    ctx.args = {"-s", "-o", "test_curl_silent.html", "http://api.example.com"};

    OutputCapture capture;
    auto status = cmd.execute(ctx);

    EXPECT_TRUE(status.isOk());
    std::string cerr_output = capture.getCerr();
    EXPECT_EQ(cerr_output.find("saved to"), std::string::npos);

    // Cleanup
    std::filesystem::remove("test_curl_silent.html");
}

TEST(CurlCommandTest, MissingUrl)
{
    homeshell::CurlCommand cmd;
    homeshell::CommandContext ctx;
    ctx.args = {};

    OutputCapture capture;
    auto status = cmd.execute(ctx);

    EXPECT_TRUE(status.isOk()); // Shows help
}

TEST(CurlCommandTest, UrlInOutput)
{
    homeshell::CurlCommand cmd;
    homeshell::CommandContext ctx;
    ctx.args = {"http://test.example.com/path"};

    OutputCapture capture;
    auto status = cmd.execute(ctx);

    EXPECT_TRUE(status.isOk());
    std::string output = capture.getCout();
    EXPECT_NE(output.find("http://test.example.com/path"), std::string::npos);
}

// ============================================================================
// INTEGRATION TESTS
// ============================================================================

TEST(FinalBatchIntegration, AllCommandsAvailable)
{
    homeshell::PsCommand ps_cmd;
    homeshell::TarCommand tar_cmd;
    homeshell::CurlCommand curl_cmd;

    EXPECT_EQ(ps_cmd.getName(), "ps");
    EXPECT_EQ(tar_cmd.getName(), "tar");
    EXPECT_EQ(curl_cmd.getName(), "curl");
}

TEST(FinalBatchIntegration, CommandDescriptions)
{
    homeshell::PsCommand ps_cmd;
    homeshell::TarCommand tar_cmd;
    homeshell::CurlCommand curl_cmd;

    EXPECT_FALSE(ps_cmd.getDescription().empty());
    EXPECT_FALSE(tar_cmd.getDescription().empty());
    EXPECT_FALSE(curl_cmd.getDescription().empty());
}

TEST(FinalBatchIntegration, AllSynchronous)
{
    homeshell::PsCommand ps_cmd;
    homeshell::TarCommand tar_cmd;
    homeshell::CurlCommand curl_cmd;

    EXPECT_EQ(ps_cmd.getType(), homeshell::CommandType::Synchronous);
    EXPECT_EQ(tar_cmd.getType(), homeshell::CommandType::Synchronous);
    EXPECT_EQ(curl_cmd.getType(), homeshell::CommandType::Synchronous);
}

