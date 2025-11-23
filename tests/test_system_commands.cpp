#include <gtest/gtest.h>
#include <homeshell/commands/TopCommand.hpp>
#include <homeshell/commands/KillCommand.hpp>
#include <unistd.h>
#include <csignal>
#include <sys/types.h>
#include <sys/wait.h>

using namespace homeshell;

class SystemCommandsTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
    }

    void TearDown() override
    {
    }
};

// TopCommand Tests
TEST_F(SystemCommandsTest, TopCommandGetName)
{
    TopCommand cmd;
    EXPECT_EQ(cmd.getName(), "top");
}

TEST_F(SystemCommandsTest, TopCommandGetDescription)
{
    TopCommand cmd;
    EXPECT_FALSE(cmd.getDescription().empty());
}

TEST_F(SystemCommandsTest, TopCommandGetType)
{
    TopCommand cmd;
    EXPECT_EQ(cmd.getType(), CommandType::Synchronous);
}

#ifdef __linux__
TEST_F(SystemCommandsTest, TopExecute)
{
    TopCommand cmd;
    CommandContext ctx;
    ctx.args = {};
    
    Status status = cmd.execute(ctx);
    EXPECT_TRUE(status.isSuccess());
}
#else
TEST_F(SystemCommandsTest, TopUnsupportedPlatform)
{
    TopCommand cmd;
    CommandContext ctx;
    ctx.args = {};
    
    Status status = cmd.execute(ctx);
    EXPECT_TRUE(status.isSuccess() == false);
}
#endif

TEST_F(SystemCommandsTest, TopSupportsCancellation)
{
    TopCommand cmd;
    EXPECT_FALSE(cmd.supportsCancellation());
}

// KillCommand Tests
TEST_F(SystemCommandsTest, KillCommandGetName)
{
    KillCommand cmd;
    EXPECT_EQ(cmd.getName(), "kill");
}

TEST_F(SystemCommandsTest, KillCommandGetDescription)
{
    KillCommand cmd;
    EXPECT_FALSE(cmd.getDescription().empty());
}

TEST_F(SystemCommandsTest, KillCommandGetType)
{
    KillCommand cmd;
    EXPECT_EQ(cmd.getType(), CommandType::Synchronous);
}

TEST_F(SystemCommandsTest, KillNoArguments)
{
    KillCommand cmd;
    CommandContext ctx;
    ctx.args = {};
    
    Status status = cmd.execute(ctx);
    EXPECT_TRUE(status.isSuccess() == false);
}

TEST_F(SystemCommandsTest, KillInvalidPID)
{
    KillCommand cmd;
    CommandContext ctx;
    ctx.args = {"invalid_pid"};
    
    Status status = cmd.execute(ctx);
    EXPECT_TRUE(status.isSuccess() == false);
}

TEST_F(SystemCommandsTest, KillNegativePID)
{
    KillCommand cmd;
    CommandContext ctx;
    ctx.args = {"-1"};
    
    Status status = cmd.execute(ctx);
    EXPECT_TRUE(status.isSuccess() == false);
}

TEST_F(SystemCommandsTest, KillZeroPID)
{
    KillCommand cmd;
    CommandContext ctx;
    ctx.args = {"0"};
    
    Status status = cmd.execute(ctx);
    EXPECT_TRUE(status.isSuccess() == false);
}

TEST_F(SystemCommandsTest, KillNonexistentPID)
{
    KillCommand cmd;
    CommandContext ctx;
    ctx.args = {"999999"};  // Very unlikely to exist
    
    Status status = cmd.execute(ctx);
    EXPECT_TRUE(status.isSuccess() == false);
}

TEST_F(SystemCommandsTest, KillWithSignalOption)
{
    // Create a child process that we can safely kill
    pid_t pid = fork();
    
    if (pid == 0)
    {
        // Child process - sleep indefinitely
        while (true)
        {
            sleep(1);
        }
        exit(0);
    }
    else if (pid > 0)
    {
        // Parent process - give child time to start
        usleep(100000);  // 100ms
        
        // Send SIGTERM to the child process
        KillCommand cmd;
        CommandContext ctx;
        ctx.args = {"-s", "TERM", std::to_string(pid)};
        
        Status status = cmd.execute(ctx);
        EXPECT_TRUE(status.isSuccess());
        
        // Wait for child to terminate
        int wait_status;
        waitpid(pid, &wait_status, 0);
        EXPECT_TRUE(WIFSIGNALED(wait_status));
        EXPECT_EQ(WTERMSIG(wait_status), SIGTERM);
    }
    else
    {
        FAIL() << "Failed to fork";
    }
}

TEST_F(SystemCommandsTest, KillWithNumericSignal)
{
    // Create a child process
    pid_t pid = fork();
    
    if (pid == 0)
    {
        // Child process - sleep indefinitely
        while (true)
        {
            sleep(1);
        }
        exit(0);
    }
    else if (pid > 0)
    {
        // Parent process
        usleep(100000);  // 100ms
        
        // Send SIGTERM using numeric format
        KillCommand cmd;
        CommandContext ctx;
        ctx.args = {"-15", std::to_string(pid)};
        
        Status status = cmd.execute(ctx);
        EXPECT_TRUE(status.isSuccess());
        
        // Wait for child
        int wait_status;
        waitpid(pid, &wait_status, 0);
        EXPECT_TRUE(WIFSIGNALED(wait_status));
    }
    else
    {
        FAIL() << "Failed to fork";
    }
}

TEST_F(SystemCommandsTest, KillMultiplePIDs)
{
    // Create two child processes
    pid_t pid1 = fork();
    
    if (pid1 == 0)
    {
        // First child
        while (true) { sleep(1); }
        exit(0);
    }
    
    pid_t pid2 = fork();
    
    if (pid2 == 0)
    {
        // Second child
        while (true) { sleep(1); }
        exit(0);
    }
    
    if (pid1 > 0 && pid2 > 0)
    {
        // Parent process
        usleep(100000);  // 100ms
        
        // Kill both processes
        KillCommand cmd;
        CommandContext ctx;
        ctx.args = {std::to_string(pid1), std::to_string(pid2)};
        
        Status status = cmd.execute(ctx);
        EXPECT_TRUE(status.isSuccess());
        
        // Wait for both children
        int wait_status;
        waitpid(pid1, &wait_status, 0);
        waitpid(pid2, &wait_status, 0);
    }
    else
    {
        FAIL() << "Failed to fork";
    }
}

TEST_F(SystemCommandsTest, KillUnknownSignal)
{
    KillCommand cmd;
    CommandContext ctx;
    ctx.args = {"-s", "UNKNOWN", "1"};
    
    Status status = cmd.execute(ctx);
    EXPECT_TRUE(status.isSuccess() == false);
}

TEST_F(SystemCommandsTest, KillSIGKILL)
{
    // Create a child process
    pid_t pid = fork();
    
    if (pid == 0)
    {
        // Child process - ignore SIGTERM but can't ignore SIGKILL
        signal(SIGTERM, SIG_IGN);
        while (true) { sleep(1); }
        exit(0);
    }
    else if (pid > 0)
    {
        // Parent process
        usleep(100000);  // 100ms
        
        // Send SIGKILL
        KillCommand cmd;
        CommandContext ctx;
        ctx.args = {"-9", std::to_string(pid)};
        
        Status status = cmd.execute(ctx);
        EXPECT_TRUE(status.isSuccess());
        
        // Wait for child
        int wait_status;
        waitpid(pid, &wait_status, 0);
        EXPECT_TRUE(WIFSIGNALED(wait_status));
        EXPECT_EQ(WTERMSIG(wait_status), SIGKILL);
    }
    else
    {
        FAIL() << "Failed to fork";
    }
}

TEST_F(SystemCommandsTest, KillSupportsCancellation)
{
    KillCommand cmd;
    EXPECT_FALSE(cmd.supportsCancellation());
}

TEST_F(SystemCommandsTest, KillMixedValidInvalid)
{
    // Create one child process
    pid_t pid = fork();
    
    if (pid == 0)
    {
        // Child process
        while (true) { sleep(1); }
        exit(0);
    }
    else if (pid > 0)
    {
        // Parent process
        usleep(100000);  // 100ms
        
        // Try to kill one valid and one invalid PID
        KillCommand cmd;
        CommandContext ctx;
        ctx.args = {std::to_string(pid), "999999"};  // Valid, then invalid
        
        Status status = cmd.execute(ctx);
        EXPECT_TRUE(status.isSuccess() == false);  // Should fail because one failed
        
        // Clean up the child that was successfully killed
        int wait_status;
        waitpid(pid, &wait_status, WNOHANG);
    }
    else
    {
        FAIL() << "Failed to fork";
    }
}

TEST_F(SystemCommandsTest, KillSignalFlagNoPID)
{
    KillCommand cmd;
    CommandContext ctx;
    ctx.args = {"-s", "TERM"};  // No PID after signal
    
    Status status = cmd.execute(ctx);
    EXPECT_TRUE(status.isSuccess() == false);
}

TEST_F(SystemCommandsTest, KillDashFormatInvalidSignal)
{
    KillCommand cmd;
    CommandContext ctx;
    ctx.args = {"-INVALID", "1"};
    
    Status status = cmd.execute(ctx);
    EXPECT_TRUE(status.isSuccess() == false);
}

