/**
 * @file DiffCommand.hpp
 * @brief Compare files line by line
 *
 * This command compares two files line by line and displays the differences,
 * similar to the Unix `diff` command. Supports unified format output.
 *
 * @author Homeshell Development Team
 * @date 2025
 */

#pragma once

#include <homeshell/Command.hpp>
#include <homeshell/Status.hpp>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace homeshell
{

class DiffCommand : public ICommand
{
public:
    std::string getName() const override
    {
        return "diff";
    }

    std::string getDescription() const override
    {
        return "Compare files line by line";
    }

    CommandType getType() const override
    {
        return CommandType::Synchronous;
    }

    Status execute(const CommandContext& context) override
    {
        if (context.args.empty() || context.args[0] == "--help" || context.args[0] == "-h")
        {
            showHelp();
            return Status::ok();
        }

        // Parse options
        bool unified = false;
        bool brief = false;
        bool ignore_case = false;
        int context_lines = 3;
        std::vector<std::string> files;

        for (size_t i = 0; i < context.args.size(); ++i)
        {
            const auto& arg = context.args[i];
            if (arg == "-u" || arg == "--unified")
            {
                unified = true;
            }
            else if (arg == "-q" || arg == "--brief")
            {
                brief = true;
            }
            else if (arg == "-i" || arg == "--ignore-case")
            {
                ignore_case = true;
            }
            else if (arg == "-U" && i + 1 < context.args.size())
            {
                context_lines = std::stoi(context.args[++i]);
                unified = true;
            }
            else if (arg[0] != '-')
            {
                files.push_back(arg);
            }
        }

        if (files.size() != 2)
        {
            std::cerr << "diff: need two files to compare\n";
            return Status::error("Invalid arguments");
        }

        // Read both files
        std::vector<std::string> lines1, lines2;
        if (!readFile(files[0], lines1))
        {
            std::cerr << "diff: " << files[0] << ": No such file or directory\n";
            return Status::error("File not found");
        }
        if (!readFile(files[1], lines2))
        {
            std::cerr << "diff: " << files[1] << ": No such file or directory\n";
            return Status::error("File not found");
        }

        // Compare files
        if (ignore_case)
        {
            normalizeCase(lines1);
            normalizeCase(lines2);
        }

        bool identical = (lines1 == lines2);

        if (brief)
        {
            if (!identical)
            {
                std::cout << "Files " << files[0] << " and " << files[1] << " differ\n";
                return Status::ok();
            }
            return Status::ok();
        }

        if (identical)
        {
            // Files are identical, no output
            return Status::ok();
        }

        // Generate and display diff
        if (unified)
        {
            displayUnifiedDiff(files[0], files[1], lines1, lines2, context_lines);
        }
        else
        {
            displayNormalDiff(files[0], files[1], lines1, lines2);
        }

        return Status::ok();
    }

private:
    void showHelp() const
    {
        std::cout << "Usage: diff [OPTION]... FILE1 FILE2\n\n"
                  << "Compare files line by line.\n\n"
                  << "Options:\n"
                  << "  -u, --unified           Output in unified format\n"
                  << "  -U NUM                  Output NUM lines of context (implies -u)\n"
                  << "  -q, --brief             Report only when files differ\n"
                  << "  -i, --ignore-case       Ignore case differences\n"
                  << "  --help                  Show this help message\n\n"
                  << "Examples:\n"
                  << "  diff file1.txt file2.txt\n"
                  << "  diff -u old.txt new.txt\n"
                  << "  diff -q file1 file2\n";
    }

    bool readFile(const std::string& filename, std::vector<std::string>& lines) const
    {
        std::ifstream file(filename);
        if (!file)
        {
            return false;
        }

        std::string line;
        while (std::getline(file, line))
        {
            lines.push_back(line);
        }
        return true;
    }

    void normalizeCase(std::vector<std::string>& lines) const
    {
        for (auto& line : lines)
        {
            std::transform(line.begin(), line.end(), line.begin(), ::tolower);
        }
    }

    void displayNormalDiff(const std::string& file1, const std::string& file2,
                           const std::vector<std::string>& lines1,
                           const std::vector<std::string>& lines2) const
    {
        // Simple line-by-line comparison
        size_t max_lines = std::max(lines1.size(), lines2.size());

        for (size_t i = 0; i < max_lines; ++i)
        {
            if (i < lines1.size() && i < lines2.size())
            {
                if (lines1[i] != lines2[i])
                {
                    std::cout << (i + 1) << "c" << (i + 1) << "\n";
                    std::cout << "< " << lines1[i] << "\n";
                    std::cout << "---\n";
                    std::cout << "> " << lines2[i] << "\n";
                }
            }
            else if (i < lines1.size())
            {
                std::cout << (i + 1) << "d" << i << "\n";
                std::cout << "< " << lines1[i] << "\n";
            }
            else
            {
                std::cout << i << "a" << (i + 1) << "\n";
                std::cout << "> " << lines2[i] << "\n";
            }
        }
    }

    void displayUnifiedDiff(const std::string& file1, const std::string& file2,
                            const std::vector<std::string>& lines1,
                            const std::vector<std::string>& lines2, int context) const
    {
        std::cout << "--- " << file1 << "\n";
        std::cout << "+++ " << file2 << "\n";

        // Find differences
        size_t i = 0, j = 0;
        while (i < lines1.size() || j < lines2.size())
        {
            if (i < lines1.size() && j < lines2.size() && lines1[i] == lines2[j])
            {
                // Lines match, move forward
                ++i;
                ++j;
            }
            else
            {
                // Found a difference, show context
                size_t start_i = (i > (size_t)context) ? i - context : 0;
                size_t start_j = (j > (size_t)context) ? j - context : 0;

                // Find end of difference block
                size_t end_i = i;
                size_t end_j = j;
                while ((end_i < lines1.size() || end_j < lines2.size()) &&
                       (end_i >= lines1.size() || end_j >= lines2.size() ||
                        lines1[end_i] != lines2[end_j]))
                {
                    if (end_i < lines1.size())
                        ++end_i;
                    if (end_j < lines2.size())
                        ++end_j;
                }

                // Add context after
                end_i = std::min(end_i + context, lines1.size());
                end_j = std::min(end_j + context, lines2.size());

                // Print hunk header
                std::cout << "@@ -" << (start_i + 1) << "," << (end_i - start_i) << " +"
                          << (start_j + 1) << "," << (end_j - start_j) << " @@\n";

                // Print context before
                for (size_t k = start_i; k < i; ++k)
                {
                    std::cout << " " << lines1[k] << "\n";
                }

                // Print differences
                while (i < end_i && i < lines1.size())
                {
                    std::cout << "-" << lines1[i] << "\n";
                    ++i;
                }
                while (j < end_j && j < lines2.size())
                {
                    std::cout << "+" << lines2[j] << "\n";
                    ++j;
                }
            }
        }
    }
};

} // namespace homeshell
