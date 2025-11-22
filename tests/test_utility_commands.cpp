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

TEST_F(ChmodCommandTest, ExecuteOctalWithLeadingZero)
{
    CommandContext context;
    context.args = {"0755", test_file_};
    
    Status status = command_->execute(context);
    EXPECT_TRUE(status.isSuccess());
    EXPECT_EQ(getFileMode(test_file_), 0755);
}

TEST_F(ChmodCommandTest, ExecuteSymbolicRemoveExecute)
{
    chmod(test_file_.c_str(), 0755);
    
    CommandContext context;
    context.args = {"-x", test_file_};
    
    Status status = command_->execute(context);
    EXPECT_TRUE(status.isSuccess());
    EXPECT_EQ(getFileMode(test_file_), 0644);
}

TEST_F(ChmodCommandTest, ExecuteSymbolicUserAdd)
{
    chmod(test_file_.c_str(), 0644);
    
    CommandContext context;
    context.args = {"u+x", test_file_};
    
    Status status = command_->execute(context);
    EXPECT_TRUE(status.isSuccess());
    EXPECT_EQ(getFileMode(test_file_), 0744);
}

TEST_F(ChmodCommandTest, ExecuteSymbolicGroupAdd)
{
    chmod(test_file_.c_str(), 0644);
    
    CommandContext context;
    context.args = {"g+x", test_file_};
    
    Status status = command_->execute(context);
    EXPECT_TRUE(status.isSuccess());
    EXPECT_EQ(getFileMode(test_file_), 0654);
}

TEST_F(ChmodCommandTest, ExecuteSymbolicOthersRemove)
{
    chmod(test_file_.c_str(), 0644);
    
    CommandContext context;
    context.args = {"o-r", test_file_};
    
    Status status = command_->execute(context);
    EXPECT_TRUE(status.isSuccess());
    EXPECT_EQ(getFileMode(test_file_), 0640);
}

TEST_F(ChmodCommandTest, ExecuteSymbolicAllAdd)
{
    chmod(test_file_.c_str(), 0644);
    
    CommandContext context;
    context.args = {"a+x", test_file_};
    
    Status status = command_->execute(context);
    EXPECT_TRUE(status.isSuccess());
    EXPECT_EQ(getFileMode(test_file_), 0755);
}

TEST_F(ChmodCommandTest, ExecuteOctal600)
{
    CommandContext context;
    context.args = {"600", test_file_};
    
    Status status = command_->execute(context);
    EXPECT_TRUE(status.isSuccess());
    EXPECT_EQ(getFileMode(test_file_), 0600);
}

TEST_F(ChmodCommandTest, ExecuteOctal444)
{
    CommandContext context;
    context.args = {"444", test_file_};
    
    Status status = command_->execute(context);
    EXPECT_TRUE(status.isSuccess());
    EXPECT_EQ(getFileMode(test_file_), 0444);
}

TEST_F(ChmodCommandTest, ExecuteInvalidOctalMode)
{
    CommandContext context;
    context.args = {"999", test_file_};  // Invalid octal
    
    Status status = command_->execute(context);
    EXPECT_FALSE(status.isSuccess());
}

TEST_F(ChmodCommandTest, ExecuteInvalidSymbolicMode)
{
    CommandContext context;
    context.args = {"xyz", test_file_};  // Invalid symbolic mode
    
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

TEST_F(PythonCommandTest, ExecuteArithmetic)
{
    CommandContext context;
    context.args = {"-c", "print(2 + 3)"};
    
    testing::internal::CaptureStdout();
    Status status = command_->execute(context);
    std::string output = testing::internal::GetCapturedStdout();
    
    EXPECT_TRUE(status.isSuccess());
    EXPECT_NE(output.find("5"), std::string::npos);
}

TEST_F(PythonCommandTest, ExecuteFloatingPoint)
{
    CommandContext context;
    context.args = {"-c", "print(10 / 3)"};
    
    testing::internal::CaptureStdout();
    Status status = command_->execute(context);
    std::string output = testing::internal::GetCapturedStdout();
    
    EXPECT_TRUE(status.isSuccess());
    EXPECT_NE(output.find("3.33"), std::string::npos);
}

TEST_F(PythonCommandTest, ExecuteFunction)
{
    CommandContext context;
    context.args = {"-c", "def add(a, b): return a + b\nprint(add(5, 10))"};
    
    testing::internal::CaptureStdout();
    Status status = command_->execute(context);
    std::string output = testing::internal::GetCapturedStdout();
    
    EXPECT_TRUE(status.isSuccess());
    EXPECT_NE(output.find("15"), std::string::npos);
}

TEST_F(PythonCommandTest, ExecuteLoops)
{
    CommandContext context;
    context.args = {"-c", "for i in range(3): print(i)"};
    
    testing::internal::CaptureStdout();
    Status status = command_->execute(context);
    std::string output = testing::internal::GetCapturedStdout();
    
    EXPECT_TRUE(status.isSuccess());
    EXPECT_NE(output.find("0"), std::string::npos);
    EXPECT_NE(output.find("1"), std::string::npos);
    EXPECT_NE(output.find("2"), std::string::npos);
}

TEST_F(PythonCommandTest, ExecuteList)
{
    CommandContext context;
    context.args = {"-c", "x = [1, 2, 3]\nprint(len(x))"};
    
    testing::internal::CaptureStdout();
    Status status = command_->execute(context);
    std::string output = testing::internal::GetCapturedStdout();
    
    EXPECT_TRUE(status.isSuccess());
    EXPECT_NE(output.find("3"), std::string::npos);
}

TEST_F(PythonCommandTest, ExecuteDict)
{
    CommandContext context;
    context.args = {"-c", "d = {'key': 'value'}\nprint(d['key'])"};
    
    testing::internal::CaptureStdout();
    Status status = command_->execute(context);
    std::string output = testing::internal::GetCapturedStdout();
    
    EXPECT_TRUE(status.isSuccess());
    EXPECT_NE(output.find("value"), std::string::npos);
}

TEST_F(PythonCommandTest, ExecuteStringOperations)
{
    CommandContext context;
    context.args = {"-c", "s = 'hello'\nprint(s.upper())"};
    
    testing::internal::CaptureStdout();
    Status status = command_->execute(context);
    std::string output = testing::internal::GetCapturedStdout();
    
    EXPECT_TRUE(status.isSuccess());
    EXPECT_NE(output.find("HELLO"), std::string::npos);
}

TEST_F(PythonCommandTest, ExecuteMultipleTimes)
{
    // Test init/deinit by running multiple times
    for (int i = 0; i < 3; ++i)
    {
        CommandContext context;
        context.args = {"-c", "print('iteration')"};
        
        testing::internal::CaptureStdout();
        Status status = command_->execute(context);
        std::string output = testing::internal::GetCapturedStdout();
        
        EXPECT_TRUE(status.isSuccess());
        EXPECT_NE(output.find("iteration"), std::string::npos);
    }
}

TEST_F(PythonCommandTest, ExecuteWithColorsDisabled)
{
    CommandContext context;
    context.args = {"-c", "print('test')"};
    context.use_colors = false;
    
    testing::internal::CaptureStdout();
    Status status = command_->execute(context);
    std::string output = testing::internal::GetCapturedStdout();
    
    EXPECT_TRUE(status.isSuccess());
    EXPECT_NE(output.find("test"), std::string::npos);
}

TEST_F(PythonCommandTest, ExecuteComplexScript)
{
    // Create more complex test script
    std::string test_script = "/tmp/test_complex_python.py";
    std::ofstream file(test_script);
    file << "# Complex Python script\n";
    file << "def factorial(n):\n";
    file << "    if n <= 1:\n";
    file << "        return 1\n";
    file << "    return n * factorial(n-1)\n";
    file << "\n";
    file << "result = factorial(5)\n";
    file << "print('Factorial:', result)\n";  // Changed from f-string to regular print
    file.close();
    
    CommandContext context;
    context.args = {test_script};
    
    testing::internal::CaptureStdout();
    Status status = command_->execute(context);
    std::string output = testing::internal::GetCapturedStdout();
    
    EXPECT_TRUE(status.isSuccess());
    EXPECT_NE(output.find("120"), std::string::npos);
    
    unlink(test_script.c_str());
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

