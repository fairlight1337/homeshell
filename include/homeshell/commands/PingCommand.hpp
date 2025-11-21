#pragma once

#include <homeshell/Command.hpp>
#include <homeshell/IcmpPing.hpp>

#include <fmt/color.h>
#include <fmt/core.h>

#include <chrono>
#include <thread>

namespace homeshell
{

class PingCommand : public ICommand
{
public:
    PingCommand()
        : pinger_(nullptr)
    {
    }

    std::string getName() const override
    {
        return "ping";
    }

    std::string getDescription() const override
    {
        return "Ping a host (ICMP echo)";
    }

    CommandType getType() const override
    {
        return CommandType::Asynchronous;
    }

    bool supportsCancellation() const override
    {
        return true;
    }

    void cancel() override
    {
        if (pinger_)
        {
            pinger_->cancel();
        }
    }

    Status execute(const CommandContext& context) override
    {
        if (context.args.empty())
        {
            return Status::error("Usage: ping <host> [count]");
        }

        std::string host = context.args[0];
        int count = 4;         // Default count
        int timeout_ms = 2000; // 2 second timeout

        // Parse optional count argument
        if (context.args.size() >= 2)
        {
            try
            {
                count = std::stoi(context.args[1]);
                if (count <= 0)
                {
                    return Status::error("Count must be positive");
                }
            }
            catch (...)
            {
                return Status::error("Invalid count: " + context.args[1]);
            }
        }

        fmt::print("PING {} ({} packets):\n", host, count);

        pinger_ = std::make_shared<IcmpPing>();
        int success_count = 0;
        int failure_count = 0;
        double total_rtt = 0.0;
        double min_rtt = 999999.0;
        double max_rtt = 0.0;

        for (int i = 1; i <= count; ++i)
        {
            if (pinger_->isCancelled())
            {
                fmt::print(fg(fmt::color::yellow), "\nPing cancelled by user\n");
                return Status::error("Cancelled");
            }

            auto result = pinger_->ping(host, timeout_ms);

            if (result.success)
            {
                success_count++;
                total_rtt += result.rtt_ms;
                min_rtt = std::min(min_rtt, result.rtt_ms);
                max_rtt = std::max(max_rtt, result.rtt_ms);

                fmt::print("Reply from {}: seq={} time={:.2f}ms\n", host, i, result.rtt_ms);
            }
            else
            {
                failure_count++;
                if (result.error == "Cancelled")
                {
                    fmt::print(fg(fmt::color::yellow), "\nPing cancelled by user\n");
                    return Status::error("Cancelled");
                }
                fmt::print(fg(fmt::color::red), "Request timeout ({})\n", result.error);
            }

            // Wait before next ping (except on last iteration)
            if (i < count && !pinger_->isCancelled())
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }
        }

        // Print statistics
        fmt::print("\n--- {} ping statistics ---\n", host);
        fmt::print("{} packets transmitted, {} received, {:.0f}% packet loss\n", count,
                   success_count, 100.0 * failure_count / count);

        if (success_count > 0)
        {
            double avg_rtt = total_rtt / success_count;
            fmt::print("rtt min/avg/max = {:.2f}/{:.2f}/{:.2f} ms\n", min_rtt, avg_rtt, max_rtt);
            fmt::print(fg(fmt::color::green), "\nPing completed successfully\n");
            return Status::ok();
        }
        else
        {
            fmt::print(fg(fmt::color::red), "\nAll pings failed\n");
            return Status::error("All pings failed");
        }
    }

private:
    std::shared_ptr<IcmpPing> pinger_;
};

} // namespace homeshell
