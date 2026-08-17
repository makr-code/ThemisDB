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

/**
 * @brief Per-shard freshness record maintained by the coordinator.
 *
 * Stores the last-refresh timestamp, TTL, staleness state, and version
 * for a single shard's advisory summary.
 */
struct ShardFreshnessRecord {
    /// Shard identifier (matches tensor::ShardSummary::shard_id).
    std::string shard_id;

    /// Epoch-milliseconds when the summary was last successfully refreshed.
    int64_t last_refresh_ms = 0;

    /// Time-to-live in seconds; after expiry the summary is considered STALE.
    uint32_t ttl_seconds = 3600;

    /// Monotonic refresh counter (incremented on each successful refresh).
    uint64_t refresh_generation = 0;

    /// Current freshness state.
    tensor::SummaryFreshnessState freshness_state = tensor::SummaryFreshnessState::STALE;

    /// Whether an exact fetch is currently in flight for this shard.
    bool exact_fetch_pending = false;

    /// Error message from last refresh attempt (empty if last refresh succeeded).
    std::string last_refresh_error;

    /**
     * @brief Check whether the record has expired given a reference time.
     *
     * @param now_ms  Current epoch-milliseconds (0 = use wall clock).
     * @return true if (now_ms - last_refresh_ms) > ttl_seconds * 1000.
     */
    [[nodiscard]] bool isExpired(int64_t now_ms = 0) const noexcept;

    /**
     * @brief Mark record as freshly refreshed at the given time.
     *
     * @param refresh_time_ms  Epoch-milliseconds of the refresh event.
     */
    void markRefreshed(int64_t refresh_time_ms) noexcept;
};

// ============================================================================
// ShardSummaryRefreshResult — outcome of a single shard refresh
// ============================================================================

/**
 * @brief Result returned by `ShardSummaryCoordinator::refresh()`.
 */
struct ShardSummaryRefreshResult {
    /// Shard that was refreshed.
    std::string shard_id;

    /// Whether the refresh completed successfully.
    bool success = false;

    /// Error message if refresh failed.
    std::string error_reason;

    /// Freshness state after the refresh.
    tensor::SummaryFreshnessState freshness_state = tensor::SummaryFreshnessState::STALE;

    /// Epoch-milliseconds when the refresh completed.
    int64_t refreshed_at_ms = 0;

    /// New summary generation after refresh.
    uint64_t generation = 0;
};

// ============================================================================
// RoutingDecision — outcome of summary-first routing for a single shard
// ============================================================================

/**
 * @brief Routing decision for a single shard produced by summary-first routing.
 *
 * The decision is advisory: the consumer must honour the `escalate_to_exact`
 * flag and issue an exact fetch before using the result as a query answer.
 */
struct RoutingDecision {
    /// Shard identifier.
    std::string shard_id;

    /// Whether to use the shard (true) or skip it (false).
    bool include_shard = false;

    /// Whether to escalate to exact-on-demand fetch (always true when shard is
    /// included but summary is STALE or INVALID).
    bool escalate_to_exact = false;

    /// Reason for the include/escalate decision (human-readable).
    std::string reason;

    /// Freshness state of the summary used for this decision.
    tensor::SummaryFreshnessState summary_freshness = tensor::SummaryFreshnessState::STALE;

    /// Advisory routing score from summary ([0.0, 1.0]).
    float advisory_score = 0.0f;
};

// ============================================================================
// AccuracyMode — controls whether exact fetch is forced
// ============================================================================

/**
 * @brief Accuracy mode for shard routing.
 *
 * - ADVISORY : use summaries where fresh; escalate only on staleness.
 * - EXACT    : always issue exact fetch, bypassing summaries entirely.
 */
enum class AccuracyMode : uint8_t {
    /// Use advisory summaries; escalate to exact only when summary is stale.
    ADVISORY = 0,
    /// Always fetch exact fragments; summaries are only used for shard selection.
    EXACT = 1,
};

// ============================================================================
// ExactFetchRequest — request for exact-on-demand tensor fragment
// ============================================================================

/**
 * @brief Request to fetch an exact tensor fragment from a specific shard.
 *
 * Issued when summary-first routing escalates due to staleness or when the
 * accuracy mode is EXACT.
 */
struct ExactFetchRequest {
    /// Shard to fetch from.
    std::string shard_id;

    /// Artifact identifier on the shard.
    std::string artifact_id;

    /// Timeout for the fetch in milliseconds.
    uint32_t timeout_ms = 5000;

    /// If true, request is deprioritised (best-effort background refresh).
    bool background_refresh = false;

    /// Correlation ID for distributed tracing.
    std::string correlation_id;
};

// ============================================================================
// ExactFetchResult — result of an exact-on-demand tensor fetch
// ============================================================================

/**
 * @brief Result of an exact-on-demand tensor fragment fetch.
 *
 * Contains the raw fragment data, integrity checksum, and fetch metadata.
 * The caller MUST verify `success == true` and check `content_hash` against
 * the manifest before consuming `fragment_data`.
 */
struct ExactFetchResult {
    /// Shard that was queried.
    std::string shard_id;

    /// Artifact that was fetched.
    std::string artifact_id;

    /// Whether the fetch completed successfully.
    bool success = false;

    /// Error description if fetch failed.
    std::string error_reason;

    /// Raw tensor fragment bytes (opaque; consumer-specific encoding).
    std::vector<uint8_t> fragment_data;

    /// SHA-256 hex digest of `fragment_data` (empty if fetch failed).
    std::string content_hash;

    /// Epoch-milliseconds when the fragment was fetched.
    int64_t fetched_at_ms = 0;

    /// Round-trip latency in milliseconds.
    float fetch_latency_ms = 0.0f;

    /// Whether the content_hash matched the manifest's expected checksum.
    bool integrity_verified = false;
};

// ============================================================================
// FreshnessConsensusResult — multi-shard quorum outcome
// ============================================================================

/**
 * @brief Result of a multi-shard freshness consensus check.
 *
 * The planner uses this to determine whether to skip exact fetch for all
 * shards or only for the fresh subset.
 */
struct FreshnessConsensusResult {
    /// Total shards checked.
    std::size_t total_shards = 0;

    /// Number of shards with FRESH summaries.
    std::size_t fresh_shards = 0;

    /// Number of shards with STALE summaries (will be escalated).
    std::size_t stale_shards = 0;

    /// Number of shards with INVALID summaries (skipped in routing).
    std::size_t invalid_shards = 0;

    /// Whether the quorum threshold was met (fresh_shards / total >= quorum_ratio).
    bool quorum_met = false;

    /// Quorum ratio that was checked.
    float quorum_ratio = 0.75f;
};

// ============================================================================
// IShardFetcher — interface for exact fragment retrieval
// ============================================================================

/**
 * @brief Interface for fetching exact tensor fragments from shards.
 *
 * Implementations handle shard-local I/O, serialization, and error handling.
 * A default no-op stub is provided for testing; production paths must supply
 * a real implementation.
 */
class IShardFetcher {
public:
    virtual ~IShardFetcher() = default;

    /**
     * @brief Fetch a single exact tensor fragment.
     *
     * @param request  Fetch request describing shard and artifact.
     * @return Fetch result (check `success` before consuming data).
     */
    [[nodiscard]] virtual ExactFetchResult fetch(
        const ExactFetchRequest& request) const noexcept = 0;
};

// ============================================================================
// ShardSummaryCoordinator — Phase C coordinator
// ============================================================================

/**
 * @brief Coordinator implementing Phase C shard summary refresh,
 *        summary-first routing with escalation, and exact-on-demand fetch.
 *
 * ### Lifecycle
 *
 * 1. Register shards via `registerShard()`.
 * 2. Call `refreshShard()` / `refreshAll()` periodically (or on-demand) to
 *    pull updated summaries from the shard layer.
 * 3. Call `routeSummaryFirst()` to obtain per-shard routing decisions.
 * 4. For shards where `RoutingDecision::escalate_to_exact == true`, call
 *    `fetchExact()` to retrieve the exact tensor fragment.
 * 5. Optionally call `checkFreshnessConsensus()` to determine whether a
 *    quorum of shards are fresh before deciding to skip exact fetch globally.
 *
 * ### Configuration
 *
 * Default TTL is 3600 seconds. The quorum ratio is 0.75 (75 % of shards must
 * be FRESH for `FreshnessConsensusResult::quorum_met` to be true).
 */
class ShardSummaryCoordinator {
public:
    /**
     * @brief Configuration for the coordinator.
     *
     * ### Byzantine Quorum Safety (SG-DT-01)
     *
     * The `freshness_quorum_ratio` enforces minimum majority participation:
     * - Must be >= 0.5f (50% + 1) to satisfy Byzantine Fault Tolerance
     * - Default 0.75f (75%) provides safety margin for transient failures
     * - Clamped to [0.5f, 1.0f] at construction and via setConfig()
     */
    struct Config {
        /// Default freshness TTL for newly registered shards (seconds).
        uint32_t default_ttl_seconds;

        /// Quorum ratio: fraction of shards that must be FRESH to skip exact fetch.
        /// MUST be >= 0.5f for Byzantine Fault Tolerance (majority quorum).
        /// Default: 0.75f (75%) — provides safety margin.
        /// Recommended values: 0.666f (2/3) or 0.75f (3/4).
        float freshness_quorum_ratio;

        /// Maximum exact fetch timeout in milliseconds.
        uint32_t exact_fetch_timeout_ms;

        /// If true, STALE shards are always escalated; if false, they are skipped.
        bool escalate_stale_shards;

        /// If true, INVALID shards are skipped entirely (not included in routing).
        bool skip_invalid_shards;

        /**
         * @brief Default constructor with proper default values.
         *
         * Initializes freshness_quorum_ratio to 0.75f (75%), which satisfies
         * Byzantine majority requirements. The constructor validates and clamps
         * all values to their safe ranges.
         */
        Config() noexcept
            : default_ttl_seconds(3600),
              freshness_quorum_ratio(0.75f),
              exact_fetch_timeout_ms(5000),
              escalate_stale_shards(true),
              skip_invalid_shards(true) {
            validateAndClamp();
        }

        /**
         * @brief Validate and clamp all configuration values to safe ranges.
         *
         * Ensures freshness_quorum_ratio >= 0.5f (Byzantine majority).
         * If clamping occurs, behavior is logged (typically WARN level).
         */
        void validateAndClamp() noexcept;

        /**
         * @brief Check if freshness_quorum_ratio meets Byzantine requirements.
         *
         * @return true if freshness_quorum_ratio >= 0.5f
         */
        [[nodiscard]] bool isQuorumSafe() const noexcept {
            return freshness_quorum_ratio >= 0.5f;
        }
    };

    /**
     * @brief Construct with optional shard fetcher and manifest store.
     *
     * @param fetcher        Implementation for exact fragment retrieval.  May be
     *                       nullptr (exact fetch will always fail gracefully).
     * @param manifest_store Advisory manifest registry.  May be nullptr.
     * @param config         Coordinator configuration.
     */
    explicit ShardSummaryCoordinator(
        std::shared_ptr<IShardFetcher> fetcher = nullptr,
        ManifestStore* manifest_store = nullptr,
        Config config = Config()) noexcept;

    ~ShardSummaryCoordinator() = default;

    // Prevent copy; allow move.
    ShardSummaryCoordinator(const ShardSummaryCoordinator&) = delete;
    ShardSummaryCoordinator& operator=(const ShardSummaryCoordinator&) = delete;
    ShardSummaryCoordinator(ShardSummaryCoordinator&&) noexcept = default;
    ShardSummaryCoordinator& operator=(ShardSummaryCoordinator&&) noexcept = default;

    // ─── Shard Registration ───────────────────────────────────────────────

    /**
     * @brief Register a shard for coordination.
     *
     * If the shard is already registered the existing record is not replaced.
     *
     * @param shard_id    Shard identifier.
     * @param ttl_seconds Per-shard TTL override (0 = use default_ttl_seconds).
     */
    void registerShard(const std::string& shard_id,
                       uint32_t ttl_seconds = 0) noexcept;

    /**
     * @brief Unregister a shard, removing its freshness record.
     *
     * @param shard_id  Shard identifier.
     */
    void unregisterShard(const std::string& shard_id) noexcept;

    // ─── Summary Refresh ─────────────────────────────────────────────────

    /**
     * @brief Refresh the advisory summary for a single shard.
     *
     * Marks the shard as FRESH and updates its freshness timestamp.  The
     * updated `ShardSummary` is stored in the provided summary (in-out) and
     * optionally persisted to the manifest store.
     *
     * @param shard_id       Shard to refresh.
     * @param summary        Advisory summary to update (in-out).
     * @param now_ms         Current epoch-milliseconds (0 = wall clock).
     * @return Refresh result.
     */
    [[nodiscard]] ShardSummaryRefreshResult refreshShard(
        const std::string& shard_id,
        tensor::ShardSummary& summary,
        int64_t now_ms = 0) noexcept;

    /**
     * @brief Refresh all registered shards using a caller-supplied summary map.
     *
     * Iterates over `summaries` and calls `refreshShard()` for each shard that
     * is registered.  Unregistered shard IDs in `summaries` are ignored.
     *
     * @param summaries  Map of shard_id → ShardSummary to refresh from.
     * @param now_ms     Current epoch-milliseconds (0 = wall clock).
     * @return Per-shard refresh results.
     */
    [[nodiscard]] std::vector<ShardSummaryRefreshResult> refreshAll(
        std::unordered_map<std::string, tensor::ShardSummary>& summaries,
        int64_t now_ms = 0) noexcept;

    // ─── Freshness Queries ────────────────────────────────────────────────

    /**
     * @brief Get the current freshness record for a shard.
     *
     * @param shard_id  Shard identifier.
     * @return Freshness record, or std::nullopt if shard not registered.
     */
    [[nodiscard]] std::optional<ShardFreshnessRecord> getFreshnessRecord(
        const std::string& shard_id) const noexcept;

    /**
     * @brief Check whether a shard summary is currently fresh.
     *
     * @param shard_id  Shard identifier.
     * @param now_ms    Current epoch-milliseconds (0 = wall clock).
     * @return true if the shard is registered and its summary is FRESH and not expired.
     */
    [[nodiscard]] bool isFresh(const std::string& shard_id,
                               int64_t now_ms = 0) const noexcept;

    /**
     * @brief Check multi-shard freshness consensus.
     *
     * @param shard_ids  Shard identifiers to check.
     * @param now_ms     Current epoch-milliseconds (0 = wall clock).
     * @return Consensus result with quorum assessment.
     */
    [[nodiscard]] FreshnessConsensusResult checkFreshnessConsensus(
        const std::vector<std::string>& shard_ids,
        int64_t now_ms = 0) const noexcept;

    // ─── Summary-First Routing with Escalation ───────────────────────────

    /**
     * @brief Perform summary-first routing over the given shard summaries.
     *
     * For each shard:
     * - If summary is FRESH → include, no escalation (unless mode == EXACT).
     * - If summary is STALE → include with escalation flag (exact fetch required).
     * - If summary is INVALID → skip (if `config.skip_invalid_shards`).
     *
     * @param summaries  Advisory shard summaries to route over.
     * @param mode       Accuracy mode (ADVISORY or EXACT).
     * @param now_ms     Current epoch-milliseconds (0 = wall clock).
     * @return Per-shard routing decisions.
     */
    [[nodiscard]] std::vector<RoutingDecision> routeSummaryFirst(
        const std::vector<tensor::ShardSummary>& summaries,
        AccuracyMode mode = AccuracyMode::ADVISORY,
        int64_t now_ms = 0) const noexcept;

    // ─── Exact-On-Demand Fetch ────────────────────────────────────────────

    /**
     * @brief Fetch an exact tensor fragment from a shard (on-demand).
     *
     * Called when routing escalates a shard to exact fetch.  The result
     * contains raw fragment bytes and a content_hash that the caller should
     * verify against the manifest before using.
     *
     * If no `IShardFetcher` was provided the result will have `success = false`
     * and `error_reason = "no_fetcher_configured"`.
     *
     * @param request  Describes which shard/artifact to fetch.
     * @return Fetch result (always set; success field indicates outcome).
     */
    [[nodiscard]] ExactFetchResult fetchExact(
        const ExactFetchRequest& request) const noexcept;

    /**
     * @brief Bulk exact fetch for all escalated routing decisions.
     *
     * Convenience helper that issues `fetchExact()` for every decision where
     * `escalate_to_exact == true`.
     *
     * @param decisions     Routing decisions from `routeSummaryFirst()`.
     * @param artifact_id   Artifact ID to pass in each fetch request.
     * @param correlation_id Tracing ID.
     * @return Fetch results for each escalated shard (1:1 with escalated decisions).
     */
    [[nodiscard]] std::vector<ExactFetchResult> fetchEscalated(
        const std::vector<RoutingDecision>& decisions,
        const std::string& artifact_id,
        const std::string& correlation_id = {}) const noexcept;

    // ─── Configuration & Statistics ──────────────────────────────────────

    /**
     * @brief Update the coordinator configuration.
     *
     * The configuration is validated and clamped to ensure Byzantine safety
     * requirements are met (freshness_quorum_ratio >= 0.5f). If the provided
     * config has an unsafe quorum ratio, it is silently adjusted and a WARN
     * diagnostic is logged.
     *
     * @param config New configuration; takes effect immediately.
     *               Must satisfy Byzantine safety requirements (SG-DT-01).
     */
    void setConfig(const Config& config) noexcept;

    /** @brief Return a copy of the current configuration. */
    [[nodiscard]] Config config() const noexcept;

    /**
     * @brief Operational statistics for observability.
     */
    struct Stats {
        /// Total `refreshShard()` calls that succeeded.
        uint64_t total_refreshes = 0;

        /// Total `refreshShard()` calls that failed.
        uint64_t total_refresh_failures = 0;

        /// Total routing decisions issued.
        uint64_t total_routing_decisions = 0;

        /// Total shards escalated to exact fetch.
        uint64_t total_escalations = 0;

        /// Total `fetchExact()` calls.
        uint64_t total_exact_fetches = 0;

        /// Total `fetchExact()` calls that succeeded.
        uint64_t total_exact_fetch_successes = 0;
    };

    /**
     * @brief Return a snapshot of current statistics.
     *
     * @return Current stats (atomic snapshot; thread-safe).
     */
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

    /// Return epoch-milliseconds; if `hint_ms` is non-zero return hint_ms.
    [[nodiscard]] static int64_t resolveNow(int64_t hint_ms) noexcept;
};

} // namespace distributed_tensor
} // namespace themis
