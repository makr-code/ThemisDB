/**
 * @file predictive_prefetcher.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Copyright 2025 ThemisDB
// Licensed under MIT License

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <array>
#include <mutex>
#include <cstdint>
#include <ctime>
#include <nlohmann/json.hpp>

namespace themis {
// Forward declaration – avoids pulling in all of rocksdb_wrapper.h
class RocksDBWrapper;

namespace cache {

/**
 * @brief Predictive pre-fetcher based on query sequence history.
 *
 * Uses a first-order Markov chain (bigram model) to predict which query
 * fingerprints are likely to be accessed next given the current fingerprint.
 *
 * For each observed (from → to) fingerprint transition a count is maintained.
 * `getPrefetchCandidates()` returns the top-N successors ranked by their
 * transition frequency relative to the observed total for that source key.
 *
 * Thread-Safety:
 * - All operations are protected by a single internal mutex.
 *
 * Memory Bounds:
 * - At most `max_tracked_keys` distinct source keys are tracked; once the
 *   table is full the oldest source key is evicted (FIFO order).
 * - Each source key stores at most `max_successors_per_key` successor counts.
 *
 * Metrics:
 * - `getStats()` reports `tracked_keys`, `total_transitions_recorded`,
 *   `candidates_generated`, and `prefetch_hits` (caller-incremented via
 *   `recordPrefetchHit()`).
 */
class PredictivePrefetcher {
public:
    struct Config {
        /// Maximum distinct source keys tracked in the transition table.
        size_t max_tracked_keys = 5000;

        /// Maximum successor entries per source key.
        size_t max_successors_per_key = 20;

        /// Minimum number of observed transitions for a successor to be
        /// returned as a candidate.  Filters noise from cold entries.
        uint32_t min_transition_count = 2;

        /// Maximum number of candidate fingerprints returned by
        /// getPrefetchCandidates().
        size_t max_predictions = 3;

        /// Minimum prediction confidence [0.0, 1.0]: fraction of transitions
        /// from the source key that must lead to the successor.
        double min_confidence = 0.0;

        /// When true, prediction scores are weighted by time-of-day access
        /// frequency (24 one-hour buckets).
        bool enable_time_of_day_weighting = false;

        /// When true, exactly 50 % of tenants are routed to the Markov model
        /// (with time-of-day weighting) and the other 50 % to the frequency
        /// baseline.  The split is deterministic: hash(tenant_id) % 2.
        bool enable_ab_test = false;

        static Config defaults() { return {}; }
    };

    explicit PredictivePrefetcher(const Config& config = Config::defaults());
    ~PredictivePrefetcher() = default;

    PredictivePrefetcher(const PredictivePrefetcher&) = delete;
    PredictivePrefetcher& operator=(const PredictivePrefetcher&) = delete;

    /**
     * @brief Record a query access and update the transition model.
     *
     * If `tenant_id` is non-empty the per-tenant last-access state is used so
     * that sessions from different tenants do not bleed into each other.
     *
     * @param fingerprint  SHA-256 hex fingerprint of the accessed query.
     * @param tenant_id    Optional tenant identifier (empty = global session).
     */
    void recordQueryAccess(const std::string& fingerprint,
                           const std::string& tenant_id = "");

    /**
     * @brief Return candidate fingerprints likely to be accessed next.
     *
     * Candidates are sorted in descending order of transition frequency.
     * Only candidates whose observed frequency is ≥ `min_transition_count`
     * and whose confidence is ≥ `min_confidence` are included.
     *
     * @param fingerprint  Current query fingerprint (the "from" key).
     * @param tenant_id    Optional tenant identifier (empty = global session).
     * @return Up to `max_predictions` candidate fingerprints.
     */
    std::vector<std::string> getPrefetchCandidates(
        const std::string& fingerprint,
        const std::string& tenant_id = "") const;

    /**
     * @brief Record that a prefetch candidate was already in the cache.
     *
     * Used externally by AdaptiveQueryCache to track effective prefetch hits
     * for metrics purposes.
     *
     * @param tenant_id  Optional tenant identifier; used to attribute the hit
     *                   to the correct A/B group when `enable_ab_test` is true.
     */
    void recordPrefetchHit(const std::string& tenant_id = "");

    /**
     * @brief Record that prefetch candidates were generated for a key.
     *
     * Called by getPrefetchCandidates() when at least one candidate is returned.
     * Also emits the current `cache.prefetch.hit_rate` gauge to MetricsCollector
     * so the gauge stays fresh even when hits are sparse.
     *
     * @param count      Number of candidates generated (1..max_predictions).
     * @param tenant_id  Optional tenant identifier; used to attribute the
     *                   generation event to the correct A/B group.
     */
    void recordCandidatesGenerated(size_t count = 1,
                                   const std::string& tenant_id = "");

    /**
     * @brief Track bytes fetched via prefetch that were never subsequently hit.
     *
     * The caller (e.g. AdaptiveQueryCache) should call this when a prefetched
     * entry expires or is evicted before being accessed.  Used to report the
     * `cache.prefetch.overhead_bytes` metric.
     *
     * @param bytes Number of overhead bytes to record.
     */
    void recordOverheadBytes(uint64_t bytes);

    /**
     * @brief Persist the Markov transition matrix to RocksDB.
     *
     * Keys are written under the prefix `prefetch_model::`.  Each key encodes
     * the (from, to) fingerprint pair; the value is a JSON object containing
     * the raw transition count and the 24-bucket time-of-day histogram.
     *
     * @param db  Open RocksDBWrapper instance.  If null this is a no-op.
     */
    void saveModel(RocksDBWrapper* db);

    /**
     * @brief Restore the Markov transition matrix from RocksDB.
     *
     * Scans `prefetch_model::` prefix and populates the in-memory transition
     * table.  Existing in-memory state is merged (not replaced) so that
     * concurrent learning is not lost.
     *
     * @param db  Open RocksDBWrapper instance.  If null this is a no-op.
     */
    void loadModel(RocksDBWrapper* db);

    /**
     * @brief Clear all transition state and reset counters.
     */
    void clear();

    /**
     * @brief Get operational statistics as JSON.
     */
    nlohmann::json getStats() const;

private:
    Config config_;

    // Transition table: source_key -> { successor_key -> count }
    // Insertion order of source keys tracked via ordered_keys_ for FIFO eviction.
    std::unordered_map<std::string,
                       std::unordered_map<std::string, uint32_t>> transitions_;
    std::vector<std::string> ordered_keys_;  // FIFO eviction order for source keys

    // Per-tenant (or global if empty) last-seen fingerprint for session tracking.
    std::unordered_map<std::string, std::string> last_fingerprint_;

    // Time-of-day access counts: from -> to -> hour[0..23].
    // Tracked when config_.enable_time_of_day_weighting is true.
    std::unordered_map<std::string,
        std::unordered_map<std::string,
            std::array<uint32_t, 24>>> tod_buckets_;

    mutable std::mutex mutex_;

    // Metrics
    uint64_t total_transitions_recorded_ = 0;
    uint64_t candidates_generated_ = 0;
    uint64_t prefetch_hits_ = 0;
    uint64_t overhead_bytes_ = 0;

    // A/B group hit-rate counters (only meaningful when enable_ab_test is true).
    // Declared mutable so they can be updated from the const getPrefetchCandidates().
    mutable uint64_t ab_markov_hits_ = 0;
    mutable uint64_t ab_markov_generated_ = 0;
    mutable uint64_t ab_baseline_hits_ = 0;
    mutable uint64_t ab_baseline_generated_ = 0;

    // Internal helpers
    /// Returns true if ToD weighting should be applied for this tenant.
    /// When enable_ab_test is true: group 0 (fnv1a(tenant_id) % 2 == 0) uses
    /// Markov + ToD; group 1 uses raw Markov frequency without ToD weighting.
    bool useToDWeighting(const std::string& tenant_id) const;

    /// Stable FNV-1a hash of a string – used for deterministic A/B routing.
    static uint64_t fnv1aHash(const std::string& s);

    /// Return the current wall-clock hour in [0, 23].
    static int currentHour();

    /// Emit `cache.prefetch.hit_rate` and `cache.prefetch.overhead_bytes` via MetricsCollector.
    void emitMetrics() const;
};

} // namespace cache
} // namespace themis
