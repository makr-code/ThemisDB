/**
 * @file search_analytics.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.43
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <atomic>
#include <chrono>
#include <map>
#include <mutex>
#include <string>
#include <vector>

namespace themis {

/**
 * @brief A single recorded search event.
 */
struct SearchEvent {
    std::string query;                          ///< Original query text
    std::chrono::system_clock::time_point ts;   ///< Timestamp of the search
    size_t result_count = 0;                    ///< Number of results returned
    double latency_ms = 0.0;                    ///< Search latency in milliseconds
    bool is_zero_result = false;                ///< True when result_count == 0
};

/**
 * @brief Aggregated performance metrics snapshot.
 */
struct SearchMetrics {
    size_t total_queries = 0;          ///< Total recorded query events
    size_t zero_result_queries = 0;    ///< Queries that returned no results
    double avg_latency_ms = 0.0;       ///< Average search latency (ms)
    double p95_latency_ms = 0.0;       ///< 95th-percentile latency (ms)
    double p99_latency_ms = 0.0;       ///< 99th-percentile latency (ms)
    double zero_result_rate = 0.0;     ///< Fraction of zero-result queries [0,1]
    std::map<std::string, size_t> top_queries; ///< Most frequent queries (up to 20)
};

/**
 * @brief In-process search analytics: query logging, performance metrics, and
 *        zero-result detection.
 *
 * SearchAnalytics is a thread-safe, append-only log that records every search
 * event and can compute aggregated performance metrics on demand.  It is
 * designed to be used as a lightweight singleton or per-instance telemetry
 * collector alongside HybridSearch.
 *
 * ### Usage
 * ```cpp
 * SearchAnalytics analytics;
 *
 * // Record events from your search loop
 * auto t0 = std::chrono::steady_clock::now();
 * auto results = hs.search(query, vec.data(), vec.size());
 * double ms = std::chrono::duration<double, std::milli>(
 *     std::chrono::steady_clock::now() - t0).count();
 *
 * analytics.record(query, results.size(), ms);
 *
 * // Check for zero-result queries periodically
 * auto zero_queries = analytics.getZeroResultQueries(10);
 *
 * // Get a performance snapshot
 * auto metrics = analytics.computeMetrics();
 * if (metrics.zero_result_rate > 0.1) {
 *     // Alert: >10% zero-result queries
 * }
 * ```
 *
 * @note Thread Safety: **All methods are thread-safe** and may be called
 *   concurrently from multiple threads.  Uses a `std::mutex` internally.
 *
 * @note Memory: The event log is bounded by `Config::max_events`; when the
 *   limit is reached the oldest events are evicted (circular buffer semantics).
 */
class SearchAnalytics {
public:
    struct Config {
        size_t max_events = 10'000; ///< Maximum events to retain in memory
        static Config defaults() { return {}; }
    };

    explicit SearchAnalytics(const Config& config = Config::defaults());

    // -----------------------------------------------------------------------
    // Event recording
    // -----------------------------------------------------------------------

    /**
     * @brief Record a completed search event.
     *
     * @param query         The original search query.
     * @param result_count  Number of results returned.
     * @param latency_ms    Search latency in milliseconds.
     */
    void record(const std::string& query,
                size_t result_count,
                double latency_ms);

    // -----------------------------------------------------------------------
    // Querying
    // -----------------------------------------------------------------------

    /**
     * @brief Return the most recently recorded zero-result queries.
     *
     * @param limit  Maximum number of events to return (most recent first).
     * @return List of SearchEvent where result_count == 0.
     */
    std::vector<SearchEvent> getZeroResultQueries(size_t limit = 100) const;

    /**
     * @brief Return the N most recent recorded events.
     *
     * @param limit  Number of events to return (most recent first).
     */
    std::vector<SearchEvent> getRecentEvents(size_t limit = 100) const;

    /**
     * @brief Return the top queries ranked by frequency.
     *
     * Returns up to @p limit query strings with their occurrence counts,
     * sorted by descending frequency (most popular first).  This is a
     * lightweight alternative to @c computeMetrics() when only the top-query
     * list is needed.
     *
     * @param limit  Maximum number of entries to return (default: 20).
     * @return Vector of (query, count) pairs, most frequent first.
     */
    std::vector<std::pair<std::string, size_t>> getTopQueries(size_t limit = 20) const;

    /**
     * @brief Compute aggregated metrics over all retained events.
     *
     * Computing metrics is O(n) in the number of retained events.
     */
    SearchMetrics computeMetrics() const;

    /**
     * @brief Return the number of currently retained events.
     */
    size_t eventCount() const;

    /**
     * @brief Clear all retained events.
     */
    void clear();

    const Config& getConfig() const { return config_; }

private:
    Config config_;
    mutable std::mutex mu_;
    std::vector<SearchEvent> events_; // oldest → newest (circular eviction from front)
};

} // namespace themis
