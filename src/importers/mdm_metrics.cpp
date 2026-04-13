/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            mdm_metrics.cpp                                    ║
  Version:         0.0.4                                              ║
  Last Modified:   2026-04-13 04:25:57                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     130                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • fffcbc1048  2026-03-11  feat(importers): implement MDM entity matching, linking &... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "importers/mdm_metrics.h"

namespace themis {
namespace importers {

// ---------------------------------------------------------------------------
// MDMMetricSnapshot serialisation
// ---------------------------------------------------------------------------

json MDMMetricSnapshot::toJson() const {
    return json{
        {"deterministic_matches",       deterministic_matches},
        {"semantic_matches",            semantic_matches},
        {"avg_semantic_confidence",     avg_semantic_confidence},
        {"links_created",               links_created},
        {"links_with_conflicts",        links_with_conflicts},
        {"conflicts_auto_resolved",     conflicts_auto_resolved},
        {"conflicts_requiring_review",  conflicts_requiring_review},
        {"avg_resolution_confidence",   avg_resolution_confidence},
        {"duplicate_records_found",     duplicate_records_found},
        {"duplicate_records_merged",    duplicate_records_merged},
        {"avg_completeness_improvement", avg_completeness_improvement},
        {"matching_time_seconds",       matching_time_seconds},
        {"linking_time_seconds",        linking_time_seconds},
        {"resolution_time_seconds",     resolution_time_seconds}
    };
}

// ---------------------------------------------------------------------------
// MDMMetrics::emitMetrics
// ---------------------------------------------------------------------------

void MDMMetrics::emitMetrics(
    const MDMMetricSnapshot& snap,
    const std::string&       collection_name,
    const MetricsCallback&   callback
) {
    if (!callback) return;

    const std::map<std::string, std::string> labels{{"collection", collection_name}};

    callback("themisdb_mdm_deterministic_matches_total",
             labels, static_cast<double>(snap.deterministic_matches));
    callback("themisdb_mdm_semantic_matches_total",
             labels, static_cast<double>(snap.semantic_matches));
    callback("themisdb_mdm_avg_semantic_confidence",
             labels, snap.avg_semantic_confidence);
    callback("themisdb_mdm_links_created_total",
             labels, static_cast<double>(snap.links_created));
    callback("themisdb_mdm_links_with_conflicts_total",
             labels, static_cast<double>(snap.links_with_conflicts));
    callback("themisdb_mdm_conflicts_auto_resolved_total",
             labels, static_cast<double>(snap.conflicts_auto_resolved));
    callback("themisdb_mdm_conflicts_review_total",
             labels, static_cast<double>(snap.conflicts_requiring_review));
    callback("themisdb_mdm_duplicates_found_total",
             labels, static_cast<double>(snap.duplicate_records_found));
    callback("themisdb_mdm_duplicates_merged_total",
             labels, static_cast<double>(snap.duplicate_records_merged));
    callback("themisdb_mdm_matching_duration_seconds",
             labels, snap.matching_time_seconds);
    callback("themisdb_mdm_linking_duration_seconds",
             labels, snap.linking_time_seconds);
    callback("themisdb_mdm_resolution_duration_seconds",
             labels, snap.resolution_time_seconds);
}

// ---------------------------------------------------------------------------
// MDMMetrics::getDashboardMetrics
// ---------------------------------------------------------------------------

json MDMMetrics::getDashboardMetrics(
    const MDMMetricSnapshot& snap,
    const std::string&       collection_name
) {
    const double total_matches = static_cast<double>(
        snap.deterministic_matches + snap.semantic_matches);
    const double dedup_rate = (snap.duplicate_records_found > 0)
                              ? static_cast<double>(snap.duplicate_records_merged)
                                / static_cast<double>(snap.duplicate_records_found)
                              : 0.0;

    return json{
        {"collection",              collection_name},
        {"summary", {
            {"total_matches",           total_matches},
            {"deterministic_matches",   snap.deterministic_matches},
            {"semantic_matches",        snap.semantic_matches},
            {"avg_semantic_confidence", snap.avg_semantic_confidence},
            {"deduplication_rate",      dedup_rate},
            {"links_created",           snap.links_created},
            {"golden_records_created",  snap.duplicate_records_merged},
            {"manual_reviews_needed",   snap.conflicts_requiring_review}
        }},
        {"performance", {
            {"matching_time_seconds",   snap.matching_time_seconds},
            {"linking_time_seconds",    snap.linking_time_seconds},
            {"resolution_time_seconds", snap.resolution_time_seconds},
            {"total_time_seconds",      snap.matching_time_seconds
                                        + snap.linking_time_seconds
                                        + snap.resolution_time_seconds}
        }},
        {"raw", snap.toJson()}
    };
}

} // namespace importers
} // namespace themis
