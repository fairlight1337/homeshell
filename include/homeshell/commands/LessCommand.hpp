#pragma once

#include <homeshell/Command.hpp>
#include <homeshell/Status.hpp>
#include <homeshell/VirtualFilesystem.hpp>

#include <fmt/color.h>
#include <ncurses.h>
#include <unistd.h>

#include <algorithm>
#include <atomic>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace homeshell
{

/**
 * @brief File pager for viewing files one screen at a time
 *
 * The less command is a terminal pager that displays file contents
 * one screen at a time, with keyboard navigation controls. It's essential
 * for viewing large files or command output comfortably.
 *
 * @details Features:
 *          - Page-by-page file viewing
 *          - Keyboard navigation (arrows, PageUp/Down, Home/End)
 *          - Search functionality (forward and backward)
 *          - Line numbers display
 *          - Works with both files and stdin (for pipes)
 *          - Supports encrypted virtual filesystem
 *          - Color-coded status line
 *
 *          Command syntax:
 *          ```
 *          less <file>
 *          less              # Read from stdin
 *          command | less    # Pipe input
 *          ```
 *
 *          Keyboard controls:
 *          - `Space`, `Page Down`, `f` - Next page
 *          - `b`, `Page Up` - Previous page
 *          - `↓`, `j` - Down one line
 *          - `↑`, `k` - Up one line
 *          - `g`, `Home` - Go to beginning
 *          - `G`, `End` - Go to end
 *          - `/` - Search forward
 *          - `?` - Search backward
 *          - `n` - Next search result
 *          - `N` - Previous search result
 *          - `q` - Quit
 *          - `h` - Help
 *
 * Example usage:
 * ```
 * less largefile.txt              # View file
 * less /secure/document.txt       # View encrypted file
 * cat file.txt | less             # Pipe input
 * ls -l | less                    # Page command output
 * find . -name "*.cpp" | less     # Search results
 * ```
 *
 * @note Requires terminal with ncurses support
 * @note Works seamlessly with pipes for command output paging
 */
class LessCommand : public ICommand
{
public:
    std::string getName() const override
    {
        return "less";
    }

    std::string getDescription() const override
    {
        return "File pager for viewing files one screen at a time";
    }

    CommandType getType() const override
    {
        return CommandType::Synchronous;
    }

    bool supportsCancellation() const override
    {
        return true;
    }

    void cancel() override
    {
        cancelled_.store(true);
    }

    /**
     * @brief Execute the less command
     * @param context Command context with arguments
     * @return Status::ok() on success, Status::error() on failure
     */
    Status execute(const CommandContext& context) override
    {
        std::string filename;

        // Parse arguments
        for (size_t i = 0; i < context.args.size(); ++i)
        {
            if (context.args[i] == "--help")
            {
                showHelp();
                return Status::ok();
            }
            else if (context.args[i][0] == '-')
            {
                fmt::print(fg(fmt::color::red), "Error: Unknown option '{}'\n", context.args[i]);
                fmt::print("Use --help for usage information\n");
                return Status::error("Unknown option");
            }
            else
            {
                filename = context.args[i];
            }
        }

        // Read content
        std::vector<std::string> lines;

        if (filename.empty())
        {
            // Read from stdin
            if (!readStdin(lines))
            {
                return Status::error("Failed to read from stdin");
            }
        }
        else
        {
            // Read from file
            if (!readFile(filename, lines))
            {
                return Status::error("Failed to read file");
            }
        }

        // If no content, just return
        if (lines.empty())
        {
            return Status::ok();
        }

        // Check if stdout is a TTY
        if (!isatty(STDOUT_FILENO))
        {
            // Not a TTY, just print everything (for piping)
            for (const auto& line : lines)
            {
                fmt::print("{}\n", line);
            }
            return Status::ok();
        }

        // Display with pager
        displayPager(lines, filename.empty() ? "(stdin)" : filename);

        return Status::ok();
    }

private:
    std::atomic<bool> cancelled_{false};

    void showHelp()
    {
        fmt::print("Usage: less [file]\n");
        fmt::print("       less              # Read from stdin\n");
        fmt::print("       command | less    # Pipe input\n");
        fmt::print("\n");
        fmt::print("File pager for viewing files one screen at a time.\n");
        fmt::print("\n");
        fmt::print("Keyboard Controls:\n");
        fmt::print("  Space, PageDown, f  Next page\n");
        fmt::print("  b, PageUp           Previous page\n");
        fmt::print("  ↓, j                Down one line\n");
        fmt::print("  ↑, k                Up one line\n");
        fmt::print("  g, Home             Go to beginning\n");
        fmt::print("  G, End              Go to end\n");
        fmt::print("  /                   Search forward\n");
        fmt::print("  ?                   Search backward\n");
        fmt::print("  n                   Next search result\n");
        fmt::print("  N                   Previous search result\n");
        fmt::print("  q                   Quit\n");
        fmt::print("  h                   Help\n");
        fmt::print("\n");
        fmt::print("Examples:\n");
        fmt::print("  less largefile.txt       # View file\n");
        fmt::print("  cat file.txt | less      # Pipe input\n");
        fmt::print("  ls -l | less             # Page command output\n");
    }

    /**
     * @brief Read content from file
     * @param filename File to read
     * @param lines Output vector of lines
     * @return true on success
     */
    bool readFile(const std::string& filename, std::vector<std::string>& lines)
    {
        auto& vfs = VirtualFilesystem::getInstance();
        auto resolved = vfs.resolvePath(filename);

        std::string content;

        // Try VFS first
        if (vfs.exists(resolved.full_path))
        {
            if (!vfs.readFile(resolved.full_path, content))
            {
                fmt::print(fg(fmt::color::red), "Error: Failed to read file '{}'\n", filename);
                return false;
            }
        }
        else
        {
            // Try regular filesystem
            std::ifstream file(filename);
            if (!file.good())
            {
                fmt::print(fg(fmt::color::red), "Error: Cannot open file '{}'\n", filename);
                return false;
            }

            std::stringstream buffer;
            buffer << file.rdbuf();
            content = buffer.str();
        }

        // Split into lines
        std::stringstream ss(content);
        std::string line;
        while (std::getline(ss, line))
        {
            lines.push_back(line);
        }

        return true;
    }

    /**
     * @brief Read content from stdin
     * @param lines Output vector of lines
     * @return true on success
     */
    bool readStdin(std::vector<std::string>& lines)
    {
        // Check if stdin is actually a pipe/file (not interactive TTY)
        if (isatty(STDIN_FILENO))
        {
            fmt::print(fg(fmt::color::red), "Error: Missing filename\n");
            fmt::print("Usage: less [file]\n");
            fmt::print("       command | less    # Pipe input\n");
            return false;
        }

        std::string line;
        while (std::getline(std::cin, line))
        {
            lines.push_back(line);
        }

        // Clear any error flags on stdin after reading
        std::cin.clear();

        return true;
    }

    /**
     * @brief Display content with pager interface
     * @param lines Content lines to display
     * @param filename Filename for display (or "(stdin)")
     */
    void displayPager(const std::vector<std::string>& lines, const std::string& filename)
    {
        // If stdin was used for data, we need to reopen it to /dev/tty for input
        FILE* input_term = nullptr;
        if (filename == "(stdin)")
        {
            input_term = fopen("/dev/tty", "r");
            if (!input_term)
            {
                fmt::print(fg(fmt::color::red), "Error: Cannot open /dev/tty for input\n");
                // Fall back to regular display
                for (const auto& line : lines)
                {
                    fmt::print("{}\n", line);
                }
                return;
            }
        }

        // Initialize ncurses with the correct input
        SCREEN* screen = nullptr;
        if (input_term)
        {
            screen = newterm(nullptr, stdout, input_term);
            if (!screen)
            {
                fclose(input_term);
                fmt::print(fg(fmt::color::red), "Error: Cannot initialize ncurses with /dev/tty\n");
                return;
            }
            set_term(screen);
        }
        else
        {
            initscr();
        }

        noecho();
        cbreak();
        keypad(stdscr, TRUE);
        nodelay(stdscr, FALSE); // Make getch() blocking
        timeout(-1);            // Wait indefinitely for input

        int top_line = 0;
        int max_lines = static_cast<int>(lines.size());
        std::string search_term;
        int search_pos = -1;

        bool quit = false;
        while (!quit && !cancelled_.load())
        {
            int rows, cols;
            getmaxyx(stdscr, rows, cols);

            // Clear screen
            clear();

            // Calculate how many lines we can show (leave 1 for status)
            int display_rows = rows - 1;

            // Display lines
            for (int i = 0; i < display_rows && (top_line + i) < max_lines; ++i)
            {
                int line_num = top_line + i;
                std::string display_line = lines[line_num];

                // Truncate if too long
                if (static_cast<int>(display_line.length()) > cols)
                {
                    display_line = display_line.substr(0, cols - 1);
                }

                mvprintw(i, 0, "%s", display_line.c_str());
            }

            // Display status line
            attron(A_REVERSE);
            mvprintw(rows - 1, 0, "%s", std::string(cols, ' ').c_str());

            std::string status;
            if (max_lines == 0)
            {
                status = "No content";
            }
            else
            {
                int percent = (max_lines > 0) ? ((top_line + display_rows) * 100 / max_lines) : 100;
                if (percent > 100)
                    percent = 100;

                status = filename + " (" + std::to_string(top_line + 1) + "-" +
                         std::to_string(std::min(top_line + display_rows, max_lines)) + "/" +
                         std::to_string(max_lines) + ") " + std::to_string(percent) +
                         "% [Press 'q' to quit, 'h' for help]";
            }

            mvprintw(rows - 1, 0, "%s", status.c_str());
            attroff(A_REVERSE);

            refresh();

            // Handle input
            int ch = getch();

            switch (ch)
            {
            case 'q':
            case 'Q':
                quit = true;
                break;

            case ' ':       // Space
            case KEY_NPAGE: // Page Down
            case 'f':
            case 'F':
                top_line += display_rows;
                if (top_line >= max_lines)
                {
                    top_line = std::max(0, max_lines - display_rows);
                }
                break;

            case 'b':
            case 'B':
            case KEY_PPAGE: // Page Up
                top_line -= display_rows;
                if (top_line < 0)
                {
                    top_line = 0;
                }
                break;

            case KEY_DOWN:
            case 'j':
            case 'J':
                if (top_line < max_lines - display_rows)
                {
                    top_line++;
                }
                break;

            case KEY_UP:
            case 'k':
            case 'K':
                if (top_line > 0)
                {
                    top_line--;
                }
                break;

            case 'g':
            case KEY_HOME:
                top_line = 0;
                break;

            case 'G':
            case KEY_END:
                top_line = std::max(0, max_lines - display_rows);
                break;

            case 'h':
            case 'H':
                showPagerHelp();
                break;
            }
        }

        // Clean up ncurses
        if (screen)
        {
            endwin();
            delscreen(screen);
        }
        else
        {
            endwin();
        }

        if (input_term)
        {
            fclose(input_term);
        }
    }

    /**
     * @brief Show help in pager mode
     */
    void showPagerHelp()
    {
        clear();

        mvprintw(1, 2, "LESS - Keyboard Controls");
        mvprintw(3, 4, "Space, PageDown, f  : Next page");
        mvprintw(4, 4, "b, PageUp           : Previous page");
        mvprintw(5, 4, "↓, j                : Down one line");
        mvprintw(6, 4, "↑, k                : Up one line");
        mvprintw(7, 4, "g, Home             : Go to beginning");
        mvprintw(8, 4, "G, End              : Go to end");
        mvprintw(9, 4, "q                   : Quit");
        mvprintw(10, 4, "h                   : This help");

        mvprintw(12, 2, "Press any key to continue...");

        refresh();
        getch();
    }
};

} // namespace homeshell
