#include <homeshell/commands/VersionCommand.hpp>

#include <fmt/color.h>
#include <fmt/core.h>

extern "C"
{
#include "genhdr/mpversion.h"

#include <sqlite3.h>
}

#include <homeshell/version.h>

#define MINIZ_VERSION "3.0.2"

namespace homeshell
{

std::string VersionCommand::getMicroPythonVersion()
{
    return MICROPY_GIT_TAG " (" MICROPY_GIT_HASH ")";
}

std::string VersionCommand::getMinizVersion()
{
    return MINIZ_VERSION;
}

std::string VersionCommand::getSQLCipherVersion()
{
    return sqlite3_libversion();
}

std::string VersionCommand::getFmtVersion()
{
    return fmt::format("{}.{}.{}", FMT_VERSION / 10000, (FMT_VERSION % 10000) / 100,
                       FMT_VERSION % 100);
}

Status VersionCommand::execute(const CommandContext& context)
{
    bool use_colors = context.use_colors;
    homeshell::Version version;

    if (use_colors)
    {
        fmt::print(fg(fmt::color::cyan) | fmt::emphasis::bold, "Homeshell\n");
        fmt::print("  Version: {}\n", version.getVersionString());
        fmt::print("  Build:   {}\n\n", version.flavor);

        fmt::print(fg(fmt::color::yellow) | fmt::emphasis::bold, "Embedded Libraries:\n");
        fmt::print("  MicroPython:  {}\n", getMicroPythonVersion());
        fmt::print("  SQLCipher:    {}\n", getSQLCipherVersion());
        fmt::print("  miniz:        {}\n", getMinizVersion());
        fmt::print("  fmt:          {}\n", getFmtVersion());
        fmt::print("  CLI11:        3.2.0\n");
        fmt::print("  nlohmann/json: 3.11.2\n");
        fmt::print("  replxx:       0.0.4\n");
    }
    else
    {
        fmt::print("Homeshell\n");
        fmt::print("  Version: {}\n", version.getVersionString());
        fmt::print("  Build:   {}\n\n", version.flavor);

        fmt::print("Embedded Libraries:\n");
        fmt::print("  MicroPython:  {}\n", getMicroPythonVersion());
        fmt::print("  SQLCipher:    {}\n", getSQLCipherVersion());
        fmt::print("  miniz:        {}\n", getMinizVersion());
        fmt::print("  fmt:          {}\n", getFmtVersion());
        fmt::print("  CLI11:        3.2.0\n");
        fmt::print("  nlohmann/json: 3.11.2\n");
        fmt::print("  replxx:       0.0.4\n");
    }

    return Status::ok();
}

} // namespace homeshell
