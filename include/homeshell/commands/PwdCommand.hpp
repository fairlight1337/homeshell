#pragma once

#include <homeshell/Command.hpp>
#include <homeshell/VirtualFilesystem.hpp>

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
        auto& vfs = VirtualFilesystem::getInstance();
        fmt::print("{}\n", vfs.getCurrentDirectory());
        return Status::ok();
    }
};

} // namespace homeshell
