/**
 * @file materialized_cte.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * Incremental View Maintenance for Materialized CTEs
 *
 * Provides delta-based maintenance for materialized CTE results:
 * when base collection data changes (INSERT / UPDATE / DELETE),
 * the materialized CTE is updated in O(1) per affected aggregation
 * group — no full re-scan required.
 *
 * Builds on analytics::IncrementalView for the core delta algorithm
 * and exposes a JSON-friendly interface aligned with the query module.
 *
 * Supported aggregations: COUNT, SUM, AVG, MIN, MAX, COUNT_DISTINCT
 *
 * Thread-safety: applyChange/applyChanges serialised by writer lock;
 * query() uses a shared reader lock.
 */

#pragma once

#include <atomic>
#include <chrono>
#include <memory>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include <nlohmann/json.hpp>

#include "analytics/incremental_view.h"

namespace themis {
namespace query {

// ============================================================================
// Change record
// ============================================================================

/**
 * Type of data change in the base collection.
 */
enum class CTEChangeType {
    INSERT,
    UPDATE,  ///< requires both before_row and after_row
    DELETE
};

/**
 * A single data-change record for incremental CTE maintenance.
 *
 * INSERT: after_row holds the new row; before_row is ignored.
 * DELETE: before_row holds the deleted row; after_row is ignored.
 * UPDATE: before_row is the old state; after_row is the new state.
 */
struct CTEDataChange {
    CTEChangeType  type       = CTEChangeType::INSERT;
    std::string    collection;
    nlohmann::json before_row;  ///< Pre-change row (UPDATE / DELETE)
    nlohmann::json after_row;   ///< Post-change row (INSERT / UPDATE)
};

// ============================================================================
// View definition
// ============================================================================

/**
 * Aggregation functions supported for incrementally maintained CTEs.
 */
enum class CTEAggFunc {
    COUNT,          ///< COUNT(*) or COUNT(field)
    SUM,            ///< SUM(field)
    AVG,            ///< AVG(field) — maintained via SUM / COUNT
    MIN,            ///< MIN(field) — correct removal via sorted multiset
    MAX,            ///< MAX(field) — correct removal via sorted multiset
    COUNT_DISTINCT  ///< COUNT(DISTINCT field) — reference-counted
};

/**
 * Aggregation specification within a materialized CTE view.
 */
struct CTEAggSpec {
    std::string output_name;   ///< Field name in the result row
    CTEAggFunc  func;
    std::string source_field;  ///< Source field (empty = COUNT(*))
};

/**
 * Optional base filter applied to each change record before maintenance.
 * Only rows that pass all base filters are incorporated into the view.
 */
struct CTEBaseFilter {
    enum class Op { EQ, NE, LT, LE, GT, GE, IS_NULL, IS_NOT_NULL };
    std::string    field;
    Op             op    = Op::EQ;
    nlohmann::json value;  ///< Reference value for comparison (unused for IS_NULL / IS_NOT_NULL)
};

/**
 * Defines what a MaterializedCTEView computes.
 */
struct MaterializedCTEDef {
    std::string                  name;
    std::string                  source_collection;
    std::vector<std::string>     dimensions;    ///< GROUP BY key fields
    std::vector<CTEAggSpec>      aggregations;
    std::vector<CTEBaseFilter>   base_filters;
    int64_t staleness_seconds = 0;  ///< 0 = no time-based staleness
};

// ============================================================================
// Query result
// ============================================================================

/**
 * One row returned by querying a materialized CTE.
 * `data` is a JSON object combining dimension key fields and aggregate values.
 */
struct MaterializedCTERow {
    nlohmann::json data;
};

/**
 * Result of querying a materialized CTE view.
 */
struct MaterializedCTEResult {
    std::vector<MaterializedCTERow>       rows;
    int64_t                               total_rows  = 0;
    bool                                  is_stale    = false;
    std::chrono::system_clock::time_point last_update;
};

// ============================================================================
// MaterializedCTEView
// ============================================================================

/**
 * @brief A single incrementally maintained materialized CTE view.
 *
 * Wraps analytics::IncrementalView with a JSON-friendly interface for
 * integration into the query module's CTE evaluation pipeline.
 *
 * The view maintains per-group aggregate states updated in O(1) per
 * change record, avoiding full re-evaluation of the CTE query.
 *
 * Thread-safety: applyChange/applyChanges use an exclusive writer lock;
 * query() uses a shared reader lock (delegated to IncrementalView).
 */
class MaterializedCTEView {
public:
    explicit MaterializedCTEView(const MaterializedCTEDef& def);
    ~MaterializedCTEView();

    MaterializedCTEView(const MaterializedCTEView&)            = delete;
    MaterializedCTEView& operator=(const MaterializedCTEView&) = delete;

    /**
     * Apply a single change record to this view.
     * @return true  if the change was applied (passed base filters and
     *               belongs to this view's source collection).
     * @return false otherwise.
     */
    bool applyChange(const CTEDataChange& change);

    /**
     * Apply a batch of change records.
     * Acquires the writer lock once for the entire batch.
     * @return number of records actually applied.
     */
    int applyChanges(const std::vector<CTEDataChange>& changes);

    /**
     * Query the current view state.
     * @param limit  Maximum rows to return (0 = all).
     * @param offset Row offset for pagination.
     */
    MaterializedCTEResult query(int64_t limit = 0, int64_t offset = 0) const;

    /** View definition. */
    const MaterializedCTEDef& definition() const { return def_; }

    /** True if any change has been applied since the last clear(). */
    bool isDirty() const;

    /** True if staleness_seconds > 0 and last update was longer ago. */
    bool isStale() const;

    /** Number of distinct groups currently tracked. */
    int64_t groupCount() const;

    /** Total number of change records applied (monotonically increasing). */
    uint64_t changeCount() const;

    /** Discard all aggregated state; after this, the view is empty. */
    void clear();

private:
    MaterializedCTEDef                                        def_;
    std::unique_ptr<themisdb::analytics::IncrementalView>    view_;

    // ---- type-conversion helpers ----

    static themisdb::analytics::ViewDefinition buildViewDef(
        const MaterializedCTEDef& def);

    static themisdb::analytics::ViewAggFunc toViewAggFunc(CTEAggFunc f);

    static themisdb::analytics::ViewFilter::Op toViewFilterOp(
        CTEBaseFilter::Op op);

    static themisdb::analytics::ChangeRecord::Row jsonToRow(
        const nlohmann::json& json_row);

    static themisdb::analytics::ChangeRecord toChangeRecord(
        const CTEDataChange& change);

    static MaterializedCTEResult fromViewQueryResult(
        const themisdb::analytics::ViewQueryResult& vqr,
        const MaterializedCTEDef& def);
};

// ============================================================================
// MaterializedCTERegistry
// ============================================================================

/**
 * @brief Registry and dispatcher for multiple MaterializedCTEViews.
 *
 * Manages a set of incrementally maintained materialized CTE views and
 * routes incoming data changes to all views that observe the affected
 * collection.
 *
 * @code
 *   MaterializedCTERegistry registry;
 *
 *   MaterializedCTEDef def;
 *   def.name = "sales_by_region";
 *   def.source_collection = "sales";
 *   def.dimensions = {"region"};
 *   def.aggregations = {
 *       {"total",  CTEAggFunc::SUM,   "amount"},
 *       {"orders", CTEAggFunc::COUNT, ""}
 *   };
 *   registry.registerCTE(def);
 *
 *   CTEDataChange change;
 *   change.type = CTEChangeType::INSERT;
 *   change.collection = "sales";
 *   change.after_row = {{"region", "EU"}, {"amount", 99.9}};
 *   registry.applyChange(change);
 *
 *   auto result = registry.query("sales_by_region");
 * @endcode
 */
class MaterializedCTERegistry {
public:
    MaterializedCTERegistry();
    ~MaterializedCTERegistry();

    /**
     * Register a new CTE for incremental maintenance.
     * @return false if a CTE with the same name already exists.
     */
    bool registerCTE(const MaterializedCTEDef& def);

    /**
     * Remove a CTE by name.
     * @return false if the name is not found.
     */
    bool unregisterCTE(const std::string& name);

    /** Check if a CTE is registered. */
    bool hasCTE(const std::string& name) const;

    /** List all registered CTE names. */
    std::vector<std::string> listCTEs() const;

    /** Get a specific CTE view (nullptr if not found). */
    std::shared_ptr<MaterializedCTEView> getView(const std::string& name) const;

    /**
     * Apply a single change to all views that observe the change's collection.
     */
    void applyChange(const CTEDataChange& change);

    /**
     * Apply a batch of changes to all relevant views.
     */
    void applyChanges(const std::vector<CTEDataChange>& changes);

    /**
     * Query a materialized CTE by name.
     * Returns an empty result if the name is not found.
     */
    MaterializedCTEResult query(
        const std::string& cte_name,
        int64_t limit  = 0,
        int64_t offset = 0
    ) const;

    /** Total changes applied across all views since registry creation. */
    uint64_t totalChanges() const { return total_changes_.load(); }

private:
    mutable std::shared_mutex registry_mutex_;
    std::unordered_map<std::string, std::shared_ptr<MaterializedCTEView>> views_;
    std::atomic<uint64_t> total_changes_{0};
};

} // namespace query
} // namespace themis
