/**
 * @file retry_policy.cpp
 * @brief Implementation of RetryPolicy and IdempotencyCache for
 *        wire-protocol transient-fault recovery (P5-S01).
 *
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Phase: 5-S01 — Wire-Protocol Retry + Idempotency
 */


#include "network/wire_protocol_server.h"

#include <algorithm>
#include <cerrno>
#include <random>

namespace themis {
namespace network {

// ============================================================================
// RetryPolicy
// ============================================================================

std::chrono::milliseconds RetryPolicy::computeDelay([[maybe_unused]] uint32_t attempt) const noexcept {
    // base_delay_ms * 2^attempt  — protect against left-shift overflow.
    uint64_t backoff = 0;
    if (attempt >= 31u) {
        backoff = static_cast<uint64_t>(max_delay_ms);
    } else {
        // Use multiplication instead of bit-shift to stay well-defined for
        // large base_delay_ms values.
        const uint64_t multiplier =
            static_cast<uint64_t>(1u) << attempt; // 2^attempt
        backoff = static_cast<uint64_t>(base_delay_ms) * multiplier;
    }

    // Add jitter using a thread-local Mersenne Twister seeded from steady_clock.
    if (jitter_ms > 0) {
        thread_local std::mt19937 rng{
            static_cast<std::mt19937::result_type>(
                std::chrono::steady_clock::now().time_since_epoch().count())};
        std::uniform_int_distribution<uint32_t> dist{0, jitter_ms};
        backoff += dist(rng);
    }

    // Cap at max_delay_ms.
    backoff = std::min(backoff, static_cast<uint64_t>(max_delay_ms));

    return std::chrono::milliseconds{backoff};
}

bool RetryPolicy::isTransient([[maybe_unused]] int error_code) noexcept {
    return error_code == ECONNRESET
        || error_code == ETIMEDOUT
        || error_code == EAGAIN
#if defined(EWOULDBLOCK) && EWOULDBLOCK != EAGAIN
        || error_code == EWOULDBLOCK
#endif
        ;
}

// ============================================================================
// IdempotencyCache
// ============================================================================

const IdempotencyCache::Entry*
IdempotencyCache::lookup(const std::string& request_id) const {
    using SnapshotMap = std::unordered_map<std::string, Entry>;
    thread_local std::unordered_map<const IdempotencyCache*, SnapshotMap> snapshots;

    auto snapshot = lookupSnapshot(request_id);
    if (!snapshot.has_value()) {
        auto cache_it = snapshots.find(this);
        if (cache_it != snapshots.end()) {
            cache_it->second.erase(request_id);
        }
        return nullptr;
    }

    auto& cache_snapshots = snapshots[this];
    auto [it, _inserted] =
        cache_snapshots.insert_or_assign(request_id, std::move(*snapshot));
    return &it->second;
}

std::optional<IdempotencyCache::Entry>
IdempotencyCache::lookupSnapshot(const std::string& request_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = cache_.find(request_id);
    if (it == cache_.end()) {
        return std::nullopt;
    }
    return it->second;
}

void IdempotencyCache::store(const std::string& request_id, std::string result) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (window_size_ == 0) {
        return;
    }

    // First-write-wins: do not replace an existing entry.
    if (cache_.count(request_id)) {
        return;
    }

    // Evict oldest entry when at capacity.
    if (window_size_ > 0 && cache_.size() >= window_size_) {
        const auto& oldest = insertion_order_.front();
        cache_.erase(oldest);
        insertion_order_.pop_front();
    }

    Entry entry{std::move(result), std::chrono::steady_clock::now()};
    cache_.emplace(request_id, std::move(entry));
    insertion_order_.push_back(request_id);
}

void IdempotencyCache::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    cache_.clear();
    insertion_order_.clear();
}

size_t IdempotencyCache::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return cache_.size();
}

} // namespace network
} // namespace themis
