#include <homeshell/Command.hpp>
#include <homeshell/PipelineExecutor.hpp>
#include <homeshell/Shell.hpp>

#include <fcntl.h>
#include <fmt/color.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>

namespace homeshell
{

bool PipelineExecutor::hasPipe(const std::string& command_line)
{
    // Simple check: does the line contain '|' that's not in quotes?
    // For now, just check for presence of |
    return command_line.find('|') != std::string::npos;
}

std::vector<std::string> PipelineExecutor::splitPipeline(const std::string& command_line)
{
    std::vector<std::string> commands;
    std::string current;
    bool in_quotes = false;

    for (size_t i = 0; i < command_line.length(); ++i)
    {
        char c = command_line[i];

        if (c == '"')
        {
            in_quotes = !in_quotes;
            current += c;
        }
        else if (c == '|' && !in_quotes)
        {
            // Found pipe operator
            if (!current.empty())
            {
                // Trim whitespace
                size_t start = current.find_first_not_of(" \t");
                size_t end = current.find_last_not_of(" \t");
                if (start != std::string::npos && end != std::string::npos)
                {
                    commands.push_back(current.substr(start, end - start + 1));
                }
            }
            current.clear();
        }
        else
        {
            current += c;
        }
    }

    // Add last command
    if (!current.empty())
    {
        size_t start = current.find_first_not_of(" \t");
        size_t end = current.find_last_not_of(" \t");
        if (start != std::string::npos && end != std::string::npos)
        {
            commands.push_back(current.substr(start, end - start + 1));
        }
    }

    return commands;
}

Status PipelineExecutor::executePipeline(const std::vector<std::string>& commands, Shell* shell)
{
    if (commands.empty())
    {
        return Status::ok();
    }

    if (commands.size() == 1)
    {
        // No pipes, execute normally
        return shell->executeCommandLine(commands[0]);
    }

    // Create pipes for each connection
    std::vector<std::array<int, 2>> pipes;
    for (size_t i = 0; i < commands.size() - 1; ++i)
    {
        std::array<int, 2> pipe_fds;
        if (pipe(pipe_fds.data()) == -1)
        {
            return Status::error(fmt::format("Failed to create pipe: {}", std::strerror(errno)));
        }
        pipes.push_back(pipe_fds);
    }

    // Store PIDs of child processes
    std::vector<pid_t> pids;

    // Execute each command
    for (size_t i = 0; i < commands.size(); ++i)
    {
        // Parse command
        std::vector<std::string> tokens = shell->tokenize(commands[i]);
        if (tokens.empty())
        {
            continue;
        }

        std::string cmd_name = tokens[0];
        bool is_executable = (cmd_name.find('/') != std::string::npos);

        // Check if it's a built-in command
        auto& registry = CommandRegistry::getInstance();
        auto command = registry.getCommand(cmd_name);

        if (command && !is_executable)
        {
            // Built-in command - redirect stdout
            pid_t pid = fork();

            if (pid == -1)
            {
                // Fork failed
                return Status::error(fmt::format("Failed to fork: {}", std::strerror(errno)));
            }
            else if (pid == 0)
            {
                // Child process

                // Setup stdin from previous pipe
                if (i > 0)
                {
                    dup2(pipes[i - 1][0], STDIN_FILENO);
                }

                // Setup stdout to next pipe
                if (i < commands.size() - 1)
                {
                    dup2(pipes[i][1], STDOUT_FILENO);
                }

                // Close all pipe file descriptors
                for (auto& p : pipes)
                {
                    close(p[0]);
                    close(p[1]);
                }

                // Execute built-in command
                CommandContext ctx;
                ctx.args = std::vector<std::string>(tokens.begin() + 1, tokens.end());
                ctx.use_colors = false; // No colors when piping

                Status status = command->execute(ctx);

                // Exit with status code
                exit(status.isSuccess() ? 0 : 1);
            }
            else
            {
                // Parent process
                pids.push_back(pid);
            }
        }
        else
        {
            // External executable or not found
            pid_t pid = fork();

            if (pid == -1)
            {
                return Status::error(fmt::format("Failed to fork: {}", std::strerror(errno)));
            }
            else if (pid == 0)
            {
                // Child process

                // Setup stdin from previous pipe
                if (i > 0)
                {
                    dup2(pipes[i - 1][0], STDIN_FILENO);
                }

                // Setup stdout to next pipe
                if (i < commands.size() - 1)
                {
                    dup2(pipes[i][1], STDOUT_FILENO);
                }

                // Close all pipe file descriptors
                for (auto& p : pipes)
                {
                    close(p[0]);
                    close(p[1]);
                }

                // Execute external command
                if (is_executable)
                {
                    // It's a file path, execute it
                    std::vector<char*> c_args;
                    for (const auto& arg : tokens)
                    {
                        c_args.push_back(const_cast<char*>(arg.c_str()));
                    }
                    c_args.push_back(nullptr);

                    execv(cmd_name.c_str(), c_args.data());

                    // If we reach here, exec failed
                    fmt::print(stderr, "Failed to execute {}: {}\n", cmd_name,
                               std::strerror(errno));
                    exit(1);
                }
                else
                {
                    // Try to find in PATH
                    std::vector<char*> c_args;
                    for (const auto& arg : tokens)
                    {
                        c_args.push_back(const_cast<char*>(arg.c_str()));
                    }
                    c_args.push_back(nullptr);

                    execvp(cmd_name.c_str(), c_args.data());

                    // If we reach here, command not found
                    fmt::print(stderr, "Command not found: {}\n", cmd_name);
                    exit(127);
                }
            }
            else
            {
                // Parent process
                pids.push_back(pid);
            }
        }
    }

    // Close all pipes in parent
    for (auto& p : pipes)
    {
        close(p[0]);
        close(p[1]);
    }

    // Wait for all children
    Status final_status = Status::ok();
    for (size_t i = 0; i < pids.size(); ++i)
    {
        int status;
        waitpid(pids[i], &status, 0);

        // Record status of last command
        if (i == pids.size() - 1)
        {
            if (WIFEXITED(status))
            {
                int exit_code = WEXITSTATUS(status);
                if (exit_code != 0)
                {
                    final_status =
                        Status::error(fmt::format("Pipeline exited with code {}", exit_code));
                }
            }
            else if (WIFSIGNALED(status))
            {
                final_status = Status::error(
                    fmt::format("Pipeline terminated by signal {}", WTERMSIG(status)));
            }
        }
    }

    return final_status;
}

} // namespace homeshell
