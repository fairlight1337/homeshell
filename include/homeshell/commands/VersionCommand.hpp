#pragma once

#include <homeshell/Command.hpp>
#include <homeshell/Status.hpp>

#include <string>
#include <vector>

namespace homeshell
{

class VersionCommand : public ICommand
{
public:
    std::string getName() const override
    {
        return "version";
    }

    std::string getDescription() const override
    {
        return "Show Homeshell and library versions";
    }

    CommandType getType() const override
    {
        return CommandType::Synchronous;
    }

    Status execute(const CommandContext& context) override;

private:
    std::string getMicroPythonVersion();
    std::string getMinizVersion();
    std::string getSQLCipherVersion();
    std::string getFmtVersion();
};

} // namespace homeshell
