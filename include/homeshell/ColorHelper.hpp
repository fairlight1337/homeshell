#pragma once

#include <fmt/color.h>
#include <fmt/core.h>

#include <string>

namespace homeshell
{

// Helper for conditional color formatting
template <typename... Args>
void print_color(bool use_colors, fmt::color color, const std::string& format_str, Args&&... args)
{
    if (use_colors)
    {
        fmt::print(fg(color), fmt::runtime(format_str), std::forward<Args>(args)...);
    }
    else
    {
        fmt::print(fmt::runtime(format_str), std::forward<Args>(args)...);
    }
}

template <typename... Args>
void print_color(bool use_colors, fmt::text_style style, const std::string& format_str,
                 Args&&... args)
{
    if (use_colors)
    {
        fmt::print(style, fmt::runtime(format_str), std::forward<Args>(args)...);
    }
    else
    {
        fmt::print(fmt::runtime(format_str), std::forward<Args>(args)...);
    }
}

} // namespace homeshell
