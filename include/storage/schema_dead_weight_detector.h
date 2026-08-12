/**
 * @file schema_dead_weight_detector.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "llm/decision_record_yaml_processor.h"

#include <chrono>
#include <cstdint>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace themis {
namespace storage {

// ---------------------------------------------------------------------------
// Supporting input types
// ---------------------------------------------------------------------------

/// @brief Per-field access time-series entry: (timestamp, read_count).
using AccessEntry = std::pair<std::chrono::system_clock::time_point, uint64_t>;

/// @brief Access statistics for all fields / collections in the schema.
///
/// Key: fully-qualified field path, e.g. "orders.user_id" or "users.email".
/// Value: chronologically ordered access log.
using SchemaAccessStats = std::map<std::string, std::vector<AccessEntry>>;

/// @brief Registry of GDPR-protected field paths.
///
/// Any field whose path appears in this set must never be flagged as
/// dead-weight regardless of its last-access timestamp.
struct GdprFieldRegistry {
    std::set<std::string> protected_paths;

    bool isProtected(const std::string& field_path) const {
        return protected_paths.count(field_path) > 0;
    }
};

// ---------------------------------------------------------------------------
// SchemaDeadWeightDetector
// ---------------------------------------------------------------------------

/**
 * @brief Layer-6 LLM Optimization: schema dead-weight detector.
 *
 * Identifies fields, collections, and indexes that have not been read over a
 * rolling 180-day window, taking into account:
 *
 *  - **GDPR protection** – protected fields are *never* candidates (0 false
 *    negatives invariant).
 *  - **Seasonality** – fields with periodic access patterns (detected via
 *    Fourier-coefficient approximation) are excluded even if the last access
 *    is older than the window.
 *
 * The detector produces a `DeadWeightReport` that serves as a DBA advisory;
 * it does **not** execute any DDL.
 *
 * ### Confidence formula
 * ```
 * confidence = (days_since_last_access / 180) * (1 - seasonality_score)
 *            * (1 - gdpr_weight)   // gdpr_weight = 1.0 iff GDPR-protected
 * ```
 *
 * ### Seasonality detection
 * `computeSeasonalityScore()` fits k=3 Fourier harmonics to the access
 * time-series and returns the fraction of total variance explained by those
 * harmonics.  A score > 0.5 indicates a significant periodic component.
 *
 * ### Decision records
 * When a `DecisionRecordYamlProcessor` is injected the detector emits one
 * `DecisionRecord{decision_type="SCHEMA_DEAD_WEIGHT"}` per `analyze()` call.
 */
class SchemaDeadWeightDetector {
public:
    // ─── DeadWeightCandidate ───────────────────────────────────────────────

    struct DeadWeightCandidate {
        /// Fully-qualified field path, e.g. "orders.archived_at".
        std::string field_path;

        /// Confidence that this field is dead-weight [0.0, 1.0].
        double confidence{0.0};

        /// Calendar days since the last recorded read access.
        uint32_t days_since_access{0};

        /// True when the field is GDPR-protected (always false in the report:
        /// GDPR fields are filtered before they reach the candidate list).
        bool gdpr_protected{false};

        /// Fraction of access variance explained by periodic harmonics [0.0, 1.0].
        double seasonality_score{0.0};

        /// Advisory recommendation: "archive" | "drop_index" | "deprecate".
        std::string recommendation;
    };

    // ─── DeadWeightReport ─────────────────────────────────────────────────

    struct DeadWeightReport {
        std::vector<DeadWeightCandidate> candidates;
        std::chrono::system_clock::time_point generated_at{
            std::chrono::system_clock::now()};

        /// Rolling window used for the analysis.
        uint32_t analysis_window_days{180};

        /// Total number of fields examined (including GDPR and seasonal).
        size_t total_fields_analyzed{0};

        /// How many fields were skipped because of GDPR protection.
        size_t gdpr_protected_skipped{0};
    };

    // ─── Config ────────────────────────────────────────────────────────────

    struct Config {
        /// Rolling window for dead-weight analysis (default: 180 days).
        uint32_t analysis_window_days{180};

        /// Seasonality score threshold above which a field is NOT a candidate.
        double seasonality_exclusion_threshold{0.7};

        /// Minimum confidence required to include a candidate in the report.
        double min_confidence{0.1};

        /// Number of Fourier harmonics used for seasonality detection.
        size_t fourier_harmonics{3};
    };

    // ─── Lifecycle ─────────────────────────────────────────────────────────

    /// Construct with default configuration.
    SchemaDeadWeightDetector();

    /**
     * @brief Construct with explicit configuration.
     *
     * @note  Two overloads instead of `= {}` default arg to work around
     *        GCC DR1607 (nested struct with non-trivially-constructible
     *        default member initialisers used in an enclosing-class declaration).
     */
    explicit SchemaDeadWeightDetector(Config config);
    ~SchemaDeadWeightDetector() = default;

    SchemaDeadWeightDetector(const SchemaDeadWeightDetector&) = delete;
    SchemaDeadWeightDetector& operator=(const SchemaDeadWeightDetector&) = delete;
    SchemaDeadWeightDetector(SchemaDeadWeightDetector&&) = default;
    SchemaDeadWeightDetector& operator=(SchemaDeadWeightDetector&&) = default;

    // ─── Dependency injection ──────────────────────────────────────────────

    void setDecisionRecordProcessor(
        std::shared_ptr<themis::llm::DecisionRecordYamlProcessor> processor);

    // ─── Primary interface ─────────────────────────────────────────────────

    /**
     * @brief Analyse schema access statistics and produce a dead-weight report.
     *
     * Algorithm:
     *  1. For each field in `stats`:
     *     a. Skip if GDPR-protected.
     *     b. Compute `days_since_access` from the most recent AccessEntry.
     *     c. Compute `seasonality_score` via `computeSeasonalityScore()`.
     *     d. Compute `confidence` using the formula above.
     *     e. Skip if `confidence < config_.min_confidence`.
     *     f. Skip if `seasonality_score >= config_.seasonality_exclusion_threshold`.
     *  2. For surviving candidates determine `recommendation`.
     *  3. Emit a DecisionRecord.
     *
     * @param stats       Per-field access time-series.
     * @param gdpr_fields Registry of GDPR-protected fields.
     * @return            Advisory report.
     */
    DeadWeightReport analyze(
        const SchemaAccessStats& stats,
        const GdprFieldRegistry& gdpr_fields) const;

    /**
     * @brief Compute a seasonality score for an access time-series.
     *
     * Uses a Fourier-coefficient approximation with `config_.fourier_harmonics`
     * harmonics.  Returns a score in [0.0, 1.0]; values above 0.5 indicate
     * a significant periodic component.
     *
     * @param access_series  Chronologically ordered access counts.
     * @return               Fraction of total variance explained by the harmonics.
     */
    double computeSeasonalityScore(
        const std::vector<AccessEntry>& access_series) const;

private:
    Config config_;
    std::shared_ptr<themis::llm::DecisionRecordYamlProcessor> dr_processor_;

    static std::string determineRecommendation(const std::string& field_path,
                                               uint32_t days_since_access);

    void emitDecisionRecord(const DeadWeightReport& report) const;
};

} // namespace storage
} // namespace themis
