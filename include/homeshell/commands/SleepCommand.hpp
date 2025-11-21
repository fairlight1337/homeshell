#pragma once

#include <homeshell/Command.hpp>

#include <fmt/color.h>
#include <fmt/core.h>

#include <atomic>
#include <chrono>
#include <thread>

namespace homeshell
{

class SleepCommand : public ICommand
{
public:
    SleepCommand()
        : cancelled_(false)
    {
    }

    std::string getName() const override
    {
        return "sleep";
    }

    std::string getDescription() const override
    {
        return "Sleep for N seconds (async demo)";
    }

    CommandType getType() const override
    {
        return CommandType::Asynchronous;
    }

    bool supportsCancellation() const override
    {
        return true;
    }

    void cancel() override
    {
        cancelled_.store(true);
    }

    Status execute(const CommandContext& context) override
    {
        if (context.args.empty())
        {
            return Status::error("Usage: sleep <seconds>");
        }

        try
        {
            int seconds = std::stoi(context.args[0]);
            fmt::print("Sleeping for {} seconds...\n", seconds);

            // Sleep in small increments to check for cancellation
            for (int i = 0; i < seconds * 10; ++i)
            {
                if (cancelled_.load())
                {
                    fmt::print(fg(fmt::color::yellow), "\nSleep cancelled!\n");
                    return Status::error("Cancelled");
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }

            fmt::print("Done sleeping!\n");
            return Status::ok();
        }
        catch (...)
        {
            return Status::error("Invalid number");
        }
    }

private:
    std::atomic<bool> cancelled_;
};

} // namespace homeshell
