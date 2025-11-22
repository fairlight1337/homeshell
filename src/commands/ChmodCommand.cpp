#include <homeshell/VirtualFilesystem.hpp>
#include <homeshell/commands/ChmodCommand.hpp>

#include <fmt/color.h>
#include <fmt/core.h>
#include <sys/stat.h>

#include <cctype>
#include <cerrno>
#include <cstring>
#include <sstream>

namespace homeshell
{

bool ChmodCommand::parseOctalMode(const std::string& mode_str, mode_t& mode)
{
    // Remove leading 0 if present
    std::string octal_str = mode_str;
    if (octal_str[0] == '0' && octal_str.length() > 1)
    {
        octal_str = octal_str.substr(1);
    }

    // Must be 3 or 4 digits
    if (octal_str.length() != 3 && octal_str.length() != 4)
    {
        return false;
    }

    // Check all digits are 0-7
    for (char c : octal_str)
    {
        if (c < '0' || c > '7')
        {
            return false;
        }
    }

    // Convert to mode_t
    mode = 0;
    for (char c : octal_str)
    {
        mode = mode * 8 + (c - '0');
    }

    return true;
}

bool ChmodCommand::parseSymbolicMode(const std::string& mode_str, mode_t current_mode,
                                     mode_t& new_mode)
{
    new_mode = current_mode;

    // Split by comma
    std::vector<std::string> parts;
    std::stringstream ss(mode_str);
    std::string part;
    while (std::getline(ss, part, ','))
    {
        parts.push_back(part);
    }

    for (const auto& expr : parts)
    {
        if (expr.empty())
            continue;

        // Parse: [ugoa...][[+-=][rwxXst...]]
        size_t i = 0;

        // Parse who (user, group, other, all)
        mode_t who_mask = 0;
        bool has_who = false;

        while (i < expr.length() &&
               (expr[i] == 'u' || expr[i] == 'g' || expr[i] == 'o' || expr[i] == 'a'))
        {
            has_who = true;
            if (expr[i] == 'u')
                who_mask |= S_IRWXU;
            else if (expr[i] == 'g')
                who_mask |= S_IRWXG;
            else if (expr[i] == 'o')
                who_mask |= S_IRWXO;
            else if (expr[i] == 'a')
                who_mask |= S_IRWXU | S_IRWXG | S_IRWXO;
            i++;
        }

        // If no who specified, apply to all (but respect umask in real chmod, we'll apply to all)
        if (!has_who)
        {
            who_mask = S_IRWXU | S_IRWXG | S_IRWXO;
        }

        // Parse operation (+, -, =)
        if (i >= expr.length())
        {
            return false; // No operation
        }

        char op = expr[i++];
        if (op != '+' && op != '-' && op != '=')
        {
            return false;
        }

        // Parse permissions
        mode_t perm_bits = 0;
        while (i < expr.length())
        {
            if (expr[i] == 'r')
            {
                if (who_mask & S_IRWXU)
                    perm_bits |= S_IRUSR;
                if (who_mask & S_IRWXG)
                    perm_bits |= S_IRGRP;
                if (who_mask & S_IRWXO)
                    perm_bits |= S_IROTH;
            }
            else if (expr[i] == 'w')
            {
                if (who_mask & S_IRWXU)
                    perm_bits |= S_IWUSR;
                if (who_mask & S_IRWXG)
                    perm_bits |= S_IWGRP;
                if (who_mask & S_IRWXO)
                    perm_bits |= S_IWOTH;
            }
            else if (expr[i] == 'x')
            {
                if (who_mask & S_IRWXU)
                    perm_bits |= S_IXUSR;
                if (who_mask & S_IRWXG)
                    perm_bits |= S_IXGRP;
                if (who_mask & S_IRWXO)
                    perm_bits |= S_IXOTH;
            }
            else
            {
                return false; // Unknown permission
            }
            i++;
        }

        // Apply operation
        if (op == '+')
        {
            new_mode |= perm_bits;
        }
        else if (op == '-')
        {
            new_mode &= ~perm_bits;
        }
        else if (op == '=')
        {
            new_mode = (new_mode & ~who_mask) | perm_bits;
        }
    }

    return true;
}

bool ChmodCommand::applyChmod(const std::string& path, mode_t mode)
{
    auto& vfs = VirtualFilesystem::getInstance();
    auto resolved_path = vfs.resolvePath(path);

    // Check if this is a virtual filesystem path
    if (resolved_path.type == PathType::Virtual)
    {
        // Virtual filesystems don't have real chmod, but we can store metadata
        // For now, just report that we don't support it
        fmt::print(fg(fmt::color::red),
                   "Error: chmod not supported on virtual filesystem paths: '{}'\n", path);
        return false;
    }

    // Apply to regular filesystem
    if (chmod(resolved_path.full_path.c_str(), mode) == 0)
    {
        fmt::print("Changed permissions of '{}' to {:04o}\n", path, mode);
        return true;
    }

    return false;
}

Status ChmodCommand::execute(const CommandContext& context)
{
    if (context.args.size() < 2)
    {
        fmt::print(fg(fmt::color::red), "Error: chmod requires mode and file arguments\n");
        fmt::print("Usage: chmod <mode> <file>...\n");
        return Status::error("Missing arguments");
    }

    std::string mode_str = context.args[0];
    mode_t mode;
    bool is_octal = false;

    // Try parsing as octal first
    if (parseOctalMode(mode_str, mode))
    {
        is_octal = true;
    }
    else
    {
        // Not octal, must be symbolic
        is_octal = false;
    }

    // Apply to all specified files
    bool all_success = true;
    for (size_t i = 1; i < context.args.size(); i++)
    {
        std::string path = context.args[i];
        mode_t file_mode = mode;

        // For symbolic modes, we need the current permissions
        if (!is_octal)
        {
            struct stat st;
            if (stat(path.c_str(), &st) == 0)
            {
                if (!parseSymbolicMode(mode_str, st.st_mode, file_mode))
                {
                    fmt::print(fg(fmt::color::red), "Error: Invalid mode '{}'\n", mode_str);
                    return Status::error("Invalid mode");
                }
            }
            else
            {
                // File doesn't exist or can't stat, use default 0644
                mode_t default_mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
                if (!parseSymbolicMode(mode_str, default_mode, file_mode))
                {
                    fmt::print(fg(fmt::color::red), "Error: Invalid mode '{}'\n", mode_str);
                    return Status::error("Invalid mode");
                }
            }
        }

        if (!applyChmod(path, file_mode))
        {
            fmt::print(fg(fmt::color::red), "Error: Could not change permissions of '{}': {}\n",
                       path, strerror(errno));
            all_success = false;
        }
    }

    return all_success ? Status::ok() : Status::error("Some chmod operations failed");
}

} // namespace homeshell
