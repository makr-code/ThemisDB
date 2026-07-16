/**
 * @file data_quality.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=7, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: data_quality.cpp | Version: 0.0.13 | Last Modified: 2026-05-31 12:17:24
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 229
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=10, L=0
 * PR History (last 5): none
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#include "importers/data_quality.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <set>
#include <sstream>

namespace themis {
namespace importers {

// ---------------------------------------------------------------------------
// DataQualityMetrics::toJson
// ---------------------------------------------------------------------------

json DataQualityFramework::DataQualityMetrics::toJson() const {
    return json{{"completeness", completeness},
                {"accuracy", accuracy},
                {"consistency", consistency},
                {"validity", validity},
                {"timeliness", timeliness},
                {"uniqueness", uniqueness},
                {"overall_quality_score", overall_quality_score}};
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

double DataQualityFramework::QualityAssessor::computeCompleteness(const std::vector<json> &rows,
                                                                  const std::string &column) const {
    if (rows.empty()) {
        return 0.0;
    }
    size_t non_null = 0;
    for (const auto &row : rows) {
        if (row.contains(column) && !row.at(column).is_null()) {
            ++non_null;
        }
    }
    return static_cast<double>(non_null) / rows.size();
}

double DataQualityFramework::QualityAssessor::computeUniqueness(const std::vector<json> &rows,
                                                                const std::string &column) const {
    if (rows.empty()) {
        return 1.0;
    }
    std::set<std::string> seen;
    for (const auto &row : rows) {
        if (row.contains(column)) {
            seen.insert(row.at(column).dump());
        }
    }
    return static_cast<double>(seen.size()) / rows.size();
}

double DataQualityFramework::QualityAssessor::computeValidity(const std::vector<json> &rows, const std::string &column,
                                                              const std::string &declared_type) const {
    if (rows.empty()) {
        return 1.0;
    }
    size_t valid = 0;
    for (const auto &row : rows) {
        if (!row.contains(column) || row.at(column).is_null()) {
            ++valid;
            continue;
        }
        const auto &v = row.at(column);
        if (declared_type == "integer" || declared_type == "bigint" || declared_type == "long") {
            if (v.is_number_integer()) {
                ++valid;
            }
        } else if (declared_type == "double" || declared_type == "real" || declared_type == "float") {
            if (v.is_number()) {
                ++valid;
            }
        } else if (declared_type == "boolean") {
            if (v.is_boolean()) {
                ++valid;
            }
        } else {
            // String-like: always valid
            ++valid;
        }
    }
    return static_cast<double>(valid) / rows.size();
}

// ---------------------------------------------------------------------------
// assessTable
// ---------------------------------------------------------------------------

DataQualityFramework::DataQualityMetrics
DataQualityFramework::QualityAssessor::assessTable(const std::string & /*table_name*/,
                                                   const std::vector<json> &sample_data,
                                                   const std::map<std::string, ColumnStatistics> &stats) {
    DataQualityMetrics metrics;
    if (sample_data.empty()) {
        metrics.overall_quality_score = 0.0;
        return metrics;
    }

    // Collect all column names from sample
    std::set<std::string> columns;
    for (const auto &row : sample_data) {
        if (row.is_object()) {
            for (auto it = row.begin(); it != row.end(); ++it) {
                columns.insert(it.key());
            }
        }
    }
    if (columns.empty()) {
        metrics.overall_quality_score = 0.0;
        return metrics;
    }

    double completeness_sum = 0.0;
    double uniqueness_sum   = 0.0;
    double validity_sum     = 0.0;

    for (const auto &col : columns) {
        completeness_sum += computeCompleteness(sample_data, col);
        uniqueness_sum += computeUniqueness(sample_data, col);
        validity_sum += computeValidity(sample_data, col, "string"); // default type
    }

    double n             = static_cast<double>(columns.size());
    metrics.completeness = completeness_sum / n;
    metrics.uniqueness   = uniqueness_sum / n;
    metrics.validity     = validity_sum / n;

    // Accuracy: based on null/distinct ratio from stats if available
    if (!stats.empty()) {
        double acc_sum = 0.0;
        size_t acc_cnt = 0;
        for (const auto &[key, st] : stats) {
            if (st.total_rows > 0) {
                acc_sum += 1.0 - static_cast<double>(st.null_count) / st.total_rows;
                ++acc_cnt;
            }
        }
        metrics.accuracy = acc_cnt > 0 ? acc_sum / acc_cnt : metrics.completeness;
    } else {
        metrics.accuracy = metrics.completeness;
    }

    // Consistency: heuristic based on completeness + validity
    metrics.consistency = (metrics.completeness + metrics.validity) / 2.0;

    // Timeliness: default 1.0 (no time reference available without config)
    metrics.timeliness = 1.0;

    // Weighted overall score [0,100]
    // Weights: completeness=0.25, accuracy=0.20, consistency=0.15,
    //          validity=0.20, timeliness=0.10, uniqueness=0.10
    metrics.overall_quality_score
        = 100.0
          * (0.25 * metrics.completeness + 0.20 * metrics.accuracy + 0.15 * metrics.consistency
             + 0.20 * metrics.validity + 0.10 * metrics.timeliness + 0.10 * metrics.uniqueness);

    return metrics;
}

// ---------------------------------------------------------------------------
// generateQualityReport
// ---------------------------------------------------------------------------

DataQualityFramework::QualityReport
DataQualityFramework::QualityAssessor::generateQualityReport(const std::vector<InferenceTableSchema> &schemas,
                                                             const std::vector<SampleData> &samples,
                                                             const std::map<std::string, ColumnStatistics> &stats) {
    QualityReport report;

    // Timestamp
    auto now = std::chrono::system_clock::now();
    auto t   = std::chrono::system_clock::to_time_t(now);
    std::ostringstream ts;
    ts << std::put_time(std::gmtime(&t), "%Y-%m-%dT%H:%M:%SZ");
    report.generation_timestamp = ts.str();

    report.metadata = json{{"standard", "NIST SP 800-188"},
                           {"framework_version", "2.2.0"},
                           {"tables_assessed", schemas.size()},
                           {"generation_timestamp", report.generation_timestamp}};

    // Build per-table sample index
    std::map<std::string, std::vector<json>> table_samples;
    for (const auto &s : samples) {
        for (const auto &v : s.values) {
            table_samples[s.table_name].push_back(json{{s.column_name, v}});
        }
    }

    // Filter stats per table
    for (const auto &schema : schemas) {
        std::map<std::string, ColumnStatistics> table_stats;
        for (const auto &[key, st] : stats) {
            if (st.table_name == schema.name) {
                table_stats[key] = st;
            }
        }

        auto &rows                       = table_samples[schema.name];
        auto metrics                     = assessTable(schema.name, rows, table_stats);
        report.table_scores[schema.name] = metrics;

        if (metrics.overall_quality_score < 60.0) {
            report.issues.push_back("Table '" + schema.name + "' has low quality score: "
                                    + std::to_string(static_cast<int>(metrics.overall_quality_score)));
            report.recommendations.push_back("Investigate null values and type mismatches in '" + schema.name + "'");
        }
    }

    return report;
}

} // namespace importers
} // namespace themis
