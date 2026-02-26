// Copyright 2025 ThemisDB
// Licensed under MIT License

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <cstdint>
#include <nlohmann/json.hpp>

namespace themis {
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
     */
    void recordPrefetchHit();

    /**
     * @brief Record that prefetch candidates were generated for a key.
     *
     * Called automatically by getPrefetchCandidates() when at least one
     * candidate is returned.
     */
    void recordCandidatesGenerated();

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

    mutable std::mutex mutex_;

    // Metrics
    uint64_t total_transitions_recorded_ = 0;
    uint64_t candidates_generated_ = 0;
    uint64_t prefetch_hits_ = 0;
};

} // namespace cache
} // namespace themis
