/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            wire_protocol_connection_pool.cpp                  ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-02-21 07:18:13                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   94.0/100                                       ║
    • Total Lines:     595                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// ThemisDB Wire Protocol Connection Pool Implementation

#include "network/wire_protocol_connection_pool.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <openssl/x509v3.h>
#include <openssl/ssl.h>

namespace themis::network {

// =============================================================================
// SocketWrapper Implementation
// =============================================================================

SocketWrapper::SocketWrapper(std::shared_ptr<tcp::socket> plain_socket)
    : plain_socket_(std::move(plain_socket))
    , ssl_socket_(nullptr)
{}

SocketWrapper::SocketWrapper(std::shared_ptr<ssl::stream<tcp::socket>> ssl_socket)
    : plain_socket_(nullptr)
    , ssl_socket_(std::move(ssl_socket))
{}

bool SocketWrapper::is_open() const {
    if (plain_socket_) {
        return plain_socket_->is_open();
    } else if (ssl_socket_) {
        return ssl_socket_->lowest_layer().is_open();
    }
    return false;
}

void SocketWrapper::close(boost::system::error_code& ec) {
    if (plain_socket_) {
        plain_socket_->close(ec);
    } else if (ssl_socket_) {
        // For SSL, shutdown the SSL connection first
        ssl_socket_->shutdown(ec);
        if (!ec || ec == net::error::eof) {
            ssl_socket_->lowest_layer().close(ec);
        }
    }
}

// =============================================================================
// WireProtocolConnectionPool Implementation
// =============================================================================

WireProtocolConnectionPool::WireProtocolConnectionPool(const Config& config)
    : config_(config)
    , io_context_(std::make_shared<net::io_context>())
{
    // Initialize SSL context if TLS is enabled
    if (config_.enable_ssl || config_.enable_mtls) {
        initializeSSLContext();
    }
    
    // Start maintenance thread for pruning stale connections
    shutdown_.store(false, std::memory_order_release);
    maintenance_thread_ = std::thread([this]() {
        while (!shutdown_.load(std::memory_order_acquire)) {
            // Use interruptible wait for faster shutdown
            std::unique_lock<std::mutex> lock(shutdown_mutex_);
            if (shutdown_cv_.wait_for(lock, std::chrono::seconds(10), 
                [this]() { return shutdown_.load(std::memory_order_acquire); })) {
                break;  // Shutdown requested
            }
            pruneStaleConnections();
        }
    });
}

WireProtocolConnectionPool::WireProtocolConnectionPool()
    : WireProtocolConnectionPool(Config{})
{
}

WireProtocolConnectionPool::~WireProtocolConnectionPool() {
    shutdown_.store(true, std::memory_order_release);
    
    // Wake up maintenance thread for fast shutdown
    shutdown_cv_.notify_all();
    
    if (maintenance_thread_.joinable()) {
        maintenance_thread_.join();
    }
    
    clear();
}

std::pair<std::string, std::string> WireProtocolConnectionPool::parseTarget(const std::string& target) {
    auto colon_pos = target.find(':');
    if (colon_pos == std::string::npos) {
        throw std::invalid_argument("Invalid target format. Expected 'host:port'");
    }
    
    std::string host = target.substr(0, colon_pos);
    std::string port = target.substr(colon_pos + 1);
    
    return {host, port};
}

void WireProtocolConnectionPool::initializeSSLContext() {
    // Create SSL context with appropriate TLS version
    // Use TLS 1.2 or higher for client connections
    ssl_context_ = std::make_shared<ssl::context>(ssl::context::tlsv12_client);
    
    // Set SSL options for security
    ssl_context_->set_options(
        ssl::context::default_workarounds |
        ssl::context::no_sslv2 |
        ssl::context::no_sslv3 |
        ssl::context::no_tlsv1 |
        ssl::context::no_tlsv1_1 |
        ssl::context::single_dh_use
    );
    
    // Enable server certificate verification for SSL (not mTLS)
    if (config_.enable_ssl && !config_.enable_mtls) {
        ssl_context_->set_verify_mode(ssl::verify_peer);
        
        // Load CA certificates for server verification
        if (!config_.ssl_ca_cert_path.empty()) {
            ssl_context_->load_verify_file(config_.ssl_ca_cert_path);
        } else {
            // Use system default CA certificates
            ssl_context_->set_default_verify_paths();
        }
    }
    
    // Configure mTLS (mutual TLS) - client presents certificate to server
    if (config_.enable_mtls) {
        // Verify server certificate (verify_fail_if_no_peer_cert is for servers, not needed here)
        ssl_context_->set_verify_mode(ssl::verify_peer);
        
        // Load CA certificate for server verification
        if (config_.ssl_ca_cert_path.empty()) {
            throw std::runtime_error("mTLS requires CA certificate path (ssl_ca_cert_path)");
        }
        ssl_context_->load_verify_file(config_.ssl_ca_cert_path);
        
        // Load client certificate and private key
        if (config_.ssl_cert_path.empty() || config_.ssl_key_path.empty()) {
            throw std::runtime_error(
                "mTLS requires client certificate path (ssl_cert_path) and "
                "private key path (ssl_key_path)"
            );
        }
        
        ssl_context_->use_certificate_chain_file(config_.ssl_cert_path);
        ssl_context_->use_private_key_file(config_.ssl_key_path, ssl::context::pem);
    }
}

std::shared_ptr<SocketWrapper> WireProtocolConnectionPool::createConnection(const std::string& target) {
    auto [host, port] = parseTarget(target);
    
    try {
        // Resolve endpoint
        tcp::resolver resolver(*io_context_);
        auto endpoints = resolver.resolve(host, port);
        
        // Create a fresh io_context for this connection
        net::io_context local_io;
        
        boost::system::error_code ec;
        
        // Create plain socket first for connection
        auto plain_socket = std::make_shared<tcp::socket>(local_io);
        
        // Set up deadline timer
        net::steady_timer timer(local_io);
        timer.expires_after(config_.connect_timeout);
        
        bool timed_out = false;
        timer.async_wait([&](const boost::system::error_code& error) {
            if (!error) {
                timed_out = true;
                plain_socket->close(ec);
            }
        });
        
        // Attempt async connect
        net::async_connect(*plain_socket, endpoints,
            [&ec](const boost::system::error_code& error, const tcp::endpoint&) {
                ec = error;
            });
        
        // Run until connect completes or timeout
        local_io.run();
        
        if (timed_out || ec) {
            failed_connections_.fetch_add(1, std::memory_order_relaxed);
            if (timed_out) {
                auto timeout_seconds = std::chrono::duration_cast<std::chrono::seconds>(config_.connect_timeout).count();
                throw std::runtime_error("Connection timed out after " + 
                    std::to_string(timeout_seconds) + " seconds");
            }
            throw std::runtime_error("Connection failed: " + ec.message());
        }
        
        // Set socket options
        plain_socket->set_option(tcp::no_delay(true));
        plain_socket->set_option(net::socket_base::keep_alive(true));
        
        std::shared_ptr<SocketWrapper> wrapper;
        
        // If SSL is enabled, wrap the socket and perform handshake
        if (config_.enable_ssl || config_.enable_mtls) {
            if (!ssl_context_) {
                throw std::runtime_error("SSL context not initialized");
            }
            
            auto ssl_stream = std::make_shared<ssl::stream<tcp::socket>>(
                std::move(*plain_socket), *ssl_context_
            );
            
            // Set SNI hostname for proper certificate verification
            if (!SSL_set_tlsext_host_name(ssl_stream->native_handle(), host.c_str())) {
                throw std::runtime_error("Failed to set SNI hostname");
            }
            
            // Set hostname verification callback for proper TLS validation
            ssl_stream->set_verify_callback(ssl::host_name_verification(host));
            
            // Perform SSL handshake with timeout
            local_io.restart();
            timer.expires_after(config_.connect_timeout);
            
            timed_out = false;
            timer.async_wait([&](const boost::system::error_code& error) {
                if (!error) {
                    timed_out = true;
                    boost::system::error_code shutdown_ec;
                    ssl_stream->lowest_layer().close(shutdown_ec);
                }
            });
            
            ec.clear();
            ssl_stream->async_handshake(ssl::stream_base::client,
                [&ec](const boost::system::error_code& error) {
                    ec = error;
                });
            
            local_io.run();
            
            if (timed_out || ec) {
                failed_connections_.fetch_add(1, std::memory_order_relaxed);
                if (timed_out) {
                    auto timeout_seconds = std::chrono::duration_cast<std::chrono::seconds>(config_.connect_timeout).count();
                    throw std::runtime_error("SSL handshake timed out after " + 
                        std::to_string(timeout_seconds) + " seconds");
                }
                throw std::runtime_error("SSL handshake failed: " + ec.message());
            }
            
            wrapper = std::make_shared<SocketWrapper>(ssl_stream);
        } else {
            wrapper = std::make_shared<SocketWrapper>(plain_socket);
        }
        
        connections_created_.fetch_add(1, std::memory_order_relaxed);
        total_connections_.fetch_add(1, std::memory_order_relaxed);
        
        return wrapper;
        
    } catch (const std::exception& e) {
        // Only count failures once (already counted above before throw)
        throw;
    }
}

std::shared_ptr<WireProtocolConnectionPool::TargetPool> 
WireProtocolConnectionPool::getOrCreateTargetPool(const std::string& target) {
    std::lock_guard<std::mutex> lock(pools_mutex_);
    
    auto it = target_pools_.find(target);
    if (it != target_pools_.end()) {
        return it->second;
    }
    
    auto pool = std::make_shared<TargetPool>();
    target_pools_[target] = pool;
    return pool;
}

WireProtocolConnectionPool::ConnectionHandle 
WireProtocolConnectionPool::acquireConnection(const std::string& target) {
    auto pool = getOrCreateTargetPool(target);
    std::unique_lock<std::mutex> lock(pool->mutex);
    
    auto deadline = std::chrono::steady_clock::now() + config_.acquire_timeout;
    
    while (true) {
        // Try to get available connection
        while (!pool->available.empty()) {
            auto conn = pool->available.front();
            pool->available.pop();
            
            // Check if connection is still healthy
            if (conn->isHealthy() && !conn->isStale(config_.idle_timeout)) {
                conn->in_use = true;
                conn->last_used = std::chrono::steady_clock::now();
                pool->active_count++;
                
                connections_reused_.fetch_add(1, std::memory_order_relaxed);
                
                return ConnectionHandle(conn->socket, this, target);
            } else {
                // Close socket before removing to avoid leaks
                if (conn->socket && conn->socket->is_open()) {
                    boost::system::error_code ec;
                    conn->socket->close(ec);
                }
                // Remove stale/unhealthy connection
                pool->all_connections.erase(conn->socket.get());
                total_connections_.fetch_sub(1, std::memory_order_relaxed);
                stale_removed_.fetch_add(1, std::memory_order_relaxed);
            }
        }
        
        // Try to create new connection if under limit
        if (pool->active_count < config_.max_connections_per_target) {
            lock.unlock();
            
            try {
                auto socket = createConnection(target);
                auto conn = std::make_shared<PooledConnection>();
                conn->socket = socket;
                conn->last_used = std::chrono::steady_clock::now();
                conn->created_at = std::chrono::steady_clock::now();
                conn->in_use = true;
                
                lock.lock();
                void* socket_key = socket.get();
                pool->all_connections[socket_key] = conn;
                pool->active_count++;
                
                return ConnectionHandle(socket, this, target);
                
            } catch (const std::exception& e) {
                lock.lock();
                // Add backoff before retry to avoid tight loop under outage
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
        
        // Wait for connection to become available or timeout
        if (std::chrono::steady_clock::now() >= deadline) {
            acquire_timeouts_.fetch_add(1, std::memory_order_relaxed);
            throw std::runtime_error("Timeout acquiring connection for " + target);
        }
        
        // Wait with deadline for a connection to become available
        auto wait_result = pool->cv.wait_until(lock, deadline);
        if (wait_result == std::cv_status::timeout) {
            acquire_timeouts_.fetch_add(1, std::memory_order_relaxed);
            throw std::runtime_error("Timeout acquiring connection for " + target);
        }
    }
}

void WireProtocolConnectionPool::releaseConnection(
    const std::string& target, 
    std::shared_ptr<SocketWrapper> socket) 
{
    auto pool = getOrCreateTargetPool(target);
    std::lock_guard<std::mutex> lock(pool->mutex);
    
    // Use raw pointer as key (works for both plain and SSL sockets)
    void* socket_key = socket.get();
    
    auto it = pool->all_connections.find(socket_key);
    if (it != pool->all_connections.end()) {
        auto conn = it->second;
        conn->in_use = false;
        conn->last_used = std::chrono::steady_clock::now();
        
        if (conn->isHealthy()) {
            pool->available.push(conn);
        } else {
            // Close socket before removing to avoid leaks
            if (conn->socket && conn->socket->is_open()) {
                boost::system::error_code ec;
                conn->socket->close(ec);
            }
            // Remove unhealthy connection
            pool->all_connections.erase(it);
            total_connections_.fetch_sub(1, std::memory_order_relaxed);
        }
        
        if (pool->active_count > 0) {
            pool->active_count--;
        }
    }
    
    pool->cv.notify_one();
}

void WireProtocolConnectionPool::warmup(const std::string& target) {
    if (!config_.enable_warmup) {
        return;
    }
    
    auto pool = getOrCreateTargetPool(target);
    
    // Create minimum connections OUTSIDE the lock to avoid blocking
    for (size_t i = 0; i < config_.min_connections_per_target; ++i) {
        try {
            // Perform potentially slow network connection outside the pool mutex
            auto socket = createConnection(target);
            auto conn = std::make_shared<PooledConnection>();
            conn->socket = socket;
            conn->last_used = std::chrono::steady_clock::now();
            conn->created_at = std::chrono::steady_clock::now();
            conn->in_use = false;
            
            // Only lock to update shared pool state
            {
                std::lock_guard<std::mutex> lock(pool->mutex);
                void* socket_key = socket.get();
                pool->all_connections[socket_key] = conn;
                pool->available.push(conn);
            }
            
        } catch (const std::exception& e) {
            // Log error but continue warmup
            std::cerr << "[WireProtocolConnectionPool] Warmup failed for " << target 
                      << ": " << e.what() << std::endl;
        }
    }
}

void WireProtocolConnectionPool::pruneStaleConnections() {
    std::lock_guard<std::mutex> pools_lock(pools_mutex_);
    
    for (auto& [target, pool] : target_pools_) {
        std::lock_guard<std::mutex> lock(pool->mutex);
        
        // Remove stale connections from available queue
        std::queue<std::shared_ptr<PooledConnection>> fresh_queue;
        
        while (!pool->available.empty()) {
            auto conn = pool->available.front();
            pool->available.pop();
            
            if (conn->isHealthy() && !conn->isStale(config_.idle_timeout)) {
                fresh_queue.push(conn);
            } else {
                // Close socket before removing to avoid leaks
                if (conn->socket && conn->socket->is_open()) {
                    boost::system::error_code ec;
                    conn->socket->close(ec);
                }
                void* socket_key = conn->socket.get();
                pool->all_connections.erase(socket_key);
                total_connections_.fetch_sub(1, std::memory_order_relaxed);
                stale_removed_.fetch_add(1, std::memory_order_relaxed);
            }
        }
        
        pool->available = std::move(fresh_queue);
    }
}

void WireProtocolConnectionPool::clear() {
    std::lock_guard<std::mutex> lock(pools_mutex_);
    
    for (auto& [target, pool] : target_pools_) {
        std::lock_guard<std::mutex> pool_lock(pool->mutex);
        
        // Close all connections
        for (auto& [socket_ptr, conn] : pool->all_connections) {
            if (conn->socket && conn->socket->is_open()) {
                boost::system::error_code ec;
                conn->socket->close(ec);
            }
        }
        
        pool->all_connections.clear();
        while (!pool->available.empty()) {
            pool->available.pop();
        }
        pool->active_count = 0;
    }
    
    target_pools_.clear();
    total_connections_.store(0, std::memory_order_release);
}

WireProtocolConnectionPool::Stats WireProtocolConnectionPool::getStats() const {
    Stats stats;
    stats.total_connections = total_connections_.load(std::memory_order_relaxed);
    stats.connections_created = connections_created_.load(std::memory_order_relaxed);
    stats.connections_reused = connections_reused_.load(std::memory_order_relaxed);
    stats.stale_connections_removed = stale_removed_.load(std::memory_order_relaxed);
    stats.failed_connections = failed_connections_.load(std::memory_order_relaxed);
    stats.acquire_timeouts = acquire_timeouts_.load(std::memory_order_relaxed);
    stats.keepalive_checks_sent = keepalive_checks_.load(std::memory_order_relaxed);
    
    std::lock_guard<std::mutex> lock(pools_mutex_);
    
    size_t available = 0;
    size_t in_use = 0;
    
    for (const auto& [target, pool] : target_pools_) {
        std::lock_guard<std::mutex> pool_lock(pool->mutex);
        available += pool->available.size();
        in_use += pool->active_count;
    }
    
    stats.available_connections = available;
    stats.in_use_connections = in_use;
    
    return stats;
}

bool WireProtocolConnectionPool::performHealthCheck(SocketWrapper& socket) {
    if (!socket.is_open()) {
        return false;
    }
    
    boost::system::error_code ec;
    
    // Try to read with MSG_PEEK to check if connection is alive
    char dummy;
    
    if (socket.is_ssl()) {
        // For SSL sockets, we can't use MSG_PEEK directly
        // Just check if the socket is open
        keepalive_checks_.fetch_add(1, std::memory_order_relaxed);
        return socket.is_open();
    } else {
        // For plain sockets, use MSG_PEEK
        socket.plain_socket()->receive(net::buffer(&dummy, 1), tcp::socket::message_peek, ec);
        
        if (ec == net::error::would_block || ec == net::error::try_again) {
            // No data available, but connection is alive
            return true;
        }
        
        keepalive_checks_.fetch_add(1, std::memory_order_relaxed);
        
        return !ec && socket.is_open();
    }
}

// =============================================================================
// ConnectionHandle Implementation
// =============================================================================

WireProtocolConnectionPool::ConnectionHandle::ConnectionHandle(
    std::shared_ptr<SocketWrapper> socket,
    WireProtocolConnectionPool* pool,
    const std::string& target)
    : socket_(std::move(socket))
    , pool_(pool)
    , target_(target)
{}

WireProtocolConnectionPool::ConnectionHandle::~ConnectionHandle() {
    if (socket_ && pool_) {
        pool_->releaseConnection(target_, socket_);
    }
}

WireProtocolConnectionPool::ConnectionHandle::ConnectionHandle(ConnectionHandle&& other) noexcept
    : socket_(std::move(other.socket_))
    , pool_(other.pool_)
    , target_(std::move(other.target_))
{
    other.pool_ = nullptr;
}

WireProtocolConnectionPool::ConnectionHandle& 
WireProtocolConnectionPool::ConnectionHandle::operator=(ConnectionHandle&& other) noexcept {
    if (this != &other) {
        // Release current connection
        if (socket_ && pool_) {
            pool_->releaseConnection(target_, socket_);
        }
        
        // Move from other
        socket_ = std::move(other.socket_);
        pool_ = other.pool_;
        target_ = std::move(other.target_);
        
        other.pool_ = nullptr;
    }
    return *this;
}

} // namespace themis::network
