#include <homeshell/OutputRedirection.hpp>

#include <fcntl.h>
#include <unistd.h>

#include <algorithm>
#include <cstdio>
#include <iostream>
#include <sstream>

namespace homeshell
{

RedirectInfo OutputRedirection::parse(const std::string& command_line)
{
    RedirectInfo info;
    info.command = command_line;

    // Look for redirection operators: >, >>, 2>, 2>>, &>, &>>
    // Priority: &> (both), 2> (stderr), > (stdout)

    size_t pos = std::string::npos;
    size_t operator_len = 0;

    // Check for &>> (both, append)
    size_t amp_append = command_line.find("&>>");
    if (amp_append != std::string::npos)
    {
        pos = amp_append;
        operator_len = 3;
        info.type = RedirectType::Both;
        info.mode = RedirectMode::Append;
    }
    // Check for &> (both, truncate)
    else
    {
        size_t amp_trunc = command_line.find("&>");
        if (amp_trunc != std::string::npos)
        {
            pos = amp_trunc;
            operator_len = 2;
            info.type = RedirectType::Both;
            info.mode = RedirectMode::Truncate;
        }
    }

    // Check for 2>> (stderr, append)
    if (pos == std::string::npos)
    {
        size_t stderr_append = command_line.find("2>>");
        if (stderr_append != std::string::npos)
        {
            pos = stderr_append;
            operator_len = 3;
            info.type = RedirectType::Stderr;
            info.mode = RedirectMode::Append;
        }
    }

    // Check for 2> (stderr, truncate)
    if (pos == std::string::npos)
    {
        size_t stderr_trunc = command_line.find("2>");
        if (stderr_trunc != std::string::npos)
        {
            pos = stderr_trunc;
            operator_len = 2;
            info.type = RedirectType::Stderr;
            info.mode = RedirectMode::Truncate;
        }
    }

    // Check for >> (stdout, append)
    if (pos == std::string::npos)
    {
        size_t stdout_append = command_line.find(">>");
        if (stdout_append != std::string::npos)
        {
            pos = stdout_append;
            operator_len = 2;
            info.type = RedirectType::Stdout;
            info.mode = RedirectMode::Append;
        }
    }

    // Check for > (stdout, truncate) - must be last to avoid matching >>
    if (pos == std::string::npos)
    {
        size_t stdout_trunc = command_line.find(">");
        if (stdout_trunc != std::string::npos)
        {
            // Make sure it's not part of >>
            if (stdout_trunc + 1 >= command_line.length() || command_line[stdout_trunc + 1] != '>')
            {
                pos = stdout_trunc;
                operator_len = 1;
                info.type = RedirectType::Stdout;
                info.mode = RedirectMode::Truncate;
            }
        }
    }

    if (pos != std::string::npos)
    {
        // Extract command part (before operator)
        info.command = command_line.substr(0, pos);

        // Remove trailing whitespace from command
        info.command.erase(info.command.find_last_not_of(" \t\r\n") + 1);

        // Extract filename (after operator)
        std::string filename_part = command_line.substr(pos + operator_len);

        // Remove leading whitespace from filename
        size_t first_non_space = filename_part.find_first_not_of(" \t\r\n");
        if (first_non_space != std::string::npos)
        {
            filename_part = filename_part.substr(first_non_space);
        }

        // Extract just the filename (stop at first space)
        std::istringstream iss(filename_part);
        iss >> info.filename;
    }

    return info;
}

bool OutputRedirection::redirect(const RedirectInfo& info)
{
    if (info.type == RedirectType::None || info.filename.empty())
    {
        return false;
    }

    // Open the output file
    const char* mode = (info.mode == RedirectMode::Append) ? "a" : "w";
    redirect_file_ = fopen(info.filename.c_str(), mode);

    if (!redirect_file_)
    {
        std::cerr << "Error: Cannot open file '" << info.filename << "' for writing\n";
        return false;
    }

    // Save original file descriptors
    saved_stdout_fd_ = dup(STDOUT_FILENO);
    saved_stderr_fd_ = dup(STDERR_FILENO);

    if (saved_stdout_fd_ == -1 || saved_stderr_fd_ == -1)
    {
        std::cerr << "Error: Failed to save file descriptors\n";
        fclose(redirect_file_);
        redirect_file_ = nullptr;
        return false;
    }

    // Redirect stdout and/or stderr
    int target_fd = fileno(redirect_file_);

    // Flush streams before redirecting
    fflush(stdout);
    fflush(stderr);

    if (info.type == RedirectType::Stdout || info.type == RedirectType::Both)
    {
        if (dup2(target_fd, STDOUT_FILENO) == -1)
        {
            std::cerr << "Error: Failed to redirect stdout\n";
            restore();
            return false;
        }
    }

    if (info.type == RedirectType::Stderr || info.type == RedirectType::Both)
    {
        if (dup2(target_fd, STDERR_FILENO) == -1)
        {
            std::cerr << "Error: Failed to redirect stderr\n";
            restore();
            return false;
        }
    }

    redirected_ = true;
    return true;
}

void OutputRedirection::restore()
{
    if (!redirected_)
    {
        return;
    }

    // Flush streams before restoring
    fflush(stdout);
    fflush(stderr);

    // Restore stdout
    if (saved_stdout_fd_ != -1)
    {
        dup2(saved_stdout_fd_, STDOUT_FILENO);
        close(saved_stdout_fd_);
        saved_stdout_fd_ = -1;
    }

    // Restore stderr
    if (saved_stderr_fd_ != -1)
    {
        dup2(saved_stderr_fd_, STDERR_FILENO);
        close(saved_stderr_fd_);
        saved_stderr_fd_ = -1;
    }

    // Close redirect file
    if (redirect_file_)
    {
        fclose(redirect_file_);
        redirect_file_ = nullptr;
    }

    redirected_ = false;
}

} // namespace homeshell
