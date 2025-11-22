#pragma once

#include <homeshell/Command.hpp>
#include <homeshell/Status.hpp>
#include <homeshell/VirtualFilesystem.hpp>

#include <fmt/color.h>
#include <fmt/core.h>

#include <fstream>
#include <string>
#include <vector>

#ifdef __unix__
#include <ncurses.h>
#endif

namespace homeshell
{

/**
 * @brief Full-screen text editor command
 *
 * Provides a nano-style text editor with full-screen terminal interface using ncurses.
 * Supports editing both regular files and files within encrypted virtual mounts.
 *
 * @details Features:
 *          - Full-screen interface with title bar and status bar
 *          - Arrow key navigation and Page Up/Down
 *          - Text insertion and deletion
 *          - Line operations (cut/paste)
 *          - Save confirmation for unsaved changes
 *
 *          Keyboard shortcuts:
 *          - Ctrl+X: Exit (prompts to save if modified)
 *          - Ctrl+O: Save file
 *          - Ctrl+K: Cut current line to clipboard
 *          - Ctrl+U: Paste line from clipboard
 *          - Ctrl+A: Jump to line start
 *          - Ctrl+E: Jump to line end
 *          - Arrow keys: Navigate cursor
 *          - Page Up/Down: Scroll by screen
 *          - Backspace: Delete character before cursor
 *          - Enter: Insert new line
 *
 *          Interface layout:
 *          ```
 *          ┌─────────────────────────────────┐
 *          │ Homeshell Editor               │  <- Title bar
 *          ├─────────────────────────────────┤
 *          │ File content here...           │
 *          │                                 │  <- Text area
 *          │                                 │
 *          ├─────────────────────────────────┤
 *          │ filename.txt [Modified]         │  <- Status bar
 *          │ ^X Exit  ^O Save  ^K Cut  ^U... │  <- Help line
 *          └─────────────────────────────────┘
 *          ```
 *
 *          The editor works transparently with both regular files and
 *          encrypted virtual filesystem files.
 *
 * @note Only available on Unix-like systems (requires ncurses).
 *       On other platforms, this command will report an error.
 *
 * Example: edit /secure/notes.txt
 */
class EditCommand : public ICommand
{
public:
    std::string getName() const override
    {
        return "edit";
    }

    std::string getDescription() const override
    {
        return "Edit a file (nano-style editor)";
    }

    CommandType getType() const override
    {
        return CommandType::Synchronous;
    }

    /**
     * @brief Execute the edit command
     * @param context Command context with filename argument
     * @return Status indicating success/failure
     */
    Status execute(const CommandContext& context) override
    {
#ifdef __unix__
        if (context.args.empty())
        {
            fmt::print(fg(fmt::color::red), "Error: No file specified\n");
            fmt::print("Usage: edit <filename>\n");
            return Status::error("No file specified");
        }

        std::string filename = context.args[0];

        // Load file content
        std::vector<std::string> lines;
        if (!loadFile(filename, lines))
        {
            // File doesn't exist, start with empty buffer
            lines.push_back("");
        }

        // Initialize ncurses
        initscr();
        raw();
        keypad(stdscr, TRUE);
        noecho();

        bool running = true;
        bool modified = false;
        int cursor_x = 0;
        int cursor_y = 0;
        int scroll_offset = 0;
        std::string clipboard; // For cut/paste operations

        while (running)
        {
            // Get screen dimensions
            int max_y, max_x;
            getmaxyx(stdscr, max_y, max_x);
            int text_area_height = max_y - 4; // Leave room for title, status and help

            // Clear screen
            clear();

            // Draw title bar
            attron(A_REVERSE);
            mvprintw(0, 0, " Homeshell Editor");
            for (int i = 17; i < max_x; ++i)
            {
                mvaddch(0, i, ' ');
            }
            attroff(A_REVERSE);

            // Draw file contents (starting at line 1)
            for (int i = 0;
                 i < text_area_height && (i + scroll_offset) < static_cast<int>(lines.size()); ++i)
            {
                int line_idx = i + scroll_offset;
                mvprintw(i + 1, 0, "%s", lines[line_idx].c_str());
            }

            // Draw status bar
            attron(A_REVERSE);
            mvprintw(max_y - 2, 0, " %s %s ", filename.c_str(), modified ? "[Modified]" : "");
            for (int i = filename.length() + (modified ? 20 : 10); i < max_x; ++i)
            {
                mvaddch(max_y - 2, i, ' ');
            }
            attroff(A_REVERSE);

            // Draw help line
            mvprintw(max_y - 1, 0, "^A/^E Line Start/End  ^X Exit  ^O Save  ^K Cut  ^U Paste");

            // Position cursor (adjust for title bar at line 0)
            move(cursor_y - scroll_offset + 1, cursor_x);
            refresh();

            // Handle input
            int ch = getch();

            switch (ch)
            {
            case 24: // Ctrl-X: Exit
                if (modified)
                {
                    // Ask to save
                    mvprintw(max_y - 1, 0, "Save modified buffer? (Y/N)");
                    clrtoeol();
                    refresh();
                    int confirm = getch();
                    if (confirm == 'y' || confirm == 'Y')
                    {
                        saveFile(filename, lines);
                    }
                }
                running = false;
                break;

            case 15: // Ctrl-O: Save
                if (saveFile(filename, lines))
                {
                    modified = false;
                    mvprintw(max_y - 1, 0, "[ Wrote %zu lines ]", lines.size());
                    clrtoeol();
                    refresh();
                    napms(1000); // Brief pause to show message
                }
                break;

            case 11: // Ctrl-K: Cut line
                clipboard = lines[cursor_y];
                lines.erase(lines.begin() + cursor_y);
                if (lines.empty())
                {
                    lines.push_back("");
                }
                if (cursor_y >= static_cast<int>(lines.size()))
                {
                    cursor_y = lines.size() - 1;
                }
                cursor_x = std::min(cursor_x, static_cast<int>(lines[cursor_y].length()));
                modified = true;
                break;

            case 21: // Ctrl-U: Paste line
                if (!clipboard.empty())
                {
                    lines.insert(lines.begin() + cursor_y, clipboard);
                    modified = true;
                }
                break;

            case 1: // Ctrl-A: Go to start of line
                cursor_x = 0;
                break;

            case 5: // Ctrl-E: Go to end of line
                cursor_x = lines[cursor_y].length();
                break;

            case KEY_UP:
                if (cursor_y > 0)
                {
                    cursor_y--;
                    cursor_x = std::min(cursor_x, static_cast<int>(lines[cursor_y].length()));
                    if (cursor_y < scroll_offset)
                    {
                        scroll_offset = cursor_y;
                    }
                }
                break;

            case KEY_DOWN:
                if (cursor_y < static_cast<int>(lines.size()) - 1)
                {
                    cursor_y++;
                    cursor_x = std::min(cursor_x, static_cast<int>(lines[cursor_y].length()));
                    if (cursor_y >= scroll_offset + text_area_height)
                    {
                        scroll_offset = cursor_y - text_area_height + 1;
                    }
                }
                break;

            case KEY_LEFT:
                if (cursor_x > 0)
                {
                    cursor_x--;
                }
                else if (cursor_y > 0)
                {
                    // Move to end of previous line
                    cursor_y--;
                    cursor_x = lines[cursor_y].length();
                    if (cursor_y < scroll_offset)
                    {
                        scroll_offset = cursor_y;
                    }
                }
                break;

            case KEY_RIGHT:
                if (cursor_x < static_cast<int>(lines[cursor_y].length()))
                {
                    cursor_x++;
                }
                else if (cursor_y < static_cast<int>(lines.size()) - 1)
                {
                    // Move to beginning of next line
                    cursor_y++;
                    cursor_x = 0;
                    if (cursor_y >= scroll_offset + text_area_height)
                    {
                        scroll_offset = cursor_y - text_area_height + 1;
                    }
                }
                break;

            case KEY_HOME:
                cursor_x = 0;
                break;

            case KEY_END:
                cursor_x = lines[cursor_y].length();
                break;

            case KEY_BACKSPACE:
            case 127: // Backspace
            case 8:
                if (cursor_x > 0)
                {
                    lines[cursor_y].erase(cursor_x - 1, 1);
                    cursor_x--;
                    modified = true;
                }
                else if (cursor_y > 0)
                {
                    // Join with previous line
                    cursor_x = lines[cursor_y - 1].length();
                    lines[cursor_y - 1] += lines[cursor_y];
                    lines.erase(lines.begin() + cursor_y);
                    cursor_y--;
                    if (cursor_y < scroll_offset)
                    {
                        scroll_offset = cursor_y;
                    }
                    modified = true;
                }
                break;

            case KEY_DC: // Delete
                if (cursor_x < static_cast<int>(lines[cursor_y].length()))
                {
                    lines[cursor_y].erase(cursor_x, 1);
                    modified = true;
                }
                else if (cursor_y < static_cast<int>(lines.size()) - 1)
                {
                    // Join with next line
                    lines[cursor_y] += lines[cursor_y + 1];
                    lines.erase(lines.begin() + cursor_y + 1);
                    modified = true;
                }
                break;

            case 10: // Enter
            case KEY_ENTER:
            {
                std::string remaining = lines[cursor_y].substr(cursor_x);
                lines[cursor_y] = lines[cursor_y].substr(0, cursor_x);
                lines.insert(lines.begin() + cursor_y + 1, remaining);
                cursor_y++;
                cursor_x = 0;
                if (cursor_y >= scroll_offset + text_area_height)
                {
                    scroll_offset = cursor_y - text_area_height + 1;
                }
                modified = true;
            }
            break;

            default:
                // Regular character input
                if (ch >= 32 && ch < 127)
                {
                    lines[cursor_y].insert(cursor_x, 1, static_cast<char>(ch));
                    cursor_x++;
                    modified = true;
                }
                break;
            }

            // Ensure we always have at least one line
            if (lines.empty())
            {
                lines.push_back("");
            }
        }

        // Cleanup ncurses
        endwin();

        return Status::ok();
#else
        fmt::print(fg(fmt::color::red), "Error: Edit command is only supported on Unix systems\n");
        return Status::error("Unsupported platform");
#endif
    }

private:
    bool loadFile(const std::string& filename, std::vector<std::string>& lines)
    {
        auto& vfs = VirtualFilesystem::getInstance();

        // Try to read from VFS first
        if (vfs.isVirtualPath(filename))
        {
            std::string content;
            if (vfs.readFile(filename, content))
            {
                // Split into lines
                std::istringstream iss(content);
                std::string line;
                while (std::getline(iss, line))
                {
                    lines.push_back(line);
                }
                return true;
            }
            return false;
        }

        // Read from regular filesystem
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

    bool saveFile(const std::string& filename, const std::vector<std::string>& lines)
    {
        auto& vfs = VirtualFilesystem::getInstance();

        // Join lines into content
        std::string content;
        for (size_t i = 0; i < lines.size(); ++i)
        {
            content += lines[i];
            if (i < lines.size() - 1)
            {
                content += '\n';
            }
        }

        // Try to write to VFS first
        if (vfs.isVirtualPath(filename))
        {
            return vfs.writeFile(filename, content);
        }

        // Write to regular filesystem
        std::ofstream file(filename);
        if (!file)
        {
            return false;
        }

        file << content;
        return file.good();
    }
};

} // namespace homeshell
