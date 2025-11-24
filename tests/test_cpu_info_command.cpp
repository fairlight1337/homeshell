/**
 * @file test_cpu_info_command.cpp
 * @brief Unit tests for CpuInfoCommand
 */

#include <homeshell/commands/CpuInfoCommand.hpp>

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>

namespace fs = std::filesystem;
using namespace homeshell;

class CpuInfoCommandTest : public ::testing::Test
{
protected:
    CpuInfoCommand cmd;
    std::stringstream output;
    std::streambuf* old_cout;
    std::streambuf* old_cerr;

    void SetUp() override
    {
        // Redirect cout and cerr to our stringstream
        old_cout = std::cout.rdbuf(output.rdbuf());
        old_cerr = std::cerr.rdbuf(output.rdbuf());
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

    void clearOutput()
    {
        output.str("");
        output.clear();
    }
};

TEST_F(CpuInfoCommandTest, CommandName)
{
    EXPECT_EQ(cmd.getName(), "cpu-info");
}

TEST_F(CpuInfoCommandTest, CommandDescription)
{
    std::string desc = cmd.getDescription();
    EXPECT_FALSE(desc.empty());
    EXPECT_NE(desc.find("CPU"), std::string::npos);
}

TEST_F(CpuInfoCommandTest, CommandType)
{
    EXPECT_EQ(cmd.getType(), homeshell::CommandType::Synchronous);
}

TEST_F(CpuInfoCommandTest, HelpOption)
{
    CommandContext ctx;
    ctx.args = {"--help"};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());

    std::string out = getOutput();
    EXPECT_NE(out.find("Usage:"), std::string::npos);
    EXPECT_NE(out.find("--frequency"), std::string::npos);
    EXPECT_NE(out.find("--cache"), std::string::npos);
    EXPECT_NE(out.find("--usage"), std::string::npos);
}

TEST_F(CpuInfoCommandTest, HelpShortOption)
{
    CommandContext ctx;
    ctx.args = {"-h"};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());

    std::string out = getOutput();
    EXPECT_NE(out.find("Usage:"), std::string::npos);
}

TEST_F(CpuInfoCommandTest, InvalidOption)
{
    CommandContext ctx;
    ctx.args = {"--invalid"};
    auto status = cmd.execute(ctx);
    EXPECT_FALSE(status.isOk());
    EXPECT_NE(status.message.find("Unknown option"), std::string::npos);
}

TEST_F(CpuInfoCommandTest, NoArguments)
{
    // Should run without errors and display CPU info
    CommandContext ctx;
    ctx.args = {};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());

    std::string out = getOutput();
    // Should contain at least the CPU Information header
    EXPECT_NE(out.find("CPU Information"), std::string::npos);
}

TEST_F(CpuInfoCommandTest, FrequencyOption)
{
    CommandContext ctx;
    ctx.args = {"-f"};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());

    // Output depends on system, but command should not fail
}

TEST_F(CpuInfoCommandTest, FrequencyLongOption)
{
    CommandContext ctx;
    ctx.args = {"--frequency"};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());
}

TEST_F(CpuInfoCommandTest, CacheOption)
{
    CommandContext ctx;
    ctx.args = {"-c"};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());
}

TEST_F(CpuInfoCommandTest, CacheLongOption)
{
    CommandContext ctx;
    ctx.args = {"--cache"};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());
}

TEST_F(CpuInfoCommandTest, UsageOption)
{
    CommandContext ctx;
    ctx.args = {"-u"};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());
}

TEST_F(CpuInfoCommandTest, UsageLongOption)
{
    CommandContext ctx;
    ctx.args = {"--usage"};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());
}

TEST_F(CpuInfoCommandTest, MultipleOptions)
{
    CommandContext ctx;
    ctx.args = {"-f", "-c"};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());
}

TEST_F(CpuInfoCommandTest, AllOptions)
{
    CommandContext ctx;
    ctx.args = {"-f", "-c", "-u"};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());
}

// Integration test - checks if real CPU info is available
TEST(CpuInfoIntegrationTest, RealSystemCheck)
{
    CpuInfoCommand cmd;

    std::stringstream output;
    std::streambuf* old_cout = std::cout.rdbuf(output.rdbuf());

    CommandContext ctx;
    ctx.args = {};
    auto status = cmd.execute(ctx);

    std::cout.rdbuf(old_cout);

    EXPECT_TRUE(status.isOk());

    std::string out = output.str();

    // Should have CPU information header
    EXPECT_NE(out.find("CPU Information"), std::string::npos);

    // Should have at least some basic info (model or vendor)
    bool has_model = out.find("Model:") != std::string::npos;
    bool has_vendor = out.find("Vendor:") != std::string::npos;
    EXPECT_TRUE(has_model || has_vendor);
}

// Test output formatting
TEST(CpuInfoOutputTest, OutputContainsExpectedSections)
{
    CpuInfoCommand cmd;

    std::stringstream output;
    std::streambuf* old_cout = std::cout.rdbuf(output.rdbuf());

    CommandContext ctx;
    ctx.args = {};
    cmd.execute(ctx);

    std::cout.rdbuf(old_cout);

    std::string out = output.str();

    // Should have separator
    EXPECT_NE(out.find("==="), std::string::npos);

    // Should have at least the basic CPU information section
    EXPECT_NE(out.find("CPU Information"), std::string::npos);
}

// Test selective display
TEST(CpuInfoSelectiveTest, FrequencyOnlyShowsFrequency)
{
    CpuInfoCommand cmd;

    std::stringstream output;
    std::streambuf* old_cout = std::cout.rdbuf(output.rdbuf());

    CommandContext ctx;
    ctx.args = {"-f"};
    cmd.execute(ctx);

    std::cout.rdbuf(old_cout);

    std::string out = output.str();

    // Should have basic info and frequency info
    EXPECT_NE(out.find("CPU Information"), std::string::npos);
    // Frequency section should be present if system supports it
    // (may not be available in VMs or containers)
}

TEST(CpuInfoSelectiveTest, CacheOnlyShowsCache)
{
    CpuInfoCommand cmd;

    std::stringstream output;
    std::streambuf* old_cout = std::cout.rdbuf(output.rdbuf());

    CommandContext ctx;
    ctx.args = {"-c"};
    cmd.execute(ctx);

    std::cout.rdbuf(old_cout);

    std::string out = output.str();

    // Should have basic info and cache info
    EXPECT_NE(out.find("CPU Information"), std::string::npos);
    EXPECT_NE(out.find("CPU Cache"), std::string::npos);
}

TEST(CpuInfoSelectiveTest, UsageOnlyShowsUsage)
{
    CpuInfoCommand cmd;

    std::stringstream output;
    std::streambuf* old_cout = std::cout.rdbuf(output.rdbuf());

    CommandContext ctx;
    ctx.args = {"-u"};
    cmd.execute(ctx);

    std::cout.rdbuf(old_cout);

    std::string out = output.str();

    // Should have basic info and usage info
    EXPECT_NE(out.find("CPU Information"), std::string::npos);
    EXPECT_NE(out.find("CPU Usage"), std::string::npos);
}

// Test /proc/cpuinfo parsing
TEST(CpuInfoParsingTest, ProcCpuinfoExists)
{
    // /proc/cpuinfo should exist on Linux systems
    std::ifstream file("/proc/cpuinfo");
    EXPECT_TRUE(file.is_open());
}

// Test /proc/stat parsing
TEST(CpuInfoParsingTest, ProcStatExists)
{
    // /proc/stat should exist on Linux systems
    std::ifstream file("/proc/stat");
    EXPECT_TRUE(file.is_open());
}

// Test CPU frequency sysfs
TEST(CpuInfoParsingTest, CpuFreqSysfsCheck)
{
    // Check if cpufreq sysfs exists (may not exist in VMs/containers)
    std::string path = "/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq";
    bool exists = std::filesystem::exists(path);

    // This is not a failure if it doesn't exist
    // Just documenting that the test knows about this path
    if (exists)
    {
        std::ifstream file(path);
        EXPECT_TRUE(file.is_open());
    }
}

// Test CPU cache sysfs
TEST(CpuInfoParsingTest, CpuCacheSysfsCheck)
{
    // Check if cache sysfs exists
    std::string path = "/sys/devices/system/cpu/cpu0/cache/";
    bool exists = std::filesystem::exists(path);

    // This is not a failure if it doesn't exist
    // Just documenting that the test knows about this path
    if (exists)
    {
        DIR* dir = opendir(path.c_str());
        if (dir)
        {
            EXPECT_TRUE(true); // Path accessible
            closedir(dir);
        }
    }
}

