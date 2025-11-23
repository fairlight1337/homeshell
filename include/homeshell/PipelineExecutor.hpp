#pragma once

#include <homeshell/Command.hpp>
#include <homeshell/Status.hpp>

#include <memory>
#include <string>
#include <vector>

namespace homeshell
{

// Forward declaration
class Shell;

/**
 * @brief Execute a pipeline of commands connected by pipes
 *
 * Handles execution of command pipelines where multiple commands
 * are connected via the pipe operator (|). Each command's stdout
 * is connected to the next command's stdin.
 *
 * @details Features:
 *          - Parse command lines with pipe operators
 *          - Create pipes between commands
 *          - Handle both built-in and external commands
 *          - Proper process management and cleanup
 *          - Error propagation through pipeline
 *
 * Example usage:
 * ```
 * ls -l | grep txt | less
 * cat file.txt | tail -n 20
 * find . -name "*.cpp" | wc -l
 * ```
 *
 * @note Built-in commands must handle stdin when used in pipes
 * @note External executables get standard pipe() connections
 */
class PipelineExecutor
{
public:
    /**
     * @brief Check if a command line contains pipes
     * @param command_line Full command line to check
     * @return true if pipes are present
     */
    static bool hasPipe(const std::string& command_line);

    /**
     * @brief Split command line on pipe operators
     * @param command_line Full command line with pipes
     * @return Vector of individual command strings
     */
    static std::vector<std::string> splitPipeline(const std::string& command_line);

    /**
     * @brief Execute a pipeline of commands
     * @param commands Vector of command strings to execute in sequence
     * @param shell Pointer to shell for executing built-in commands
     * @return Status of the last command in the pipeline
     */
    static Status executePipeline(const std::vector<std::string>& commands, Shell* shell);
};

} // namespace homeshell
