#pragma once

#include <cstdlib>
#include <string>
#include <unistd.h>

namespace homeshell
{

enum class ColorSupport
{
    None,
    Basic8,      // 8 colors
    Extended16,  // 16 colors
    Colors256,   // 256 colors
    TrueColor    // 24-bit RGB
};

class TerminalInfo
{
public:
    static TerminalInfo detect()
    {
        TerminalInfo info;

        // Check if we're in a TTY
        info.is_tty_ = isatty(STDOUT_FILENO);

        if (!info.is_tty_)
        {
            return info;
        }

        // Detect color support
        const char* term = std::getenv("TERM");
        const char* colorterm = std::getenv("COLORTERM");

        if (colorterm != nullptr)
        {
            std::string colorterm_str(colorterm);
            if (colorterm_str.find("truecolor") != std::string::npos ||
                colorterm_str.find("24bit") != std::string::npos)
            {
                info.color_support_ = ColorSupport::TrueColor;
            }
        }

        if (info.color_support_ == ColorSupport::None && term != nullptr)
        {
            std::string term_str(term);
            if (term_str.find("256color") != std::string::npos)
            {
                info.color_support_ = ColorSupport::Colors256;
            }
            else if (term_str.find("color") != std::string::npos ||
                     term_str.find("xterm") != std::string::npos ||
                     term_str.find("screen") != std::string::npos ||
                     term_str.find("linux") != std::string::npos)
            {
                info.color_support_ = ColorSupport::Extended16;
            }
        }

        // Detect UTF-8/emoji support
        const char* lang = std::getenv("LANG");
        const char* lc_all = std::getenv("LC_ALL");

        std::string locale;
        if (lc_all != nullptr)
        {
            locale = lc_all;
        }
        else if (lang != nullptr)
        {
            locale = lang;
        }

        if (locale.find("UTF-8") != std::string::npos ||
            locale.find("utf8") != std::string::npos)
        {
            info.utf8_support_ = true;
            info.emoji_support_ = true;  // Assume emoji if UTF-8
        }

        return info;
    }

    bool isTTY() const
    {
        return is_tty_;
    }

    ColorSupport getColorSupport() const
    {
        return color_support_;
    }

    bool hasColorSupport() const
    {
        return color_support_ != ColorSupport::None;
    }

    bool hasUTF8Support() const
    {
        return utf8_support_;
    }

    bool hasEmojiSupport() const
    {
        return emoji_support_;
    }

private:
    TerminalInfo() = default;

    bool is_tty_ = false;
    ColorSupport color_support_ = ColorSupport::None;
    bool utf8_support_ = false;
    bool emoji_support_ = false;
};

} // namespace homeshell

