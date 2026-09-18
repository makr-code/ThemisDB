/// @file shard_summary_coordinator.h
/// @brief Phase C: distributed shard summary refresh, summary-first routing,
///        and exact-on-demand tensor fetch for multi-shard coordination.
///
/// ## Phase C Overview
///
/// Phase C extends the distributed tensor infrastructure with:
/// 1. **Shard summary refresh** — per-shard advisory summaries carry freshness
///    timestamps and are refreshed on a configurable TTL schedule.
/// 2. **Summary-first routing with escalation** — the planner uses advisory
///    summaries to select shards cheaply; if a shard's summary exceeds the
///    freshness threshold the planner escalates to exact-on-demand fetch.
/// 3. **Exact-on-demand tensor fetch** — replaces the advisory summary when
///    accuracy is required; the exact fragment is fetched from the shard and
///    validated before being used in the query result.
/// 4. **Multi-shard freshness consensus** — a quorum of shards must agree that
///    their summaries are fresh before the planner skips exact fetch.
///
/// ## Advisory-Only Invariant (preserved from Phase A/B)
///
/// Shard summaries are still advisory only.  Callers MUST NOT treat a summary
/// as a final query result.  The exact-on-demand path is mandatory whenever:
///   - a shard's summary is STALE or INVALID, OR
///   - the requested accuracy mode is EXACT, OR
///   - consensus quorum is not met across shards.
///
/// ## Byzantine Fault Tolerance and Quorum Safety (SG-DT-01)
///
/// Quorum-based consensus decisions (e.g., "skip exact fetch") require strict
/// majority participation to ensure Byzantine Fault Tolerance:
/// - Minimum quorum ratio: 0.5f (50% + 1 participant) — simple majority
/// - Recommended quorum ratio: 0.666f (66.7%) or 0.75f (75%) for Byzantine safety
/// - All quorum_ratio values are validated at construction and configuration time
/// - Attempting to set quorum_ratio < 0.5f will be rejected with error logging
///
/// Reference: PRODUCTION_REQUIREMENTS.md §2.3 Safety Gates (SG-DT-01)
///
/// ## Thread Safety
///
/// `ShardSummaryCoordinator` is thread-safe for concurrent `refresh()`,
/// `route()`, and `fetchExact()` calls.  Internal per-shard state is
/// protected by a `std::mutex`; the freshness consensus check is lock-free
/// after the state snapshot is taken.

#pragma once

#include "artifact_manifest.h"
#include "manifest_store.h"

#include "tensor/tensor_summary_types.h"

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace themis {
namespace distributed_tensor {

// ============================================================================
// ShardFreshnessRecord — per-shard freshness state with timestamps
// ============================================================================

struct ShardFreshnessRecord {
    std::string shard_id;

    int64_t last_refresh_ms = 0;

    uint32_t ttl_seconds = 3600;

    uint64_t refresh_generation = 0;

    tensor::SummaryFreshnessState freshness_state = tensor::SummaryFreshnessState::STALE;

    bool exact_fetch_pending = false;

    std::string last_refresh_error = {};

    [[nodiscard]] bool isExpired(int64_t now_ms = 0) const noexcept;

    /**
     * @brief Mark Refreshed.
     * @param[in] refresh_time_ms Input parameter.
     * @note Exception safety: noexcept.
     */
    void markRefreshed(int64_t refresh_time_ms) noexcept;
};

// ============================================================================
// ShardSummaryRefreshResult — outcome of a single shard refresh
// ============================================================================

struct ShardSummaryRefreshResult {
    std::string shard_id;

    bool success = false;

    std::string error_reason;

    tensor::SummaryFreshnessState freshness_state = tensor::SummaryFreshnessState::STALE;

    int64_t refreshed_at_ms = 0;

    uint64_t generation = 0;
};

// ============================================================================
// RoutingDecision — outcome of summary-first routing for a single shard
// ============================================================================

struct RoutingDecision {
    std::string shard_id;

    bool include_shard = false;

    bool escalate_to_exact = false;

    std::string reason;

    tensor::SummaryFreshnessState summary_freshness = tensor::SummaryFreshnessState::STALE;

    float advisory_score = 0.0f;
};

// ============================================================================
// AccuracyMode — controls whether exact fetch is forced
// ============================================================================

enum class AccuracyMode : uint8_t {
    ADVISORY = 0,
    EXACT = 1,
};

// ============================================================================
// ExactFetchRequest — request for exact-on-demand tensor fragment
// ============================================================================

struct ExactFetchRequest {
    std::string shard_id;

    std::string artifact_id;

    uint32_t timeout_ms = 5000;

    bool background_refresh = false;

    std::string correlation_id;
};

// ============================================================================
// ExactFetchResult — result of an exact-on-demand tensor fetch
// ============================================================================

struct ExactFetchResult {
    std::string shard_id;

    std::string artifact_id;

    bool success = false;

    std::string error_reason;

    std::vector<uint8_t> fragment_data;

    std::string content_hash = {};

    int64_t fetched_at_ms = 0;

    float fetch_latency_ms = 0.0f;

    bool integrity_verified = false;
};

// ============================================================================
// FreshnessConsensusResult — multi-shard quorum outcome
// ============================================================================

struct FreshnessConsensusResult {
    std::size_t total_shards = 0;

    std::size_t fresh_shards = 0;

    std::size_t stale_shards = 0;

    std::size_t invalid_shards = 0;

    bool quorum_met = false;

    float quorum_ratio = 0.75f;
};

// ============================================================================
// IShardFetcher — interface for exact fragment retrieval
// ============================================================================

class IShardFetcher {
public:
    /**
     * @brief IShard Fetcher.
     * @return Return value.
     */
    virtual ~IShardFetcher() = default;

    [[nodiscard]] virtual ExactFetchResult fetch(
        const ExactFetchRequest& request) const noexcept = 0;
};

// ============================================================================
// ShardSummaryCoordinator — Phase C coordinator
// ============================================================================

class ShardSummaryCoordinator {
public:
    struct Config {
        uint32_t default_ttl_seconds;

        float freshness_quorum_ratio;

        uint32_t exact_fetch_timeout_ms;

        bool escalate_stale_shards;

        bool skip_invalid_shards;

        Config() noexcept
            : default_ttl_seconds(3600),
              freshness_quorum_ratio(0.75f),
              exact_fetch_timeout_ms(5000),
              escalate_stale_shards(true),
              skip_invalid_shards(true) {
            validateAndClamp();
        }

        /**
         * @brief Validate And Clamp.
         * @note Exception safety: noexcept.
         */
        void validateAndClamp() noexcept;

        [[nodiscard]] bool isQuorumSafe() const noexcept {
            return freshness_quorum_ratio >= 0.5f;
        }
    };

    explicit ShardSummaryCoordinator(
        std::shared_ptr<IShardFetcher> fetcher = nullptr,
        ManifestStore* manifest_store = nullptr,
        Config config = Config()) noexcept;

    ~ShardSummaryCoordinator() = default;

    // Prevent copy; mutex and atomics are non-movable
    ShardSummaryCoordinator(const ShardSummaryCoordinator&) = delete;
    ShardSummaryCoordinator& operator=(const ShardSummaryCoordinator&) = delete;
    ShardSummaryCoordinator(ShardSummaryCoordinator&&) = delete;
    ShardSummaryCoordinator& operator=(ShardSummaryCoordinator&&) = delete;

    // ─── Shard Registration ───────────────────────────────────────────────

    void registerShard(const std::string& shard_id,
                       uint32_t ttl_seconds = 0) noexcept;

    /**
     * @brief Unregister Shard.
     * @param[in] shard_id Identifier of the shard.
     * @note Exception safety: noexcept.
     */
    void unregisterShard(const std::string& shard_id) noexcept;

    // ─── Summary Refresh ─────────────────────────────────────────────────

    [[nodiscard]] ShardSummaryRefreshResult refreshShard(
        const std::string& shard_id,
        tensor::ShardSummary& summary,
        int64_t now_ms = 0) noexcept;

    [[nodiscard]] std::vector<ShardSummaryRefreshResult> refreshAll(
        std::unordered_map<std::string, tensor::ShardSummary>& summaries,
        int64_t now_ms = 0) noexcept;

    // ─── Freshness Queries ────────────────────────────────────────────────

    [[nodiscard]] std::optional<ShardFreshnessRecord> getFreshnessRecord(
        const std::string& shard_id) const noexcept;

    [[nodiscard]] bool isFresh(const std::string& shard_id,
                               int64_t now_ms = 0) const noexcept;

    [[nodiscard]] FreshnessConsensusResult checkFreshnessConsensus(
        const std::vector<std::string>& shard_ids,
        int64_t now_ms = 0) const noexcept;

    // ─── Summary-First Routing with Escalation ───────────────────────────

    [[nodiscard]] std::vector<RoutingDecision> routeSummaryFirst(
        const std::vector<tensor::ShardSummary>& summaries,
        AccuracyMode mode = AccuracyMode::ADVISORY,
        int64_t now_ms = 0) const noexcept;

    // ─── Exact-On-Demand Fetch ────────────────────────────────────────────

    [[nodiscard]] ExactFetchResult fetchExact(
        const ExactFetchRequest& request) const noexcept;

    [[nodiscard]] std::vector<ExactFetchResult> fetchEscalated(
        const std::vector<RoutingDecision>& decisions,
        const std::string& artifact_id,
        const std::string& correlation_id = {}) const noexcept;

    /**
     * @brief ─── Configuration & Statistics ──────────────────────────────────────
     * @param[in] config Input parameter.
     * @note Exception safety: noexcept.
     */

    void setConfig(const Config& config) noexcept;

    [[nodiscard]] Config config() const noexcept;

    struct Stats {
        uint64_t total_refreshes = 0;

        uint64_t total_refresh_failures = 0;

        uint64_t total_routing_decisions = 0;

        uint64_t total_escalations = 0;

        uint64_t total_exact_fetches = 0;

        uint64_t total_exact_fetch_successes = 0;
    };

    [[nodiscard]] Stats stats() const noexcept;

private:
    mutable std::mutex records_mutex_;
    std::unordered_map<std::string, ShardFreshnessRecord> records_;

    std::shared_ptr<IShardFetcher> fetcher_;
    ManifestStore* manifest_store_ = nullptr;
    Config config_;

    // Mutable atomics for lock-free stats
    mutable std::atomic<uint64_t> stat_refreshes_{0};
    mutable std::atomic<uint64_t> stat_refresh_failures_{0};
    mutable std::atomic<uint64_t> stat_routing_decisions_{0};
    mutable std::atomic<uint64_t> stat_escalations_{0};
    mutable std::atomic<uint64_t> stat_exact_fetches_{0};
    mutable std::atomic<uint64_t> stat_exact_fetch_successes_{0};

    [[nodiscard]] static int64_t resolveNow(int64_t hint_ms) noexcept;
};

} // namespace distributed_tensor
} // namespace themis
