#pragma once

#include <homeshell/Command.hpp>
#include <fmt/core.h>
#include <fmt/color.h>

namespace homeshell
{

class HelpCommand : public ICommand
{
public:
    std::string getName() const override
    {
        return "help";
    }

    std::string getDescription() const override
    {
        return "Show available commands";
    }

    CommandType getType() const override
    {
        return CommandType::Synchronous;
    }

    Status execute(const CommandContext& context) override
    {
        fmt::print(fmt::emphasis::bold, "Available commands:\n\n");

        auto& registry = CommandRegistry::getInstance();
        for (const auto& pair : registry.getAll())
        {
            fmt::print("  {:<15} - {}\n", 
                      fmt::format(fg(fmt::color::cyan), "{}", pair.first),
                      pair.second->getDescription());
        }

        fmt::print("\n");
        return Status::ok();
    }
};

} // namespace homeshell

