/**
 * @file socket_timeout_manager.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=10, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "network/socket_timeout_manager.h"
#include <spdlog/spdlog.h>
#include <cstring>
#include <cerrno>
#include <memory>

#ifdef _WIN32
    #include <winsock2.h>
    #include <mstcpip.h>
    #pragma comment(lib, "ws2_32.lib")
#endif

namespace themis {
namespace network {

SocketTimeoutManager::SocketTimeoutManager(const SocketTimeoutConfig& config)
    : config_(config),
      last_circuit_open_time_(std::chrono::steady_clock::now()) {
    spdlog::info("SocketTimeoutManager initialized with accept_timeout={}ms, read_timeout={}ms, write_timeout={}ms",
                 config_.accept_timeout.count(),
                 config_.read_timeout.count(),
                 config_.write_timeout.count());
}

SocketTimeoutManager::~SocketTimeoutManager() noexcept {
    try {
        spdlog::info("SocketTimeoutManager destroyed. Stats: accept_timeouts={}, read_timeouts={}, write_timeouts={}, "
                     "successful_ops={}, timeout_rate={:.2f}%",
                     stats_.accept_timeouts.load(),
                     stats_.read_timeouts.load(),
                     stats_.write_timeouts.load(),
                     stats_.successful_operations.load(),
                     stats_.getTimeoutRate() * 100.0);
    } catch (...) {
        // Suppress exceptions in destructor; logging failure is non-critical
    }
}

bool SocketTimeoutManager::configureSocket(socket_t socket) {
    if (socket == INVALID_SOCKET_VALUE) {
        spdlog::error("Cannot configure invalid socket");
        return false;
    }
    
    bool success = true;
    
    // Set read timeout
#ifdef _WIN32
    DWORD timeout = static_cast<DWORD>(config_.read_timeout.count());
    if (setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, 
                   reinterpret_cast<const char*>(&timeout), sizeof(timeout)) != 0) {
        spdlog::warn("Failed to set SO_RCVTIMEO: {}", WSAGetLastError());
        success = false;
    }
#else
    struct timeval timeout;
    timeout.tv_sec = config_.read_timeout.count() / 1000;
    timeout.tv_usec = (config_.read_timeout.count() % 1000) * 1000;
    if (setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) != 0) {
        spdlog::warn("Failed to set SO_RCVTIMEO: {} ({})", strerror(errno), errno);
        success = false;
    }
#endif
    
    // Set write timeout
#ifdef _WIN32
    timeout = static_cast<DWORD>(config_.write_timeout.count());
    if (setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO,
                   reinterpret_cast<const char*>(&timeout), sizeof(timeout)) != 0) {
        spdlog::warn("Failed to set SO_SNDTIMEO: {}", WSAGetLastError());
        success = false;
    }
#else
    timeout.tv_sec = config_.write_timeout.count() / 1000;
    timeout.tv_usec = (config_.write_timeout.count() % 1000) * 1000;
    if (setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) != 0) {
        spdlog::warn("Failed to set SO_SNDTIMEO: {} ({})", strerror(errno), errno);
        success = false;
    }
#endif
    
    // Configure TCP keepalive if enabled
    if (config_.enable_tcp_keepalive) {
        if (!configureTCPKeepalive(socket)) {
            spdlog::warn("Failed to configure TCP keepalive");
            success = false;
        }
    }
    
    // Configure TCP nodelay if enabled
    if (config_.enable_tcp_nodelay) {
        if (!configureTCPNoDelay(socket)) {
            spdlog::warn("Failed to configure TCP nodelay");
            success = false;
        }
    }
    
    return success;
}

bool SocketTimeoutManager::setNonBlocking(socket_t socket) {
#ifdef _WIN32
    u_long mode = 1;
    if (ioctlsocket(socket, FIONBIO, &mode) != 0) {
        spdlog::error("Failed to set non-blocking mode: {}", WSAGetLastError());
        return false;
    }
#else
    int flags = fcntl(socket, F_GETFL, 0);
    if (flags == -1) {
        spdlog::error("Failed to get socket flags: {} ({})", strerror(errno), errno);
        return false;
    }
    if (fcntl(socket, F_SETFL, flags | O_NONBLOCK) == -1) {
        spdlog::error("Failed to set non-blocking mode: {} ({})", strerror(errno), errno);
        return false;
    }
#endif
    return true;
}

bool SocketTimeoutManager::configureTCPKeepalive(socket_t socket) {
    int enable = 1;
#ifdef _WIN32
    if (setsockopt(socket, SOL_SOCKET, SO_KEEPALIVE,
                   reinterpret_cast<const char*>(&enable), sizeof(enable)) != 0) {
        spdlog::error("Failed to enable TCP keepalive: {}", WSAGetLastError());
        return false;
    }
    
    // Windows-specific keepalive settings
    tcp_keepalive settings;
    settings.onoff = 1;
    settings.keepalivetime = static_cast<ULONG>(config_.keepalive_interval.count());
    settings.keepaliveinterval = 1000;  // 1 second between probes
    
    DWORD bytes_returned;
    if (WSAIoctl(socket, SIO_KEEPALIVE_VALS, &settings, sizeof(settings),
                 nullptr, 0, &bytes_returned, nullptr, nullptr) != 0) {
        spdlog::warn("Failed to configure keepalive parameters: {}", WSAGetLastError());
    }
#else
    if (setsockopt(socket, SOL_SOCKET, SO_KEEPALIVE, &enable, sizeof(enable)) != 0) {
        spdlog::error("Failed to enable TCP keepalive: {} ({})", strerror(errno), errno);
        return false;
    }
    
    // Linux/Unix-specific keepalive settings
    #ifdef TCP_KEEPIDLE
    int idle_time = static_cast<int>(config_.keepalive_interval.count() / 1000);
    setsockopt(socket, IPPROTO_TCP, TCP_KEEPIDLE, &idle_time, sizeof(idle_time));
    #endif
    
    #ifdef TCP_KEEPINTVL
    int interval = 1;  // 1 second between probes
    setsockopt(socket, IPPROTO_TCP, TCP_KEEPINTVL, &interval, sizeof(interval));
    #endif
    
    #ifdef TCP_KEEPCNT
    int count = 3;  // 3 probes before giving up
    setsockopt(socket, IPPROTO_TCP, TCP_KEEPCNT, &count, sizeof(count));
    #endif
#endif
    return true;
}

bool SocketTimeoutManager::configureTCPNoDelay(socket_t socket) {
    int enable = 1;
#ifdef _WIN32
    if (setsockopt(socket, IPPROTO_TCP, TCP_NODELAY,
                   reinterpret_cast<const char*>(&enable), sizeof(enable)) != 0) {
        spdlog::error("Failed to enable TCP_NODELAY: {}", WSAGetLastError());
        return false;
    }
#else
    if (setsockopt(socket, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(enable)) != 0) {
        spdlog::error("Failed to enable TCP_NODELAY: {} ({})", strerror(errno), errno);
        return false;
    }
#endif
    return true;
}

socket_t SocketTimeoutManager::acceptWithTimeout(socket_t server_socket,
                                                   std::chrono::milliseconds timeout_ms) {
    if (!shouldAcceptConnection()) {
        spdlog::warn("Circuit breaker is open, refusing new connections");
        return INVALID_SOCKET_VALUE;
    }
    
    auto timeout = timeout_ms.count() > 0 ? timeout_ms : config_.accept_timeout;
    
#ifdef _WIN32
    // Use select() for accept timeout on Windows
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(server_socket, &read_fds);
    
    struct timeval tv;
    tv.tv_sec = static_cast<long>(timeout.count() / 1000);
    tv.tv_usec = static_cast<long>((timeout.count() % 1000) * 1000);
    
    int result = select(0, &read_fds, nullptr, nullptr, &tv);
    if (result == 0) {
        // Timeout
        stats_.accept_timeouts++;
        recordTimeout();
        spdlog::debug("Accept timeout after {}ms", timeout.count());
        return INVALID_SOCKET_VALUE;
    } else if (result == SOCKET_ERROR) {
        spdlog::error("select() failed: {}", WSAGetLastError());
        stats_.failed_operations++;
        return INVALID_SOCKET_VALUE;
    }
    
    socket_t client_socket = accept(server_socket, nullptr, nullptr);
    if (client_socket == INVALID_SOCKET) {
        spdlog::error("accept() failed: {}", WSAGetLastError());
        stats_.failed_operations++;
        return INVALID_SOCKET_VALUE;
    }
    // RAII guard: ensures client_socket is closed on any exception or early
    // return before we explicitly release ownership back to the caller.
    auto sock_ptr = std::shared_ptr<socket_t>(new socket_t(client_socket),
        [](socket_t* s) {
            if (s && *s != static_cast<socket_t>(INVALID_SOCKET_VALUE)) {
                closesocket(*s);
            }
            delete s;
        });
#else
    // Use poll() for accept timeout on Unix
    struct pollfd pfd;
    pfd.fd = server_socket;
    pfd.events = POLLIN;
    
    int result = poll(&pfd, 1, static_cast<int>(timeout.count()));
    if (result == 0) {
        // Timeout
        stats_.accept_timeouts++;
        recordTimeout();
        spdlog::debug("Accept timeout after {}ms", timeout.count());
        return INVALID_SOCKET_VALUE;
    } else if (result < 0) {
        spdlog::error("poll() failed: {} ({})", strerror(errno), errno);
        stats_.failed_operations++;
        return INVALID_SOCKET_VALUE;
    }
    
    socket_t client_socket = accept(server_socket, nullptr, nullptr);
    if (client_socket < 0) {
        spdlog::error("accept() failed: {} ({})", strerror(errno), errno);
        stats_.failed_operations++;
        return INVALID_SOCKET_VALUE;
    }
    // RAII guard: ensures client_socket is closed on any exception or early
    // return before we explicitly release ownership back to the caller.
    auto sock_ptr = std::shared_ptr<socket_t>(new socket_t(client_socket),
        [](socket_t* s) {
            if (s && *s >= 0) {
                ::close(*s);
            }
            delete s;
        });
#endif
    
    // Configure the accepted socket
    if (!configureSocket(client_socket)) {
        spdlog::warn("Failed to configure accepted socket, continuing anyway");
    }
    
    recordSuccess();
    stats_.successful_operations++;
    spdlog::debug("Accepted connection successfully");
    // Transfer ownership to caller — socket will NOT be closed by sock_ptr.
    *sock_ptr = INVALID_SOCKET_VALUE;
    return client_socket;
}

ssize_t SocketTimeoutManager::readWithTimeout(socket_t socket, void* buffer, size_t size,
                                                [[maybe_unused]] std::chrono::milliseconds timeout_ms) {
    if (socket == INVALID_SOCKET_VALUE || buffer == nullptr || size == 0) {
        return -1;
    }
    
    // Socket is already configured with SO_RCVTIMEO, so recv() will timeout automatically
#ifdef _WIN32
    int bytes = recv(socket, static_cast<char*>(buffer), static_cast<int>(size), 0);
    if (bytes == SOCKET_ERROR) {
        int error = WSAGetLastError();
        if (error == WSAETIMEDOUT) {
            stats_.read_timeouts++;
            recordTimeout();
            spdlog::debug("Read timeout on socket");
        } else {
            spdlog::error("recv() failed: {}", error);
            stats_.failed_operations++;
        }
        return -1;
    }
#else
    ssize_t bytes = recv(socket, buffer, size, 0);
    if (bytes < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            stats_.read_timeouts++;
            recordTimeout();
            spdlog::debug("Read timeout on socket");
        } else {
            spdlog::error("recv() failed: {} ({})", strerror(errno), errno);
            stats_.failed_operations++;
        }
        return -1;
    }
#endif
    
    if (bytes > 0) {
        stats_.total_bytes_read += bytes;
        stats_.successful_operations++;
        recordSuccess();
    }
    
    return bytes;
}

ssize_t SocketTimeoutManager::writeWithTimeout(socket_t socket, const void* buffer, size_t size,
                                                 [[maybe_unused]] std::chrono::milliseconds timeout_ms) {
    if (socket == INVALID_SOCKET_VALUE || buffer == nullptr || size == 0) {
        return -1;
    }
    
    // Socket is already configured with SO_SNDTIMEO, so send() will timeout automatically
#ifdef _WIN32
    int bytes = send(socket, static_cast<const char*>(buffer), static_cast<int>(size), 0);
    if (bytes == SOCKET_ERROR) {
        int error = WSAGetLastError();
        if (error == WSAETIMEDOUT) {
            stats_.write_timeouts++;
            recordTimeout();
            spdlog::debug("Write timeout on socket");
        } else {
            spdlog::error("send() failed: {}", error);
            stats_.failed_operations++;
        }
        return -1;
    }
#else
    ssize_t bytes = send(socket, buffer, size, MSG_NOSIGNAL);  // MSG_NOSIGNAL prevents SIGPIPE
    if (bytes < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            stats_.write_timeouts++;
            recordTimeout();
            spdlog::debug("Write timeout on socket");
        } else {
            spdlog::error("send() failed: {} ({})", strerror(errno), errno);
            stats_.failed_operations++;
        }
        return -1;
    }
#endif
    
    if (bytes > 0) {
        stats_.total_bytes_written += bytes;
        stats_.successful_operations++;
        recordSuccess();
    }
    
    return bytes;
}

void SocketTimeoutManager::closeSocket(socket_t socket) {
    if (socket == INVALID_SOCKET_VALUE) {
        return;
    }
    
#ifdef _WIN32
    closesocket(socket);
#else
    close(socket);
#endif
    
    spdlog::debug("Socket closed");
}

bool SocketTimeoutManager::shouldAcceptConnection() const {
    if (health_state_ != SocketHealthState::CIRCUIT_OPEN) {
        return true;
    }
    
    // Check if reset timeout has elapsed
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_circuit_open_time_);
    return elapsed >= config_.reset_timeout;
}

void SocketTimeoutManager::recordTimeout() {
    consecutive_timeouts_++;
    updateHealthState();
}

void SocketTimeoutManager::recordSuccess() {
    consecutive_timeouts_ = 0;
    // Any successful operation resets timeout streak and should recover health state.
    updateHealthState();
}

void SocketTimeoutManager::updateHealthState() {
    SocketHealthState old_state = health_state_;
    size_t timeouts = consecutive_timeouts_.load();
    
    if (timeouts == 0) {
        health_state_ = SocketHealthState::HEALTHY;
    } else if (timeouts < config_.timeout_threshold / 2) {
        health_state_ = SocketHealthState::HEALTHY;
    } else if (timeouts < config_.timeout_threshold) {
        health_state_ = SocketHealthState::DEGRADED;
    } else {
        health_state_ = SocketHealthState::CIRCUIT_OPEN;
        last_circuit_open_time_ = std::chrono::steady_clock::now();
    }
    
    if (old_state != health_state_) {
        std::string state_str;
        switch (health_state_) {
            case SocketHealthState::HEALTHY: state_str = "HEALTHY"; break;
            case SocketHealthState::DEGRADED: state_str = "DEGRADED"; break;
            case SocketHealthState::CIRCUIT_OPEN: state_str = "CIRCUIT_OPEN"; break;
        }
        
        std::string message = "Network health state changed to " + state_str +
                             " (consecutive_timeouts=" + std::to_string(timeouts) + ")";
        spdlog::warn("{}", message);
        triggerAlert(health_state_, message);
    }
}

void SocketTimeoutManager::triggerAlert(SocketHealthState new_state, const std::string& message) {
    if ([[maybe_unused]] alert_callback_) {
        try {
            alert_callback_(new_state, message);
        } catch (const std::exception& e) {
            spdlog::error("Alert callback threw exception: {}", e.what());
        }
    }
}

} // namespace network
} // namespace themis

