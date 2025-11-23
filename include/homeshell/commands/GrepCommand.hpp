#pragma once

#include <homeshell/Command.hpp>
#include <homeshell/Status.hpp>
#include <homeshell/VirtualFilesystem.hpp>

#include <fmt/color.h>
#include <fmt/core.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

namespace homeshell
{

/**
 * @brief Search for patterns in files.
 *
 * The `grep` command searches for PATTERN in each FILE. By default, it prints
 * each line that matches the pattern.
 *
 * @details This command supports:
 *          - Pattern matching using regular expressions.
 *          - Case-insensitive search (`-i`).
 *          - Recursive directory search (`-r`).
 *          - Reading from standard input when no files specified.
 *          - Colored output highlighting matches (when stdout is a TTY).
 *          - Works with both regular filesystem and encrypted virtual mounts.
 *          - Multiple file search with filename prefixes.
 *
 *          Command syntax:
 *          ```
 *          grep [OPTIONS] PATTERN [FILE...]
 *          ```
 *
 *          Options:
 *          - `-i`, `--ignore-case`: Ignore case distinctions in pattern and data.
 *          - `-r`, `--recursive`: Read all files under each directory, recursively.
 *          - `-n`, `--line-number`: Prefix each line with line number.
 *          - `-H`, `--with-filename`: Print filename for each match (default when multiple files).
 *          - `-h`, `--no-filename`: Suppress filename prefix (default when single file).
 *          - `--color`: Highlight matching strings (auto-enabled for TTY).
 *          - `--help`: Display help message.
 *
 *          Example usage:
 *          ```
 *          grep "error" log.txt              # Search for "error" in log.txt
 *          grep -i "warning" *.log           # Case-insensitive search in log files
 *          grep -r "TODO" src/               # Recursive search in src/ directory
 *          grep -rn "bug" .                  # Recursive with line numbers
 *          cat file.txt | grep "pattern"     # Search piped input
 *          grep -i "error" /secure/logs.txt  # Search in encrypted mount
 *          ls -l | grep "\.cpp"              # Filter ls output
 *          ```
 *
 * @note Patterns are regular expressions (ECMAScript syntax).
 * @note Color output is automatically disabled when output is piped.
 */
class GrepCommand : public ICommand
{
public:
    std::string getName() const override
    {
        return "grep";
    }

    std::string getDescription() const override
    {
        return "Search for patterns in files";
    }

    CommandType getType() const override
    {
        return CommandType::Synchronous;
    }

    Status execute(const CommandContext& context) override
    {
        bool ignore_case = false;
        bool recursive = false;
        bool show_line_numbers = false;
        bool with_filename = false;
        bool no_filename = false;
        bool use_color = isatty(STDOUT_FILENO); // Auto-detect TTY

        std::string pattern;
        std::vector<std::string> files;

        // Parse arguments
        for (size_t i = 0; i < context.args.size(); ++i)
        {
            const auto& arg = context.args[i];

            if (arg == "--help")
            {
                showHelp();
                return Status::ok();
            }
            else if (arg == "-i" || arg == "--ignore-case")
            {
                ignore_case = true;
            }
            else if (arg == "-r" || arg == "--recursive")
            {
                recursive = true;
            }
            else if (arg == "-n" || arg == "--line-number")
            {
                show_line_numbers = true;
            }
            else if (arg == "-H" || arg == "--with-filename")
            {
                with_filename = true;
                no_filename = false;
            }
            else if (arg == "-h" || arg == "--no-filename")
            {
                no_filename = true;
                with_filename = false;
            }
            else if (arg == "--color")
            {
                use_color = true;
            }
            else if (arg == "--no-color")
            {
                use_color = false;
            }
            else if (arg[0] == '-' && arg.length() > 1 && arg[1] != '-')
            {
                // Parse combined short options like -rni
                bool valid = true;
                for (size_t j = 1; j < arg.length(); ++j)
                {
                    switch (arg[j])
                    {
                    case 'i':
                        ignore_case = true;
                        break;
                    case 'r':
                        recursive = true;
                        break;
                    case 'n':
                        show_line_numbers = true;
                        break;
                    case 'H':
                        with_filename = true;
                        no_filename = false;
                        break;
                    case 'h':
                        no_filename = true;
                        with_filename = false;
                        break;
                    default:
                        fmt::print(fg(fmt::color::red), "Error: Unknown option '-{}'\n", arg[j]);
                        fmt::print("Use --help for usage information\n");
                        valid = false;
                        break;
                    }
                    if (!valid)
                        break;
                }
                if (!valid)
                    return Status::error("Unknown option");
            }
            else if (arg[0] == '-')
            {
                fmt::print(fg(fmt::color::red), "Error: Unknown option '{}'\n", arg);
                fmt::print("Use --help for usage information\n");
                return Status::error("Unknown option");
            }
            else if (pattern.empty())
            {
                pattern = arg;
            }
            else
            {
                files.push_back(arg);
            }
        }

        if (pattern.empty())
        {
            fmt::print(fg(fmt::color::red), "Error: Missing pattern\n");
            fmt::print("Usage: grep [OPTIONS] PATTERN [FILE...]\n");
            return Status::error("Missing pattern");
        }

        // Compile regex
        std::regex regex_pattern;
        try
        {
            auto flags = std::regex::ECMAScript;
            if (ignore_case)
            {
                flags |= std::regex::icase;
            }
            regex_pattern = std::regex(pattern, flags);
        }
        catch (const std::regex_error& e)
        {
            fmt::print(fg(fmt::color::red), "Error: Invalid pattern: {}\n", e.what());
            return Status::error("Invalid pattern");
        }

        int match_count = 0;

        // Determine if we should show filenames
        bool show_filename = with_filename;
        if (!no_filename && !with_filename)
        {
            // Default: show filename if multiple files or recursive
            show_filename = (files.size() > 1) || recursive;
        }

        if (files.empty())
        {
            // Read from stdin
            match_count += searchStream(std::cin, regex_pattern, "(stdin)", show_line_numbers,
                                        false, use_color);
        }
        else
        {
            for (const auto& file : files)
            {
                if (recursive)
                {
                    match_count += searchRecursive(file, regex_pattern, show_line_numbers,
                                                   show_filename, use_color);
                }
                else
                {
                    match_count += searchFile(file, regex_pattern, show_line_numbers,
                                             show_filename, use_color);
                }
            }
        }

        // grep returns success if at least one match found
        return match_count > 0 ? Status::ok() : Status::error("No matches found");
    }

private:
    void showHelp() const
    {
        fmt::print("Usage: grep [OPTIONS] PATTERN [FILE...]\n");
        fmt::print("       grep [OPTIONS] PATTERN       # Read from stdin\n");
        fmt::print("       command | grep PATTERN       # Pipe input\n");
        fmt::print("\n");
        fmt::print("Search for PATTERN in each FILE or standard input.\n");
        fmt::print("\n");
        fmt::print("Options:\n");
        fmt::print("  -i, --ignore-case     Ignore case distinctions\n");
        fmt::print("  -r, --recursive       Search directories recursively\n");
        fmt::print("  -n, --line-number     Show line numbers\n");
        fmt::print("  -H, --with-filename   Print filename for each match\n");
        fmt::print("  -h, --no-filename     Suppress filename prefix\n");
        fmt::print("  --color               Highlight matches (default for TTY)\n");
        fmt::print("  --no-color            Disable highlighting\n");
        fmt::print("  --help                Show this help\n");
        fmt::print("\n");
        fmt::print("Examples:\n");
        fmt::print("  grep \"error\" log.txt          # Search for error\n");
        fmt::print("  grep -i \"warning\" *.log       # Case-insensitive\n");
        fmt::print("  grep -r \"TODO\" src/           # Recursive search\n");
        fmt::print("  grep -rn \"bug\" .              # With line numbers\n");
        fmt::print("  cat file.txt | grep \"pattern\" # Pipe input\n");
    }

    /**
     * @brief Search in a single file
     */
    int searchFile(const std::string& filename, const std::regex& pattern, bool show_line_numbers,
                   bool show_filename, bool use_color)
    {
        auto& vfs = VirtualFilesystem::getInstance();
        auto resolved = vfs.resolvePath(filename);

        std::string content;

        // Try VFS first
        if (vfs.exists(resolved.full_path))
        {
            if (!vfs.readFile(resolved.full_path, content))
            {
                fmt::print(fg(fmt::color::red), "grep: {}: Failed to read file\n", filename);
                return 0;
            }
        }
        else
        {
            // Try regular filesystem
            std::ifstream file(filename);
            if (!file.good())
            {
                fmt::print(fg(fmt::color::red), "grep: {}: No such file or directory\n",
                          filename);
                return 0;
            }

            std::stringstream buffer;
            buffer << file.rdbuf();
            content = buffer.str();
        }

        std::istringstream stream(content);
        return searchStream(stream, pattern, filename, show_line_numbers, show_filename,
                           use_color);
    }

    /**
     * @brief Search in a stream
     */
    int searchStream(std::istream& stream, const std::regex& pattern,
                     const std::string& filename, bool show_line_numbers, bool show_filename,
                     bool use_color)
    {
        int match_count = 0;
        std::string line;
        int line_number = 0;

        while (std::getline(stream, line))
        {
            line_number++;

            std::smatch match;
            if (std::regex_search(line, match, pattern))
            {
                match_count++;

                // Build output line
                std::string output;

                if (show_filename)
                {
                    if (use_color)
                    {
                        output += fmt::format(fg(fmt::color::magenta), "{}", filename);
                    }
                    else
                    {
                        output += filename;
                    }
                    output += ":";
                }

                if (show_line_numbers)
                {
                    if (use_color)
                    {
                        output += fmt::format(fg(fmt::color::green), "{}", line_number);
                    }
                    else
                    {
                        output += std::to_string(line_number);
                    }
                    output += ":";
                }

                // Highlight matches in the line
                if (use_color)
                {
                    std::string highlighted = highlightMatches(line, pattern);
                    output += highlighted;
                }
                else
                {
                    output += line;
                }

                fmt::print("{}\n", output);
            }
        }

        return match_count;
    }

    /**
     * @brief Search recursively in a directory
     */
    int searchRecursive(const std::string& path, const std::regex& pattern,
                       bool show_line_numbers, bool show_filename, bool use_color)
    {
        int match_count = 0;

        try
        {
            std::filesystem::path fs_path(path);

            if (!std::filesystem::exists(fs_path))
            {
                fmt::print(fg(fmt::color::red), "grep: {}: No such file or directory\n", path);
                return 0;
            }

            if (std::filesystem::is_regular_file(fs_path))
            {
                return searchFile(path, pattern, show_line_numbers, show_filename, use_color);
            }

            if (std::filesystem::is_directory(fs_path))
            {
                for (const auto& entry : std::filesystem::recursive_directory_iterator(
                         fs_path, std::filesystem::directory_options::skip_permission_denied))
                {
                    if (entry.is_regular_file())
                    {
                        match_count += searchFile(entry.path().string(), pattern,
                                                 show_line_numbers, show_filename, use_color);
                    }
                }
            }
        }
        catch (const std::filesystem::filesystem_error& e)
        {
            fmt::print(fg(fmt::color::red), "grep: {}: {}\n", path, e.what());
        }

        return match_count;
    }

    /**
     * @brief Highlight all matches in a line
     */
    std::string highlightMatches(const std::string& line, const std::regex& pattern) const
    {
        std::string result;
        auto words_begin = std::sregex_iterator(line.begin(), line.end(), pattern);
        auto words_end = std::sregex_iterator();

        size_t last_pos = 0;

        for (std::sregex_iterator i = words_begin; i != words_end; ++i)
        {
            std::smatch match = *i;

            // Add text before match
            result += line.substr(last_pos, match.position() - last_pos);

            // Add highlighted match
            result += fmt::format(fg(fmt::color::red) | fmt::emphasis::bold, "{}", match.str());

            last_pos = match.position() + match.length();
        }

        // Add remaining text
        result += line.substr(last_pos);

        return result;
    }
};

} // namespace homeshell

