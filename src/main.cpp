#include <homeshell/Config.hpp>
#include <homeshell/Homeshell.hpp>

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

    // Parse arguments
    CLI11_PARSE(app, argc, argv);

    // Handle version flag
    if (show_version)
    {
        Version version;
        std::cout << "Homeshell v" << version.getVersionString() << std::endl;
        return 0;
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
                std::cout << "Loaded config from: " << config_file << std::endl;
                std::cout << "Prompt format: " << config.prompt_format << std::endl;
            }
        }
        catch (const std::exception& e)
        {
            std::cerr << "Error loading config: " << e.what() << std::endl;
            return 1;
        }
    }
    else
    {
        config = Config::loadDefault();
        if (verbose)
        {
            std::cout << "Using default configuration" << std::endl;
        }
    }

    Homeshell homeshell;
    return homeshell.run().code;
}
