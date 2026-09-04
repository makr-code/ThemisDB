/**
 * @file adaptive_rate_limiter.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "server/adaptive_rate_limiter.h"
#include "utils/logger.h"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace themis {
namespace server {

// ============================================================================
// Construction
// ============================================================================

AdaptiveRateLimiter::AdaptiveRateLimiter(const Config& config)
    : config_(config)
{
    // Pre-create the global (empty tenant-id) state so the first call
    // to allowRequest("") doesn't have to acquire a write-lock on the
    // hot path.
    tenants_.emplace("", TenantState{config_.base_capacity});
}

// ============================================================================
// Public API
// ============================================================================

void AdaptiveRateLimiter::recordSample(const std::string& tenant_id,
                                       const BackendHealthSample& sample)
{
    std::unique_lock<std::shared_mutex> lock(tenants_mutex_);

    auto it = tenants_.find(tenant_id);
    if (it == tenants_.end()) {
        it = tenants_.emplace(tenant_id, TenantState{config_.base_capacity}).first;
    }

    auto& state = it->second;

    TimedSample ts;
    ts.ts         = std::chrono::steady_clock::now();
    ts.latency_ms = sample.latency_ms;
    ts.is_error   = sample.is_error;
    state.window.push_back(ts);

    // Adapt capacity whenever we have accumulated enough samples.
    if (state.window.size() >= config_.min_samples_to_adapt) {
        pruneAndAdapt(state);
    }
}

bool AdaptiveRateLimiter::allowRequest(const std::string& tenant_id)
{
    total_requests_.fetch_add(1, std::memory_order_relaxed);

    std::unique_lock<std::shared_mutex> lock(tenants_mutex_);

    auto it = tenants_.find(tenant_id);
    if (it == tenants_.end()) {
        it = tenants_.emplace(tenant_id, TenantState{config_.base_capacity}).first;
    }

    auto& state = it->second;

    // Replenish tokens if a full window has elapsed since the last reset.
    const auto now = std::chrono::steady_clock::now();
    if (now - state.window_start >= std::chrono::seconds(config_.window_seconds)) {
        state.window_start    = now;
        state.available_tokens = state.current_capacity;
    }

    // Also prune stale health samples and re-evaluate capacity.
    pruneAndAdapt(state);

    if (state.available_tokens == 0) {
        total_rejections_.fetch_add(1, std::memory_order_relaxed);
        THEMIS_DEBUG("AdaptiveRateLimiter: tenant='{}' rejected (cap={}, avail=0)",
                     tenant_id, state.current_capacity);
        return false;
    }

    --state.available_tokens;
    return true;
}

size_t AdaptiveRateLimiter::getCurrentCapacity(const std::string& tenant_id) const
{
    std::shared_lock<std::shared_mutex> lock(tenants_mutex_);

    auto it = tenants_.find(tenant_id);
    if (it == tenants_.end()) {
        return config_.base_capacity;
    }
    return it->second.current_capacity;
}

void AdaptiveRateLimiter::reset()
{
    std::unique_lock<std::shared_mutex> lock(tenants_mutex_);
    tenants_.clear();
    tenants_.emplace("", TenantState{config_.base_capacity});
    total_requests_.store(0, std::memory_order_relaxed);
    total_rejections_.store(0, std::memory_order_relaxed);
}

// ============================================================================
// Private helpers
// ============================================================================

void AdaptiveRateLimiter::pruneAndAdapt(TenantState& state)
{
    // Remove samples outside the sliding window.
    auto now    = std::chrono::steady_clock::now();
    auto cutoff = now - std::chrono::seconds(config_.window_seconds);

    state.window.erase(
        std::remove_if(state.window.begin(), state.window.end(),
                       [&]([[maybe_unused]] const TimedSample& s){ return s.ts < cutoff; }),
        state.window.end());

    // Don't adapt until we have enough samples.
    if (state.window.size() < config_.min_samples_to_adapt) {
        return;
    }

    const auto p99        = computeP99(state.window);
    const double err_rate = computeErrorRate(state.window);

    const size_t old_cap = state.current_capacity;

    if (err_rate > config_.high_error_rate) {
        // Backend errors > threshold → reduce to 20 % of base.
        state.current_capacity = std::max<size_t>(
            1, static_cast<size_t>(config_.base_capacity * 0.20));
        THEMIS_WARN("AdaptiveRateLimiter: high error rate ({:.1f}%); "
                    "reducing capacity {} → {}",
                    err_rate * 100.0, old_cap, state.current_capacity);
    } else if (p99.count() >
               static_cast<int64_t>(config_.high_latency_threshold_ms)) {
        // p99 latency above high threshold → reduce to 50 %.
        state.current_capacity = std::max<size_t>(
            1, static_cast<size_t>(config_.base_capacity * 0.50));
        THEMIS_WARN("AdaptiveRateLimiter: high p99 latency ({}ms); "
                    "reducing capacity {} → {}",
                    p99.count(), old_cap, state.current_capacity);
    } else if (p99.count() <
                   static_cast<int64_t>(config_.low_latency_threshold_ms) &&
               err_rate < config_.low_error_rate) {
        // Backend healthy → step capacity back up toward base.
        const size_t step =
            std::max<size_t>(1, static_cast<size_t>(
                config_.base_capacity * config_.recovery_step));
        state.current_capacity =
            std::min(config_.base_capacity, state.current_capacity + step);

        if (state.current_capacity != old_cap) {
            THEMIS_INFO("AdaptiveRateLimiter: backend recovered; "
                        "increasing capacity {} → {}",
                        old_cap, state.current_capacity);
        }
    }

    // Clamp available_tokens to the new capacity so we don't hand out more
    // tokens than the adapted limit allows.
    if (state.available_tokens > state.current_capacity) {
        state.available_tokens = state.current_capacity;
    }
}

std::chrono::milliseconds AdaptiveRateLimiter::computeP99(
    const std::vector<TimedSample>& samples)
{
    if (samples.empty()) {
        return std::chrono::milliseconds{0};
    }

    std::vector<int64_t> latencies;
    latencies.reserve(samples.size());
    for (const auto& s : samples) {
        latencies.push_back(s.latency_ms.count());
    }

    std::sort(latencies.begin(), latencies.end());

    // p99 index (ceiling).
    const size_t idx =
        static_cast<size_t>(std::ceil(0.99 * static_cast<double>(latencies.size()))) - 1;
    const size_t clamped = std::min(idx, latencies.size() - 1);

    return std::chrono::milliseconds{latencies[clamped]};
}

double AdaptiveRateLimiter::computeErrorRate(
    const std::vector<TimedSample>& samples)
{
    if (samples.empty()) {
        return 0.0;
    }

    const size_t errors =
        static_cast<size_t>(std::count_if(
            samples.begin(), samples.end(),
            [](const TimedSample& s){ return s.is_error; }));

    return static_cast<double>(errors) / static_cast<double>(samples.size());
}

} // namespace server
} // namespace themis
