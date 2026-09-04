/**
 * @file smart_routing.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=0, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "server/smart_routing.h"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <cmath>
#include <numeric>
#include <stdexcept>
#include <vector>

namespace themis::server {

// ===========================================================================
// Construction
// ===========================================================================

/** @brief Construct router with default config values. */
SmartRouter::SmartRouter()
    : config_{}
{}

/**
 * @brief Construct router with explicit config.
 * @param config Runtime routing configuration.
 */
SmartRouter::SmartRouter(const Config& config)
    : config_(config)
{}

// ===========================================================================
// Backend registry
// ===========================================================================

/**
 * @brief Register backend endpoint if not already present.
 * @param endpoint Backend descriptor.
 */
void SmartRouter::addBackend(const BackendEndpoint& endpoint) {
    std::unique_lock lock(mutex_);
    if (backends_.count(endpoint.backend_id)) {
        return; // already registered
    }
    BackendState& state = backends_[endpoint.backend_id];
    state.endpoint = endpoint;
    spdlog::debug("SmartRouter: added backend '{}' ({}:{})",
                  endpoint.backend_id, endpoint.address, endpoint.port);
}

/**
 * @brief Remove backend endpoint by id.
 * @param backend_id Backend identifier.
 */
void SmartRouter::removeBackend(const std::string& backend_id) {
    std::unique_lock lock(mutex_);
    backends_.erase(backend_id);
    spdlog::debug("SmartRouter: removed backend '{}'", backend_id);
}

/** @brief Return snapshot list of all registered backends. */
std::vector<BackendEndpoint> SmartRouter::listBackends() const {
    std::shared_lock lock(mutex_);
    std::vector<BackendEndpoint> result = {};

    result.reserve(backends_.size());
    for (const auto& [id, state] : backends_) {
        result.push_back(state.endpoint);
    }
    return result;
}

// ===========================================================================
// Feedback
// ===========================================================================

/**
 * @brief Record one latency sample for a backend.
 * @param backend_id Backend identifier.
 * @param latency_ms Request latency in milliseconds.
 */
void SmartRouter::recordLatency(const std::string& backend_id,
                                double latency_ms)
{
    std::unique_lock lock(mutex_);
    auto it = backends_.find(backend_id);
    if (it == backends_.end()) {
      return;
    }

    auto& state = it->second;
    state.latency_window.push_back(latency_ms);
    if (state.latency_window.size() > config_.latency_window_size) {
        state.latency_window.pop_front();
    }
    state.total_requests++;
    // Compute eagerly while the unique_lock is still held so that
    // readers under shared_lock always see consistent cached values
    // without needing to write to them (avoids data race).
    refreshStats(state);
}

/**
 * @brief Record cache hit for backend/resource key pair.
 * @param backend_id Backend identifier.
 * @param resource_key Resource key.
 */
void SmartRouter::recordCacheHit(const std::string& backend_id,
                                 const std::string& resource_key)
{
    std::unique_lock lock(mutex_);
    auto it = backends_.find(backend_id);
    if (it == backends_.end()) {
      return;
    }

    auto& state = it->second;
    state.cache_hits++;
    state.key_hit_counts[resource_key]++;
}

/**
 * @brief Record cache miss for backend/resource key pair.
 * @param backend_id Backend identifier.
 */
void SmartRouter::recordCacheMiss(const std::string& backend_id,
                                  [[maybe_unused]] const std::string& resource_key)
{
    std::unique_lock lock(mutex_);
    auto it = backends_.find(backend_id);
    if (it == backends_.end()) {
      return;
    }

    // key not tracked for misses (only hits drive prediction)
    it->second.cache_misses++;
}

/**
 * @brief Increment active connection counter for backend.
 * @param backend_id Backend identifier.
 */
void SmartRouter::incrementActiveConnections(const std::string& backend_id) {
    // active_connections is atomic; no need for the write lock.
    std::shared_lock lock(mutex_);
    auto it = backends_.find(backend_id);
    if (it != backends_.end()) {
        it->second.active_connections.fetch_add(1, std::memory_order_relaxed);
    }
}

/**
 * @brief Decrement active connection counter for backend.
 * @param backend_id Backend identifier.
 */
void SmartRouter::decrementActiveConnections(const std::string& backend_id) {
    std::shared_lock lock(mutex_);
    auto it = backends_.find(backend_id);
    if (it != backends_.end()) {
        auto prev = it->second.active_connections.fetch_sub(
            1, std::memory_order_relaxed);
        if (prev == 0) {
            // Underflow guard: reset to 0 rather than wrap around.
            it->second.active_connections.store(0, std::memory_order_relaxed);
        }
    }
}

// ===========================================================================
// Routing decisions
// ===========================================================================

/**
 * @brief Route request key to best backend using multi-phase strategy.
 * @param resource_key Resource identifier.
 * @return Selected backend, or std::nullopt if none are available.
 */
std::optional<BackendEndpoint> SmartRouter::route(
    const std::string& resource_key) const
{
    std::shared_lock lock(mutex_);
    if (backends_.empty()) {
      return std::nullopt;
    }

    // Phase 1: Cache-hit prediction
    if (config_.enable_cache_prediction) {
        uint32_t best_hits  = config_.min_cache_prediction_hits;
        const BackendState* best = nullptr;

        for (const auto& [id, state] : backends_) {
            auto kit = state.key_hit_counts.find(resource_key);
            if (kit != state.key_hit_counts.end() && kit->second >= best_hits) {
                best_hits = kit->second;
                best      = &state;
            }
        }

        if (best) {
            spdlog::debug("SmartRouter: cache-predict '{}' → backend '{}'",
                          resource_key, best->endpoint.backend_id);
            return best->endpoint;
        }
    }

    // Phase 2: Build candidate list, filtering out high-tail backends when
    //          lower-latency alternatives are available.
    std::vector<const BackendState*> candidates = {};

    candidates.reserve(backends_.size());
    for (const auto& [id, state] : backends_) {
        candidates.push_back(&state);
    }

    // Determine whether at least one candidate has an acceptable p99.
    bool has_acceptable = std::any_of(
        candidates.begin(), candidates.end(),
        [this](const BackendState* s) {
            return s->cached_p99_latency <= config_.tail_latency_threshold_ms ||
                   s->latency_window.empty();
        });

    if (has_acceptable) {
        // Remove high-tail backends from the candidate list.
        candidates.erase(
            std::remove_if(candidates.begin(), candidates.end(),
                [this](const BackendState* s) {
                    return !s->latency_window.empty() &&
                           s->cached_p99_latency > config_.tail_latency_threshold_ms;
                }),
            candidates.end());
    }
    // If all backends are high-tail, keep all of them (no choice).

    if (candidates.empty()) {
      return std::nullopt;
    }

    // Phase 3: Least-loaded among candidates (tie-break by avg latency).
    const BackendState* chosen = nullptr;
    for (const auto* s : candidates) {
        if (!chosen) {
            chosen = s;
            continue;
        }
        uint32_t sc = s->active_connections.load(std::memory_order_relaxed);
        uint32_t cc = chosen->active_connections.load(std::memory_order_relaxed);
        if (sc < cc || (sc == cc && s->cached_avg_latency < chosen->cached_avg_latency)) {
            chosen = s;
        }
    }

    if (chosen) {
        spdlog::debug("SmartRouter: least-loaded '{}' → backend '{}' "
                      "(active={}, avg_lat={:.1f}ms)",
                      resource_key, chosen->endpoint.backend_id,
                      chosen->active_connections.load(std::memory_order_relaxed),
                      chosen->cached_avg_latency);
        return chosen->endpoint;
    }

    return std::nullopt;
}

/**
 * @brief Select least-loaded backend with tail-latency filtering.
 * @return Selected backend or std::nullopt.
 */
std::optional<BackendEndpoint> SmartRouter::routeLeastLoaded() const {
    std::shared_lock lock(mutex_);
    if (backends_.empty()) {
      return std::nullopt;
    }

    // Check if any backends have acceptable latency.
    bool has_acceptable = std::any_of(
        backends_.begin(), backends_.end(),
        [this](const auto& kv) {
            return kv.second.latency_window.empty() ||
                   kv.second.cached_p99_latency <= config_.tail_latency_threshold_ms;
        });

    const BackendState* chosen = nullptr;
    for (const auto& [id, state] : backends_) {
        // Skip high-tail backends if alternatives exist.
        if (has_acceptable && !state.latency_window.empty() &&
            state.cached_p99_latency > config_.tail_latency_threshold_ms) {
            continue;
        }

        if (!chosen) {
            chosen = &state;
            continue;
        }
        uint32_t sc = state.active_connections.load(std::memory_order_relaxed);
        uint32_t cc = chosen->active_connections.load(std::memory_order_relaxed);
        if (sc < cc || (sc == cc && state.cached_avg_latency < chosen->cached_avg_latency)) {
            chosen = &state;
        }
    }

    return chosen ? std::optional<BackendEndpoint>{chosen->endpoint} : std::nullopt;
}

/**
 * @brief Predict backend likely to hold resource in cache.
 * @param resource_key Resource identifier.
 * @return Predicted backend, or std::nullopt when history is insufficient.
 */
std::optional<BackendEndpoint> SmartRouter::predictCachedBackend(
    const std::string& resource_key) const
{
    std::shared_lock lock(mutex_);
    if (backends_.empty()) {
      return std::nullopt;
    }

    uint32_t best_hits = 0;
    const BackendState* best = nullptr;

    for (const auto& [id, state] : backends_) {
        auto it = state.key_hit_counts.find(resource_key);
        if (it != state.key_hit_counts.end() && it->second > best_hits) {
            best_hits = it->second;
            best      = &state;
        }
    }

    if (best && best_hits >= config_.min_cache_prediction_hits) {
        return best->endpoint;
    }
    return std::nullopt;
}

// ===========================================================================
// Observability
// ===========================================================================

/** @brief Return stats snapshots for all backends. */
std::vector<SmartRouter::BackendStats> SmartRouter::getAllStats() const {
    std::shared_lock lock(mutex_);
    std::vector<BackendStats> result = {};

    result.reserve(backends_.size());

    for (const auto& [id, state] : backends_) {
        BackendStats bs;
        bs.backend_id        = id;
        bs.avg_latency_ms    = state.cached_avg_latency;
        bs.p99_latency_ms    = state.cached_p99_latency;
        bs.total_requests    = state.total_requests;
        bs.active_connections = state.active_connections.load(std::memory_order_relaxed);
        bs.cache_hits        = state.cache_hits;
        bs.cache_misses      = state.cache_misses;
        bs.latency_samples   = static_cast<uint32_t>(state.latency_window.size());
        result.push_back(bs);
    }
    return result;
}

/**
 * @brief Return stats snapshot for one backend.
 * @param backend_id Backend identifier.
 * @return Backend statistics snapshot.
 * @throws std::out_of_range If backend is unknown.
 */
SmartRouter::BackendStats SmartRouter::getBackendStats(
    const std::string& backend_id) const
{
    std::shared_lock lock(mutex_);
    auto it = backends_.find(backend_id);
    if (it == backends_.end()) {
        throw std::out_of_range("SmartRouter: unknown backend '" + backend_id + "'");
    }

    // Stats are computed eagerly in recordLatency() under unique_lock,
    // so cached_avg_latency / cached_p99_latency are always up-to-date.
    const auto& state = it->second;
    BackendStats bs;
    bs.backend_id        = backend_id;
    bs.avg_latency_ms    = state.cached_avg_latency;
    bs.p99_latency_ms    = state.cached_p99_latency;
    bs.total_requests    = state.total_requests;
    bs.active_connections = state.active_connections.load(std::memory_order_relaxed);
    bs.cache_hits        = state.cache_hits;
    bs.cache_misses      = state.cache_misses;
    bs.latency_samples   = static_cast<uint32_t>(state.latency_window.size());
    return bs;
}

// ===========================================================================
// Private helpers
// ===========================================================================

/* static */
/** @brief Compute arithmetic mean of latency window. */
double SmartRouter::computeAvg(const std::deque<double>& window) noexcept {
    if (window.empty()) {
      return 0.0;
    }
    double sum = std::accumulate(window.begin(), window.end(), 0.0);
    return sum / static_cast<double>(window.size());
}

/* static */
/** @brief Compute p99 latency via nearest-rank method. */
double SmartRouter::computeP99(const std::deque<double>& window) {
    if (window.empty()) {
      return 0.0;
    }
    std::vector<double> sorted(window.begin(), window.end());
    std::sort(sorted.begin(), sorted.end());
    // Nearest-rank: ceil(p * n / 100) – 1
    std::size_t idx = static_cast<std::size_t>(
        std::ceil(99.0 * static_cast<double>(sorted.size()) / 100.0));
    if (idx > 0) {
      --idx;
    }
    idx = std::min(idx, sorted.size() - 1);
    return sorted[idx];
}

/* static */
/** @brief Refresh cached average and p99 values for backend state. */
void SmartRouter::refreshStats(BackendState& state) noexcept {
    state.cached_avg_latency = computeAvg(state.latency_window);
    state.cached_p99_latency = computeP99(state.latency_window);
}

/**
 * @brief Determine whether backend is high-tail under current threshold.
 * @param state Backend runtime state.
 * @param has_other_candidates Whether alternatives exist.
 * @return true when backend should be treated as high-tail.
 */
bool SmartRouter::isHighTail(const BackendState& state,
                             bool has_other_candidates) const noexcept
{
    if (state.latency_window.empty()) {
      return false;
    }
    return has_other_candidates &&
           state.cached_p99_latency > config_.tail_latency_threshold_ms;
}

} // namespace themis::server
