#pragma once

#include <homeshell/Command.hpp>
#include <homeshell/Status.hpp>
#include <homeshell/VirtualFilesystem.hpp>

#include <string>
#include <vector>

extern "C"
{
#include "port/micropython_embed.h"
}

namespace homeshell
{

/**
 * @brief MicroPython interpreter command
 *
 * Provides access to an embedded MicroPython interpreter with both
 * interactive REPL and script execution capabilities. MicroPython is
 * statically linked, requiring no external Python installation.
 *
 * @details Usage: python [options] [file]
 *
 *          Interactive REPL:
 *          - python         (start interactive interpreter)
 *
 *          Script execution:
 *          - python script.py          (execute Python script)
 *          - python -c "print('hi')"   (execute inline code)
 *
 *          Features:
 *          - Python 3 syntax
 *          - Floating point arithmetic
 *          - Core modules: math, collections, json, re, hashlib, time
 *          - Multiline editing in REPL
 *          - Persistent history
 *
 *          Limitations:
 *          - No file I/O (import from files disabled)
 *          - No external packages/pip
 *          - Limited to core Python features
 *
 * Example: python -c "print(sum(range(100)))"
 */
class PythonCommand : public ICommand
{
public:
    std::string getName() const override
    {
        return "python";
    }

    std::string getDescription() const override
    {
        return "Execute Python code or run Python scripts (MicroPython)";
    }

    CommandType getType() const override
    {
        return CommandType::Synchronous;
    }

    /**
     * @brief Execute the python command
     * @param context Command context with script path or code
     * @return Status indicating success/failure
     */
    Status execute(const CommandContext& context) override;

private:
    static bool micropython_initialized_; ///< Global initialization state
    static char heap_[64 * 1024];         ///< MicroPython heap (64 KB)

    /**
     * @brief Initialize MicroPython runtime
     *
     * Initializes the MicroPython interpreter if not already initialized.
     * This includes setting up the heap and runtime environment.
     */
    void initializeMicroPython();

    /**
     * @brief Deinitialize MicroPython runtime
     *
     * Cleans up MicroPython resources. Called during shutdown.
     */
    void deinitializeMicroPython();

    /**
     * @brief Run interactive Python REPL
     *
     * Starts an interactive read-eval-print loop with:
     * - Line editing and history
     * - Multiline code support
     * - Syntax highlighting hints
     */
    void runREPL();

    /**
     * @brief Execute Python code string
     * @param code Python code to execute
     */
    void executeString(const std::string& code);

    /**
     * @brief Execute Python script from file
     * @param filename Path to Python script file
     */
    void executeFile(const std::string& filename);
};

} // namespace homeshell
