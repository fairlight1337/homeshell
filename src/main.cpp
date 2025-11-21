#include <homeshell/Config.hpp>
#include <homeshell/Shell.hpp>
#include <homeshell/TerminalInfo.hpp>
#include <homeshell/Command.hpp>
#include <homeshell/commands/HelpCommand.hpp>
#include <homeshell/commands/ExitCommand.hpp>
#include <homeshell/commands/EchoCommand.hpp>
#include <homeshell/commands/SleepCommand.hpp>
#include <homeshell/version.h>

#include <CLI/CLI.hpp>
#include <fmt/core.h>
#include <iostream>

int main(int argc, char** argv)
{
    using namespace homeshell;

    CLI::App app{"Homeshell - A modern shell for home automation"};

    // Add version flag
    bool show_version = false;
    app.add_flag("-v,--version", show_version, "Show version information");

    // Add configuration file option
    std::string config_file = "";
    app.add_option("-c,--config", config_file, "Configuration file path");

    // Add verbose flag
    bool verbose = false;
    app.add_flag("--verbose", verbose, "Enable verbose output");

    // Parse arguments
    CLI11_PARSE(app, argc, argv);

    // Handle version flag
    if (show_version)
    {
        Version version;
        fmt::print("Homeshell v{}\n", version.getVersionString());
        return 0;
    }

    // Detect terminal capabilities
    TerminalInfo terminal_info = TerminalInfo::detect();

    if (verbose)
    {
        fmt::print("Terminal Info:\n");
        fmt::print("  TTY: {}\n", terminal_info.isTTY() ? "yes" : "no");
        fmt::print("  Colors: {}\n", terminal_info.hasColorSupport() ? "yes" : "no");
        fmt::print("  UTF-8: {}\n", terminal_info.hasUTF8Support() ? "yes" : "no");
        fmt::print("  Emoji: {}\n", terminal_info.hasEmojiSupport() ? "yes" : "no");
        fmt::print("\n");
    }

    // Load configuration
    Config config;
    if (!config_file.empty())
    {
        try
        {
            config = Config::loadFromFile(config_file);
            if (verbose)
            {
                fmt::print("Loaded config from: {}\n", config_file);
                fmt::print("Prompt format: {}\n", config.prompt_format);
            }
        }
        catch (const std::exception& e)
        {
            fmt::print(stderr, "Error loading config: {}\n", e.what());
            return 1;
        }
    }
    else
    {
        config = Config::loadDefault();
        if (verbose)
        {
            fmt::print("Using default configuration\n");
        }
    }

    // Register built-in commands
    auto& registry = CommandRegistry::getInstance();
    registry.registerCommand(std::make_shared<HelpCommand>());
    registry.registerCommand(std::make_shared<ExitCommand>());
    registry.registerCommand(std::make_shared<EchoCommand>());
    registry.registerCommand(std::make_shared<SleepCommand>());

    // Create and run the shell
    Shell shell(config, terminal_info);
    shell.run();

    return 0;
}
