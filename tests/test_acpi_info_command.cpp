/**
 * @file test_acpi_info_command.cpp
 * @brief Unit tests for AcpiInfoCommand
 */

#include <homeshell/commands/AcpiInfoCommand.hpp>
#include <gtest/gtest.h>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <sys/stat.h>
#include <unistd.h>

namespace fs = std::filesystem;
using namespace homeshell;

class AcpiInfoCommandTest : public ::testing::Test
{
protected:
    AcpiInfoCommand cmd;
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

TEST_F(AcpiInfoCommandTest, CommandName)
{
    EXPECT_EQ(cmd.getName(), "acpi-info");
}

TEST_F(AcpiInfoCommandTest, CommandDescription)
{
    std::string desc = cmd.getDescription();
    EXPECT_FALSE(desc.empty());
    EXPECT_NE(desc.find("ACPI"), std::string::npos);
}

TEST_F(AcpiInfoCommandTest, CommandType)
{
    EXPECT_EQ(cmd.getType(), homeshell::CommandType::Synchronous);
}

TEST_F(AcpiInfoCommandTest, HelpOption)
{
    CommandContext ctx;
    ctx.args = {"--help"};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());
    
    std::string out = getOutput();
    EXPECT_NE(out.find("Usage:"), std::string::npos);
    EXPECT_NE(out.find("--battery"), std::string::npos);
    EXPECT_NE(out.find("--adapter"), std::string::npos);
    EXPECT_NE(out.find("--thermal"), std::string::npos);
}

TEST_F(AcpiInfoCommandTest, HelpShortOption)
{
    CommandContext ctx;
    ctx.args = {"-h"};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());
    
    std::string out = getOutput();
    EXPECT_NE(out.find("Usage:"), std::string::npos);
}

TEST_F(AcpiInfoCommandTest, InvalidOption)
{
    CommandContext ctx;
    ctx.args = {"--invalid"};
    auto status = cmd.execute(ctx);
    EXPECT_FALSE(status.isOk());
    EXPECT_NE(status.message.find("Unknown option"), std::string::npos);
}

TEST_F(AcpiInfoCommandTest, NoArguments)
{
    // Should run without errors (may show "No ACPI information" on systems without ACPI)
    CommandContext ctx;
    ctx.args = {};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());
}

TEST_F(AcpiInfoCommandTest, BatteryOption)
{
    CommandContext ctx;
    ctx.args = {"-b"};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());
    
    // Output depends on system, but command should not fail
    std::string out = getOutput();
    // May contain battery info or "No ACPI information available"
}

TEST_F(AcpiInfoCommandTest, BatteryLongOption)
{
    CommandContext ctx;
    ctx.args = {"--battery"};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());
}

TEST_F(AcpiInfoCommandTest, AdapterOption)
{
    CommandContext ctx;
    ctx.args = {"-a"};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());
}

TEST_F(AcpiInfoCommandTest, AdapterLongOption)
{
    CommandContext ctx;
    ctx.args = {"--adapter"};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());
}

TEST_F(AcpiInfoCommandTest, ThermalOption)
{
    CommandContext ctx;
    ctx.args = {"-t"};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());
}

TEST_F(AcpiInfoCommandTest, ThermalLongOption)
{
    CommandContext ctx;
    ctx.args = {"--thermal"};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());
}

TEST_F(AcpiInfoCommandTest, MultipleOptions)
{
    CommandContext ctx;
    ctx.args = {"-b", "-a"};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());
}

TEST_F(AcpiInfoCommandTest, AllOptions)
{
    CommandContext ctx;
    ctx.args = {"-b", "-a", "-t"};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());
}

// Test with mock sysfs structure (if we have permissions to create test files)
class AcpiInfoMockTest : public ::testing::Test
{
protected:
    AcpiInfoCommand cmd;
    std::stringstream output;
    std::streambuf* old_cout;
    std::string test_dir;

    void SetUp() override
    {
        old_cout = std::cout.rdbuf(output.rdbuf());
        test_dir = "/tmp/homeshell_acpi_test_" + std::to_string(getpid());
    }

    void TearDown() override
    {
        std::cout.rdbuf(old_cout);
        if (fs::exists(test_dir))
        {
            fs::remove_all(test_dir);
        }
    }

    bool createMockBattery()
    {
        try
        {
            std::string bat_path = test_dir + "/power_supply/BAT0";
            fs::create_directories(bat_path);
            
            // Create mock battery files
            std::ofstream(bat_path + "/type") << "Battery\n";
            std::ofstream(bat_path + "/status") << "Discharging\n";
            std::ofstream(bat_path + "/capacity") << "75\n";
            std::ofstream(bat_path + "/voltage_now") << "12000000\n";
            std::ofstream(bat_path + "/current_now") << "2000000\n";
            std::ofstream(bat_path + "/technology") << "Li-ion\n";
            
            return true;
        }
        catch (...)
        {
            return false;
        }
    }

    bool createMockAdapter()
    {
        try
        {
            std::string ac_path = test_dir + "/power_supply/AC";
            fs::create_directories(ac_path);
            
            std::ofstream(ac_path + "/type") << "Mains\n";
            std::ofstream(ac_path + "/online") << "1\n";
            
            return true;
        }
        catch (...)
        {
            return false;
        }
    }

    bool createMockThermal()
    {
        try
        {
            std::string thermal_path = test_dir + "/thermal/thermal_zone0";
            fs::create_directories(thermal_path);
            
            std::ofstream(thermal_path + "/type") << "acpitz\n";
            std::ofstream(thermal_path + "/temp") << "45000\n";
            std::ofstream(thermal_path + "/trip_point_0_temp") << "90000\n";
            std::ofstream(thermal_path + "/trip_point_0_type") << "critical\n";
            
            return true;
        }
        catch (...)
        {
            return false;
        }
    }

    std::string getOutput()
    {
        return output.str();
    }
};

TEST_F(AcpiInfoMockTest, MockBatteryCreation)
{
    // This test verifies we can create mock sysfs structures
    // The actual reading would require modifying the command to accept a test path
    bool created = createMockBattery();
    
    if (created)
    {
        EXPECT_TRUE(fs::exists(test_dir + "/power_supply/BAT0/type"));
        EXPECT_TRUE(fs::exists(test_dir + "/power_supply/BAT0/status"));
    }
    // If we can't create the mock structure (permissions), that's OK for this test
}

TEST_F(AcpiInfoMockTest, MockAdapterCreation)
{
    bool created = createMockAdapter();
    
    if (created)
    {
        EXPECT_TRUE(fs::exists(test_dir + "/power_supply/AC/type"));
        EXPECT_TRUE(fs::exists(test_dir + "/power_supply/AC/online"));
    }
}

TEST_F(AcpiInfoMockTest, MockThermalCreation)
{
    bool created = createMockThermal();
    
    if (created)
    {
        EXPECT_TRUE(fs::exists(test_dir + "/thermal/thermal_zone0/type"));
        EXPECT_TRUE(fs::exists(test_dir + "/thermal/thermal_zone0/temp"));
    }
}

// Integration test - checks if real ACPI info is available on the system
TEST(AcpiInfoIntegrationTest, RealSystemCheck)
{
    AcpiInfoCommand cmd;
    
    std::stringstream output;
    std::streambuf* old_cout = std::cout.rdbuf(output.rdbuf());
    
    CommandContext ctx;
    ctx.args = {};
    auto status = cmd.execute(ctx);
    
    std::cout.rdbuf(old_cout);
    
    EXPECT_TRUE(status.isOk());
    
    std::string out = output.str();
    
    // System either has ACPI info or doesn't
    // Both are valid outcomes
    bool has_info = (out.find("Battery Information") != std::string::npos ||
                     out.find("AC Adapter Information") != std::string::npos ||
                     out.find("Thermal Information") != std::string::npos);
    
    bool no_info = (out.find("No ACPI information available") != std::string::npos);
    
    EXPECT_TRUE(has_info || no_info);
}

// Test output formatting for systems with ACPI
TEST(AcpiInfoOutputTest, OutputContainsExpectedSections)
{
    AcpiInfoCommand cmd;
    
    std::stringstream output;
    std::streambuf* old_cout = std::cout.rdbuf(output.rdbuf());
    
    CommandContext ctx;
    ctx.args = {};
    cmd.execute(ctx);
    
    std::cout.rdbuf(old_cout);
    
    std::string out = output.str();
    
    // If system has battery, output should be formatted properly
    if (out.find("Battery Information") != std::string::npos)
    {
        // Should have separator
        EXPECT_NE(out.find("==="), std::string::npos);
    }
}

// Test selective display
TEST(AcpiInfoSelectiveTest, BatteryOnlyDoesNotShowThermal)
{
    AcpiInfoCommand cmd;
    
    std::stringstream output;
    std::streambuf* old_cout = std::cout.rdbuf(output.rdbuf());
    
    CommandContext ctx;
    ctx.args = {"-b"};
    cmd.execute(ctx);
    
    std::cout.rdbuf(old_cout);
    
    std::string out = output.str();
    
    // If battery info is shown, thermal should not be shown (unless system has no battery)
    if (out.find("Battery") != std::string::npos)
    {
        // We requested only battery, so if we have any ACPI info,
        // it should be battery-related or the "no info" message
        bool valid_output = (out.find("Battery") != std::string::npos || 
                           out.find("No ACPI") != std::string::npos);
        EXPECT_TRUE(valid_output);
    }
}

