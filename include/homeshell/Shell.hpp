#pragma once

#include <homeshell/Command.hpp>
#include <homeshell/Config.hpp>
#include <homeshell/PromptFormatter.hpp>
#include <homeshell/Status.hpp>
#include <homeshell/TerminalInfo.hpp>
#include <homeshell/VirtualFilesystem.hpp>

#include <fmt/color.h>
#include <fmt/core.h>

#include <atomic>
#include <csignal>
#include <future>
#include <memory>
#include <replxx.hxx>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace homeshell
{

class Shell
{
public:
    Shell(const Config& config, const TerminalInfo& terminal_info)
        : config_(config)
        , terminal_info_(terminal_info)
        , replxx_()
        , current_command_(nullptr)
        , interrupt_received_(false)
        , history_file_(config.expandPath(config.history_file))
    {
        setupReplxx();
        setupSignalHandler();
        loadHistory();
    }

    ~Shell()
    {
        // Save history before exiting
        saveHistory();

        // Restore default signal handler
        std::signal(SIGINT, SIG_DFL);
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

    Status executeCommandLine(const std::string& command_line)
    {
        return executeCommand(command_line);
    }

    static Shell* instance_;

private:
    static void signalHandler(int signal)
    {
        if (signal == SIGINT && instance_)
        {
            instance_->handleInterrupt();
        }
    }

    void handleInterrupt()
    {
        interrupt_received_.store(true);

        if (current_command_ && current_command_->supportsCancellation())
        {
            fmt::print("\n"); // Newline after ^C
            fmt::print(fg(fmt::color::yellow), "Cancelling command...\n");
            current_command_->cancel();
        }
        else if (current_command_)
        {
            fmt::print("\n");
            fmt::print(
                fg(fmt::color::yellow),
                "Command does not support cancellation. Press CTRL-C again to force exit.\n");
        }
        else
        {
            // No command running, just print newline
            fmt::print("\n");
        }
    }

    void setupSignalHandler()
    {
        instance_ = this;
        std::signal(SIGINT, signalHandler);
    }

    void loadHistory()
    {
        if (history_file_.empty())
        {
            return;
        }

        std::ifstream file(history_file_);
        if (!file.is_open())
        {
            // History file doesn't exist yet, that's okay
            return;
        }

        std::string line;
        while (std::getline(file, line))
        {
            if (!line.empty())
            {
                replxx_.history_add(line);
            }
        }
    }

    void saveHistory()
    {
        if (history_file_.empty())
        {
            return;
        }

        std::ofstream file(history_file_);
        if (!file.is_open())
        {
            // Can't save history, but don't fail
            return;
        }

        // Get all history and save it
        replxx::Replxx::HistoryScan history(replxx_.history_scan());
        while (history.next())
        {
            file << history.get().text() << "\n";
        }
    }

    void setupReplxx()
    {
        // Set up history file
        replxx_.set_max_history_size(1000);
        replxx_.set_max_hint_rows(3);

        // Enable word break characters
        replxx_.set_word_break_characters(" \t.,-%!;:=*~^'\"/?<>|[](){}");

        // Set up tab completion
        replxx_.set_completion_callback([this](const std::string& context, int& contextLen)
                                        { return getCompletions(context, contextLen); });

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
        // Format the prompt with tokens replaced
        std::string formatted_prompt = PromptFormatter::format(config_.prompt_format);

        if (terminal_info_.hasColorSupport())
        {
            return fmt::format(fg(fmt::color::green) | fmt::emphasis::bold, "{}", formatted_prompt);
        }
        return formatted_prompt;
    }

    void printWelcome()
    {
        if (!config_.motd.empty())
        {
            fmt::print("{}\n\n", config_.motd);
        }

        if (terminal_info_.hasEmojiSupport() && config_.motd.find("✨") == std::string::npos)
        {
            fmt::print("✨ Emoji support detected!\n\n");
        }
    }

    replxx::Replxx::completions_t getCompletions(const std::string& context, int& contextLen)
    {
        replxx::Replxx::completions_t completions;

        // Split the context to get the current word
        std::string current_word;
        size_t last_space = context.find_last_of(' ');
        bool is_first_word = (last_space == std::string::npos);

        if (is_first_word)
        {
            current_word = context;
            contextLen = static_cast<int>(current_word.length());
        }
        else
        {
            current_word = context.substr(last_space + 1);
            contextLen = static_cast<int>(current_word.length());
        }

        // If it's the first word, complete commands
        if (is_first_word)
        {
            auto& registry = CommandRegistry::getInstance();
            for (const auto& name : registry.getAllCommandNames())
            {
                if (name.find(current_word) == 0)
                {
                    completions.push_back(name);
                }
            }
        }
        else
        {
            // For subsequent words, complete file/directory names
            completeFilesAndDirectories(current_word, completions);
        }

        return completions;
    }

    void completeFilesAndDirectories(const std::string& prefix,
                                     replxx::Replxx::completions_t& completions)
    {
        auto& vfs = VirtualFilesystem::getInstance();
        std::string dir_path;
        std::string search_prefix;

        // Determine directory to search
        size_t last_slash = prefix.find_last_of('/');
        if (last_slash != std::string::npos)
        {
            dir_path = prefix.substr(0, last_slash);
            search_prefix = prefix.substr(last_slash + 1);
            if (dir_path.empty())
            {
                dir_path = "/";
            }
        }
        else
        {
            dir_path = vfs.getCurrentDirectory();
            search_prefix = prefix;
        }

        // Get directory listing
        try
        {
            if (!vfs.exists(dir_path) || !vfs.isDirectory(dir_path))
            {
                return;
            }

            auto entries = vfs.listDirectory(dir_path);
            for (const auto& entry : entries)
            {
                // Check if entry name starts with the search prefix
                if (entry.name.find(search_prefix) == 0)
                {
                    std::string completion;
                    if (last_slash != std::string::npos)
                    {
                        completion = dir_path;
                        if (dir_path != "/")
                        {
                            completion += "/";
                        }
                        completion += entry.name;
                    }
                    else
                    {
                        completion = entry.name;
                    }

                    // Add trailing slash for directories
                    if (entry.is_directory)
                    {
                        completion += "/";
                    }

                    completions.push_back(completion);
                }
            }
        }
        catch (...)
        {
            // Ignore errors during completion
        }
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
        context.verbose = false; // Could be set from config or flag

        if (command->getType() == CommandType::Asynchronous)
        {
            return executeAsync(command, context);
        }
        else
        {
            return command->execute(context);
        }
    }

    Status executeAsync(std::shared_ptr<ICommand> command, const CommandContext& context)
    {
        // Store current command for cancellation
        current_command_ = command;
        interrupt_received_.store(false);

        // Launch async task
        auto future = std::async(std::launch::async,
                                 [command, context]() { return command->execute(context); });

        // Wait for completion
        Status result = future.get();

        // Clear current command
        current_command_ = nullptr;

        return result;
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
    std::shared_ptr<ICommand> current_command_;
    std::atomic<bool> interrupt_received_;
    std::string history_file_;
};

Shell* Shell::instance_ = nullptr;

} // namespace homeshell
