#pragma once

#include <cstdio>
#include <memory>
#include <string>

namespace homeshell
{

/**
 * @brief Type of output stream to redirect
 */
enum class RedirectType
{
    None,   ///< No redirection
    Stdout, ///< Redirect stdout only
    Stderr, ///< Redirect stderr only
    Both    ///< Redirect both stdout and stderr
};

/**
 * @brief Redirection mode (truncate or append)
 */
enum class RedirectMode
{
    Truncate, ///< Overwrite file (>)
    Append    ///< Append to file (>>)
};

/**
 * @brief Parsed redirection information
 *
 * Contains all details about output redirection parsed from a command line.
 */
struct RedirectInfo
{
    RedirectType type = RedirectType::None;     ///< Which streams to redirect
    RedirectMode mode = RedirectMode::Truncate; ///< Truncate or append mode
    std::string filename;                       ///< Target file for redirection
    std::string command;                        ///< Command without redirection part
};

/**
 * @brief Output stream redirection manager
 *
 * Handles redirection of stdout and stderr to files, similar to bash redirection operators.
 * Automatically restores original streams when destroyed (RAII pattern).
 *
 * @details Features:
 *          - Parse redirection operators from command line (>, >>)
 *          - Redirect stdout, stderr, or both
 *          - Truncate or append modes
 *          - Automatic stream restoration via RAII
 *          - Flush streams before/after redirection
 *          - Works with shell's color output control
 *
 *          Supported operators:
 *          - `>` - Redirect stdout, truncate file
 *          - `>>` - Redirect stdout, append to file
 *          - `2>` - Redirect stderr, truncate file
 *          - `2>>` - Redirect stderr, append to file
 *          - `&>` - Redirect both stdout and stderr
 *
 * Example usage:
 * @code
 * RedirectInfo info = OutputRedirection::parse("ls > output.txt");
 * OutputRedirection redirector;
 * if (redirector.redirect(info)) {
 *     // Execute command - output goes to file
 * }
 * // redirector destructor restores streams automatically
 * @endcode
 *
 * @note The redirector flushes streams before applying redirection to ensure
 *       all buffered data is written. Stream restoration is automatic via destructor.
 */
class OutputRedirection
{
public:
    /**
     * @brief Parse command line for redirection operators
     * @param command_line Full command line that may contain redirection
     * @return RedirectInfo with parsed redirection details and cleaned command
     */
    static RedirectInfo parse(const std::string& command_line);

    /**
     * @brief Apply output redirection
     * @param info Redirection information (from parse())
     * @return true on success, false on error (e.g., cannot open file)
     */
    bool redirect(const RedirectInfo& info);

    /**
     * @brief Restore original stdout/stderr
     *
     * Restores the original file descriptors and closes redirect file.
     * Called automatically by destructor.
     */
    void restore();

    ~OutputRedirection()
    {
        restore();
    }

private:
    FILE* original_stdout_ = nullptr;
    FILE* original_stderr_ = nullptr;
    FILE* redirect_file_ = nullptr;
    int saved_stdout_fd_ = -1;
    int saved_stderr_fd_ = -1;
    bool redirected_ = false;
};

} // namespace homeshell
