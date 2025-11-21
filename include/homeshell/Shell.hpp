#pragma once

#include <homeshell/Command.hpp>
#include <homeshell/Config.hpp>
#include <homeshell/Status.hpp>
#include <homeshell/TerminalInfo.hpp>
#include <replxx.hxx>
#include <fmt/core.h>
#include <fmt/color.h>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include <future>
#include <thread>

namespace homeshell
{

class Shell
{
public:
    Shell(const Config& config, const TerminalInfo& terminal_info)
        : config_(config)
        , terminal_info_(terminal_info)
        , replxx_()
    {
        setupReplxx();
    }

    void run()
    {
        printWelcome();

        while (true)
        {
            const char* input = replxx_.input(getPrompt());

            if (input == nullptr)
            {
                // EOF (Ctrl+D)
                fmt::print("\n");
                break;
            }

            std::string line(input);

            // Skip empty lines
            if (line.empty())
            {
                continue;
            }

            // Add to history
            replxx_.history_add(line);

            // Parse and execute command
            Status status = executeCommand(line);

            if (status.isExit())
            {
                break;
            }

            if (!status.isOk())
            {
                if (terminal_info_.hasColorSupport())
                {
                    fmt::print(fg(fmt::color::red), "Error: {}\n", status.message);
                }
                else
                {
                    fmt::print("Error: {}\n", status.message);
                }
            }
        }
    }

private:
    void setupReplxx()
    {
        // Set up history file
        replxx_.set_max_history_size(1000);
        replxx_.set_max_hint_rows(3);

        // Enable word break characters
        replxx_.set_word_break_characters(" \t.,-%!;:=*~^'\"/?<>|[](){}");

        // Set up tab completion
        replxx_.set_completion_callback(
            [this](const std::string& context, int& contextLen)
            {
                return getCompletions(context, contextLen);
            });

        // Set up syntax highlighting if colors are supported
        if (terminal_info_.hasColorSupport())
        {
            replxx_.set_highlighter_callback(
                [](const std::string& context, replxx::Replxx::colors_t& colors)
                {
                    // Highlight commands in cyan
                    for (size_t i = 0; i < context.size(); ++i)
                    {
                        if (i == 0 || context[i - 1] == ' ')
                        {
                            size_t end = i;
                            while (end < context.size() && context[end] != ' ')
                            {
                                end++;
                            }
                            std::string word = context.substr(i, end - i);

                            auto& registry = CommandRegistry::getInstance();
                            if (registry.getCommand(word) != nullptr)
                            {
                                for (size_t j = i; j < end; ++j)
                                {
                                    colors[j] = replxx::Replxx::Color::CYAN;
                                }
                            }
                            i = end;
                        }
                    }
                });
        }
    }

    std::string getPrompt() const
    {
        if (terminal_info_.hasColorSupport())
        {
            return fmt::format(fg(fmt::color::green) | fmt::emphasis::bold, "{}", 
                             config_.prompt_format);
        }
        return config_.prompt_format;
    }

    void printWelcome()
    {
        if (terminal_info_.hasColorSupport())
        {
            fmt::print(fg(fmt::color::cyan) | fmt::emphasis::bold,
                      "Welcome to Homeshell!\n");
            fmt::print("Type {} for available commands.\n\n",
                      fmt::format(fg(fmt::color::yellow), "'help'"));
        }
        else
        {
            fmt::print("Welcome to Homeshell!\n");
            fmt::print("Type 'help' for available commands.\n\n");
        }

        if (terminal_info_.hasEmojiSupport())
        {
            fmt::print("✨ Emoji support detected!\n\n");
        }
    }

    replxx::Replxx::completions_t getCompletions(const std::string& context, 
                                                   int& contextLen)
    {
        replxx::Replxx::completions_t completions;

        // Split the context to get the current word
        std::string current_word;
        size_t last_space = context.find_last_of(' ');

        if (last_space == std::string::npos)
        {
            current_word = context;
            contextLen = static_cast<int>(current_word.length());
        }
        else
        {
            current_word = context.substr(last_space + 1);
            contextLen = static_cast<int>(current_word.length());
        }

        // Get matching commands
        auto& registry = CommandRegistry::getInstance();
        for (const auto& name : registry.getAllCommandNames())
        {
            if (name.find(current_word) == 0)
            {
                completions.push_back(name);
            }
        }

        return completions;
    }

    Status executeCommand(const std::string& line)
    {
        // Parse command line
        std::vector<std::string> tokens = tokenize(line);
        if (tokens.empty())
        {
            return Status::ok();
        }

        std::string cmd_name = tokens[0];
        std::vector<std::string> args(tokens.begin() + 1, tokens.end());

        // Look up command
        auto& registry = CommandRegistry::getInstance();
        auto command = registry.getCommand(cmd_name);

        if (!command)
        {
            return Status::error("Unknown command: " + cmd_name);
        }

        // Execute command
        CommandContext context;
        context.args = args;
        context.verbose = false;  // Could be set from config or flag

        if (command->getType() == CommandType::Asynchronous)
        {
            return executeAsync(command, context);
        }
        else
        {
            return command->execute(context);
        }
    }

    Status executeAsync(std::shared_ptr<ICommand> command, 
                       const CommandContext& context)
    {
        // Launch async task
        auto future = std::async(std::launch::async, 
            [command, context]()
            {
                return command->execute(context);
            });

        // For now, just wait for completion
        // In a more sophisticated version, we could allow multiple
        // async tasks and manage their output
        return future.get();
    }

    std::vector<std::string> tokenize(const std::string& line)
    {
        std::vector<std::string> tokens;
        std::istringstream iss(line);
        std::string token;

        while (iss >> token)
        {
            tokens.push_back(token);
        }

        return tokens;
    }

    Config config_;
    TerminalInfo terminal_info_;
    replxx::Replxx replxx_;
};

} // namespace homeshell

