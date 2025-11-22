#include <homeshell/VirtualFilesystem.hpp>
#include <homeshell/commands/FindCommand.hpp>

#include <fmt/color.h>
#include <fmt/core.h>

#include <algorithm>
#include <cctype>
#include <filesystem>

namespace homeshell
{

Status FindCommand::execute(const CommandContext& context)
{
    FindOptions options;

    // Parse arguments
    size_t i = 0;
    while (i < context.args.size())
    {
        std::string arg = context.args[i];

        if (arg == "-name" && i + 1 < context.args.size())
        {
            options.name_pattern = context.args[i + 1];
            i += 2;
        }
        else if (arg == "-iname" && i + 1 < context.args.size())
        {
            options.name_pattern = context.args[i + 1];
            options.case_insensitive = true;
            i += 2;
        }
        else if (arg == "-type" && i + 1 < context.args.size())
        {
            std::string type_arg = context.args[i + 1];
            if (type_arg == "f")
            {
                options.type = FileType::File;
            }
            else if (type_arg == "d")
            {
                options.type = FileType::Directory;
            }
            else
            {
                fmt::print(fg(fmt::color::red),
                           "Error: Invalid type '{}'. Use 'f' for file or 'd' for directory\n",
                           type_arg);
                return Status::error("Invalid type argument");
            }
            i += 2;
        }
        else if (arg == "-maxdepth" && i + 1 < context.args.size())
        {
            try
            {
                options.max_depth = std::stoi(context.args[i + 1]);
                if (options.max_depth < 0)
                {
                    fmt::print(fg(fmt::color::red), "Error: -maxdepth must be non-negative\n");
                    return Status::error("Invalid maxdepth value");
                }
            }
            catch (...)
            {
                fmt::print(fg(fmt::color::red), "Error: Invalid -maxdepth value '{}'\n",
                           context.args[i + 1]);
                return Status::error("Invalid maxdepth value");
            }
            i += 2;
        }
        else if (arg == "--help")
        {
            fmt::print("Usage: find [path] [options]\n");
            fmt::print("\nOptions:\n");
            fmt::print(
                "  -name <pattern>      Search for files matching pattern (wildcards: *, ?)\n");
            fmt::print("  -iname <pattern>     Case-insensitive name search\n");
            fmt::print("  -type <f|d>          Filter by type: f=file, d=directory\n");
            fmt::print("  -maxdepth <n>        Descend at most n levels\n");
            fmt::print("  --help               Show this help message\n");
            fmt::print("\nExamples:\n");
            fmt::print(
                "  find                 List all files recursively from current directory\n");
            fmt::print("  find /tmp            List all files in /tmp\n");
            fmt::print("  find . -name '*.txt' Find all .txt files\n");
            fmt::print("  find . -type d       Find all directories\n");
            fmt::print("  find . -maxdepth 2   Search up to 2 levels deep\n");
            return Status::ok();
        }
        else if (arg[0] != '-')
        {
            // First non-option argument is the start path
            options.start_path = arg;
            i++;
        }
        else
        {
            fmt::print(fg(fmt::color::red), "Error: Unknown option '{}'\n", arg);
            return Status::error("Unknown option");
        }
    }

    // Resolve the start path
    auto& vfs = VirtualFilesystem::getInstance();
    auto resolved = vfs.resolvePath(options.start_path);

    // Check if path exists
    std::error_code ec;
    if (!std::filesystem::exists(resolved.full_path, ec))
    {
        fmt::print(fg(fmt::color::red), "Error: Path '{}' does not exist\n", options.start_path);
        return Status::error("Path not found");
    }

    // Start recursive search
    findRecursive(resolved.full_path, options, 0, context);

    return Status::ok();
}

void FindCommand::findRecursive(const std::string& path, const FindOptions& options,
                                int current_depth, const CommandContext& context)
{
    std::error_code ec;

    // Check if path is a directory
    if (!std::filesystem::is_directory(path, ec))
    {
        // It's a file, check if it matches our criteria
        std::string filename = std::filesystem::path(path).filename().string();

        bool type_matches = (options.type == FileType::All || options.type == FileType::File);
        bool name_matches =
            options.name_pattern.empty() ||
            matchesPattern(filename, options.name_pattern, options.case_insensitive);

        if (type_matches && name_matches)
        {
            if (context.use_colors)
            {
                fmt::print("{}\n", path);
            }
            else
            {
                fmt::print("{}\n", path);
            }
        }
        return;
    }

    // Process directory itself first
    // current_depth is the depth of this directory from the starting point
    // Only print it if it's not the starting directory and within max depth
    if (current_depth > 0)
    {
        // Check if this directory exceeds max depth
        if (options.max_depth >= 0 && current_depth > options.max_depth)
        {
            return;
        }

        std::string dirname = std::filesystem::path(path).filename().string();
        bool type_matches = (options.type == FileType::All || options.type == FileType::Directory);
        bool name_matches = options.name_pattern.empty() ||
                            matchesPattern(dirname, options.name_pattern, options.case_insensitive);

        if (type_matches && name_matches)
        {
            if (context.use_colors)
            {
                fmt::print(fg(fmt::color::blue) | fmt::emphasis::bold, "{}\n", path);
            }
            else
            {
                fmt::print("{}\n", path);
            }
        }
    }

    // Only iterate through directory contents if we haven't exceeded max depth
    // Note: current_depth is the depth of the current directory
    // Files in this directory are at the same depth as the directory
    try
    {
        for (const auto& entry : std::filesystem::directory_iterator(path, ec))
        {
            if (ec)
            {
                continue; // Skip entries we can't read
            }

            std::string entry_path = entry.path().string();
            std::string filename = entry.path().filename().string();

            if (entry.is_directory(ec) && !ec)
            {
                // Subdirectories in this directory are at current_depth + 1
                // Always recurse (the recursive call will handle depth checking)
                findRecursive(entry_path, options, current_depth + 1, context);
            }
            else if (entry.is_regular_file(ec) && !ec)
            {
                // Files in this directory are at current_depth + 1 from the starting point
                // Only print them if we haven't exceeded max depth
                if (options.max_depth >= 0 && current_depth + 1 > options.max_depth)
                {
                    continue;
                }

                // Check if file matches criteria
                bool type_matches =
                    (options.type == FileType::All || options.type == FileType::File);
                bool name_matches =
                    options.name_pattern.empty() ||
                    matchesPattern(filename, options.name_pattern, options.case_insensitive);

                if (type_matches && name_matches)
                {
                    if (context.use_colors)
                    {
                        fmt::print("{}\n", entry_path);
                    }
                    else
                    {
                        fmt::print("{}\n", entry_path);
                    }
                }
            }
        }
    }
    catch (const std::filesystem::filesystem_error& e)
    {
        // Silently skip directories we can't read
    }
}

bool FindCommand::matchesPattern(const std::string& filename, const std::string& pattern,
                                 bool case_insensitive)
{
    std::string file = case_insensitive ? toLower(filename) : filename;
    std::string patt = case_insensitive ? toLower(pattern) : pattern;

    // Simple wildcard matching (* and ?)
    size_t f = 0, p = 0;
    size_t star_idx = std::string::npos;
    size_t match_idx = 0;

    while (f < file.length())
    {
        if (p < patt.length() && (patt[p] == '?' || patt[p] == file[f]))
        {
            f++;
            p++;
        }
        else if (p < patt.length() && patt[p] == '*')
        {
            star_idx = p;
            match_idx = f;
            p++;
        }
        else if (star_idx != std::string::npos)
        {
            p = star_idx + 1;
            match_idx++;
            f = match_idx;
        }
        else
        {
            return false;
        }
    }

    while (p < patt.length() && patt[p] == '*')
    {
        p++;
    }

    return p == patt.length();
}

std::string FindCommand::toLower(const std::string& str)
{
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return result;
}

} // namespace homeshell
