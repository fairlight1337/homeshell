#pragma once

#include <unistd.h>

#include <cstdlib>
#include <string>

namespace homeshell
{

/**
 * @brief Terminal color capability levels
 *
 * Defines the various levels of color support that a terminal may provide.
 * Used to adapt output formatting to terminal capabilities.
 */
enum class ColorSupport
{
    None,       ///< No color support (monochrome)
    Basic8,     ///< Basic 8 ANSI colors
    Extended16, ///< Extended 16 ANSI colors
    Colors256,  ///< 256-color palette
    TrueColor   ///< 24-bit RGB true color (16.7M colors)
};

/**
 * @brief Terminal capability detection
 *
 * Detects and reports various terminal capabilities including color support,
 * UTF-8/emoji support, and TTY status. Used to adapt shell output to the
 * terminal's capabilities.
 *
 * @details Detection process:
 *          - TTY: Checks if stdout is a terminal (vs. pipe/redirect)
 *          - Colors: Examines $COLORTERM and $TERM environment variables
 *          - UTF-8: Checks $LANG and $LC_ALL for UTF-8 locales
 *          - Emoji: Assumes support if UTF-8 is available
 *
 *          This class is not instantiable directly; use detect() static method.
 *
 * Example usage:
 * ```cpp
 * TerminalInfo info = TerminalInfo::detect();
 * if (info.hasColorSupport()) {
 *     // Use colored output
 * }
 * if (info.hasEmojiSupport()) {
 *     // Use emoji in messages
 * }
 * ```
 */
class TerminalInfo
{
public:
    /**
     * @brief Detect current terminal capabilities
     * @return TerminalInfo struct with detected capabilities
     *
     * @details Performs detection by checking:
     *          - isatty() for TTY status
     *          - COLORTERM env var for true color
     *          - TERM env var for color support level
     *          - LANG/LC_ALL for UTF-8 encoding
     */
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

        if (locale.find("UTF-8") != std::string::npos || locale.find("utf8") != std::string::npos)
        {
            info.utf8_support_ = true;
            info.emoji_support_ = true; // Assume emoji if UTF-8
        }

        return info;
    }

    /**
     * @brief Check if running in a TTY
     * @return true if stdout is a terminal, false if piped/redirected
     */
    bool isTTY() const
    {
        return is_tty_;
    }

    /**
     * @brief Get the color support level
     * @return Color capability enum value
     */
    ColorSupport getColorSupport() const
    {
        return color_support_;
    }

    /**
     * @brief Check if terminal supports colors
     * @return true if any color support available, false if monochrome
     */
    bool hasColorSupport() const
    {
        return color_support_ != ColorSupport::None;
    }

    /**
     * @brief Check if terminal supports UTF-8 encoding
     * @return true if UTF-8 supported, false otherwise
     */
    bool hasUTF8Support() const
    {
        return utf8_support_;
    }

    /**
     * @brief Check if terminal supports emoji
     * @return true if emoji supported, false otherwise
     */
    bool hasEmojiSupport() const
    {
        return emoji_support_;
    }

private:
    /**
     * @brief Private constructor - use detect() instead
     */
    TerminalInfo() = default;

    bool is_tty_ = false;
    ColorSupport color_support_ = ColorSupport::None;
    bool utf8_support_ = false;
    bool emoji_support_ = false;
};

} // namespace homeshell
