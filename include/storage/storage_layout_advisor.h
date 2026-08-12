/**
 * @file storage_layout_advisor.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "storage/schema_dead_weight_detector.h"   // reuse GdprFieldRegistry
#include "llm/decision_record_yaml_processor.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace themis {
namespace storage {

// ---------------------------------------------------------------------------
// CollectionAccessStats
// ---------------------------------------------------------------------------

/**
 * @brief Access pattern summary for a single collection.
 *
 * All ratio fields are in [0.0, 1.0] and should sum to ≤ 1.0; the remainder
 * represents write-only or uncategorised accesses.
 */
struct CollectionAccessStats {
    /// Fraction of accesses that are single-key point lookups.
    double point_lookup_ratio{0.0};

    /// Fraction of accesses that are multi-key range scans.
    double range_scan_ratio{0.0};

    /// Fraction of accesses that include GROUP-BY / aggregate functions.
    double aggregation_ratio{0.0};

    /// Fraction of accesses that touch metadata fields only (exclude BLOB).
    double metadata_only_access_ratio{0.0};

    /// True when the collection contains at least one BLOB / large-payload field.
    bool has_blob_field{false};

    /// True when the primary timestamp column is monotonically increasing.
    bool has_monotonic_timestamp{false};

    /**
     * @brief Sample of normalised access timestamps (epoch seconds) used for
     *        Fourier-based time-series detection.
     *
     * Elements should be in chronological order.  Providing at least 16
     * samples enables the seasonality estimator.
     */
    std::vector<double> timestamp_series;
};

// ---------------------------------------------------------------------------
// SchemaInfo
// ---------------------------------------------------------------------------

/**
 * @brief Structural metadata for a collection schema.
 *
 * Used by the layout advisor to identify float-heavy, UUID-keyed, or
 * BLOB-containing collections.
 */
struct SchemaInfo {
    /// Collection name (informational).
    std::string collection_name;

    /// Ordered list of field names.
    std::vector<std::string> field_names;

    /**
     * @brief Maps each field name to its canonical type token.
     *
     * Recognised type tokens: "Float", "Int", "DateTime", "UUID",
     * "String", "BLOB", "Boolean".  Unknown types are treated as "String".
     */
    std::map<std::string, std::string> field_types;

    /// True when at least one field has type "BLOB".
    bool has_blob{false};
};

// ---------------------------------------------------------------------------
// StorageLayoutAdvisor
// ---------------------------------------------------------------------------

/**
 * @brief Layer-10 LLM Optimization: storage layout recommendation engine.
 *
 * Analyses a collection's schema and access pattern to recommend the optimal
 * physical storage layout (Row vs. Columnar vs. Hybrid vs. Tiered).  The
 * advisor is **advisory-only** — it never executes DDL.
 *
 * ### Decision logic
 * ```
 * if isTimeSeries(stats) AND aggregation_ratio > 0.7:
 *     → COLUMNAR_COMPRESSED  (≥ 5× compression for float columns)
 * elif point_lookup_ratio > 0.8:
 *     → ROW_ORIENTED          (UUID/PK lookups need full row)
 * elif has_blob_field AND metadata_only_access_ratio > 0.5:
 *     → HYBRID                (metadata columnar, BLOB row)
 * else:
 *     → ROW_ORIENTED          (safe default)
 * ```
 *
 * ### Performance target (Paper §Layer-10)
 * `estimated_compression_ratio >= 5.0` for float-heavy time-series collections.
 *
 * ### GDPR protection
 * When a layout change would affect a GDPR-protected field the recommendation
 * sets `gdpr_approval_required = true`.  ThemisDB never applies such a
 * recommendation automatically.
 *
 * ### Decision records
 * When a `DecisionRecordYamlProcessor` is injected the advisor emits one
 * `DecisionRecord{decision_type="LAYOUT_RECOMMENDATION"}` per `analyze()` call.
 *
 * ### Thread safety
 * `StorageLayoutAdvisor` is stateless after construction and safe for
 * concurrent use.
 */
class StorageLayoutAdvisor {
public:
    // ─── LayoutType ───────────────────────────────────────────────────────

    /// Recommended physical storage layout.
    enum class LayoutType {
        ROW_ORIENTED,         ///< Standard row store — good for point lookups
        COLUMNAR_COMPRESSED,  ///< Columnar store — good for aggregations / time-series
        HYBRID,               ///< Metadata columnar, large payload row
        TIERED                ///< Hot / warm / cold tiering recommendation
    };

    // ─── LayoutRecommendation ─────────────────────────────────────────────

    struct LayoutRecommendation {
        /// Name of the analysed collection.
        std::string collection_name;

        /// Recommended layout after analysis.
        LayoutType recommended_layout{LayoutType::ROW_ORIENTED};

        /// Current (assumed) layout before migration.
        LayoutType current_layout{LayoutType::ROW_ORIENTED};

        /// Estimated compression improvement factor (> 1.0 = better compression).
        double estimated_compression_ratio{1.0};

        /// Estimated query throughput improvement factor (1.0 = no change).
        double estimated_query_speedup{1.0};

        /// Confidence in the recommendation [0.0, 1.0].
        double confidence{0.0};

        /// True when affected fields include GDPR-protected fields.
        bool gdpr_approval_required{false};

        /// One-sentence DBA-readable rationale.
        std::string rationale;
    };

    // ─── Lifecycle ────────────────────────────────────────────────────────

    StorageLayoutAdvisor() = default;
    ~StorageLayoutAdvisor() = default;

    StorageLayoutAdvisor(const StorageLayoutAdvisor&) = default;
    StorageLayoutAdvisor& operator=(const StorageLayoutAdvisor&) = default;

    // ─── Dependency injection ─────────────────────────────────────────────

    void setDecisionRecordProcessor(
        std::shared_ptr<themis::llm::DecisionRecordYamlProcessor> processor);

    // ─── Core API ─────────────────────────────────────────────────────────

    /**
     * @brief Analyse a collection and produce a layout recommendation.
     *
     * Algorithm:
     *  1. Determine whether the collection is time-series via `isTimeSeries()`.
     *  2. Apply the decision logic described in the class docstring.
     *  3. Compute compression and speedup estimates.
     *  4. Check whether any schema field is GDPR-protected.
     *  5. Emit a LAYOUT_RECOMMENDATION decision record.
     *
     * @param collection_name  Human-readable name of the collection.
     * @param stats            Access pattern statistics.
     * @param schema           Field metadata.
     * @param gdpr_fields      Registry of GDPR-protected field paths.
     * @return                 Layout recommendation with rationale.
     */
    LayoutRecommendation analyze(
        const std::string&            collection_name,
        const CollectionAccessStats&  stats,
        const SchemaInfo&             schema,
        const GdprFieldRegistry&      gdpr_fields) const;

    /**
     * @brief Detect whether a collection exhibits time-series access patterns.
     *
     * Returns true when the stats indicate a monotonic timestamp AND the
     * Fourier-coefficient analysis of `stats.timestamp_series` shows
     * periodic / sequential structure (variance explained > 0.3).
     *
     * @param stats  Collection access statistics.
     * @return       True when time-series pattern is detected.
     */
    bool isTimeSeries(const CollectionAccessStats& stats) const;

    /// Human-readable name for a LayoutType value.
    static std::string layoutName(LayoutType t);

private:
    std::shared_ptr<themis::llm::DecisionRecordYamlProcessor> dr_processor_;

    /// Estimate compression ratio for the given layout and schema.
    static double estimateCompressionRatio(LayoutType layout,
                                           const SchemaInfo& schema);

    /// Estimate query speedup for the given layout and access pattern.
    static double estimateQuerySpeedup(LayoutType layout,
                                       const CollectionAccessStats& stats);

    /// Build a one-sentence rationale for the recommendation.
    static std::string buildRationale(LayoutType layout,
                                      const CollectionAccessStats& stats,
                                      bool gdpr_affected);

    void emitDecisionRecord(const LayoutRecommendation& rec) const;
};

} // namespace storage
} // namespace themis
