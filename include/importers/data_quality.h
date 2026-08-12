/**
 * @file data_quality.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/**
 * @brief Quality score constants (Phase 2 T2.3.2).
 *
 * PHASE-2-HARDENING: Quality Score Bounds & Audit Integration
 * Bounded: all scores in [0, 100]
 * Audit: quality gates tracked in audit trail
 * Determinism: no randomness in scoring
 */
constexpr uint8_t kMinQualityThreshold = 0;
constexpr uint8_t kMaxQualityThreshold = 100;
constexpr uint8_t kDefaultQualityThreshold = 50;  // 50% checks pass
constexpr size_t kMaxQualityCheckNameLength = 64;

/**
 * @brief Quality check result structure (Phase 2 T2.3.2).
 *
 * PHASE-2-HARDENING: Quality Score Bounds & Audit Integration
 * Determinism: yes (formula is deterministic)
 * Audit: all results auditable with full context
 * Bounded: score always in [0, 100]
 */
struct QualityCheckResult {
    uint8_t score;                          ///< Overall quality score [0, 100] (bounded)
    std::string check_type;                 ///< Check type name (max 64 chars)
    bool passed;                            ///< Did the check pass?
    float null_coverage;                    ///< Null ratio [0.0, 1.0]
    std::string comment;                    ///< Additional context (max 256 chars)

    /**
     * @brief Convert result to JSON for audit trail.
     * @return JSON representation suitable for audit trail
     */
    json toJson() const;
};

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

        /**
         * @brief Compute quality score with audit integration (Phase 2 T2.3.2).
         *
         * PHASE-2-HARDENING: Quality Score Bounds & Audit Integration
         * Determinism: yes (formula is deterministic)
         * Audit: emits quality check or bypass events
         * Bounded: quality checks ≤ 500ms
         *
         * Applies the Phase 2 quality score formula:
         *   score = min(100, max(0, round(
         *     (pass_rate * 80) + ((100 - null_ratio) * 0.2)
         *   )))
         *
         * When quality gate bypass is needed (user override, timeout, schema mismatch),
         * emits audit event with full context.
         *
         * @param table_name      Name of the table being assessed.
         * @param sample_data     Sampled rows (JSON objects).
         * @param check_type      Type of quality check (e.g., "SCHEMA_MATCH", "NULL_RATIO").
         * @param audit_event_id  Correlation ID for audit trail linking.
         * @param stats           Optional pre-computed column statistics.
         * @param bypass_reason   Optional bypass reason; if provided, quality gate is bypassed
         *                       and audit event emitted with this reason.
         * @return                QualityCheckResult with score [0, 100] and audit context.
         */
        QualityCheckResult scoreWithAudit(
            const std::string& table_name,
            const std::vector<json>& sample_data,
            const std::string& check_type,
            const std::string& audit_event_id,
            const std::map<std::string, ColumnStatistics>& stats = {},
            const std::string& bypass_reason = ""
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
