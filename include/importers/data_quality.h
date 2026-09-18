/**
 * @file data_quality.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "importers/schema_inference.h"
#include <string>
#include <vector>
#include <map>
#include <nlohmann/json.hpp>

namespace themis {
namespace importers {

constexpr uint8_t kMinQualityThreshold = 0;
constexpr uint8_t kMaxQualityThreshold = 100;
constexpr uint8_t kDefaultQualityThreshold = 50;  // 50% checks pass
constexpr size_t kMaxQualityCheckNameLength = 64;

struct QualityCheckResult {
    uint8_t score;                          ///< Overall quality score [0, 100] (bounded)
    std::string check_type;                 ///< Check type name (max 64 chars)
    bool passed;                            ///< Did the check pass?
    float null_coverage;                    ///< Null ratio [0.0, 1.0]
    std::string comment;                    ///< Additional context (max 256 chars)

    /**
     * @brief To Json.
     * @return Return value.
     */
    json toJson() const;
};

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

        /**
         * @brief To Json.
         * @return Return value.
         */
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
        DataQualityMetrics assessTable(
            const std::string& table_name,
            const std::vector<json>& sample_data,
            const std::map<std::string, ColumnStatistics>& stats = {}
        );

        QualityReport generateQualityReport(
            const std::vector<InferenceTableSchema>& schemas,
            const std::vector<SampleData>& samples = {},
            const std::map<std::string, ColumnStatistics>& stats = {}
        );

        QualityCheckResult scoreWithAudit(
            const std::string& table_name,
            const std::vector<json>& sample_data,
            const std::string& check_type,
            const std::string& audit_event_id,
            const std::map<std::string, ColumnStatistics>& stats = {},
            const std::string& bypass_reason = ""
        );

    private:
        /**
         * @brief Compute Completeness.
         * @param[in] rows Input parameter.
         * @param[in] column Input parameter.
         * @return Return value.
         */
        double computeCompleteness(const std::vector<json>& rows,
                                   const std::string& column) const;
        /**
         * @brief Compute Uniqueness.
         * @param[in] rows Input parameter.
         * @param[in] column Input parameter.
         * @return Return value.
         */
        double computeUniqueness(const std::vector<json>& rows,
                                 const std::string& column) const;
        /**
         * @brief Compute Validity.
         * @param[in] rows Input parameter.
         * @param[in] column Input parameter.
         * @param[in] declared_type Input parameter.
         * @return Return value.
         */
        double computeValidity(const std::vector<json>& rows,
                               const std::string& column,
                               const std::string& declared_type) const;
    };
};

} // namespace importers
} // namespace themis
