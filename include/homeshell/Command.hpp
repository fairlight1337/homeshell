#pragma once

#include <homeshell/Status.hpp>

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace homeshell
{

enum class CommandType
{
    Synchronous,
    Asynchronous
};

struct CommandContext
{
    std::vector<std::string> args;
    bool verbose = false;
};

class ICommand
{
public:
    virtual ~ICommand() = default;

    virtual std::string getName() const = 0;
    virtual std::string getDescription() const = 0;
    virtual CommandType getType() const = 0;
    virtual Status execute(const CommandContext& context) = 0;

    // Called when command should be cancelled (e.g., CTRL-C pressed)
    virtual void cancel()
    {
    }

    // Check if command supports cancellation
    virtual bool supportsCancellation() const
    {
        return false;
    }
};

class CommandRegistry
{
public:
    static CommandRegistry& getInstance()
    {
        static CommandRegistry instance;
        return instance;
    }

    void registerCommand(std::shared_ptr<ICommand> command)
    {
        commands_[command->getName()] = command;
    }

    std::shared_ptr<ICommand> getCommand(const std::string& name) const
    {
        auto it = commands_.find(name);
        if (it != commands_.end())
        {
            return it->second;
        }
        return nullptr;
    }

    std::vector<std::string> getAllCommandNames() const
    {
        std::vector<std::string> names;
        for (const auto& pair : commands_)
        {
            names.push_back(pair.first);
        }
        return names;
    }

    const std::map<std::string, std::shared_ptr<ICommand>>& getAll() const
    {
        return commands_;
    }

private:
    CommandRegistry() = default;
    std::map<std::string, std::shared_ptr<ICommand>> commands_;
};

} // namespace homeshell
