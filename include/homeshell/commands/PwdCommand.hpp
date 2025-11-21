#pragma once

#include <homeshell/Command.hpp>
#include <homeshell/FilesystemHelper.hpp>

#include <fmt/core.h>

namespace homeshell
{

class PwdCommand : public ICommand
{
public:
    std::string getName() const override
    {
        return "pwd";
    }

    std::string getDescription() const override
    {
        return "Print working directory";
    }

    CommandType getType() const override
    {
        return CommandType::Synchronous;
    }

    Status execute(const CommandContext& context) override
    {
        fmt::print("{}\n", FilesystemHelper::getCurrentDirectory().string());
        return Status::ok();
    }
};

} // namespace homeshell
