/**
 * @file WcCommand.hpp
 * @brief Count lines, words, and bytes in files
 *
 * This command counts lines, words, characters, and bytes in files or standard input.
 *
 * @author Homeshell Development Team
 * @date 2025
 */

#pragma once

#include <homeshell/Command.hpp>
#include <homeshell/Status.hpp>
#include <homeshell/VirtualFilesystem.hpp>

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace homeshell
{

/**
 * @class WcCommand
 * @brief Command to count lines, words, and bytes
 *
 * Counts lines, words, characters, and bytes in files.
 *
 * @section Usage
 * @code
 * wc file.txt           # Show lines, words, bytes
 * wc -l file.txt        # Show only lines
 * wc -w file.txt        # Show only words
 * wc -c file.txt        # Show only bytes
 * cat file | wc         # Read from stdin
 * wc --help             # Show help message
 * @endcode
 */
class WcCommand : public ICommand
{
public:
    /**
     * @brief Get command name
     * @return Command name "wc"
     */
    std::string getName() const override
    {
        return "wc";
    }

    /**
     * @brief Get command description
     * @return Brief description of the command
     */
    std::string getDescription() const override
    {
        return "Print newline, word, and byte counts for files";
    }

    /**
     * @brief Get command type
     * @return CommandType::Synchronous
     */
    CommandType getType() const override
    {
        return CommandType::Synchronous;
    }

    /**
     * @brief Execute the wc command
     * @param context Command context with arguments
     * @return Status indicating success or failure
     */
    Status execute(const CommandContext& context) override
    {
        bool count_lines = false;
        bool count_words = false;
        bool count_chars = false;
        bool count_bytes = false;
        std::vector<std::string> files;

        // Parse arguments
        for (size_t i = 0; i < context.args.size(); ++i)
        {
            const std::string& arg = context.args[i];

            if (arg == "--help")
            {
                showHelp();
                return Status::ok();
            }
            else if (arg[0] == '-' && arg.length() > 1 && arg[1] != '-')
            {
                // Short options
                for (size_t j = 1; j < arg.length(); ++j)
                {
                    if (arg[j] == 'l')
                        count_lines = true;
                    else if (arg[j] == 'w')
                        count_words = true;
                    else if (arg[j] == 'c')
                        count_bytes = true;
                    else if (arg[j] == 'm')
                        count_chars = true;
                    else
                    {
                        return Status::error("Unknown option: -" + std::string(1, arg[j]) +
                                             "\nUse --help for usage information");
                    }
                }
            }
            else if (arg == "--lines")
            {
                count_lines = true;
            }
            else if (arg == "--words")
            {
                count_words = true;
            }
            else if (arg == "--bytes")
            {
                count_bytes = true;
            }
            else if (arg == "--chars")
            {
                count_chars = true;
            }
            else if (arg[0] == '-')
            {
                return Status::error("Unknown option: " + arg +
                                     "\nUse --help for usage information");
            }
            else
            {
                files.push_back(arg);
            }
        }

        // If no specific counts requested, show all
        if (!count_lines && !count_words && !count_chars && !count_bytes)
        {
            count_lines = true;
            count_words = true;
            count_bytes = true;
        }

        // If no files specified, read from stdin
        if (files.empty())
        {
            auto counts = countFromStdin();
            printCounts(counts, "", count_lines, count_words, count_bytes, count_chars);
            return Status::ok();
        }

        // Process each file
        Counts total = {0, 0, 0, 0};
        for (const auto& file : files)
        {
            auto counts = countFromFile(file);
            if (counts.lines == -1)
            {
                std::cerr << "wc: " << file << ": No such file or directory\n";
                continue;
            }

            printCounts(counts, file, count_lines, count_words, count_bytes, count_chars);

            total.lines += counts.lines;
            total.words += counts.words;
            total.bytes += counts.bytes;
            total.chars += counts.chars;
        }

        // Print total if multiple files
        if (files.size() > 1)
        {
            printCounts(total, "total", count_lines, count_words, count_bytes, count_chars);
        }

        return Status::ok();
    }

private:
    /**
     * @brief Counts structure
     */
    struct Counts
    {
        int64_t lines;
        int64_t words;
        int64_t bytes;
        int64_t chars;
    };

    /**
     * @brief Display help information
     */
    void showHelp() const
    {
        std::cout << "Usage: wc [OPTION]... [FILE]...\n\n";
        std::cout << "Print newline, word, and byte counts for each FILE.\n";
        std::cout << "With no FILE, or when FILE is -, read standard input.\n\n";
        std::cout << "Options:\n";
        std::cout << "  -c, --bytes    Print the byte counts\n";
        std::cout << "  -m, --chars    Print the character counts\n";
        std::cout << "  -l, --lines    Print the newline counts\n";
        std::cout << "  -w, --words    Print the word counts\n";
        std::cout << "  --help         Show this help message\n";
    }

    /**
     * @brief Count lines, words, and bytes in content
     * @param content Content to count
     * @return Counts structure
     */
    Counts countContent(const std::string& content) const
    {
        Counts counts = {0, 0, 0, 0};

        counts.bytes = content.length();
        counts.chars = content.length(); // Simplified (doesn't handle UTF-8)

        bool in_word = false;
        for (char c : content)
        {
            if (c == '\n')
            {
                counts.lines++;
            }

            if (std::isspace(c))
            {
                in_word = false;
            }
            else
            {
                if (!in_word)
                {
                    counts.words++;
                    in_word = true;
                }
            }
        }

        return counts;
    }

    /**
     * @brief Count from stdin
     * @return Counts structure
     */
    Counts countFromStdin() const
    {
        std::string content;
        std::string line;
        while (std::getline(std::cin, line))
        {
            content += line + "\n";
        }
        return countContent(content);
    }

    /**
     * @brief Count from file
     * @param path File path
     * @return Counts structure (lines=-1 on error)
     */
    Counts countFromFile(const std::string& path) const
    {
        auto& vfs = VirtualFilesystem::getInstance();

        if (!vfs.exists(path))
        {
            return {-1, 0, 0, 0};
        }

        if (vfs.isDirectory(path))
        {
            return {-1, 0, 0, 0};
        }

        std::string content;
        if (!vfs.readFile(path, content))
        {
            return {-1, 0, 0, 0};
        }

        return countContent(content);
    }

    /**
     * @brief Print counts
     * @param counts Counts to print
     * @param filename Filename to print
     * @param show_lines Show line count
     * @param show_words Show word count
     * @param show_bytes Show byte count
     * @param show_chars Show char count
     */
    void printCounts(const Counts& counts, const std::string& filename, bool show_lines,
                     bool show_words, bool show_bytes, bool show_chars) const
    {
        if (show_lines)
            std::cout << " " << std::setw(7) << counts.lines;
        if (show_words)
            std::cout << " " << std::setw(7) << counts.words;
        if (show_chars)
            std::cout << " " << std::setw(7) << counts.chars;
        if (show_bytes)
            std::cout << " " << std::setw(7) << counts.bytes;

        if (!filename.empty())
            std::cout << " " << filename;
        std::cout << "\n";
    }
};

} // namespace homeshell
