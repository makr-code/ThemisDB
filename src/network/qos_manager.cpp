/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            qos_manager.cpp                                    ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-02-21 10:41:47                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     414                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b4dc54fdd  2026-02-20  Network module: QoS manager with token bucket, backpressu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// ThemisDB Network QoS Manager – Implementation
// Token Bucket rate limiting, Priority Queues, and Backpressure control

#include "network/qos_manager.h"

#include <algorithm>
#include <thread>

namespace themis {
namespace network {

// =============================================================================
// TokenBucket
// =============================================================================

TokenBucket::TokenBucket(uint64_t rate_bps, uint64_t burst_bytes)
    : tokens_(static_cast<double>(burst_bytes))
    , rate_bps_(rate_bps)
    , burst_bytes_(burst_bytes)
    , last_refill_(std::chrono::steady_clock::now())
{}

void TokenBucket::refill() {
    auto now     = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration<double>(now - last_refill_).count();
    last_refill_ = now;

    if (rate_bps_ > 0) {
        double rate_bytes_per_sec = static_cast<double>(rate_bps_) / 8.0;
        tokens_ += elapsed * rate_bytes_per_sec;
        tokens_  = std::min(tokens_, static_cast<double>(burst_bytes_));
    } else {
        // Unlimited – always full
        tokens_ = static_cast<double>(burst_bytes_);
    }
}

bool TokenBucket::tryConsume(uint64_t bytes) {
    std::lock_guard<std::mutex> lock(mutex_);
    refill();

    if (rate_bps_ == 0) {
        // Unlimited bandwidth
        return true;
    }

    if (tokens_ >= static_cast<double>(bytes)) {
        tokens_ -= static_cast<double>(bytes);
        return true;
    }
    return false;
}

bool TokenBucket::consume(uint64_t bytes, std::chrono::milliseconds timeout) {
    auto deadline = std::chrono::steady_clock::now() + timeout;

    while (true) {
        if (tryConsume(bytes)) {
            return true;
        }

        auto now = std::chrono::steady_clock::now();
        if (timeout.count() == 0 || now >= deadline) {
            return false;
        }

        // Sleep for a short interval proportional to the deficit
        double rate_bytes_per_sec;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            rate_bytes_per_sec = static_cast<double>(rate_bps_) / 8.0;
        }

        double deficit = static_cast<double>(bytes) - availableBytes();
        if (rate_bytes_per_sec > 0.0 && deficit > 0.0) {
            auto wait_ms = static_cast<int64_t>((deficit / rate_bytes_per_sec) * 1000.0);
            wait_ms      = std::max<int64_t>(1, std::min(wait_ms, static_cast<int64_t>(10)));
            std::this_thread::sleep_for(std::chrono::milliseconds(wait_ms));
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
}

void TokenBucket::reconfigure(uint64_t rate_bps, uint64_t burst_bytes) {
    std::lock_guard<std::mutex> lock(mutex_);
    rate_bps_    = rate_bps;
    burst_bytes_ = burst_bytes;
    // Clamp current tokens to new burst
    tokens_ = std::min(tokens_, static_cast<double>(burst_bytes_));
}

double TokenBucket::availableBytes() const {
    std::lock_guard<std::mutex> lock(mutex_);
    // Perform a non-destructive refill estimate
    auto now     = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration<double>(now - last_refill_).count();

    if (rate_bps_ == 0) {
        return static_cast<double>(burst_bytes_);
    }

    double rate_bytes_per_sec = static_cast<double>(rate_bps_) / 8.0;
    double estimated          = tokens_ + elapsed * rate_bytes_per_sec;
    return std::min(estimated, static_cast<double>(burst_bytes_));
}

uint64_t TokenBucket::rateBps() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return rate_bps_;
}

uint64_t TokenBucket::burstBytes() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return burst_bytes_;
}

// =============================================================================
// QoSManager
// =============================================================================

QoSManager::QoSManager(const Config& config)
    : config_(config)
{
    // Initialise per-priority counters
    bytes_per_priority_[Priority::CRITICAL] = 0;
    bytes_per_priority_[Priority::HIGH]     = 0;
    bytes_per_priority_[Priority::MEDIUM]   = 0;
    bytes_per_priority_[Priority::LOW]      = 0;
}

QoSManager::QoSManager()
    : QoSManager(Config{})
{}

QoSManager::~QoSManager() = default;

// -------------------------------------------------------------------------
// Internal helpers
// -------------------------------------------------------------------------

std::shared_ptr<QoSManager::ConnectionState>
QoSManager::findConnection(uint64_t id) const {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    auto it = connections_.find(id);
    if (it == connections_.end()) {
        return nullptr;
    }
    return it->second;
}

// -------------------------------------------------------------------------
// Connection lifecycle
// -------------------------------------------------------------------------

void QoSManager::registerConnection(uint64_t connection_id, Priority priority) {
    auto state            = std::make_shared<ConnectionState>();
    state->connection_id  = connection_id;
    state->priority.store(static_cast<uint8_t>(priority), std::memory_order_relaxed);

    if (config_.default_rate_bps > 0) {
        uint64_t burst = config_.default_burst_bytes > 0
                             ? config_.default_burst_bytes
                             : config_.default_rate_bps / 8;  // 1 s worth of data
        state->token_bucket = std::make_shared<TokenBucket>(config_.default_rate_bps, burst);
    }

    std::lock_guard<std::mutex> lock(connections_mutex_);
    connections_[connection_id] = std::move(state);
}

void QoSManager::unregisterConnection(uint64_t connection_id) {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    connections_.erase(connection_id);
}

// -------------------------------------------------------------------------
// Per-connection controls
// -------------------------------------------------------------------------

void QoSManager::setPriority(uint64_t connection_id, Priority priority) {
    auto state = findConnection(connection_id);
    if (!state) {
        return;
    }
    state->priority.store(static_cast<uint8_t>(priority), std::memory_order_relaxed);
}

void QoSManager::setTokenBucket(uint64_t connection_id,
                                 uint64_t rate_bps,
                                 uint64_t burst_bytes) {
    auto state = findConnection(connection_id);
    if (!state) {
        return;
    }

    uint64_t burst = burst_bytes > 0 ? burst_bytes : config_.default_burst_bytes;
    if (burst == 0 && rate_bps > 0) {
        burst = rate_bps / 8;  // Default burst = 1 second of sustained rate
    }

    std::lock_guard<std::mutex> lock(state->token_bucket_mutex);
    if (state->token_bucket) {
        state->token_bucket->reconfigure(rate_bps, burst);
    } else {
        state->token_bucket = std::make_shared<TokenBucket>(rate_bps, burst);
    }
}

void QoSManager::clearTokenBucket(uint64_t connection_id) {
    auto state = findConnection(connection_id);
    if (!state) {
        return;
    }
    std::lock_guard<std::mutex> lock(state->token_bucket_mutex);
    state->token_bucket.reset();
}

// -------------------------------------------------------------------------
// Hot-path
// -------------------------------------------------------------------------

bool QoSManager::allowSend(uint64_t connection_id,
                            uint64_t bytes,
                            std::chrono::milliseconds timeout) {
    auto state = findConnection(connection_id);
    if (!state) {
        // Unknown connection – allow by default
        return true;
    }

    // --- Backpressure check ---
    if (config_.max_queue_bytes > 0 &&
        state->queue_depth.load(std::memory_order_relaxed) + bytes >
            config_.max_queue_bytes) {
        state->backpressure_events.fetch_add(1, std::memory_order_relaxed);
        total_backpressure_events_.fetch_add(1, std::memory_order_relaxed);

        // Fire callback if set
        {
            std::lock_guard<std::mutex> cb_lock(callback_mutex_);
            if (backpressure_cb_) {
                backpressure_cb_(connection_id, bytes);
            }
        }
        return false;
    }

    // --- Token bucket check ---
    // Snapshot the bucket pointer under the lock, then release before blocking.
    // TokenBucket is internally thread-safe, so calling consume/tryConsume on
    // the snapshot without holding token_bucket_mutex is safe.
    std::shared_ptr<TokenBucket> bucket;
    {
        std::lock_guard<std::mutex> tb_lock(state->token_bucket_mutex);
        bucket = state->token_bucket;
    }
    if (bucket) {
        bool ok = (timeout.count() > 0)
                      ? bucket->consume(bytes, timeout)
                      : bucket->tryConsume(bytes);

        if (!ok) {
            state->bytes_shaped.fetch_add(bytes, std::memory_order_relaxed);
            total_bytes_shaped_.fetch_add(bytes, std::memory_order_relaxed);
            return false;
        }
    }

    // Reserve queue space
    state->queue_depth.fetch_add(bytes, std::memory_order_relaxed);
    return true;
}

void QoSManager::recordBytesSent(uint64_t connection_id, uint64_t bytes) {
    auto state = findConnection(connection_id);
    if (!state) {
        return;
    }

    state->bytes_sent.fetch_add(bytes, std::memory_order_relaxed);

    // Release queue reservation
    uint64_t current = state->queue_depth.load(std::memory_order_relaxed);
    uint64_t release  = std::min(current, bytes);
    state->queue_depth.fetch_sub(release, std::memory_order_relaxed);

    total_bytes_sent_.fetch_add(bytes, std::memory_order_relaxed);

    // Update per-priority counter
    {
        std::lock_guard<std::mutex> lock(priority_stats_mutex_);
        bytes_per_priority_[static_cast<Priority>(state->priority.load(std::memory_order_relaxed))] += bytes;
    }
}

void QoSManager::recordBytesReceived(uint64_t connection_id, uint64_t bytes) {
    auto state = findConnection(connection_id);
    if (!state) {
        return;
    }

    state->bytes_received.fetch_add(bytes, std::memory_order_relaxed);
    total_bytes_received_.fetch_add(bytes, std::memory_order_relaxed);
}

// -------------------------------------------------------------------------
// Statistics
// -------------------------------------------------------------------------

QoSManager::Stats QoSManager::getStats() const {
    Stats s;
    s.total_bytes_sent     = total_bytes_sent_.load(std::memory_order_relaxed);
    s.total_bytes_received = total_bytes_received_.load(std::memory_order_relaxed);
    s.total_bytes_shaped   = total_bytes_shaped_.load(std::memory_order_relaxed);
    s.backpressure_events  = total_backpressure_events_.load(std::memory_order_relaxed);

    {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        s.active_connections = connections_.size();
    }

    {
        std::lock_guard<std::mutex> lock(priority_stats_mutex_);
        s.bytes_per_priority = bytes_per_priority_;
    }

    return s;
}

QoSManager::ConnectionStats
QoSManager::getConnectionStats(uint64_t connection_id) const {
    auto state = findConnection(connection_id);
    if (!state) {
        return ConnectionStats{};
    }

    ConnectionStats cs;
    cs.connection_id        = state->connection_id;
    cs.priority             = static_cast<Priority>(state->priority.load(std::memory_order_relaxed));
    cs.bytes_sent           = state->bytes_sent.load(std::memory_order_relaxed);
    cs.bytes_received       = state->bytes_received.load(std::memory_order_relaxed);
    cs.bytes_shaped         = state->bytes_shaped.load(std::memory_order_relaxed);
    cs.queue_depth          = state->queue_depth.load(std::memory_order_relaxed);
    cs.backpressure_events  = state->backpressure_events.load(std::memory_order_relaxed);
    cs.has_token_bucket     = false;
    cs.token_bucket_rate_bps    = 0;
    cs.token_bucket_burst_bytes = 0;
    {
        std::lock_guard<std::mutex> tb_lock(state->token_bucket_mutex);
        cs.has_token_bucket = (state->token_bucket != nullptr);
        if (cs.has_token_bucket) {
            cs.token_bucket_rate_bps    = state->token_bucket->rateBps();
            cs.token_bucket_burst_bytes = state->token_bucket->burstBytes();
        }
    }
    return cs;
}

std::vector<QoSManager::ConnectionStats>
QoSManager::getAllConnectionStats() const {
    std::vector<uint64_t> ids;
    {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        ids.reserve(connections_.size());
        for (const auto& [id, _] : connections_) {
            ids.push_back(id);
        }
    }

    std::vector<ConnectionStats> result;
    result.reserve(ids.size());
    for (uint64_t id : ids) {
        result.push_back(getConnectionStats(id));
    }
    return result;
}

// -------------------------------------------------------------------------
// Callbacks
// -------------------------------------------------------------------------

void QoSManager::setBackpressureCallback(
    std::function<void(uint64_t, uint64_t)> cb) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    backpressure_cb_ = std::move(cb);
}

}  // namespace network
}  // namespace themis
