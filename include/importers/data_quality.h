/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            data_quality.h                                     ║
  Version:         0.0.11                                             ║
  Last Modified:   2026-04-15 07:06:50                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     114                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 9efa3acd76  2026-03-11  feat(importers): add PostgreSQL Importer v2.1+ with 12 ne... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "importers/schema_inference.h"
#include <string>
#include <vector>
#include <map>
#include <nlohmann/json.hpp>

namespace themis {
namespace importers {

/**
 * @brief NIST SP 800-188 compliant Data Quality Framework.
 *
 * Evaluates six quality dimensions (completeness, accuracy, consistency,
 * validity, timeliness, uniqueness) and emits structured reports.
 *
 * Standards:
 *   - NIST SP 800-188 (Data Quality)
 *   - FAIR Data Principles (Findable, Accessible, Interoperable, Reusable)
 *   - DIN EN 15943
 */
class DataQualityFramework {
public:
    // ------------------------------------------------------------------
    // Metrics
    // ------------------------------------------------------------------
    struct DataQualityMetrics {
        double completeness{0.0};   ///< Fraction of non-null values  [0,1]
        double accuracy{0.0};       ///< Pattern / format conformance  [0,1]
        double consistency{0.0};    ///< Referential integrity score   [0,1]
        double validity{0.0};       ///< Type conformance              [0,1]
        double timeliness{0.0};     ///< Recency score                 [0,1]
        double uniqueness{0.0};     ///< 1 – duplicate_rate            [0,1]
        double overall_quality_score{0.0}; ///< Weighted average       [0,100]

        json toJson() const;
    };

    // ------------------------------------------------------------------
    // Report
    // ------------------------------------------------------------------
    struct QualityReport {
        json metadata;
        std::map<std::string, DataQualityMetrics> table_scores;
        std::vector<std::string> issues;
        std::vector<std::string> recommendations;
        std::string generation_timestamp; ///< ISO 8601
    };

    // ------------------------------------------------------------------
    // Assessor
    // ------------------------------------------------------------------
    class QualityAssessor {
    public:
        /**
         * @brief Compute quality metrics for a single table.
         * @param table_name  Name of the table being assessed.
         * @param sample_data  Sampled rows (JSON objects).
         * @param stats       Optional pre-computed column statistics.
         */
        DataQualityMetrics assessTable(
            const std::string& table_name,
            const std::vector<json>& sample_data,
            const std::map<std::string, ColumnStatistics>& stats = {}
        );

        /**
         * @brief Generate a full quality report for all tables.
         * @param schemas   Schema descriptions.
         * @param samples   Per-column sample data.
         * @param stats     Column statistics.
         */
        QualityReport generateQualityReport(
            const std::vector<InferenceTableSchema>& schemas,
            const std::vector<SampleData>& samples = {},
            const std::map<std::string, ColumnStatistics>& stats = {}
        );

    private:
        double computeCompleteness(const std::vector<json>& rows,
                                   const std::string& column) const;
        double computeUniqueness(const std::vector<json>& rows,
                                 const std::string& column) const;
        double computeValidity(const std::vector<json>& rows,
                               const std::string& column,
                               const std::string& declared_type) const;
    };
};

} // namespace importers
} // namespace themis
