#pragma once

#include <homeshell/Command.hpp>
#include <homeshell/Config.hpp>
#include <homeshell/OutputRedirection.hpp>
#include <homeshell/PipelineExecutor.hpp>
#include <homeshell/PromptFormatter.hpp>
#include <homeshell/Status.hpp>
#include <homeshell/TerminalInfo.hpp>
#include <homeshell/VirtualFilesystem.hpp>

#include <fmt/color.h>
#include <fmt/core.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include <atomic>
#include <csignal>
#include <cstring>
#include <future>
#include <memory>
#include <replxx.hxx>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace homeshell
{

/**
 * @brief Main shell execution engine
 *
 * The Shell class is the heart of Homeshell, providing:
 * - Interactive REPL (Read-Eval-Print-Loop)
 * - Command parsing and execution
 * - Signal handling (Ctrl+C, Ctrl+D)
 * - Command history management
 * - Tab completion for commands and files
 * - Output redirection (>, >>)
 * - Asynchronous command execution
 * - Executable file execution (./ prefix)
 *
 * @details The shell operates in a continuous loop:
 *          1. Display prompt with current directory
 *          2. Read user input (with completion and history)
 *          3. Parse command line (handle redirection, tokenization)
 *          4. Execute command (sync or async)
 *          5. Handle signals (interrupts, cancellation)
 *          6. Display results or errors
 *
 *          Special features:
 *          - Tab completion for commands, files, and directories
 *          - Ctrl+C interrupt handling (cancels async commands)
 *          - Ctrl+D exits the shell
 *          - Persistent command history (saved on exit)
 *          - Output redirection to files (> and >>)
 *          - Color stripping for redirected output
 *          - Executable file support (./ prefix or containing /)
 *          - Virtual filesystem integration
 *
 *          Architecture:
 *          - Uses replxx library for advanced line editing
 *          - CommandRegistry for command lookup
 *          - VirtualFilesystem for unified file access
 *          - OutputRedirection for stream management
 *          - Signal handler for interrupt processing
 *
 * Example usage:
 * ```cpp
 * Config config = Config::loadFromFile("config.json");
 * TerminalInfo term_info = TerminalInfo::detect();
 * Shell shell(config, term_info);
 * shell.run();  // Start REPL
 * ```
 */
class Shell
{
    friend class PipelineExecutor;

public:
    /**
     * @brief Construct shell with configuration and terminal info
     * @param config Shell configuration (prompt, history, mounts, etc.)
     * @param terminal_info Detected terminal capabilities (colors, UTF-8, etc.)
     */
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

    /**
     * @brief Destructor - cleanup resources
     *
     * Performs cleanup:
     * - Saves command history to disk
     * - Restores default signal handlers
     * - Allows VirtualFilesystem cleanup
     */
    ~Shell()
    {
        // Save history before exiting
        saveHistory();

        // Restore default signal handler
        std::signal(SIGINT, SIG_DFL);
    }

    /**
     * @brief Start the interactive REPL loop
     *
     * Main shell loop that:
     * - Displays welcome message (MOTD)
     * - Shows prompt and waits for input
     * - Executes commands
     * - Handles EOF (Ctrl+D) and exit command
     * - Continues until user exits
     *
     * @note This function blocks until the shell exits
     */
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

    /**
     * @brief Execute a command line string
     * @param command_line Full command line to execute
     * @return Status indicating success, failure, or exit
     *
     * @details Parses and executes a command line, handling:
     *          - Command tokenization
     *          - Output redirection detection
     *          - Executable file execution
     *          - Built-in command dispatch
     *
     *          This is a convenience method that wraps executeCommand()
     *          and is useful for programmatic command execution.
     */
    Status executeCommandLine(const std::string& command_line)
    {
        return executeCommand(command_line);
    }

    /**
     * @brief Global shell instance for signal handling
     *
     * Static pointer to current shell instance, used by the signal
     * handler to access the shell's interrupt handling methods.
     */
    static Shell* instance_;

private:
    /**
     * @brief Static signal handler for SIGINT (Ctrl+C)
     * @param signal Signal number (SIGINT)
     *
     * @details Called by the OS when user presses Ctrl+C.
     *          Forwards the interrupt to the current shell instance
     *          via handleInterrupt().
     */
    static void signalHandler(int signal)
    {
        if (signal == SIGINT && instance_)
        {
            instance_->handleInterrupt();
        }
    }

    /**
     * @brief Handle interrupt signal (Ctrl+C)
     *
     * @details Processes SIGINT by:
     *          - Setting interrupt flag
     *          - Cancelling current async command (if any)
     *          - Printing cancellation message
     *          - Resetting interrupt flag after handling
     */
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

    /**
     * @brief Setup signal handler for interrupts
     *
     * Registers the static signalHandler() for SIGINT (Ctrl+C) events.
     * Sets the global instance pointer for signal handler access.
     */
    void setupSignalHandler()
    {
        instance_ = this;
        std::signal(SIGINT, signalHandler);
    }

    /**
     * @brief Load command history from disk
     *
     * Reads the history file (if it exists) and populates the
     * replxx history. Silently continues if file doesn't exist.
     */
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

    /**
     * @brief Save command history to disk
     *
     * Writes the current replxx history to the history file.
     * Creates the file if it doesn't exist, truncates if it does.
     */
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

    /**
     * @brief Execute a command line
     * @param line Full command line to execute (may include redirection)
     * @return Status indicating success, failure, or exit
     *
     * @details Main command execution flow:
     *          1. Parse output redirection (>, >>)
     *          2. Tokenize command into name and arguments
     *          3. Check for executable file (./ or containing /)
     *          4. Look up command in registry
     *          5. Setup output redirection if needed
     *          6. Execute command (sync or async)
     *          7. Restore output streams
     *          8. Return result status
     *
     *          This method handles:
     *          - Built-in commands (ls, cd, etc.)
     *          - Executable files (./script.sh)
     *          - Output redirection (command > file)
     *          - Color stripping for redirected output
     */
    Status executeCommand(const std::string& line)
    {
        // Check for pipes first (before output redirection parsing)
        if (PipelineExecutor::hasPipe(line))
        {
            auto commands = PipelineExecutor::splitPipeline(line);
            return PipelineExecutor::executePipeline(commands, this);
        }

        // Parse for output redirection
        RedirectInfo redirect_info = OutputRedirection::parse(line);

        // Use the command part (without redirection) for execution
        std::string command_line = redirect_info.command;

        // Parse command line
        std::vector<std::string> tokens = tokenize(command_line);
        if (tokens.empty())
        {
            return Status::ok();
        }

        std::string cmd_name = tokens[0];
        std::vector<std::string> args(tokens.begin() + 1, tokens.end());

        // Check if this is an executable file path (starts with ./, ../, or / or contains /)
        if (cmd_name.find("/") != std::string::npos)
        {
            return executeExecutableFile(cmd_name, args);
        }

        // Look up command
        auto& registry = CommandRegistry::getInstance();
        auto command = registry.getCommand(cmd_name);

        if (!command)
        {
            return Status::error("Unknown command: " + cmd_name);
        }

        // Setup output redirection if needed
        OutputRedirection redirector;
        if (redirect_info.type != RedirectType::None)
        {
            if (!redirector.redirect(redirect_info))
            {
                return Status::error("Failed to setup output redirection");
            }
        }

        // Execute command
        CommandContext context;
        context.args = args;
        context.verbose = false; // Could be set from config or flag
        // Disable colors when output is redirected to a file
        context.use_colors = (redirect_info.type == RedirectType::None);

        Status result = (command->getType() == CommandType::Asynchronous)
                            ? executeAsync(command, context)
                            : command->execute(context);

        // Redirection is automatically restored by OutputRedirection destructor
        return result;
    }

    /**
     * @brief Execute an external executable file
     * @param path Path to executable file
     * @param args Command-line arguments to pass to executable
     * @return Status indicating success or failure
     *
     * @details Executes external scripts and binaries by:
     *          1. Checking file exists and has execute permission
     *          2. Forking a child process
     *          3. Executing the file with execv()
     *          4. Waiting for child completion
     *          5. Returning child exit status
     *
     *          Supports both shebang scripts (#!/bin/bash) and native binaries.
     *          The parent process waits synchronously for child completion.
     */
    Status executeExecutableFile(const std::string& path, const std::vector<std::string>& args)
    {
        // Check if file exists and is executable
        struct stat st;
        if (stat(path.c_str(), &st) != 0)
        {
            return Status::error(fmt::format("File not found: {}", path));
        }

        if (!(st.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)))
        {
            return Status::error(fmt::format("File is not executable: {}", path));
        }

        // Fork and execute
        pid_t pid = fork();
        if (pid == -1)
        {
            return Status::error(fmt::format("Failed to fork: {}", strerror(errno)));
        }

        if (pid == 0) // Child process
        {
            // Prepare arguments
            std::vector<char*> c_args;
            c_args.push_back(const_cast<char*>(path.c_str()));
            for (const auto& arg : args)
            {
                c_args.push_back(const_cast<char*>(arg.c_str()));
            }
            c_args.push_back(nullptr);

            // Execute
            execv(path.c_str(), c_args.data());

            // If we reach here, exec failed
            fmt::print(stderr, "Failed to execute {}: {}\n", path, strerror(errno));
            exit(1);
        }
        else // Parent process
        {
            int status;
            waitpid(pid, &status, 0);

            if (WIFEXITED(status))
            {
                int exit_code = WEXITSTATUS(status);
                if (exit_code != 0)
                {
                    return Status::error(fmt::format("Process exited with code {}", exit_code));
                }
                return Status::ok();
            }
            else if (WIFSIGNALED(status))
            {
                return Status::error(
                    fmt::format("Process terminated by signal {}", WTERMSIG(status)));
            }

            return Status::error("Process terminated abnormally");
        }
    }

    /**
     * @brief Execute command asynchronously
     * @param command Command to execute
     * @param context Command execution context
     * @return Status from command execution
     *
     * @details Runs command in a background thread using std::async.
     *          This enables:
     *          - Non-blocking command execution
     *          - Interrupt handling during execution
     *          - Cancellation support (Ctrl+C)
     *
     *          The method waits for completion but allows signal
     *          handling during execution. The current_command_ pointer
     *          is set to enable cancellation via handleInterrupt().
     */
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

    /**
     * @brief Tokenize command line into words
     * @param line Command line string
     * @return Vector of tokens (command name and arguments)
     *
     * @details Simple whitespace-based tokenization.
     *          Does not handle:
     *          - Quoted strings with spaces
     *          - Escape sequences
     *          - Environment variable expansion
     */
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

} // namespace homeshell
