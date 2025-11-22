#pragma once

#include <homeshell/Status.hpp>

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace homeshell
{

/**
 * @brief Type of command execution
 *
 * Commands can be either synchronous (blocking) or asynchronous (non-blocking).
 * Asynchronous commands can be cancelled mid-execution.
 */
enum class CommandType
{
    Synchronous, ///< Blocking command that completes before returning
    Asynchronous ///< Non-blocking command that can run in background
};

/**
 * @brief Context passed to command execution
 *
 * Contains all necessary information for a command to execute, including
 * arguments, verbosity settings, and output formatting preferences.
 */
struct CommandContext
{
    std::vector<std::string> args; ///< Command arguments (excluding command name)
    bool verbose = false;          ///< Enable verbose output
    bool use_colors = true; ///< Enable ANSI color codes in output (disabled for file redirection)
};

/**
 * @brief Abstract interface for all shell commands
 *
 * All built-in commands must implement this interface. Commands are registered
 * with the CommandRegistry and can be invoked by name through the shell.
 *
 * @details Commands can be synchronous or asynchronous. Asynchronous commands
 *          support cancellation via CTRL-C signals. The execute() method receives
 *          a CommandContext with parsed arguments and execution preferences.
 */
class ICommand
{
public:
    virtual ~ICommand() = default;

    /**
     * @brief Get the command name
     * @return Command name as it appears in the shell
     */
    virtual std::string getName() const = 0;

    /**
     * @brief Get a brief description of the command
     * @return One-line description shown in help text
     */
    virtual std::string getDescription() const = 0;

    /**
     * @brief Get the command execution type
     * @return CommandType::Synchronous or CommandType::Asynchronous
     */
    virtual CommandType getType() const = 0;

    /**
     * @brief Execute the command
     * @param context Execution context containing arguments and settings
     * @return Status indicating success/failure and any error messages
     */
    virtual Status execute(const CommandContext& context) = 0;

    /**
     * @brief Cancel command execution (for async commands)
     *
     * Called when CTRL-C is pressed during command execution.
     * Only meaningful for asynchronous commands that support cancellation.
     */
    virtual void cancel()
    {
    }

    /**
     * @brief Check if command supports cancellation
     * @return true if command can be cancelled mid-execution, false otherwise
     */
    virtual bool supportsCancellation() const
    {
        return false;
    }
};

/**
 * @brief Singleton registry for all shell commands
 *
 * Maintains a centralized registry of all available commands in the shell.
 * Commands are registered by name and can be retrieved for execution.
 *
 * @details This class implements the Singleton pattern to ensure a single
 *          global registry. Commands are typically registered during shell
 *          initialization in main().
 */
class CommandRegistry
{
public:
    /**
     * @brief Get the singleton instance
     * @return Reference to the global CommandRegistry instance
     */
    static CommandRegistry& getInstance()
    {
        static CommandRegistry instance;
        return instance;
    }

    /**
     * @brief Register a command with the registry
     * @param command Shared pointer to the command to register
     *
     * @details The command is registered using its getName() value as the key.
     *          If a command with the same name already exists, it will be replaced.
     */
    void registerCommand(std::shared_ptr<ICommand> command)
    {
        commands_[command->getName()] = command;
    }

    /**
     * @brief Retrieve a command by name
     * @param name The command name to look up
     * @return Shared pointer to the command, or nullptr if not found
     */
    std::shared_ptr<ICommand> getCommand(const std::string& name) const
    {
        auto it = commands_.find(name);
        if (it != commands_.end())
        {
            return it->second;
        }
        return nullptr;
    }

    /**
     * @brief Get all registered command names
     * @return Vector of all command names in the registry
     */
    std::vector<std::string> getAllCommandNames() const
    {
        std::vector<std::string> names;
        for (const auto& pair : commands_)
        {
            names.push_back(pair.first);
        }
        return names;
    }

    /**
     * @brief Get all registered commands
     * @return Map of command names to command instances
     */
    const std::map<std::string, std::shared_ptr<ICommand>>& getAll() const
    {
        return commands_;
    }

private:
    CommandRegistry() = default;
    std::map<std::string, std::shared_ptr<ICommand>>
        commands_; ///< Map of command name to command instance
};

} // namespace homeshell
