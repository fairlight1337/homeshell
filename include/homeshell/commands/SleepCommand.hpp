#pragma once

#include <homeshell/Command.hpp>

#include <fmt/core.h>

#include <chrono>
#include <thread>

namespace homeshell
{

class SleepCommand : public ICommand
{
public:
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
            std::this_thread::sleep_for(std::chrono::seconds(seconds));
            fmt::print("Done sleeping!\n");
            return Status::ok();
        }
        catch (...)
        {
            return Status::error("Invalid number");
        }
    }
};

} // namespace homeshell
