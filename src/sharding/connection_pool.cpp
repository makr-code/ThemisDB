// Copyright 2025 ThemisDB
// Licensed under MIT License

#include "sharding/connection_pool.h"

namespace themisdb {
namespace sharding {

ConnectionPool::ConnectionPool(const Config& config)
    : config_(config) {
    // Pre-create initial connections
    for (size_t i = 0; i < config_.initial_size; ++i) {
        if (config_.create_connection) {
            Connection conn = createConnection();
            if (conn.handle) {
                available_.push(conn);
                idle_times_[conn] = std::chrono::system_clock::now();
            }
        }
    }
}

ConnectionPool::~ConnectionPool() {
    shutdown();
}

std::optional<Connection> ConnectionPool::acquire(std::chrono::milliseconds timeout) {
    std::unique_lock<std::mutex> lock(mutex_);
    
    auto deadline = std::chrono::steady_clock::now() + timeout;
    
    while (true) {
        if (shutdown_) {
            return std::nullopt;
        }
        
        // Try to get from available pool
        if (!available_.empty()) {
            Connection conn = available_.front();
            available_.pop();
            idle_times_.erase(conn);
            
            // Update last used time
            conn.last_used = std::chrono::system_clock::now();
            in_use_.insert(conn);
            return conn;
        }
        
        // Try to create new connection if under limit
        if (canCreateMore()) {
            lock.unlock();
            Connection conn = createConnection();
            lock.lock();
            
            if (conn.handle) {
                in_use_.insert(conn);
                return conn;
            }
        }
        
        // Wait for connection to become available
        if (cv_.wait_until(lock, deadline) == std::cv_status::timeout) {
            ++timeout_errors_;
            return std::nullopt;
        }
    }
}

void ConnectionPool::release(Connection conn) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = in_use_.find(conn);
    if (it == in_use_.end()) {
        // Connection not from this pool
        return;
    }
    
    in_use_.erase(it);
    available_.push(conn);
    idle_times_[conn] = std::chrono::system_clock::now();
    
    cv_.notify_one();
}

size_t ConnectionPool::closeIdleConnections() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto now = std::chrono::system_clock::now();
    size_t closed = 0;
    
    std::queue<Connection> new_available;
    
    while (!available_.empty()) {
        Connection conn = available_.front();
        available_.pop();
        
        auto idle_it = idle_times_.find(conn);
        if (idle_it != idle_times_.end()) {
            auto idle_duration = now - idle_it->second;
            if (idle_duration > config_.idle_timeout) {
                // Close idle connection
                destroyConnection(conn);
                idle_times_.erase(idle_it);
                ++closed;
                continue;
            }
        }
        
        new_available.push(conn);
    }
    
    available_ = std::move(new_available);
    return closed;
}

ConnectionPool::Stats ConnectionPool::getStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    return Stats{
        available_.size() + in_use_.size(),
        available_.size(),
        in_use_.size(),
        created_total_,
        closed_total_,
        timeout_errors_
    };
}

void ConnectionPool::shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);
    shutdown_ = true;
    
    // Close all available connections
    while (!available_.empty()) {
        Connection conn = available_.front();
        available_.pop();
        destroyConnection(conn);
    }
    
    // Close all in-use connections
    for (const Connection& conn : in_use_) {
        destroyConnection(conn);
    }
    in_use_.clear();
    idle_times_.clear();
    
    cv_.notify_all();
}

Connection ConnectionPool::createConnection() {
    if (config_.create_connection) {
        Connection conn = config_.create_connection();
        if (conn.handle) {
            ++created_total_;
        }
        return conn;
    }
    return Connection{};
}

void ConnectionPool::destroyConnection(Connection conn) {
    if (config_.destroy_connection && conn.handle) {
        config_.destroy_connection(conn);
        ++closed_total_;
    }
}

bool ConnectionPool::canCreateMore() const {
    size_t total = available_.size() + in_use_.size();
    return total < config_.max_size;
}

} // namespace sharding
} // namespace themisdb
