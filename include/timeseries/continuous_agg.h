#ifndef THEMIS_CONTINUOUS_AGG_H
#define THEMIS_CONTINUOUS_AGG_H

#include <string>
#include <chrono>
#include <optional>
#include <vector>

namespace themis {

class TSStore;

struct AggWindow {
    std::chrono::milliseconds size{std::chrono::minutes(1)};
};

enum class AggFunc { Min, Max, Avg, Sum, Count };

struct AggConfig {
    std::string metric;
    std::optional<std::string> entity; // nullopt = for all entities (not supported in MVP)
    AggWindow window;
    // For MVP we always compute min/max/avg/sum/count and store avg as value with metadata for others
};

/**
 * Rollup hierarchy definition.
 * Each level aggregates the previous level's output.
 * E.g.: {1m, 5m, 1h, 1d} means:
 *   raw → 1m aggregates → 5m aggregates → 1h aggregates → 1d aggregates
 */
struct RollupHierarchy {
    std::string metric;
    std::optional<std::string> entity;
    std::vector<std::chrono::milliseconds> levels;  // ordered from smallest to largest

    // Default hierarchy: 1m → 5m → 1h → 1d
    static RollupHierarchy defaultHierarchy(const std::string& metric,
                                             const std::optional<std::string>& entity = std::nullopt);
};

class ContinuousAggregateManager {
public:
    explicit ContinuousAggregateManager(TSStore* store) : store_(store) {}
    // Compute aggregates for [from,to] and store as derived metric
    // Derived metric name: metric + "__agg_" + window_ms
    void refresh(const AggConfig& cfg, int64_t from_ms, int64_t to_ms);

    /**
     * Refresh all levels of a rollup hierarchy.
     * Processes levels from smallest window to largest.
     * Each level reads from the previous level's output (or raw data for the first level).
     * @param hierarchy   Rollup level definitions
     * @param from_ms     Start of refresh window (milliseconds)
     * @param to_ms       End of refresh window (milliseconds)
     */
    void refreshHierarchy(const RollupHierarchy& hierarchy, int64_t from_ms, int64_t to_ms);

    static std::string derivedMetricName(const std::string& base, std::chrono::milliseconds win);

private:
    TSStore* store_;
};

} // namespace themis

#endif // THEMIS_CONTINUOUS_AGG_H
