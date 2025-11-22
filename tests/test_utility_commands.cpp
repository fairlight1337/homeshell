#include <gtest/gtest.h>
#include <homeshell/commands/ChmodCommand.hpp>
#include <homeshell/commands/VersionCommand.hpp>
#include <homeshell/commands/PythonCommand.hpp>
#include <homeshell/Shell.hpp>
#include <homeshell/VirtualFilesystem.hpp>
#include <homeshell/Config.hpp>
#include <homeshell/TerminalInfo.hpp>

#include <fstream>
#include <sys/stat.h>
#include <unistd.h>

namespace homeshell
{

// ============================================================================
// ChmodCommand Tests
// ============================================================================

class ChmodCommandTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        command_ = std::make_shared<ChmodCommand>();
        test_file_ = "/tmp/test_chmod_file.txt";
        
        // Create test file
        std::ofstream file(test_file_);
        file << "test content\n";
        file.close();
        
        // Set initial permissions to 644
        chmod(test_file_.c_str(), 0644);
    }

    void TearDown() override
    {
        // Clean up test file
        unlink(test_file_.c_str());
    }

    mode_t getFileMode(const std::string& path)
    {
        struct stat st;
        if (stat(path.c_str(), &st) == 0)
        {
            return st.st_mode & 0777;
        }
        return 0;
    }

    std::shared_ptr<ChmodCommand> command_;
    std::string test_file_;
};

TEST_F(ChmodCommandTest, BasicInfo)
{
    EXPECT_EQ(command_->getName(), "chmod");
    EXPECT_FALSE(command_->getDescription().empty());
    EXPECT_EQ(command_->getType(), CommandType::Synchronous);
}

TEST_F(ChmodCommandTest, ExecuteOctalMode)
{
    CommandContext context;
    context.args = {"755", test_file_};
    
    Status status = command_->execute(context);
    EXPECT_TRUE(status.isSuccess());
    EXPECT_EQ(getFileMode(test_file_), 0755);
}

TEST_F(ChmodCommandTest, ExecuteSymbolicMode)
{
    CommandContext context;
    context.args = {"+x", test_file_};
    
    Status status = command_->execute(context);
    EXPECT_TRUE(status.isSuccess());
    EXPECT_EQ(getFileMode(test_file_), 0755);
}

TEST_F(ChmodCommandTest, ExecuteMultipleFiles)
{
    std::string test_file2 = "/tmp/test_chmod_file2.txt";
    std::ofstream file(test_file2);
    file << "test content\n";
    file.close();
    chmod(test_file2.c_str(), 0644);
    
    CommandContext context;
    context.args = {"755", test_file_, test_file2};
    
    Status status = command_->execute(context);
    EXPECT_TRUE(status.isSuccess());
    EXPECT_EQ(getFileMode(test_file_), 0755);
    EXPECT_EQ(getFileMode(test_file2), 0755);
    
    unlink(test_file2.c_str());
}

TEST_F(ChmodCommandTest, ExecuteMissingArguments)
{
    CommandContext context;
    context.args = {"755"}; // Missing file argument
    
    Status status = command_->execute(context);
    EXPECT_FALSE(status.isSuccess());
}

TEST_F(ChmodCommandTest, ExecuteNonexistentFile)
{
    CommandContext context;
    context.args = {"755", "/tmp/nonexistent_file_xyz.txt"};
    
    Status status = command_->execute(context);
    EXPECT_FALSE(status.isSuccess());
}

// ============================================================================
// VersionCommand Tests
// ============================================================================

class VersionCommandTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        command_ = std::make_shared<VersionCommand>();
    }

    std::shared_ptr<VersionCommand> command_;
};

TEST_F(VersionCommandTest, BasicInfo)
{
    EXPECT_EQ(command_->getName(), "version");
    EXPECT_FALSE(command_->getDescription().empty());
    EXPECT_EQ(command_->getType(), CommandType::Synchronous);
}

TEST_F(VersionCommandTest, Execute)
{
    CommandContext context;
    
    // Redirect stdout to capture output
    testing::internal::CaptureStdout();
    
    Status status = command_->execute(context);
    
    std::string output = testing::internal::GetCapturedStdout();
    
    EXPECT_TRUE(status.isSuccess());
    EXPECT_FALSE(output.empty());
    
    // Check for key version strings
    EXPECT_NE(output.find("Homeshell"), std::string::npos);
    EXPECT_NE(output.find("MicroPython"), std::string::npos);
    EXPECT_NE(output.find("SQLCipher"), std::string::npos);
    EXPECT_NE(output.find("miniz"), std::string::npos);
    EXPECT_NE(output.find("fmt"), std::string::npos);
}

// ============================================================================
// PythonCommand Tests
// ============================================================================

class PythonCommandTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        command_ = std::make_shared<PythonCommand>();
    }

    std::shared_ptr<PythonCommand> command_;
};

TEST_F(PythonCommandTest, BasicInfo)
{
    EXPECT_EQ(command_->getName(), "python");
    EXPECT_FALSE(command_->getDescription().empty());
    EXPECT_EQ(command_->getType(), CommandType::Synchronous);
}

TEST_F(PythonCommandTest, ExecuteInlineCode)
{
    CommandContext context;
    context.args = {"-c", "print('Hello from Python')"};
    
    testing::internal::CaptureStdout();
    
    Status status = command_->execute(context);
    
    std::string output = testing::internal::GetCapturedStdout();
    
    EXPECT_TRUE(status.isSuccess());
    EXPECT_NE(output.find("Hello from Python"), std::string::npos);
}

TEST_F(PythonCommandTest, ExecuteScriptFile)
{
    // Create test Python file
    std::string test_script = "/tmp/test_python_script.py";
    std::ofstream file(test_script);
    file << "x = 10\n";
    file << "y = x + 5\n";
    file.close();
    
    CommandContext context;
    context.args = {test_script};
    
    Status status = command_->execute(context);
    
    EXPECT_TRUE(status.isSuccess());
    
    unlink(test_script.c_str());
}

TEST_F(PythonCommandTest, ExecuteMissingCodeArgument)
{
    CommandContext context;
    context.args = {"-c"}; // Missing code argument
    
    Status status = command_->execute(context);
    EXPECT_FALSE(status.isSuccess());
}

TEST_F(PythonCommandTest, ExecuteNonexistentFile)
{
    CommandContext context;
    context.args = {"/tmp/nonexistent_python_file.py"};
    
    // MicroPython will print error message
    Status status = command_->execute(context);
    
    // Status is OK but error is printed to stderr
    EXPECT_TRUE(status.isSuccess());
}

// ============================================================================
// Executable File Detection Tests
// ============================================================================

class ExecutableFileTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        Config config = Config::loadDefault();
        TerminalInfo term_info = TerminalInfo::detect();
        shell_ = std::make_unique<Shell>(config, term_info);
        test_script_ = "/tmp/test_executable_script.sh";
        
        // Create executable test script
        std::ofstream file(test_script_);
        file << "#!/bin/bash\n";
        file << "echo 'Script executed successfully'\n";
        file.close();
        
        chmod(test_script_.c_str(), 0755);
    }

    void TearDown() override
    {
        unlink(test_script_.c_str());
    }

    std::unique_ptr<Shell> shell_;
    std::string test_script_;
};

TEST_F(ExecutableFileTest, ExecuteAbsolutePath)
{
    testing::internal::CaptureStdout();
    
    Status status = shell_->executeCommandLine(test_script_);
    
    std::string output = testing::internal::GetCapturedStdout();
    
    EXPECT_TRUE(status.isSuccess());
    EXPECT_NE(output.find("Script executed successfully"), std::string::npos);
}

TEST_F(ExecutableFileTest, ExecuteRelativePath)
{
    // Create script in /tmp and execute as ./script
    std::string cwd = getcwd(nullptr, 0);
    chdir("/tmp");
    
    testing::internal::CaptureStdout();
    
    Status status = shell_->executeCommandLine("./test_executable_script.sh");
    
    std::string output = testing::internal::GetCapturedStdout();
    
    chdir(cwd.c_str());
    
    EXPECT_TRUE(status.isSuccess());
    EXPECT_NE(output.find("Script executed successfully"), std::string::npos);
}

TEST_F(ExecutableFileTest, ExecuteNonexistentFile)
{
    Status status = shell_->executeCommandLine("/tmp/nonexistent_script.sh");
    
    EXPECT_FALSE(status.isSuccess());
    EXPECT_NE(status.message.find("File not found"), std::string::npos);
}

TEST_F(ExecutableFileTest, ExecuteNonExecutableFile)
{
    std::string non_exec = "/tmp/test_non_executable.txt";
    std::ofstream file(non_exec);
    file << "This is not executable\n";
    file.close();
    chmod(non_exec.c_str(), 0644);
    
    Status status = shell_->executeCommandLine(non_exec);
    
    EXPECT_FALSE(status.isSuccess());
    EXPECT_NE(status.message.find("not executable"), std::string::npos);
    
    unlink(non_exec.c_str());
}

TEST_F(ExecutableFileTest, ExecuteWithArguments)
{
    // Create script that uses arguments
    std::string script = "/tmp/test_args_script.sh";
    std::ofstream file(script);
    file << "#!/bin/bash\n";
    file << "echo \"Arg1: $1\"\n";
    file << "echo \"Arg2: $2\"\n";
    file.close();
    chmod(script.c_str(), 0755);
    
    testing::internal::CaptureStdout();
    
    Status status = shell_->executeCommandLine(script + " hello world");
    
    std::string output = testing::internal::GetCapturedStdout();
    
    EXPECT_TRUE(status.isSuccess());
    EXPECT_NE(output.find("Arg1: hello"), std::string::npos);
    EXPECT_NE(output.find("Arg2: world"), std::string::npos);
    
    unlink(script.c_str());
}

} // namespace homeshell

