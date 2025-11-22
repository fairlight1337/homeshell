#pragma once

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/ip_icmp.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <cstring>
#include <string>

namespace homeshell
{

/**
 * @brief Result of a single ICMP ping operation
 *
 * @details Contains the outcome of a ping attempt, including success status,
 * round-trip time, and any error messages.
 */
struct PingResult
{
    bool success = false; ///< Whether the ping was successful
    double rtt_ms = 0.0;  ///< Round-trip time in milliseconds
    std::string error;    ///< Error message if ping failed
};

/**
 * @brief Low-level ICMP ping implementation
 *
 * @details Provides raw ICMP echo request/reply functionality for network diagnostics.
 * Implements cancellable ping operations using raw sockets.
 *
 * **Features:**
 * - Raw ICMP socket implementation
 * - Cancellable operations
 * - Configurable timeout
 * - Round-trip time measurement (microsecond precision)
 * - Hostname resolution (DNS lookup)
 * - IPv4 support
 *
 * **Requirements:**
 * - Linux/Unix platform
 * - CAP_NET_RAW capability or root privileges
 * - Set capability: `sudo setcap cap_net_raw+ep ./homeshell-linux`
 *
 * **Usage Example:**
 * @code
 * IcmpPing pinger;
 *
 * // Ping with 1 second timeout
 * PingResult result = pinger.ping("8.8.8.8", 1000);
 * if (result.success) {
 *     fmt::print("RTT: {:.2f}ms\n", result.rtt_ms);
 * } else {
 *     fmt::print("Error: {}\n", result.error);
 * }
 *
 * // Cancellable ping
 * std::thread ping_thread([&pinger]() {
 *     pinger.ping("example.com", 5000);
 * });
 *
 * // Cancel from another thread
 * pinger.cancel();
 * ping_thread.join();
 * @endcode
 *
 * **Error Conditions:**
 * - "Failed to resolve host" - DNS lookup failed
 * - "Failed to create socket" - Insufficient privileges (need CAP_NET_RAW)
 * - "Request timeout" - No response within timeout period
 * - "Cancelled" - Operation cancelled by user
 *
 * @note Requires raw socket access (CAP_NET_RAW capability on Linux)
 * @note Thread-safe cancellation via atomic flag
 * @note Uses ICMP echo (type 8) and echo reply (type 0)
 */
class IcmpPing
{
public:
    IcmpPing()
        : cancelled_(false)
    {
    }

    /**
     * @brief Cancel ongoing ping operation
     *
     * @details Sets cancellation flag to abort current or future ping operations.
     * Thread-safe and can be called from any thread.
     */
    void cancel()
    {
        cancelled_.store(true);
    }

    /**
     * @brief Check if ping has been cancelled
     *
     * @return true if cancel() has been called, false otherwise
     */
    bool isCancelled() const
    {
        return cancelled_.load();
    }

    /**
     * @brief Send ICMP echo request and wait for reply
     *
     * @param host Hostname or IP address to ping
     * @param timeout_ms Maximum time to wait for response in milliseconds (default: 1000ms)
     * @return PingResult containing success status, RTT, or error message
     *
     * @details Performs DNS lookup if hostname provided, creates raw ICMP socket,
     * sends echo request, and waits for echo reply. Operation can be cancelled
     * via cancel() method.
     *
     * @note Requires CAP_NET_RAW capability or root privileges
     * @note Blocks until reply received, timeout expires, or operation cancelled
     */
    PingResult ping(const std::string& host, int timeout_ms = 1000)
    {
        PingResult result;

        // Resolve hostname
        struct addrinfo hints
        {
        }, *res = nullptr;
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_RAW;
        hints.ai_protocol = IPPROTO_ICMP;

        if (getaddrinfo(host.c_str(), nullptr, &hints, &res) != 0)
        {
            result.error = "Failed to resolve host: " + host;
            return result;
        }

        if (res == nullptr)
        {
            result.error = "No address found for host: " + host;
            return result;
        }

        struct sockaddr_in addr;
        std::memcpy(&addr, res->ai_addr, sizeof(addr));
        freeaddrinfo(res);

        // Create raw socket
        int sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
        if (sock < 0)
        {
            result.error = "Failed to create socket (need root/CAP_NET_RAW)";
            return result;
        }

        // Set socket timeout
        struct timeval tv;
        tv.tv_sec = timeout_ms / 1000;
        tv.tv_usec = (timeout_ms % 1000) * 1000;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

        // Send ICMP echo request
        auto send_time = std::chrono::steady_clock::now();
        if (!sendEchoRequest(sock, addr))
        {
            close(sock);
            result.error = "Failed to send ICMP packet";
            return result;
        }

        // Wait for response with cancellation check
        char buffer[1024];
        struct sockaddr_in recv_addr;
        socklen_t addr_len = sizeof(recv_addr);

        struct pollfd pfd;
        pfd.fd = sock;
        pfd.events = POLLIN;

        while (!cancelled_.load())
        {
            int poll_result = poll(&pfd, 1, 100); // Check every 100ms

            if (poll_result < 0)
            {
                close(sock);
                result.error = "Poll error";
                return result;
            }

            if (poll_result == 0)
            {
                // Timeout - check if total timeout exceeded
                auto elapsed = std::chrono::steady_clock::now() - send_time;
                if (std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count() >
                    timeout_ms)
                {
                    close(sock);
                    result.error = "Request timeout";
                    return result;
                }
                continue;
            }

            ssize_t bytes =
                recvfrom(sock, buffer, sizeof(buffer), 0, (struct sockaddr*)&recv_addr, &addr_len);

            if (bytes < 0)
            {
                close(sock);
                result.error = "Failed to receive response";
                return result;
            }

            auto recv_time = std::chrono::steady_clock::now();

            // Parse ICMP response
            if (parseEchoReply(buffer, bytes))
            {
                auto rtt =
                    std::chrono::duration_cast<std::chrono::microseconds>(recv_time - send_time);
                result.success = true;
                result.rtt_ms = rtt.count() / 1000.0;
                close(sock);
                return result;
            }
        }

        close(sock);
        result.error = "Cancelled";
        return result;
    }

private:
    std::atomic<bool> cancelled_;
    static uint16_t seq_num_;

    uint16_t checksum(uint16_t* buf, int len)
    {
        uint32_t sum = 0;

        while (len > 1)
        {
            sum += *buf++;
            len -= 2;
        }

        if (len == 1)
        {
            sum += *(uint8_t*)buf;
        }

        sum = (sum >> 16) + (sum & 0xFFFF);
        sum += (sum >> 16);

        return ~sum;
    }

    bool sendEchoRequest(int sock, const struct sockaddr_in& addr)
    {
        char packet[64];
        std::memset(packet, 0, sizeof(packet));

        struct icmphdr* icmp = (struct icmphdr*)packet;
        icmp->type = ICMP_ECHO;
        icmp->code = 0;
        icmp->un.echo.id = getpid() & 0xFFFF;
        icmp->un.echo.sequence = seq_num_++;
        icmp->checksum = 0;
        icmp->checksum = checksum((uint16_t*)packet, sizeof(packet));

        ssize_t sent =
            sendto(sock, packet, sizeof(packet), 0, (struct sockaddr*)&addr, sizeof(addr));

        return sent == sizeof(packet);
    }

    bool parseEchoReply(const char* buffer, ssize_t len)
    {
        if (len < static_cast<ssize_t>(sizeof(struct iphdr) + sizeof(struct icmphdr)))
        {
            return false;
        }

        struct iphdr* ip = (struct iphdr*)buffer;
        int ip_header_len = ip->ihl * 4;

        struct icmphdr* icmp = (struct icmphdr*)(buffer + ip_header_len);

        return (icmp->type == ICMP_ECHOREPLY);
    }
};

uint16_t IcmpPing::seq_num_ = 0;

} // namespace homeshell
