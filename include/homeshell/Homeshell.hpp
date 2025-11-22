#pragma once

#include <homeshell/Status.hpp>
#include <homeshell/version.h>

namespace homeshell
{

/**
 * @brief Main application class for Homeshell
 *
 * @details The `Homeshell` class is the top-level application controller that
 * initializes, configures, and runs the interactive shell. It serves as the
 * primary entry point for the shell application.
 *
 * **Responsibilities:**
 * - Application initialization
 * - Configuration management
 * - Shell lifecycle management
 * - Resource cleanup on exit
 *
 * **Architecture:**
 * - Singleton command registry setup
 * - Virtual filesystem initialization
 * - Terminal capability detection
 * - REPL (Read-Eval-Print Loop) execution
 * - Signal handler installation
 * - Graceful shutdown handling
 *
 * **Typical Usage:**
 * @code
 * int main(int argc, char* argv[])
 * {
 *     Homeshell app;
 *     Status status = app.run();
 *     return status.isSuccess() ? 0 : 1;
 * }
 * @endcode
 *
 * **Lifecycle:**
 * 1. **Construction** - Initialize core components
 * 2. **run()** - Enter interactive shell loop
 * 3. **Destruction** - Clean up resources
 *
 * @note Manages application-wide resources and state
 * @note Ensures proper cleanup on both normal and abnormal termination
 * @see Shell class for REPL implementation details
 */
class Homeshell
{
public:
    /**
     * @brief Construct Homeshell application
     *
     * @details Initializes the shell application, sets up core subsystems,
     * and prepares for interactive execution.
     */
    Homeshell();

    /**
     * @brief Destruct Homeshell application
     *
     * @details Performs cleanup of resources, flushes buffers, and
     * ensures proper shutdown of all subsystems.
     */
    ~Homeshell();

    /**
     * @brief Run the interactive shell
     *
     * @details Starts the main REPL (Read-Eval-Print Loop), processes user
     * input, executes commands, and handles signals until the user exits
     * or an error occurs.
     *
     * @return Status indicating success or failure
     *         - Status::ok() if shell exited normally
     *         - Status::error() if a fatal error occurred
     *
     * @note Blocks until shell exits
     * @note Handles Ctrl+C for command cancellation (not shell termination)
     * @note Use 'exit' command to terminate the shell
     */
    Status run();
};

} // namespace homeshell
