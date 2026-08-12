/**
 * @file mdm_metrics.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "importers/importer_interface.h"
#include <string>
#include <nlohmann/json.hpp>

namespace themis {
namespace importers {

using json = nlohmann::json;

/**
 * @brief Snapshot of MDM metrics collected during a single workflow run.
 */
struct MDMMetricSnapshot {
    // Matching
    size_t deterministic_matches      = 0;
    size_t semantic_matches           = 0;
    double avg_semantic_confidence    = 0.0;

    // Linking
    size_t links_created              = 0;
    size_t links_with_conflicts       = 0;

    // Resolution
    size_t conflicts_auto_resolved    = 0;
    size_t conflicts_requiring_review = 0;
    double avg_resolution_confidence  = 0.0;

    // Deduplication
    size_t duplicate_records_found    = 0;
    size_t duplicate_records_merged   = 0;
    double avg_completeness_improvement = 0.0; ///< Average delta in completeness score

    // Performance (wall-clock seconds)
    double matching_time_seconds      = 0.0;
    double linking_time_seconds       = 0.0;
    double resolution_time_seconds    = 0.0;

    json toJson() const;
};

/**
 * @brief Observability layer for MDM workflow runs.
 *
 * Converts MDMMetricSnapshot values into Prometheus-style gauge / counter
 * payloads and exposes a dashboard-friendly JSON summary.
 *
 * Metrics are emitted via the optional MetricsCallback stored in
 * @c ImportOptions.  If no callback is set, emitMetrics() is a no-op.
 *
 * Thread-safety: all public methods are stateless and safe to call
 * from multiple threads.
 */
class MDMMetrics {
public:
    MDMMetrics() = default;

    /**
     * @brief Emit all metrics from a snapshot via the provided callback.
     *
     * Each metric is emitted as:
     *   metric name → labels map → value
     *
     * Standard metric names:
     *   "themisdb_mdm_deterministic_matches_total"
     *   "themisdb_mdm_semantic_matches_total"
     *   "themisdb_mdm_avg_semantic_confidence"
     *   "themisdb_mdm_links_created_total"
     *   "themisdb_mdm_links_with_conflicts_total"
     *   "themisdb_mdm_conflicts_auto_resolved_total"
     *   "themisdb_mdm_conflicts_review_total"
     *   "themisdb_mdm_duplicates_found_total"
     *   "themisdb_mdm_duplicates_merged_total"
     *   "themisdb_mdm_matching_duration_seconds"
     *   "themisdb_mdm_linking_duration_seconds"
     *   "themisdb_mdm_resolution_duration_seconds"
     *
     * @param snapshot         Metric values to emit.
     * @param collection_name  Label value for all emitted metrics.
     * @param callback         Destination callback (may be nullptr → no-op).
     */
    static void emitMetrics(
        const MDMMetricSnapshot& snapshot,
        const std::string&       collection_name,
        const MetricsCallback&   callback
    );

    /**
     * @brief Build a dashboard-friendly JSON summary from a snapshot.
     *
     * @param snapshot         Metric values.
     * @param collection_name  Collection label.
     * @return                 JSON object ready for serialisation.
     */
    static json getDashboardMetrics(
        const MDMMetricSnapshot& snapshot,
        const std::string&       collection_name
    );
};

} // namespace importers
} // namespace themis
