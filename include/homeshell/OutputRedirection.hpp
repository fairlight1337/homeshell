#pragma once

#include <cstdio>
#include <memory>
#include <string>

namespace homeshell
{

enum class RedirectType
{
    None,
    Stdout,
    Stderr,
    Both
};

enum class RedirectMode
{
    Truncate, // >
    Append    // >>
};

struct RedirectInfo
{
    RedirectType type = RedirectType::None;
    RedirectMode mode = RedirectMode::Truncate;
    std::string filename;
    std::string command; // Command without redirection part
};

class OutputRedirection
{
public:
    // Parse command line for redirection operators
    static RedirectInfo parse(const std::string& command_line);

    // Apply redirection (returns true on success)
    bool redirect(const RedirectInfo& info);

    // Restore original stdout/stderr
    void restore();

    ~OutputRedirection()
    {
        restore();
    }

private:
    FILE* original_stdout_ = nullptr;
    FILE* original_stderr_ = nullptr;
    FILE* redirect_file_ = nullptr;
    int saved_stdout_fd_ = -1;
    int saved_stderr_fd_ = -1;
    bool redirected_ = false;
};

} // namespace homeshell

