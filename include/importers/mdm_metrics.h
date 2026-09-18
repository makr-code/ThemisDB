/**
 * @file mdm_metrics.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

    /**
     * @brief To Json.
     * @return Return value.
     */
    json toJson() const;
};

class MDMMetrics {
public:
    MDMMetrics() = default;

    /**
     * @brief Emit Metrics.
     * @param[in] snapshot Input parameter.
     * @param[in] collection_name Name of the collection.
     * @param[in] callback Input parameter.
     */
    static void emitMetrics(
        const MDMMetricSnapshot& snapshot,
        const std::string&       collection_name,
        const MetricsCallback&   callback
    );

    /**
     * @brief Get Dashboard Metrics.
     * @param[in] snapshot Input parameter.
     * @param[in] collection_name Name of the collection.
     * @return Return value.
     */
    static json getDashboardMetrics(
        const MDMMetricSnapshot& snapshot,
        const std::string&       collection_name
    );
};

} // namespace importers
} // namespace themis
