#pragma once

#include <homeshell/Command.hpp>

#include <fmt/color.h>
#include <fmt/core.h>

#include <atomic>
#include <chrono>
#include <thread>

namespace homeshell
{

/**
 * @brief Sleep for specified duration
 *
 * Pauses execution for a specified number of seconds. Demonstrates
 * asynchronous command execution and cancellation support.
 *
 * @details The sleep command is an asynchronous operation that:
 *          - Runs in a background thread
 *          - Can be cancelled with Ctrl+C
 *          - Checks cancellation periodically (every 100ms)
 *          - Provides progress feedback
 *
 *          This command is primarily useful for:
 *          - Testing asynchronous command infrastructure
 *          - Demonstrating signal handling
 *          - Scripting delays
 *          - Performance testing
 *
 *          Command syntax:
 *          ```
 *          sleep <seconds>
 *          ```
 *
 *          Behavior:
 *          - Accepts integer seconds argument
 *          - Prints start message
 *          - Sleeps in 100ms increments (for cancellation responsiveness)
 *          - Prints completion or cancellation message
 *          - Can be interrupted with Ctrl+C
 *
 * Example usage:
 * ```
 * sleep 5                     # Sleep for 5 seconds
 * sleep 60                    # Sleep for 1 minute
 * sleep 10                    # Then press Ctrl+C to cancel
 * ```
 *
 * @note This command type is Asynchronous, meaning it can run
 *       in the background and respond to cancellation requests.
 */
class SleepCommand : public ICommand
{
public:
    /**
     * @brief Construct sleep command with cancellation support
     */
    SleepCommand()
        : cancelled_(false)
    {
    }

    std::string getName() const override
    {
        return "sleep";
    }

    std::string getDescription() const override
    {
        return "Sleep for N seconds (async demo)";
    }

    CommandType getType() const override
    {
        return CommandType::Asynchronous;
    }

    /**
     * @brief Check if command supports cancellation
     * @return true (sleep can be cancelled)
     */
    bool supportsCancellation() const override
    {
        return true;
    }

    /**
     * @brief Cancel the sleep operation
     *
     * Sets the cancellation flag, which is checked periodically
     * during the sleep loop to terminate early.
     */
    void cancel() override
    {
        cancelled_.store(true);
    }

    /**
     * @brief Execute the sleep command
     * @param context Command context with duration argument
     * @return Status::ok() after full sleep, Status::error() if cancelled or invalid
     */
    Status execute(const CommandContext& context) override
    {
        if (context.args.empty())
        {
            return Status::error("Usage: sleep <seconds>");
        }

        try
        {
            int seconds = std::stoi(context.args[0]);
            fmt::print("Sleeping for {} seconds...\n", seconds);

            // Sleep in small increments to check for cancellation
            for (int i = 0; i < seconds * 10; ++i)
            {
                if (cancelled_.load())
                {
                    fmt::print(fg(fmt::color::yellow), "\nSleep cancelled!\n");
                    return Status::error("Cancelled");
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }

            fmt::print("Done sleeping!\n");
            return Status::ok();
        }
        catch (...)
        {
            return Status::error("Invalid number");
        }
    }

private:
    std::atomic<bool> cancelled_; ///< Cancellation flag (atomic for thread safety)
};

} // namespace homeshell
