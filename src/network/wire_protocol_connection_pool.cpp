/**
 * @file wire_protocol_connection_pool.cpp
 * @brief ThemisDB wire-protocol connection pool with TLS/mTLS support.
 *
 * Manages a per-target pool of reusable TCP (plain or TLS) connections for
 * outbound wire-protocol communication.  Key responsibilities:
 *  - Create, health-check, and recycle pooled connections.
 *  - Acquire connections with a deadline-bounded wait (acquire_timeout).
 *  - Prune stale connections in a background maintenance thread.
 *  - Graceful shutdown via timedJoin on the maintenance thread.
 *
 * @note thread_join_no_timeout (W3): maintenance_thread_.join() is replaced by
 *   timedJoin() to prevent indefinite block if the thread is stuck.
 *
 * @version 0.0.48
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 88/100
 * @note Gap Summary: total=4; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=1, Debt=0, C=0, H=17, M=4, L=0
 * @note Status: Production Ready
 */


// ThemisDB Wire Protocol Connection Pool Implementation

#include "network/wire_protocol_connection_pool.h"
#include "utils/logger.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <future>
#include <string_view>
#include <thread>
#include <openssl/x509v3.h>
#include <openssl/ssl.h>

namespace themis::network {

namespace {

uint64_t fnv1a64Pool(std::string_view value) {
    uint64_t hash = 1469598103934665603ULL;
    for (unsigned char ch : value) {
        hash ^= static_cast<uint64_t>(ch);
        hash *= 1099511628211ULL;
    }
    return hash;
}

std::string anonymizeTargetForLog(std::string_view target) {
    if (target.empty()) {
        return "target#unknown";
    }
    std::ostringstream oss;
    oss << "target#" << std::hex << fnv1a64Pool(target);
    return oss.str();
}

} // anonymous namespace (log helpers)

// =============================================================================
// File-local shutdown helpers
// =============================================================================

namespace {

/// Maximum ms to wait for the maintenance thread to join during destruction.
/// thread_join_no_timeout (W3): capped to prevent indefinite block.
constexpr int kPoolJoinTimeoutMs = 5000;

/// @brief Join @p t within @p timeout_ms; log and detach on timeout.
///
/// @param t         Thread to join (moved into the watcher).
/// @param label     Human-readable label for warning messages.
/// @param timeout_ms  Maximum wait time in milliseconds.
static void timedJoin(std::thread& t,
                      std::string_view label,
                      int timeout_ms = kPoolJoinTimeoutMs) noexcept {
    if (!t.joinable()) return;
    std::promise<void> done;
    auto fut = done.get_future();
    std::thread watcher([inner = std::move(t), p = std::move(done)]() mutable {
        if (inner.joinable()) inner.join();
        p.set_value();
    });
    watcher.detach();
    if (fut.wait_for(std::chrono::milliseconds(timeout_ms)) !=
            std::future_status::ready) {
        THEMIS_WARN("[WireProtocolConnectionPool] '{}' did not finish within {} ms; "
                    "detaching.", label, timeout_ms);
    }
}

} // anonymous namespace (shutdown helpers)

// =============================================================================
// AdaptivePoolingStrategy Implementation
// =============================================================================

AdaptivePoolingStrategy::AdaptivePoolingStrategy(const Config& config)
    : config_(config)
{}

AdaptivePoolingStrategy::AdaptivePoolingStrategy()
    : config_(Config{})
{}

size_t AdaptivePoolingStrategy::getIdealConnectionCount(
    size_t current_count,
    [[maybe_unused]] size_t active_count,
    double load)
{
    if (current_count == 0) return 1;

    if (load > config_.scale_up_threshold) {
        return static_cast<size_t>(
            std::ceil(static_cast<double>(current_count) * config_.scale_up_factor));
    }
    if (load < config_.scale_down_threshold && current_count > 1) {
        return std::max(size_t{1},
            static_cast<size_t>(
                std::floor(static_cast<double>(current_count) * config_.scale_down_factor)));
    }
    return current_count;
}

bool AdaptivePoolingStrategy::shouldCreateConnection(
    size_t current_count,
    size_t max_count,
    size_t available_count)
{
    if (current_count >= max_count) return false;
    if (current_count == 0) return true;
    // Create when the fraction of available (idle) connections is below the low-water mark
    double available_ratio = static_cast<double>(available_count)
                           / static_cast<double>(current_count);
    return available_ratio < (1.0 - config_.scale_up_threshold);
}

bool AdaptivePoolingStrategy::shouldRemoveConnection(
    size_t current_count,
    size_t min_count,
    size_t available_count,
    std::chrono::seconds idle_time)
{
    if (current_count <= min_count) return false;
    if (available_count == 0) return false;
    // Only remove connections that have been idle long enough
    return idle_time >= config_.min_idle_time;
}

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

    // Ensure an adaptive strategy object exists when adaptive sizing is enabled
    if (config_.enable_adaptive_sizing && !config_.adaptive_strategy) {
        config_.adaptive_strategy = std::make_shared<AdaptivePoolingStrategy>();
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
            if (config_.enable_adaptive_sizing) {
                adaptPoolSize();
            }
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
        // thread_join_no_timeout (W3): bounded join via timedJoin helper.
        timedJoin(maintenance_thread_, "maintenance_thread");
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
        bool connect_completed = false;
        timer.async_wait([&]([[maybe_unused]] const boost::system::error_code& error) {
            if (!error && !connect_completed) {
                timed_out = true;
                boost::system::error_code close_ec;
                plain_socket->cancel(close_ec);
                plain_socket->close(close_ec);
            }
        });
        
        // Attempt async connect
        net::async_connect(*plain_socket, endpoints,
            [&](const boost::system::error_code& error, const tcp::endpoint&) {
                connect_completed = true;
                ec = error;
                timer.cancel();
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
            ssl_stream->set_verify_callback([[maybe_unused]] ssl::host_name_verification(host));
            
            // Perform SSL handshake with timeout
            local_io.restart();
            timer.expires_after(config_.connect_timeout);
            
            timed_out = false;
            bool handshake_completed = false;
            timer.async_wait([&]([[maybe_unused]] const boost::system::error_code& error) {
                if (!error && !handshake_completed) {
                    timed_out = true;
                    boost::system::error_code shutdown_ec;
                    ssl_stream->lowest_layer().cancel(shutdown_ec);
                    ssl_stream->lowest_layer().close(shutdown_ec);
                }
            });
            
            ec.clear();
            ssl_stream->async_handshake(ssl::stream_base::client,
                [&]([[maybe_unused]] const boost::system::error_code& error) {
                    handshake_completed = true;
                    ec = error;
                    timer.cancel();
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
        
    } catch ([[maybe_unused]] const std::exception& e) {
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
    // Validate the target format before creating pool state or entering the
    // retry loop so malformed inputs fail fast with std::invalid_argument.
    static_cast<void>(parseTarget(target));

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
                
            } catch ([[maybe_unused]] const std::exception& e) {
                const auto now = std::chrono::steady_clock::now();
                if (now >= deadline) {
                    acquire_timeouts_.fetch_add(1, std::memory_order_relaxed);
                    throw std::runtime_error("Timeout acquiring connection for " + target);
                }
                const auto remaining = deadline - now;
                const auto backoff = std::min(
                    std::chrono::duration_cast<std::chrono::milliseconds>(remaining),
                    std::chrono::milliseconds(100));
                std::this_thread::sleep_for(backoff);
                lock.lock();
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
            THEMIS_WARN("[WireProtocolConnectionPool] Warmup failed for {}: {}",
                        anonymizeTargetForLog(target), e.what());
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

void WireProtocolConnectionPool::adaptPoolSize() {
    if (!config_.enable_adaptive_sizing || !config_.adaptive_strategy) return;

    // Collect current target names (under lock) to avoid holding pools_mutex_
    // while performing potentially slow createConnection() calls.
    std::vector<std::string> targets;
    {
        std::lock_guard<std::mutex> lock(pools_mutex_);
        targets.reserve(target_pools_.size());
        for (const auto& [t, _] : target_pools_) {
            targets.push_back(t);
        }
    }

    for (const auto& target : targets) {
        auto pool = getOrCreateTargetPool(target);

        size_t current_count;
        size_t active_count;
        size_t available_count;
        std::chrono::seconds oldest_idle{0};

        {
            std::lock_guard<std::mutex> pool_lock(pool->mutex);
            active_count    = pool->active_count;
            available_count = pool->available.size();
            current_count   = active_count + available_count;

            if (!pool->available.empty()) {
                auto now = std::chrono::steady_clock::now();
                oldest_idle = std::chrono::duration_cast<std::chrono::seconds>(
                    now - pool->available.front()->last_used);
            }
        }

        // --- Compute load and ideal connection count ---
        // load = fraction of connections currently in use (0.0 – 1.0).
        // The strategy returns an ideal total count; we clamp it to the
        // configured [min_connections_per_target, max_connections_per_target]
        // range before using it to drive scale-up / scale-down decisions.
        double load = (current_count > 0)
            ? static_cast<double>(active_count) / static_cast<double>(current_count)
            : 0.0;

        size_t ideal = config_.adaptive_strategy->getIdealConnectionCount(
            current_count, active_count, load);

        // Clamp ideal to configured limits
        ideal = std::max(ideal, config_.min_connections_per_target);
        ideal = std::min(ideal, config_.max_connections_per_target);

        // --- Scale up: pre-create a connection if the strategy recommends it ---
        if (ideal > current_count &&
            config_.adaptive_strategy->shouldCreateConnection(
                current_count, config_.max_connections_per_target, available_count)) {
            try {
                auto socket = createConnection(target);
                auto conn   = std::make_shared<PooledConnection>();
                conn->socket     = socket;
                conn->last_used  = std::chrono::steady_clock::now();
                conn->created_at = conn->last_used;
                conn->in_use     = false;

                {
                    std::lock_guard<std::mutex> pool_lock(pool->mutex);
                    void* key = socket.get();
                    pool->all_connections[key] = conn;
                    pool->available.push(conn);
                }

                total_connections_.fetch_add(1, std::memory_order_relaxed);
                connections_created_.fetch_add(1, std::memory_order_relaxed);
                pool_size_adaptations_.fetch_add(1, std::memory_order_relaxed);
                pool->cv.notify_one();
            } catch (const std::exception& e) {
                // Log but continue — scale-up failure is non-fatal
                THEMIS_WARN("[AdaptivePool] scale-up failed for {}: {}",
                            anonymizeTargetForLog(target), e.what());
            }
        }

        // --- Scale down: remove the oldest idle connection if eligible ---
        if (ideal < current_count &&
            config_.adaptive_strategy->shouldRemoveConnection(
                current_count, config_.min_connections_per_target,
                available_count, oldest_idle)) {
            std::lock_guard<std::mutex> pool_lock(pool->mutex);
            if (!pool->available.empty()) {
                auto conn = pool->available.front();
                pool->available.pop();
                if (conn->socket && conn->socket->is_open()) {
                    boost::system::error_code ec;
                    conn->socket->close(ec);
                }
                pool->all_connections.erase(conn->socket.get());
                total_connections_.fetch_sub(1, std::memory_order_relaxed);
                pool_size_adaptations_.fetch_add(1, std::memory_order_relaxed);
            }
        }
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
    stats.pool_size_adaptations = pool_size_adaptations_.load(std::memory_order_relaxed);
    
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

    // Compute overall utilization across all targets
    size_t total = in_use + available;
    stats.utilization = (total > 0)
        ? static_cast<double>(in_use) / static_cast<double>(total)
        : 0.0;
    
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
