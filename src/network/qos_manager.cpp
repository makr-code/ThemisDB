/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            qos_manager.cpp                                    ║
  Version:         0.0.33                                             ║
  Last Modified:   2026-03-02 03:58:51                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     598                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • b437bbe00  2026-02-25  fix(network): audit – 3 bugs fixed in per-tenant bandwidt... ║
    • a57c9c42c  2026-02-25  feat(network): implement per-tenant network bandwidth quotas ║
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
    // Clean up tenant assignment and decrement tenant connection count
    std::string old_tenant_id;
    {
        std::lock_guard<std::mutex> lock(tenant_assignments_mutex_);
        auto it = tenant_assignments_.find(connection_id);
        if (it != tenant_assignments_.end()) {
            old_tenant_id = it->second;
            tenant_assignments_.erase(it);
        }
    }
    if (!old_tenant_id.empty()) {
        auto ts = findTenant(old_tenant_id);
        if (ts && ts->active_connections.load(std::memory_order_relaxed) > 0) {
            ts->active_connections.fetch_sub(1, std::memory_order_relaxed);
        }
    }

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

    // --- Per-tenant quota check ---
    // Evaluate the shared tenant bucket BEFORE consuming per-connection tokens.
    // If the tenant quota rejects the send, no per-connection tokens should be
    // charged (tenant is the outer "budget owner").
    std::string tenant_id;
    {
        std::lock_guard<std::mutex> lock(tenant_assignments_mutex_);
        auto ta_it = tenant_assignments_.find(connection_id);
        if (ta_it != tenant_assignments_.end()) {
            tenant_id = ta_it->second;
        }
    }
    if (!tenant_id.empty()) {
        auto ts = findTenant(tenant_id);
        if (ts) {
            std::shared_ptr<TokenBucket> tenant_bucket;
            {
                std::lock_guard<std::mutex> tb_lock(ts->token_bucket_mutex);
                tenant_bucket = ts->token_bucket;
            }
            if (tenant_bucket) {
                bool ok = (timeout.count() > 0)
                              ? tenant_bucket->consume(bytes, timeout)
                              : tenant_bucket->tryConsume(bytes);
                if (!ok) {
                    ts->bytes_shaped.fetch_add(bytes, std::memory_order_relaxed);
                    total_bytes_shaped_.fetch_add(bytes, std::memory_order_relaxed);
                    return false;
                }
            }
        }
    }

    // --- Token bucket check (per-connection) ---
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

    // Update per-tenant bytes_sent counter
    std::string tenant_id;
    {
        std::lock_guard<std::mutex> lock(tenant_assignments_mutex_);
        auto it = tenant_assignments_.find(connection_id);
        if (it != tenant_assignments_.end()) {
            tenant_id = it->second;
        }
    }
    if (!tenant_id.empty()) {
        auto ts = findTenant(tenant_id);
        if (ts) {
            ts->bytes_sent.fetch_add(bytes, std::memory_order_relaxed);
        }
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

// =============================================================================
// Per-tenant bandwidth quota management
// =============================================================================

std::shared_ptr<QoSManager::TenantState>
QoSManager::findTenant(const std::string& id) const {
    std::lock_guard<std::mutex> lock(tenants_mutex_);
    auto it = tenants_.find(id);
    if (it == tenants_.end()) {
        return nullptr;
    }
    return it->second;
}

void QoSManager::registerTenantQuota(const std::string& tenant_id,
                                      uint64_t rate_bps,
                                      uint64_t burst_bytes) {
    uint64_t burst = burst_bytes > 0 ? burst_bytes
                                      : (rate_bps > 0 ? rate_bps / 8 : 0);

    std::lock_guard<std::mutex> lock(tenants_mutex_);
    auto it = tenants_.find(tenant_id);
    if (it != tenants_.end()) {
        // Update existing entry in-place
        auto& ts = it->second;
        std::lock_guard<std::mutex> tb_lock(ts->token_bucket_mutex);
        if (ts->token_bucket) {
            ts->token_bucket->reconfigure(rate_bps, burst > 0 ? burst : 1);
        } else if (rate_bps > 0) {
            ts->token_bucket = std::make_shared<TokenBucket>(rate_bps, burst > 0 ? burst : 1);
        }
    } else {
        auto ts        = std::make_shared<TenantState>();
        ts->tenant_id  = tenant_id;
        if (rate_bps > 0) {
            ts->token_bucket = std::make_shared<TokenBucket>(rate_bps, burst > 0 ? burst : 1);
        }
        tenants_[tenant_id] = std::move(ts);
    }
}

void QoSManager::unregisterTenantQuota(const std::string& tenant_id) {
    std::lock_guard<std::mutex> lock(tenants_mutex_);
    tenants_.erase(tenant_id);
}

void QoSManager::setTenantQuota(const std::string& tenant_id,
                                  uint64_t rate_bps,
                                  uint64_t burst_bytes) {
    registerTenantQuota(tenant_id, rate_bps, burst_bytes);
}

void QoSManager::assignTenant(uint64_t connection_id,
                                const std::string& tenant_id) {
    std::string old_tenant_id;
    {
        std::lock_guard<std::mutex> lock(tenant_assignments_mutex_);
        auto it = tenant_assignments_.find(connection_id);
        if (it != tenant_assignments_.end()) {
            if (it->second == tenant_id) {
                return;  // Already assigned to this tenant
            }
            old_tenant_id = it->second;
        }
        tenant_assignments_[connection_id] = tenant_id;
    }

    // Adjust active_connections counters outside tenant_assignments_mutex_
    // to avoid potential lock-order inversion with tenants_mutex_.
    if (!old_tenant_id.empty()) {
        auto old_ts = findTenant(old_tenant_id);
        if (old_ts) {
            old_ts->active_connections.fetch_sub(1, std::memory_order_relaxed);
        }
    }
    auto new_ts = findTenant(tenant_id);
    if (new_ts) {
        new_ts->active_connections.fetch_add(1, std::memory_order_relaxed);
    }
}

QoSManager::TenantQuotaStats
QoSManager::getTenantStats(const std::string& tenant_id) const {
    auto ts = findTenant(tenant_id);
    if (!ts) {
        return TenantQuotaStats{};
    }

    TenantQuotaStats result;
    result.tenant_id         = ts->tenant_id;
    result.bytes_sent        = ts->bytes_sent.load(std::memory_order_relaxed);
    result.bytes_shaped      = ts->bytes_shaped.load(std::memory_order_relaxed);
    result.active_connections = ts->active_connections.load(std::memory_order_relaxed);

    {
        std::lock_guard<std::mutex> tb_lock(ts->token_bucket_mutex);
        if (ts->token_bucket) {
            result.rate_bps    = ts->token_bucket->rateBps();
            result.burst_bytes = ts->token_bucket->burstBytes();
        }
    }
    return result;
}

std::vector<QoSManager::TenantQuotaStats>
QoSManager::getAllTenantStats() const {
    std::vector<std::string> ids;
    {
        std::lock_guard<std::mutex> lock(tenants_mutex_);
        ids.reserve(tenants_.size());
        for (const auto& [id, _] : tenants_) {
            ids.push_back(id);
        }
    }

    std::vector<TenantQuotaStats> result;
    result.reserve(ids.size());
    for (const auto& id : ids) {
        result.push_back(getTenantStats(id));
    }
    return result;
}

}  // namespace network
}  // namespace themis
