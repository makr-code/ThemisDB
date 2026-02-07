// ThemisDB Wire Protocol Connection Pool Implementation

#include "network/wire_protocol_connection_pool.h"
#include <iostream>
#include <sstream>
#include <algorithm>

namespace themis::network {

// =============================================================================
// WireProtocolConnectionPool Implementation
// =============================================================================

WireProtocolConnectionPool::WireProtocolConnectionPool(const Config& config)
    : config_(config)
    , io_context_(std::make_shared<net::io_context>())
{
    // NOTE: TLS/mTLS is not currently implemented for this connection pool.
    // Reject configurations that attempt to enable it to avoid a false sense of security.
    if (config_.enable_ssl || config_.enable_mtls) {
        throw std::runtime_error(
            "WireProtocolConnectionPool does not yet support SSL/mTLS. "
            "Disable 'enable_ssl' and 'enable_mtls' in the configuration. "
            "TLS support is planned for a future release."
        );
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

std::shared_ptr<tcp::socket> WireProtocolConnectionPool::createConnection(const std::string& target) {
    auto [host, port] = parseTarget(target);
    
    auto socket = std::make_shared<tcp::socket>(*io_context_);
    
    try {
        // Resolve endpoint
        tcp::resolver resolver(*io_context_);
        auto endpoints = resolver.resolve(host, port);
        
        // Use synchronous connect with timeout
        // Create a fresh io_context for this connection
        net::io_context local_io;
        auto local_socket = std::make_shared<tcp::socket>(local_io);
        
        boost::system::error_code ec;
        
        // Set up deadline timer
        net::steady_timer timer(local_io);
        timer.expires_after(config_.connect_timeout);
        
        bool timed_out = false;
        timer.async_wait([&](const boost::system::error_code& error) {
            if (!error) {
                timed_out = true;
                local_socket->close(ec);
            }
        });
        
        // Attempt async connect
        net::async_connect(*local_socket, endpoints,
            [&ec](const boost::system::error_code& error, const tcp::endpoint&) {
                ec = error;
            });
        
        // Run until connect completes or timeout
        local_io.run();
        
        if (timed_out || ec) {
            failed_connections_.fetch_add(1, std::memory_order_relaxed);
            if (timed_out) {
                throw std::runtime_error("Connection timed out after " + 
                    std::to_string(config_.connect_timeout.count()) + " seconds");
            }
            throw std::runtime_error("Connection failed: " + ec.message());
        }
        
        // Transfer the connected socket to our pool's io_context
        // Since we can't transfer sockets between contexts, create new socket and connect
        socket = local_socket;  // Use the successfully connected socket
        
        // Set socket options
        socket->set_option(tcp::no_delay(true));
        socket->set_option(net::socket_base::keep_alive(true));
        
        connections_created_.fetch_add(1, std::memory_order_relaxed);
        total_connections_.fetch_add(1, std::memory_order_relaxed);
        
        return socket;
        
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
                pool->all_connections[socket.get()] = conn;
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
    std::shared_ptr<tcp::socket> socket) 
{
    auto pool = getOrCreateTargetPool(target);
    std::lock_guard<std::mutex> lock(pool->mutex);
    
    auto it = pool->all_connections.find(socket.get());
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
                pool->all_connections[socket.get()] = conn;
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
                pool->all_connections.erase(conn->socket.get());
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

bool WireProtocolConnectionPool::performHealthCheck(tcp::socket& socket) {
    if (!socket.is_open()) {
        return false;
    }
    
    boost::system::error_code ec;
    
    // Try to read with MSG_PEEK to check if connection is alive
    char dummy;
    socket.receive(net::buffer(&dummy, 1), tcp::socket::message_peek, ec);
    
    if (ec == net::error::would_block || ec == net::error::try_again) {
        // No data available, but connection is alive
        return true;
    }
    
    keepalive_checks_.fetch_add(1, std::memory_order_relaxed);
    
    return !ec && socket.is_open();
}

// =============================================================================
// ConnectionHandle Implementation
// =============================================================================

WireProtocolConnectionPool::ConnectionHandle::ConnectionHandle(
    std::shared_ptr<tcp::socket> socket,
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
