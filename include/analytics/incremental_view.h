/**
 * @file incremental_view.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.32
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * ThemisDB Incremental Materialized Views
 *
 * Delta-based view maintenance engine: applies INSERT / UPDATE / DELETE
 * change records to pre-computed GROUP BY aggregations in O(1) per change
 * per affected group (no full re-scan required).
 *
 * Supported aggregations (all incremental):
 *   COUNT, SUM, AVG, MIN, MAX, STDDEV, VARIANCE, COUNT_DISTINCT,
 *   FIRST, LAST
 *
 * Supports:
 *   - Multiple concurrent views
 *   - CDC (Change Data Capture) integration via applyChanges()
 *   - Staleness tracking per view
 *   - Filter push-down (base filters evaluated per change)
 *   - Thread-safe reads and writes
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <algorithm>
#include <atomic>
#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace themisdb {
namespace analytics {

// ============================================================================
// Value type
// ============================================================================

/**
 * A field value in a change record.
 */
using FieldValue = std::variant<
    std::nullptr_t,  // null
    bool,
    int64_t,
    double,
    std::string
>;

// ============================================================================
// Aggregation function
// ============================================================================

enum class ViewAggFunc {
    COUNT,
    SUM,
    AVG,
    MIN,
    MAX,
    STDDEV,
    VARIANCE,
    COUNT_DISTINCT,
    FIRST,
    LAST
};

// ============================================================================
// Change record
// ============================================================================

/**
 * Type of change in a CDC event.
 */
enum class ChangeType {
    INSERT,
    UPDATE,   ///< requires both before_row and after_row
    DELETE
};

/**
 * A single data-change record for CDC-based view maintenance.
 *
 * For INSERT:  after_row contains the new row, before_row is empty.
 * For DELETE:  before_row contains the deleted row, after_row is empty.
 * For UPDATE:  before_row is the old state, after_row the new state.
 */
struct ChangeRecord {
    using Row = std::unordered_map<std::string, FieldValue>;

    ChangeType type = ChangeType::INSERT;
    std::string collection;
    Row before_row;
    Row after_row;
    std::chrono::system_clock::time_point change_time;
};

// ============================================================================
// View definition
// ============================================================================

/**
 * Aggregate specification within a view.
 */
struct ViewAggSpec {
    std::string output_name;   ///< Name in query result
    ViewAggFunc func;
    std::string source_field;  ///< Source field in base rows (empty → COUNT(*))
};

/**
 * Base filter condition for the view.
 */
struct ViewFilter {
    enum class Op { EQ, NE, LT, LE, GT, GE, IS_NULL, IS_NOT_NULL };
    std::string field;
    Op op = Op::EQ;
    FieldValue value;
};

/**
 * Defines what an IncrementalView computes.
 */
struct ViewDefinition {
    std::string name;
    std::string source_collection;

    /// Dimensions for GROUP BY
    std::vector<std::string> dimensions;

    /// Aggregations
    std::vector<ViewAggSpec> aggregations;

    /// Optional base filter (applied to each change record before maintenance)
    std::vector<ViewFilter> base_filters;

    /// Staleness threshold in seconds (0 = never stale based on time)
    int64_t staleness_seconds = 0;
};

// ============================================================================
// View result row
// ============================================================================

/**
 * One row in a view query result.
 * `group_key` maps dimension names → their string-serialized values.
 * `values` maps aggregation output names → results.
 */
struct ViewRow {
    std::unordered_map<std::string, std::string> group_key;
    std::unordered_map<std::string, FieldValue>  values;
};

/**
 * Query result from an IncrementalView.
 */
struct ViewQueryResult {
    std::vector<ViewRow>   rows;
    int64_t                total_rows = 0;
    bool                   is_stale   = false;
    std::chrono::system_clock::time_point last_update;
};

// ============================================================================
// IncrementalView
// ============================================================================

/**
 * A single incrementally maintained materialized view.
 *
 * The view maintains per-group aggregate states that are updated in O(1) per
 * change, avoiding full re-computation.
 *
 * MIN/MAX use a sorted multiset to correctly handle removals.
 * STDDEV/VARIANCE use Welford's online algorithm.
 * COUNT_DISTINCT tracks unique string representations.
 * FIRST/LAST are maintained per insertion order.
 *
 * Thread-safety: ingest/applyChange is serialized via a writer mutex;
 * query() uses a shared (reader) lock.
 */
class IncrementalView {
public:
    explicit IncrementalView(const ViewDefinition& def);
    ~IncrementalView();

    // Non-copyable
    IncrementalView(const IncrementalView&) = delete;
    IncrementalView& operator=(const IncrementalView&) = delete;

    /**
     * Apply a single change record to this view.
     * Thread-safe.
     *
     * @return true  if the record was applied (passed base filters).
     * @return false if the record was filtered out or belongs to a different
     *               collection.
     */
    bool applyChange(const ChangeRecord& change);

    /**
     * Apply a batch of change records.
     * Thread-safe. Processes changes in micro-batches (≤ 256 rows) to allow
     * concurrent readers to acquire the shared lock between batches.
     * Base filters are evaluated outside the write lock.
     */
    int applyChanges(const std::vector<ChangeRecord>& changes);

    /**
     * Query the current view state.
     *
     * @param filters  Additional runtime filters on dimension values.
     * @param limit    Maximum rows to return (0 = all).
     * @param offset   Row offset for pagination.
     */
    ViewQueryResult query(
        const std::vector<ViewFilter>& filters = {},
        int64_t limit  = 0,
        int64_t offset = 0
    ) const;

    /**
     * Get the view definition.
     */
    const ViewDefinition& definition() const { return def_; }

    /**
     * Discard all aggregated state. After this, the view is empty.
     */
    void clear();

    /**
     * Number of groups currently tracked.
     */
    int64_t groupCount() const;

    /**
     * True if the view has been modified since it was last marked fresh.
     */
    bool isDirty() const { return dirty_.load(); }

    /**
     * True if staleness_seconds > 0 and the last update was more than
     * staleness_seconds ago.
     */
    bool isStale() const;

    /**
     * Total number of changes applied (monotonically increasing).
     */
    uint64_t changeCount() const { return change_count_.load(); }

    std::chrono::system_clock::time_point lastUpdateTime() const;

private:
    ViewDefinition def_;

    // Per-group aggregate state
    struct AggState {
        // COUNT / SUM / AVG
        int64_t count = 0;
        double  sum   = 0.0;

        // MIN / MAX (sorted multiset for correct removal semantics)
        std::multiset<double> min_max_values;

        // STDDEV / VARIANCE via Welford's online algorithm
        double welford_mean = 0.0;
        double welford_m2   = 0.0;

        // COUNT_DISTINCT — reference-counted to handle duplicate field values correctly
        std::map<std::string, int> distinct_ref_counts;

        // FIRST / LAST (stored as string representation)
        FieldValue first_val{nullptr};
        FieldValue last_val{nullptr};
        bool       has_first = false;

        /// Add a field value contribution (+1 add, -1 remove).
        void add(const FieldValue& v, int sign);

        /// Compute result for a given function.
        FieldValue result(ViewAggFunc func) const;
    };

    // group_key_string → {agg_output_name → AggState}
    using GroupKey = std::string;
    std::map<GroupKey, std::unordered_map<std::string, AggState>> groups_;

    mutable std::shared_mutex rw_mutex_;
    std::atomic<bool>     dirty_{false};
    std::atomic<uint64_t> change_count_{0};
    std::atomic<int64_t>  last_update_us_{0};

    // Internal helpers
    GroupKey makeGroupKey(const ChangeRecord::Row& row) const;
    std::unordered_map<std::string, std::string> parseGroupKey(const GroupKey& gk) const;
    bool passesBaseFilters(const ChangeRecord::Row& row) const;
    bool passesRuntimeFilters(const std::unordered_map<std::string, std::string>& gk,
                               const std::vector<ViewFilter>& filters) const;
    void applyRow(const ChangeRecord::Row& row, int sign);
    void pruneEmptyGroup(const GroupKey& gk);
};

// ============================================================================
// IncrementalViewManager
// ============================================================================

/**
 * Registry and dispatcher for multiple IncrementalViews.
 *
 * Usage:
 * @code
 *   IncrementalViewManager mgr;
 *
 *   ViewDefinition def;
 *   def.name = "sales_by_region";
 *   def.source_collection = "sales";
 *   def.dimensions = {"region", "product"};
 *   def.aggregations = {
 *       {"total",  ViewAggFunc::SUM,   "amount"},
 *       {"orders", ViewAggFunc::COUNT, ""}
 *   };
 *   mgr.createView(def);
 *
 *   // On INSERT from CDC:
 *   ChangeRecord rec;
 *   rec.type = ChangeType::INSERT;
 *   rec.collection = "sales";
 *   rec.after_row = {{"region","EU"}, {"product","X"}, {"amount",99.9}};
 *   mgr.applyChange(rec);
 *
 *   // Query:
 *   auto result = mgr.query("sales_by_region");
 * @endcode
 */
class IncrementalViewManager {
public:
    IncrementalViewManager();
    ~IncrementalViewManager();

    /**
     * Register a new incremental view.
     * @return false if a view with the same name already exists.
     */
    bool createView(const ViewDefinition& def);

    /**
     * Remove a view by name.
     */
    bool dropView(const std::string& name);

    /**
     * Check if a view exists.
     */
    bool hasView(const std::string& name) const;

    /**
     * Get a view by name.
     */
    std::shared_ptr<IncrementalView> getView(const std::string& name) const;

    /**
     * List all registered view names.
     */
    std::vector<std::string> listViews() const;

    /**
     * Apply a single change to all views that observe the change's collection.
     */
    void applyChange(const ChangeRecord& change);

    /**
     * Apply a batch of changes to all relevant views.
     */
    void applyChanges(const std::vector<ChangeRecord>& changes);

    /**
     * Query a view by name. Returns empty result if not found.
     */
    ViewQueryResult query(
        const std::string& view_name,
        const std::vector<ViewFilter>& filters = {},
        int64_t limit  = 0,
        int64_t offset = 0
    ) const;

    /**
     * Total changes applied across all views (since creation).
     */
    uint64_t totalChanges() const { return total_changes_.load(); }

private:
    mutable std::shared_mutex views_mutex_;
    std::unordered_map<std::string, std::shared_ptr<IncrementalView>> views_;
    std::atomic<uint64_t> total_changes_{0};
};

// ============================================================================
// Utility
// ============================================================================

/**
 * Convert a FieldValue to a human-readable string.
 */
std::string fieldValueToStr(const FieldValue& v);

/**
 * Convert ViewAggFunc to string.
 */
inline const char* viewAggFuncToString(ViewAggFunc f) {
    switch (f) {
        case ViewAggFunc::COUNT:          return "COUNT";
        case ViewAggFunc::SUM:            return "SUM";
        case ViewAggFunc::AVG:            return "AVG";
        case ViewAggFunc::MIN:            return "MIN";
        case ViewAggFunc::MAX:            return "MAX";
        case ViewAggFunc::STDDEV:         return "STDDEV";
        case ViewAggFunc::VARIANCE:       return "VARIANCE";
        case ViewAggFunc::COUNT_DISTINCT: return "COUNT_DISTINCT";
        case ViewAggFunc::FIRST:          return "FIRST";
        case ViewAggFunc::LAST:           return "LAST";
        default:                          return "UNKNOWN";
    }
}

} // namespace analytics
} // namespace themisdb
