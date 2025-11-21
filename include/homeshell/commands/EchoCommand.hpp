#pragma once

#include <homeshell/Command.hpp>

#include <fmt/core.h>

#include <iostream>

namespace homeshell
{

class EchoCommand : public ICommand
{
public:
    std::string getName() const override
    {
        return "echo";
    }

    std::string getDescription() const override
    {
        return "Echo the arguments";
    }

    CommandType getType() const override
    {
        return CommandType::Synchronous;
    }

    Status execute(const CommandContext& context) override
    {
        for (size_t i = 0; i < context.args.size(); ++i)
        {
            if (i > 0)
            {
                fmt::print(" ");
            }
            fmt::print("{}", context.args[i]);
        }
        fmt::print("\n");
        return Status::ok();
    }
};

} // namespace homeshell
