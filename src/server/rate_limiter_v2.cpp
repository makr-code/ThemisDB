/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            rate_limiter_v2.cpp                                ║
  Version:         0.0.27                                             ║
  Last Modified:   2026-02-22 08:56:26                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     239                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "server/rate_limiter_v2.h"
#include "utils/logger.h"

namespace themis {
namespace server {

// ===== TokenBucketRateLimiter =====

TokenBucketRateLimiter::TokenBucketRateLimiter(const Config& config)
    : config_(config)
{
    // Create buckets for each priority lane
    if (config_.enable_priority_lanes) {
        buckets_[Priority::HIGH] = std::make_unique<Bucket>(
            config_.high_capacity, config_.high_refill_rate);
        buckets_[Priority::NORMAL] = std::make_unique<Bucket>(
            config_.capacity, config_.refill_rate);
        buckets_[Priority::LOW] = std::make_unique<Bucket>(
            config_.low_capacity, config_.low_refill_rate);
    } else {
        // Single bucket for all priorities
        buckets_[Priority::NORMAL] = std::make_unique<Bucket>(
            config_.capacity, config_.refill_rate);
    }
}

bool TokenBucketRateLimiter::tryAcquire(size_t tokens, Priority prio) {
    total_requests_.fetch_add(1, std::memory_order_relaxed);

    // If priority lanes disabled, use NORMAL bucket for all
    auto bucket_it = buckets_.find(prio);
    if (bucket_it == buckets_.end()) {
        bucket_it = buckets_.find(Priority::NORMAL);
    }

    if (bucket_it == buckets_.end()) {
        THEMIS_ERROR("RateLimiter: No bucket configured for priority {}", static_cast<int>(prio));
        return false; // Safe fallback: reject
    }

    auto& bucket = bucket_it->second;
    bucket->refill();

    if (bucket->consume(tokens)) {
        return true;
    } else {
        total_rejections_.fetch_add(1, std::memory_order_relaxed);
        return false;
    }
}

size_t TokenBucketRateLimiter::getAvailableTokens(Priority prio) const {
    auto bucket_it = buckets_.find(prio);
    if (bucket_it == buckets_.end()) {
        bucket_it = buckets_.find(Priority::NORMAL);
    }

    if (bucket_it == buckets_.end()) {
        return 0;
    }

    return bucket_it->second->tokens.load(std::memory_order_relaxed);
}

void TokenBucketRateLimiter::reset() {
    total_requests_.store(0, std::memory_order_relaxed);
    total_rejections_.store(0, std::memory_order_relaxed);

    for (auto& [prio, bucket] : buckets_) {
        std::lock_guard<std::mutex> lock(bucket->mutex);
        bucket->tokens.store(bucket->capacity, std::memory_order_relaxed);
        bucket->last_refill = std::chrono::steady_clock::now();
    }
}

// ===== Bucket Implementation =====

void TokenBucketRateLimiter::Bucket::refill() {
    std::lock_guard<std::mutex> lock(mutex);

    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_refill);

    if (elapsed.count() <= 0) {
        return; // No time elapsed
    }

    // Calculate tokens to add: (refill_rate * elapsed_seconds)
    double elapsed_seconds = elapsed.count() / 1000.0;
    size_t tokens_to_add = static_cast<size_t>(refill_rate * elapsed_seconds);

    if (tokens_to_add > 0) {
        size_t current = tokens.load(std::memory_order_relaxed);
        size_t new_tokens = std::min(current + tokens_to_add, capacity);
        tokens.store(new_tokens, std::memory_order_relaxed);
        last_refill = now;
    }
}

bool TokenBucketRateLimiter::Bucket::consume(size_t count) {
    // Atomic decrement if sufficient tokens available
    size_t current = tokens.load(std::memory_order_acquire);

    while (current >= count) {
        if (tokens.compare_exchange_weak(current, current - count,
                                          std::memory_order_release,
                                          std::memory_order_acquire)) {
            return true; // Success
        }
        // CAS failed, retry with updated 'current'
    }

    return false; // Insufficient tokens
}

// ===== PerClientRateLimiter =====

PerClientRateLimiter::PerClientRateLimiter()
    : PerClientRateLimiter(Config{}) {
}

PerClientRateLimiter::PerClientRateLimiter(const Config& config)
    : config_(config)
    , last_cleanup_(std::chrono::steady_clock::now())
{
}

bool PerClientRateLimiter::allowRequest(
    const std::string& client_id,
    size_t tokens,
    TokenBucketRateLimiter::Priority prio
) {
    // Periodic cleanup of idle clients
    auto now = std::chrono::steady_clock::now();
    if (now - last_cleanup_ > config_.cleanup_interval) {
        cleanupIdleClients();
        last_cleanup_ = now;
    }

    std::unique_lock<std::mutex> lock(clients_mutex_);

    // Get or create client bucket
    auto it = client_buckets_.find(client_id);
    if (it == client_buckets_.end()) {
        // Enforce max clients limit
        if (client_buckets_.size() >= config_.max_clients) {
            THEMIS_WARN("PerClientRateLimiter: Max clients ({}) reached, rejecting new client: {}",
                        config_.max_clients, client_id);
            return false;
        }

        // Create new bucket for client
        auto client_bucket = std::make_unique<ClientBucket>();
        client_bucket->limiter = std::make_unique<TokenBucketRateLimiter>(
            TokenBucketRateLimiter::Config{
                .capacity = config_.capacity_per_client,
                .refill_rate = config_.refill_rate_per_client,
                .enable_priority_lanes = false // Per-client uses single bucket
            }
        );
        client_bucket->last_access = now;

        it = client_buckets_.emplace(client_id, std::move(client_bucket)).first;
    }

    auto& client_bucket = it->second;
    client_bucket->last_access = now;
    client_bucket->total_requests.fetch_add(1, std::memory_order_relaxed);

    lock.unlock(); // Unlock before trying to acquire tokens

    bool allowed = client_bucket->limiter->tryAcquire(tokens, prio);

    if (!allowed) {
        client_bucket->total_rejections.fetch_add(1, std::memory_order_relaxed);
    }

    return allowed;
}

PerClientRateLimiter::ClientMetrics
PerClientRateLimiter::getClientMetrics(const std::string& client_id) const {
    std::lock_guard<std::mutex> lock(clients_mutex_);

    auto it = client_buckets_.find(client_id);
    if (it == client_buckets_.end()) {
        return ClientMetrics{};
    }

    const auto& bucket = it->second;
    return ClientMetrics{
        .total_requests = bucket->total_requests.load(std::memory_order_relaxed),
        .total_rejections = bucket->total_rejections.load(std::memory_order_relaxed),
        .available_tokens = bucket->limiter->getAvailableTokens()
    };
}

size_t PerClientRateLimiter::getActiveClients() const {
    std::lock_guard<std::mutex> lock(clients_mutex_);
    return client_buckets_.size();
}

void PerClientRateLimiter::cleanupIdleClients() {
    std::lock_guard<std::mutex> lock(clients_mutex_);

    auto now = std::chrono::steady_clock::now();
    auto idle_threshold = std::chrono::minutes(10); // Remove after 10min idle

    for (auto it = client_buckets_.begin(); it != client_buckets_.end(); ) {
        if (now - it->second->last_access > idle_threshold) {
            THEMIS_DEBUG("PerClientRateLimiter: Removing idle client: {}", it->first);
            it = client_buckets_.erase(it);
        } else {
            ++it;
        }
    }
}

} // namespace server
} // namespace themis
