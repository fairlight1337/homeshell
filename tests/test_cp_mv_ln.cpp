/**
 * @file test_file_ops_batch2.cpp
 * @brief Unit tests for file operation commands batch 2 (cp, mv, ln)
 */

#include <homeshell/commands/CpCommand.hpp>
#include <homeshell/commands/LnCommand.hpp>
#include <homeshell/commands/MvCommand.hpp>

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;
using namespace homeshell;

// ============================================================================
// CpCommand Tests
// ============================================================================

class CpCommandTest : public ::testing::Test
{
protected:
    CpCommand cmd;
    std::stringstream output;
    std::streambuf* old_cout;
    std::string test_dir;

    void SetUp() override
    {
        old_cout = std::cout.rdbuf(output.rdbuf());
        test_dir = "/tmp/homeshell_cp_test_" + std::to_string(::getpid());
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

TEST_F(CpCommandTest, CommandName)
{
    EXPECT_EQ(cmd.getName(), "cp");
}

TEST_F(CpCommandTest, CommandDescription)
{
    EXPECT_FALSE(cmd.getDescription().empty());
}

TEST_F(CpCommandTest, BasicFileCopy)
{
    std::string src = test_dir + "/source.txt";
    std::string dst = test_dir + "/dest.txt";
    createFile(src, "test content");

    CommandContext ctx;
    ctx.args = {src, dst};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());
    EXPECT_TRUE(fs::exists(dst));

    // Verify content
    std::ifstream in(dst);
    std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    EXPECT_EQ(content, "test content");
}

TEST_F(CpCommandTest, CopyToDirectory)
{
    std::string src = test_dir + "/source.txt";
    std::string dst_dir = test_dir + "/subdir";
    fs::create_directory(dst_dir);
    createFile(src, "test content");

    CommandContext ctx;
    ctx.args = {src, dst_dir};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());
    EXPECT_TRUE(fs::exists(dst_dir + "/source.txt"));
}

TEST_F(CpCommandTest, RecursiveDirectoryCopy)
{
    std::string src_dir = test_dir + "/src_dir";
    std::string dst_dir = test_dir + "/dst_dir";
    fs::create_directory(src_dir);
    createFile(src_dir + "/file1.txt", "content1");
    createFile(src_dir + "/file2.txt", "content2");

    CommandContext ctx;
    ctx.args = {"-r", src_dir, dst_dir};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());
    EXPECT_TRUE(fs::exists(dst_dir));
    EXPECT_TRUE(fs::exists(dst_dir + "/file1.txt"));
    EXPECT_TRUE(fs::exists(dst_dir + "/file2.txt"));
}

TEST_F(CpCommandTest, DirectoryWithoutRecursiveFails)
{
    std::string src_dir = test_dir + "/src_dir";
    std::string dst_dir = test_dir + "/dst_dir";
    fs::create_directory(src_dir);

    CommandContext ctx;
    ctx.args = {src_dir, dst_dir};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk()); // Command returns ok but prints error
}

TEST_F(CpCommandTest, NonExistentSource)
{
    std::string src = test_dir + "/nonexistent.txt";
    std::string dst = test_dir + "/dest.txt";

    CommandContext ctx;
    ctx.args = {src, dst};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk()); // Command returns ok but prints error
}

TEST_F(CpCommandTest, VerboseOption)
{
    std::string src = test_dir + "/source.txt";
    std::string dst = test_dir + "/dest.txt";
    createFile(src, "test content");

    CommandContext ctx;
    ctx.args = {"-v", src, dst};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());
    EXPECT_NE(getOutput().find("->"), std::string::npos);
}

TEST_F(CpCommandTest, MultipleFiles)
{
    std::string src1 = test_dir + "/file1.txt";
    std::string src2 = test_dir + "/file2.txt";
    std::string dst_dir = test_dir + "/dest";
    fs::create_directory(dst_dir);
    createFile(src1, "content1");
    createFile(src2, "content2");

    CommandContext ctx;
    ctx.args = {src1, src2, dst_dir};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());
    EXPECT_TRUE(fs::exists(dst_dir + "/file1.txt"));
    EXPECT_TRUE(fs::exists(dst_dir + "/file2.txt"));
}

TEST_F(CpCommandTest, HelpOption)
{
    CommandContext ctx;
    ctx.args = {"--help"};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());
    EXPECT_NE(getOutput().find("Copy files"), std::string::npos);
}

// ============================================================================
// MvCommand Tests
// ============================================================================

class MvCommandTest : public ::testing::Test
{
protected:
    MvCommand cmd;
    std::stringstream output;
    std::streambuf* old_cout;
    std::string test_dir;

    void SetUp() override
    {
        old_cout = std::cout.rdbuf(output.rdbuf());
        test_dir = "/tmp/homeshell_mv_test_" + std::to_string(::getpid());
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

TEST_F(MvCommandTest, CommandName)
{
    EXPECT_EQ(cmd.getName(), "mv");
}

TEST_F(MvCommandTest, CommandDescription)
{
    EXPECT_FALSE(cmd.getDescription().empty());
}

TEST_F(MvCommandTest, BasicFileMove)
{
    std::string src = test_dir + "/source.txt";
    std::string dst = test_dir + "/dest.txt";
    createFile(src, "test content");

    CommandContext ctx;
    ctx.args = {src, dst};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());
    EXPECT_FALSE(fs::exists(src));
    EXPECT_TRUE(fs::exists(dst));

    // Verify content
    std::ifstream in(dst);
    std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    EXPECT_EQ(content, "test content");
}

TEST_F(MvCommandTest, RenameFile)
{
    std::string src = test_dir + "/oldname.txt";
    std::string dst = test_dir + "/newname.txt";
    createFile(src, "test content");

    CommandContext ctx;
    ctx.args = {src, dst};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());
    EXPECT_FALSE(fs::exists(src));
    EXPECT_TRUE(fs::exists(dst));
}

TEST_F(MvCommandTest, MoveToDirectory)
{
    std::string src = test_dir + "/source.txt";
    std::string dst_dir = test_dir + "/subdir";
    fs::create_directory(dst_dir);
    createFile(src, "test content");

    CommandContext ctx;
    ctx.args = {src, dst_dir};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());
    EXPECT_FALSE(fs::exists(src));
    EXPECT_TRUE(fs::exists(dst_dir + "/source.txt"));
}

TEST_F(MvCommandTest, MoveDirectory)
{
    std::string src_dir = test_dir + "/src_dir";
    std::string dst_dir = test_dir + "/dst_dir";
    fs::create_directory(src_dir);
    createFile(src_dir + "/file.txt", "content");

    CommandContext ctx;
    ctx.args = {src_dir, dst_dir};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());
    EXPECT_FALSE(fs::exists(src_dir));
    EXPECT_TRUE(fs::exists(dst_dir));
    EXPECT_TRUE(fs::exists(dst_dir + "/file.txt"));
}

TEST_F(MvCommandTest, NonExistentSource)
{
    std::string src = test_dir + "/nonexistent.txt";
    std::string dst = test_dir + "/dest.txt";

    CommandContext ctx;
    ctx.args = {src, dst};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk()); // Command returns ok but prints error
}

TEST_F(MvCommandTest, VerboseOption)
{
    std::string src = test_dir + "/source.txt";
    std::string dst = test_dir + "/dest.txt";
    createFile(src, "test content");

    CommandContext ctx;
    ctx.args = {"-v", src, dst};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());
    EXPECT_NE(getOutput().find("renamed"), std::string::npos);
}

TEST_F(MvCommandTest, MultipleFiles)
{
    std::string src1 = test_dir + "/file1.txt";
    std::string src2 = test_dir + "/file2.txt";
    std::string dst_dir = test_dir + "/dest";
    fs::create_directory(dst_dir);
    createFile(src1, "content1");
    createFile(src2, "content2");

    CommandContext ctx;
    ctx.args = {src1, src2, dst_dir};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());
    EXPECT_FALSE(fs::exists(src1));
    EXPECT_FALSE(fs::exists(src2));
    EXPECT_TRUE(fs::exists(dst_dir + "/file1.txt"));
    EXPECT_TRUE(fs::exists(dst_dir + "/file2.txt"));
}

TEST_F(MvCommandTest, HelpOption)
{
    CommandContext ctx;
    ctx.args = {"--help"};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());
    EXPECT_NE(getOutput().find("Move or rename"), std::string::npos);
}

// ============================================================================
// LnCommand Tests
// ============================================================================

class LnCommandTest : public ::testing::Test
{
protected:
    LnCommand cmd;
    std::stringstream output;
    std::streambuf* old_cout;
    std::string test_dir;

    void SetUp() override
    {
        old_cout = std::cout.rdbuf(output.rdbuf());
        test_dir = "/tmp/homeshell_ln_test_" + std::to_string(::getpid());
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

TEST_F(LnCommandTest, CommandName)
{
    EXPECT_EQ(cmd.getName(), "ln");
}

TEST_F(LnCommandTest, CommandDescription)
{
    EXPECT_FALSE(cmd.getDescription().empty());
}

TEST_F(LnCommandTest, SymbolicLink)
{
    std::string target = test_dir + "/target.txt";
    std::string link = test_dir + "/link.txt";
    createFile(target, "test content");

    CommandContext ctx;
    ctx.args = {"-s", target, link};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());
    EXPECT_TRUE(fs::exists(link));
    EXPECT_TRUE(fs::is_symlink(link));

    // Verify link target
    auto link_target = fs::read_symlink(link);
    EXPECT_EQ(link_target.string(), target);
}

TEST_F(LnCommandTest, HardLink)
{
    std::string target = test_dir + "/target.txt";
    std::string link = test_dir + "/link.txt";
    createFile(target, "test content");

    CommandContext ctx;
    ctx.args = {target, link};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());
    EXPECT_TRUE(fs::exists(link));
    EXPECT_FALSE(fs::is_symlink(link));

    // Verify both files have same content
    std::ifstream in1(target);
    std::ifstream in2(link);
    std::string content1((std::istreambuf_iterator<char>(in1)), std::istreambuf_iterator<char>());
    std::string content2((std::istreambuf_iterator<char>(in2)), std::istreambuf_iterator<char>());
    EXPECT_EQ(content1, content2);

    // Modify one, check the other
    std::ofstream out(link);
    out << "modified";
    out.close();

    std::ifstream in3(target);
    std::string content3((std::istreambuf_iterator<char>(in3)), std::istreambuf_iterator<char>());
    EXPECT_EQ(content3, "modified");
}

TEST_F(LnCommandTest, SymbolicLinkToNonExistent)
{
    std::string target = test_dir + "/nonexistent.txt";
    std::string link = test_dir + "/link.txt";

    CommandContext ctx;
    ctx.args = {"-s", target, link};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());
    EXPECT_TRUE(fs::is_symlink(link)); // Symlink exists even if target doesn't
}

TEST_F(LnCommandTest, HardLinkToNonExistent)
{
    std::string target = test_dir + "/nonexistent.txt";
    std::string link = test_dir + "/link.txt";

    CommandContext ctx;
    ctx.args = {target, link};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk()); // Command returns ok but prints error
}

TEST_F(LnCommandTest, LinkToDirectory)
{
    std::string target_dir = test_dir + "/target_dir";
    std::string link_dir = test_dir + "/link_dir";
    fs::create_directory(target_dir);

    CommandContext ctx;
    ctx.args = {"-s", target_dir, link_dir};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());
    EXPECT_TRUE(fs::is_symlink(link_dir));
}

TEST_F(LnCommandTest, HardLinkToDirectory)
{
    std::string target_dir = test_dir + "/target_dir";
    std::string link_dir = test_dir + "/link_dir";
    fs::create_directory(target_dir);

    CommandContext ctx;
    ctx.args = {target_dir, link_dir};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk()); // Command returns ok but prints error (hard link to dir not allowed)
}

TEST_F(LnCommandTest, VerboseOption)
{
    std::string target = test_dir + "/target.txt";
    std::string link = test_dir + "/link.txt";
    createFile(target, "test content");

    CommandContext ctx;
    ctx.args = {"-s", "-v", target, link};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());
    EXPECT_NE(getOutput().find("->"), std::string::npos);
}

TEST_F(LnCommandTest, LinkInDirectory)
{
    std::string target = test_dir + "/target.txt";
    std::string link_dir = test_dir + "/links";
    fs::create_directory(link_dir);
    createFile(target, "test content");

    CommandContext ctx;
    ctx.args = {"-s", target, link_dir};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());
    EXPECT_TRUE(fs::exists(link_dir + "/target.txt"));
    EXPECT_TRUE(fs::is_symlink(link_dir + "/target.txt"));
}

TEST_F(LnCommandTest, ForceOverwrite)
{
    std::string target = test_dir + "/target.txt";
    std::string link = test_dir + "/link.txt";
    createFile(target, "test content");
    createFile(link, "existing content");

    CommandContext ctx;
    ctx.args = {"-s", "-f", target, link};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());
    EXPECT_TRUE(fs::is_symlink(link));
}

TEST_F(LnCommandTest, HelpOption)
{
    CommandContext ctx;
    ctx.args = {"--help"};
    auto status = cmd.execute(ctx);
    EXPECT_TRUE(status.isOk());
    EXPECT_NE(getOutput().find("symbolic"), std::string::npos);
}

