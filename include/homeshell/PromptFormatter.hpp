#pragma once

#include <homeshell/FilesystemHelper.hpp>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>
#include <unistd.h>

namespace homeshell
{

class PromptFormatter
{
public:
    static std::string format(const std::string& format_string)
    {
        std::string result = format_string;

        // Replace tokens
        result = replaceToken(result, "%path%", getCurrentPath());
        result = replaceToken(result, "%folder%", getCurrentFolder());
        result = replaceToken(result, "%user%", getCurrentUser());
        result = replaceToken(result, "%time%", getCurrentTime());
        result = replaceToken(result, "%date%", getCurrentDate());

        return result;
    }

private:
    static std::string replaceToken(const std::string& str,
                                     const std::string& token,
                                     const std::string& value)
    {
        std::string result = str;
        size_t pos = 0;

        while ((pos = result.find(token, pos)) != std::string::npos)
        {
            result.replace(pos, token.length(), value);
            pos += value.length();
        }

        return result;
    }

    static std::string getCurrentPath()
    {
        return FilesystemHelper::getCurrentDirectory().string();
    }

    static std::string getCurrentFolder()
    {
        auto path = FilesystemHelper::getCurrentDirectory();
        return path.filename().string();
    }

    static std::string getCurrentUser()
    {
        const char* user = std::getenv("USER");
#ifdef _WIN32
        if (!user)
        {
            user = std::getenv("USERNAME");
        }
#endif
        return user ? std::string(user) : "unknown";
    }

    static std::string getCurrentTime()
    {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        std::tm tm = *std::localtime(&time);

        std::ostringstream oss;
        oss << std::setfill('0') << std::setw(2) << tm.tm_hour << ":"
            << std::setfill('0') << std::setw(2) << tm.tm_min << ":"
            << std::setfill('0') << std::setw(2) << tm.tm_sec;

        return oss.str();
    }

    static std::string getCurrentDate()
    {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        std::tm tm = *std::localtime(&time);

        std::ostringstream oss;
        oss << (tm.tm_year + 1900) << "-"
            << std::setfill('0') << std::setw(2) << (tm.tm_mon + 1) << "-"
            << std::setfill('0') << std::setw(2) << tm.tm_mday;

        return oss.str();
    }
};

} // namespace homeshell

