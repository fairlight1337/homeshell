#pragma once

#include <homeshell/Command.hpp>

namespace homeshell
{

class ExitCommand : public ICommand
{
public:
    std::string getName() const override
    {
        return "exit";
    }

    std::string getDescription() const override
    {
        return "Exit the shell";
    }

    CommandType getType() const override
    {
        return CommandType::Synchronous;
    }

    Status execute(const CommandContext& context) override
    {
        return Status::exit();
    }
};

} // namespace homeshell

