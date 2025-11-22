#pragma once

#include <homeshell/Command.hpp>
#include <homeshell/Status.hpp>

#include <string>

namespace homeshell
{

/**
 * @brief Version information command
 *
 * Displays Homeshell version information along with versions of all
 * embedded libraries. Useful for debugging and verifying the build.
 *
 * @details Displays:
 *          - Homeshell version and build flavor
 *          - MicroPython version and commit hash
 *          - SQLCipher version
 *          - miniz version
 *          - fmt library version
 *          - CLI11 version
 *          - nlohmann/json version
 *          - replxx version
 *
 * Example output:
 * ```
 * Homeshell
 *   Version: 1.0.0-stable
 *   Build:   stable
 *
 * Embedded Libraries:
 *   MicroPython:  v1.27.0-preview.440.ga6864109db
 *   SQLCipher:    3.50.4
 *   miniz:        3.0.2
 *   ...
 * ```
 *
 * Example: version
 */
class VersionCommand : public ICommand
{
public:
    std::string getName() const override
    {
        return "version";
    }

    std::string getDescription() const override
    {
        return "Show Homeshell and library versions";
    }

    CommandType getType() const override
    {
        return CommandType::Synchronous;
    }

    /**
     * @brief Execute the version command
     * @param context Command context (no arguments needed)
     * @return Status indicating success
     */
    Status execute(const CommandContext& context) override;

private:
    /**
     * @brief Get Homeshell version string
     * @return Version string (e.g., "1.0.0-stable")
     */
    std::string getHomeshellVersion();

    /**
     * @brief Get MicroPython version string
     * @return MicroPython version with git tag and hash
     */
    std::string getMicroPythonVersion();

    /**
     * @brief Get SQLCipher version string
     * @return SQLCipher/SQLite version
     */
    std::string getSQLCipherVersion();

    /**
     * @brief Get miniz version string
     * @return miniz library version
     */
    std::string getMinizVersion();

    /**
     * @brief Get fmt library version string
     * @return fmt library version
     */
    std::string getFmtVersion();
};

} // namespace homeshell
