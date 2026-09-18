/**
 * @file predictive_prefetcher.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

class PredictivePrefetcher {
public:
    struct Config {
        size_t max_tracked_keys = 5000;

        size_t max_successors_per_key = 20;

        uint32_t min_transition_count = 2;

        size_t max_predictions = 3;

        double min_confidence = 0.0;

        bool enable_time_of_day_weighting = false;

        bool enable_ab_test = false;

        /**
         * @brief Defaults.
         * @return Return value.
         * @details Implements defaults without additional internal calls.
         */
        static Config defaults() { return {}; }
    };

    explicit PredictivePrefetcher(const Config& config = Config::defaults());
    ~PredictivePrefetcher() = default;

    PredictivePrefetcher(const PredictivePrefetcher&) = delete;
    PredictivePrefetcher& operator=(const PredictivePrefetcher&) = delete;

    void recordQueryAccess(const std::string& fingerprint,
                           const std::string& tenant_id = "");

    std::vector<std::string> getPrefetchCandidates(
        const std::string& fingerprint,
        const std::string& tenant_id = "") const;

    void recordPrefetchHit(const std::string& tenant_id = "");

    void recordCandidatesGenerated(size_t count = 1,
                                   const std::string& tenant_id = "");

    /**
     * @brief Record Overhead Bytes.
     * @param[in] bytes Input parameter.
     */
    void recordOverheadBytes(uint64_t bytes);

    /**
     * @brief Save Model.
     * @param[in,out] db Input/output parameter.
     */
    void saveModel(RocksDBWrapper* db);

    /**
     * @brief Load Model.
     * @param[in,out] db Input/output parameter.
     */
    void loadModel(RocksDBWrapper* db);

    /**
     * @brief Clear.
     */
    void clear();

    /**
     * @brief Get Stats.
     * @return Return value.
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
    /**
     * @brief Use To DWeighting.
     * @param[in] tenant_id Identifier of the tenant.
     * @return True when the operation succeeds.
     */
    bool useToDWeighting(const std::string& tenant_id) const;

    /**
     * @brief Fnv1a Hash.
     * @param[in] s Input parameter.
     * @return Return value.
     */
    static uint64_t fnv1aHash(const std::string& s);

    /**
     * @brief Current Hour.
     * @return Return value.
     */
    static int currentHour();

    /**
     * @brief Emit Metrics.
     */
    void emitMetrics() const;
};

} // namespace cache
} // namespace themis
