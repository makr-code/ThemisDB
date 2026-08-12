/**
 * @file socket_timeout_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <chrono>
#include <string>
#include <atomic>
#include <memory>
#include <functional>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    typedef SOCKET socket_t;
    typedef SSIZE_T ssize_t;
    #define INVALID_SOCKET_VALUE INVALID_SOCKET
#else
    #include <sys/socket.h>
    #include <sys/select.h>
    #include <poll.h>
    #include <unistd.h>
    #include <fcntl.h>
    #include <netinet/in.h>
    #include <netinet/tcp.h>
    typedef int socket_t;
    #define INVALID_SOCKET_VALUE -1
#endif

namespace themis {
namespace network {

/**
 * @brief Socket timeout configuration
 */
struct SocketTimeoutConfig {
    std::chrono::milliseconds accept_timeout{5000};    // 5s default
    std::chrono::milliseconds read_timeout{30000};     // 30s default
    std::chrono::milliseconds write_timeout{30000};    // 30s default
    std::chrono::milliseconds keepalive_interval{60000}; // 60s default
    bool enable_tcp_keepalive{true};
    bool enable_tcp_nodelay{true};  // Disable Nagle's algorithm
    int max_retry_attempts{3};
    
    // Timeout thresholds for circuit breaker
    size_t timeout_threshold{10};  // Open circuit after 10 timeouts
    std::chrono::seconds reset_timeout{60};  // Try again after 60s
};

/**
 * @brief Socket timeout statistics
 */
struct SocketTimeoutStats {
    std::atomic<uint64_t> accept_timeouts{0};
    std::atomic<uint64_t> read_timeouts{0};
    std::atomic<uint64_t> write_timeouts{0};
    std::atomic<uint64_t> successful_operations{0};
    std::atomic<uint64_t> failed_operations{0};
    std::atomic<uint64_t> total_bytes_read{0};
    std::atomic<uint64_t> total_bytes_written{0};
    
    void reset() {
        accept_timeouts = 0;
        read_timeouts = 0;
        write_timeouts = 0;
        successful_operations = 0;
        failed_operations = 0;
        total_bytes_read = 0;
        total_bytes_written = 0;
    }
    
    double getTimeoutRate() const {
        uint64_t total = accept_timeouts + read_timeouts + write_timeouts;
        uint64_t all_ops = total + successful_operations;
        return all_ops > 0 ? static_cast<double>(total) / all_ops : 0.0;
    }
};

/**
 * @brief Health state for circuit breaker pattern
 */
enum class SocketHealthState {
    HEALTHY,      // Normal operation
    DEGRADED,     // Experiencing timeouts but still operational
    CIRCUIT_OPEN  // Too many timeouts, refuse new connections temporarily
};

/**
 * @brief Socket Timeout Manager
 * 
 * Manages socket timeouts to prevent hanging connections and resource exhaustion.
 * Implements circuit breaker pattern for connections with repeated timeouts.
 * 
 * Features:
 * - Accept timeout (non-blocking accept with timeout)
 * - Read timeout (SO_RCVTIMEO)
 * - Write timeout (SO_SNDTIMEO)
 * - TCP keepalive configuration
 * - Circuit breaker for problematic connections
 * - Platform-independent implementation (Windows/Unix)
 * 
 * Example usage:
 * @code
 * SocketTimeoutManager manager(config);
 * 
 * // Accept with timeout
 * auto client_socket = manager.acceptWithTimeout(server_socket);
 * if (client_socket == INVALID_SOCKET_VALUE) {
 *     // Timeout or error
 * }
 * 
 * // Read with timeout
 * std::vector<char> buffer(4096);
 * ssize_t bytes = manager.readWithTimeout(client_socket, buffer.data(), buffer.size());
 * 
 * // Write with timeout
 * ssize_t sent = manager.writeWithTimeout(client_socket, data.data(), data.size());
 * @endcode
 */
class SocketTimeoutManager {
public:
    explicit SocketTimeoutManager(const SocketTimeoutConfig& config = SocketTimeoutConfig());
    ~SocketTimeoutManager();
    
    // Non-copyable, movable
    SocketTimeoutManager(const SocketTimeoutManager&) = delete;
    SocketTimeoutManager& operator=(const SocketTimeoutManager&) = delete;
    SocketTimeoutManager(SocketTimeoutManager&&) = default;
    SocketTimeoutManager& operator=(SocketTimeoutManager&&) = default;
    
    /**
     * @brief Configure socket with timeouts
     * @param socket Socket to configure
     * @return true on success, false on failure
     */
    bool configureSocket(socket_t socket);
    
    /**
     * @brief Accept connection with timeout
     * @param server_socket Server socket to accept on
     * @param timeout_ms Timeout in milliseconds (0 = use config default)
     * @return Accepted socket or INVALID_SOCKET_VALUE on timeout/error
     */
    socket_t acceptWithTimeout(socket_t server_socket, 
                                std::chrono::milliseconds timeout_ms = std::chrono::milliseconds(0));
    
    /**
     * @brief Read from socket with timeout
     * @param socket Socket to read from
     * @param buffer Buffer to read into
     * @param size Size of buffer
     * @param timeout_ms Timeout in milliseconds (0 = use config default)
     * @return Number of bytes read, -1 on error, 0 on connection close
     */
    ssize_t readWithTimeout(socket_t socket, void* buffer, size_t size,
                            std::chrono::milliseconds timeout_ms = std::chrono::milliseconds(0));
    
    /**
     * @brief Write to socket with timeout
     * @param socket Socket to write to
     * @param buffer Buffer to write from
     * @param size Size of buffer
     * @param timeout_ms Timeout in milliseconds (0 = use config default)
     * @return Number of bytes written, -1 on error
     */
    ssize_t writeWithTimeout(socket_t socket, const void* buffer, size_t size,
                             std::chrono::milliseconds timeout_ms = std::chrono::milliseconds(0));
    
    /**
     * @brief Close socket with proper cleanup
     * @param socket Socket to close
     */
    void closeSocket(socket_t socket);
    
    /**
     * @brief Check if should accept new connections (circuit breaker)
     * @return true if healthy enough to accept, false if circuit is open
     */
    bool shouldAcceptConnection() const;
    
    /**
     * @brief Record timeout for circuit breaker
     */
    void recordTimeout();
    
    /**
     * @brief Record successful operation for circuit breaker
     */
    void recordSuccess();
    
    /**
     * @brief Get current health state
     */
    SocketHealthState getHealthState() const { return health_state_; }
    
    /**
     * @brief Get timeout statistics
     */
    const SocketTimeoutStats& getStats() const { return stats_; }
    
    /**
     * @brief Reset statistics
     */
    void resetStats() { stats_.reset(); consecutive_timeouts_ = 0; }
    
    /**
     * @brief Get configuration
     */
    const SocketTimeoutConfig& getConfig() const { return config_; }
    
    /**
     * @brief Set alert callback for circuit breaker state changes
     */
    void setAlertCallback(std::function<void(SocketHealthState, const std::string&)> callback) {
        alert_callback_ = std::move(callback);
    }
    
private:
    /**
     * @brief Set socket to non-blocking mode
     */
    bool setNonBlocking(socket_t socket);
    
    /**
     * @brief Configure TCP keepalive
     */
    bool configureTCPKeepalive(socket_t socket);
    
    /**
     * @brief Configure TCP nodelay (disable Nagle's algorithm)
     */
    bool configureTCPNoDelay(socket_t socket);
    
    /**
     * @brief Update health state based on timeout count
     */
    void updateHealthState();
    
    /**
     * @brief Trigger alert if callback is set
     */
    void triggerAlert(SocketHealthState new_state, const std::string& message);
    
    SocketTimeoutConfig config_;
    SocketTimeoutStats stats_;
    SocketHealthState health_state_{SocketHealthState::HEALTHY};
    std::atomic<size_t> consecutive_timeouts_{0};
    std::chrono::steady_clock::time_point last_circuit_open_time_;
    std::function<void(SocketHealthState, const std::string&)> alert_callback_;
};

/**
 * @brief RAII guard for socket timeout operations
 * 
 * Automatically closes socket on scope exit if not released.
 * 
 * Example usage:
 * @code
 * SocketTimeoutGuard guard(manager, client_socket);
 * 
 * // Do operations with socket
 * auto bytes = manager.readWithTimeout(guard.get(), buffer, size);
 * 
 * if (success) {
 *     guard.release();  // Keep socket open
 * }
 * // Otherwise socket is automatically closed
 * @endcode
 */
class SocketTimeoutGuard {
public:
    SocketTimeoutGuard(SocketTimeoutManager& manager, socket_t socket)
        : manager_(manager), socket_(socket), owns_(true) {}
    
    ~SocketTimeoutGuard() {
        if (owns_ && socket_ != INVALID_SOCKET_VALUE) {
            manager_.closeSocket(socket_);
        }
    }
    
    // Non-copyable, movable
    SocketTimeoutGuard(const SocketTimeoutGuard&) = delete;
    SocketTimeoutGuard& operator=(const SocketTimeoutGuard&) = delete;
    
    SocketTimeoutGuard(SocketTimeoutGuard&& other) noexcept
        : manager_(other.manager_), socket_(other.socket_), owns_(other.owns_) {
        other.owns_ = false;
    }
    
    socket_t get() const { return socket_; }
    socket_t release() { owns_ = false; return socket_; }
    bool valid() const { return socket_ != INVALID_SOCKET_VALUE; }
    
private:
    SocketTimeoutManager& manager_;
    socket_t socket_;
    bool owns_;
};

} // namespace network
} // namespace themis
