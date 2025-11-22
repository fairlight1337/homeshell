#pragma once

#include <homeshell/Command.hpp>
#include <homeshell/Status.hpp>

#include <string>
#include <vector>

namespace homeshell
{

class ChmodCommand : public ICommand
{
public:
    std::string getName() const override
    {
        return "chmod";
    }

    std::string getDescription() const override
    {
        return "Change file permissions";
    }

    CommandType getType() const override
    {
        return CommandType::Synchronous;
    }

    Status execute(const CommandContext& context) override;

private:
    bool parseOctalMode(const std::string& mode_str, mode_t& mode);
    bool parseSymbolicMode(const std::string& mode_str, mode_t current_mode, mode_t& new_mode);
    bool applyChmod(const std::string& path, mode_t mode);
};

} // namespace homeshell
