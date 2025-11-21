#include <homeshell/Command.hpp>
#include <homeshell/Config.hpp>
#include <homeshell/EncryptedMount.hpp>
#include <homeshell/PasswordInput.hpp>
#include <homeshell/Shell.hpp>
#include <homeshell/TerminalInfo.hpp>
#include <homeshell/VirtualFilesystem.hpp>
#include <homeshell/commands/CatCommand.hpp>
#include <homeshell/commands/CdCommand.hpp>
#include <homeshell/commands/DateTimeCommand.hpp>
#include <homeshell/commands/EchoCommand.hpp>
#include <homeshell/commands/ExitCommand.hpp>
#include <homeshell/commands/HelpCommand.hpp>
#include <homeshell/commands/LsCommand.hpp>
#include <homeshell/commands/MkdirCommand.hpp>
#include <homeshell/commands/MountCommand.hpp>
#include <homeshell/commands/PingCommand.hpp>
#include <homeshell/commands/PwdCommand.hpp>
#include <homeshell/commands/RmCommand.hpp>
#include <homeshell/commands/SleepCommand.hpp>
#include <homeshell/commands/TouchCommand.hpp>
#include <homeshell/commands/UnmountCommand.hpp>
#include <homeshell/commands/VfsCommand.hpp>
#include <homeshell/version.h>

#include <fmt/core.h>

#include <CLI/CLI.hpp>
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

    // Add execute flag
    std::string execute_command = "";
    app.add_option("-e,--execute", execute_command, "Execute command and exit");

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

    // Auto-mount encrypted mounts from config
    auto& vfs = VirtualFilesystem::getInstance();
    for (const auto& mount_config : config.encrypted_mounts)
    {
        if (!mount_config.auto_mount)
        {
            continue;
        }

        std::string expanded_path = config.expandPath(mount_config.db_path);
        auto mount = std::make_shared<EncryptedMount>(
            mount_config.name, expanded_path, mount_config.mount_point, mount_config.max_size_mb);

        // Get password - prompt if not in config or empty
        std::string password = mount_config.password;
        if (password.empty())
        {
            // Only prompt if we have a TTY (interactive terminal)
            if (terminal_info.isTTY())
            {
                std::string prompt =
                    fmt::format("Enter password for mount '{}': ", mount_config.name);
                password = PasswordInput::readPassword(prompt);

                if (password.empty())
                {
                    fmt::print(stderr, "Warning: Skipping mount '{}' (empty password)\n",
                               mount_config.name);
                    continue;
                }
            }
            else
            {
                fmt::print(stderr,
                           "Warning: Skipping mount '{}' (no password in config "
                           "and not running in interactive terminal)\n",
                           mount_config.name);
                continue;
            }
        }

        if (mount->mount(password))
        {
            vfs.addMount(mount);
            if (verbose)
            {
                fmt::print("Auto-mounted '{}' at '{}'\n", mount_config.name,
                           mount_config.mount_point);
            }
        }
        else
        {
            fmt::print(stderr, "Warning: Failed to auto-mount '{}' (incorrect password?)\n",
                       mount_config.name);
        }
    }

    // Register built-in commands
    auto& registry = CommandRegistry::getInstance();
    registry.registerCommand(std::make_shared<HelpCommand>());
    registry.registerCommand(std::make_shared<ExitCommand>());
    registry.registerCommand(std::make_shared<EchoCommand>());
    registry.registerCommand(std::make_shared<SleepCommand>());
    registry.registerCommand(std::make_shared<DateTimeCommand>());

    // Filesystem commands
    registry.registerCommand(std::make_shared<LsCommand>());
    registry.registerCommand(std::make_shared<CdCommand>());
    registry.registerCommand(std::make_shared<PwdCommand>());
    registry.registerCommand(std::make_shared<CatCommand>());
    registry.registerCommand(std::make_shared<MkdirCommand>());
    registry.registerCommand(std::make_shared<TouchCommand>());
    registry.registerCommand(std::make_shared<RmCommand>());

    // Virtual filesystem commands
    registry.registerCommand(std::make_shared<MountCommand>());
    registry.registerCommand(std::make_shared<UnmountCommand>());
    registry.registerCommand(std::make_shared<VfsCommand>());

    // Network commands
    registry.registerCommand(std::make_shared<PingCommand>());

    // Create the shell
    Shell shell(config, terminal_info);

    // Execute command if -e flag was provided
    if (!execute_command.empty())
    {
        Status status = shell.executeCommandLine(execute_command);
        return status.code;
    }

    // Otherwise run interactive REPL
    shell.run();

    return 0;
}
