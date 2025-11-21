#include <homeshell/Command.hpp>
#include <homeshell/commands/LsblkCommand.hpp>
#include <homeshell/commands/LspciCommand.hpp>
#include <homeshell/commands/LsusbCommand.hpp>
#include <homeshell/commands/SysinfoCommand.hpp>
#include <gtest/gtest.h>
#include <iostream>
#include <sstream>

using namespace homeshell;

// Test fixture for sysinfo commands
class SysinfoCommandsTest : public ::testing::Test
{
protected:
    CommandContext context_;
};

// Test lsusb command
TEST_F(SysinfoCommandsTest, LsusbBasicTest)
{
    LsusbCommand cmd;
    
    EXPECT_EQ(cmd.getName(), "lsusb");
    EXPECT_EQ(cmd.getType(), CommandType::Synchronous);
    EXPECT_FALSE(cmd.getDescription().empty());
}

TEST_F(SysinfoCommandsTest, LsusbExecute)
{
    LsusbCommand cmd;
    
    Status status = cmd.execute(context_);
    
    // Should succeed on Linux, or fail gracefully on other platforms
#ifdef __linux__
    EXPECT_TRUE(status.isSuccess());
#else
    // On non-Linux, should return error
    EXPECT_TRUE(status.isError());
#endif
}

// Test lspci command
TEST_F(SysinfoCommandsTest, LspciBasicTest)
{
    LspciCommand cmd;
    
    EXPECT_EQ(cmd.getName(), "lspci");
    EXPECT_EQ(cmd.getType(), CommandType::Synchronous);
    EXPECT_FALSE(cmd.getDescription().empty());
}

TEST_F(SysinfoCommandsTest, LspciExecute)
{
    LspciCommand cmd;
    
    Status status = cmd.execute(context_);
    
#ifdef __linux__
    EXPECT_TRUE(status.isSuccess());
#else
    EXPECT_TRUE(status.isError());
#endif
}

// Test lsblk command
TEST_F(SysinfoCommandsTest, LsblkBasicTest)
{
    LsblkCommand cmd;
    
    EXPECT_EQ(cmd.getName(), "lsblk");
    EXPECT_EQ(cmd.getType(), CommandType::Synchronous);
    EXPECT_FALSE(cmd.getDescription().empty());
}

TEST_F(SysinfoCommandsTest, LsblkExecute)
{
    LsblkCommand cmd;
    
    Status status = cmd.execute(context_);
    
#ifdef __linux__
    EXPECT_TRUE(status.isSuccess());
#else
    EXPECT_TRUE(status.isError());
#endif
}

// Test sysinfo command
TEST_F(SysinfoCommandsTest, SysinfoBasicTest)
{
    SysinfoCommand cmd;
    
    EXPECT_EQ(cmd.getName(), "sysinfo");
    EXPECT_EQ(cmd.getType(), CommandType::Synchronous);
    EXPECT_FALSE(cmd.getDescription().empty());
}

TEST_F(SysinfoCommandsTest, SysinfoExecute)
{
    SysinfoCommand cmd;
    
    Status status = cmd.execute(context_);
    
#ifdef __linux__
    EXPECT_TRUE(status.isSuccess());
#else
    EXPECT_TRUE(status.isError());
#endif
}

