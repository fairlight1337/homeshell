#include <homeshell/Status.hpp>
#include <gtest/gtest.h>

using namespace homeshell;

TEST(StatusTest, OkStatus)
{
    Status status = Status::ok();
    EXPECT_EQ(status.code, 0);
    EXPECT_TRUE(status.isOk());
    EXPECT_TRUE(status.isSuccess());
    EXPECT_FALSE(status.isExit());
}

TEST(StatusTest, ErrorStatus)
{
    Status status = Status::error("test error");
    EXPECT_EQ(status.code, 1);
    EXPECT_FALSE(status.isOk());
    EXPECT_FALSE(status.isSuccess());
    EXPECT_FALSE(status.isExit());
    EXPECT_EQ(status.message, "test error");
}

TEST(StatusTest, ExitStatus)
{
    Status status = Status::exit();
    EXPECT_EQ(status.code, -1);
    EXPECT_FALSE(status.isOk());
    EXPECT_TRUE(status.isExit());
}

TEST(StatusTest, CustomStatus)
{
    Status status(42, "custom message");
    EXPECT_EQ(status.code, 42);
    EXPECT_EQ(status.message, "custom message");
    EXPECT_FALSE(status.isOk());
    EXPECT_FALSE(status.isSuccess());
}

