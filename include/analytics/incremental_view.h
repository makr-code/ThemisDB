/**
 * @file incremental_view.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.32
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

enum class ChangeType {
    INSERT,
    UPDATE,   ///< requires both before_row and after_row
    DELETE
};

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

struct ViewAggSpec {
    std::string output_name;   ///< Name in query result
    ViewAggFunc func;
    std::string source_field;  ///< Source field in base rows (empty → COUNT(*))
};

struct ViewFilter {
    enum class Op { EQ, NE, LT, LE, GT, GE, IS_NULL, IS_NOT_NULL };
    std::string field;
    Op op = Op::EQ;
    FieldValue value;
};

struct ViewDefinition {
    std::string name;
    std::string source_collection;

    std::vector<std::string> dimensions;

    std::vector<ViewAggSpec> aggregations;

    std::vector<ViewFilter> base_filters;

    int64_t staleness_seconds = 0;
};

// ============================================================================
// View result row
// ============================================================================

struct ViewRow {
    std::unordered_map<std::string, std::string> group_key;
    std::unordered_map<std::string, FieldValue>  values;
};

struct ViewQueryResult {
    std::vector<ViewRow>   rows;
    int64_t                total_rows = 0;
    bool                   is_stale   = false;
    std::chrono::system_clock::time_point last_update;
};

// ============================================================================
// IncrementalView
// ============================================================================

class IncrementalView {
public:
    /**
     * @brief Incremental View.
     * @param[in] def Input parameter.
     * @return Return value.
     */
    explicit IncrementalView(const ViewDefinition& def);
    ~IncrementalView();

    // Non-copyable
    IncrementalView(const IncrementalView&) = delete;
    IncrementalView& operator=(const IncrementalView&) = delete;

    /**
     * @brief Apply Change.
     * @param[in] change Input parameter.
     * @return True when the operation succeeds.
     */
    bool applyChange(const ChangeRecord& change);

    /**
     * @brief Apply Changes.
     * @param[in] changes Input parameter.
     * @return Return value.
     */
    int applyChanges(const std::vector<ChangeRecord>& changes);

    ViewQueryResult query(
        const std::vector<ViewFilter>& filters = {},
        int64_t limit  = 0,
        int64_t offset = 0
    ) const;

    const ViewDefinition& definition() const { return def_; }

    /**
     * @brief Clear.
     */
    void clear();

    /**
     * @brief Group Count.
     * @return Return value.
     */
    int64_t groupCount() const;

    bool isDirty() const { return dirty_.load(); }

    /**
     * @brief Is Stale.
     * @return True when the operation succeeds.
     */
    bool isStale() const;

    uint64_t changeCount() const { return change_count_.load(); }

    /**
     * @brief Last Update Time.
     * @return Return value.
     */
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

        /**
         * @brief Add.
         * @param[in] v Input parameter.
         * @param[in] sign Input parameter.
         */
        void add(const FieldValue& v, int sign);

        /**
         * @brief Result.
         * @param[in] func Input parameter.
         * @return Return value.
         */
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
    /**
     * @brief Make Group Key.
     * @param[in] row Input parameter.
     * @return Return value.
     */
    GroupKey makeGroupKey(const ChangeRecord::Row& row) const;
    std::unordered_map<std::string, std::string> parseGroupKey(const GroupKey& gk) const;
    /**
     * @brief Passes Base Filters.
     * @param[in] row Input parameter.
     * @return True when the operation succeeds.
     */
    bool passesBaseFilters(const ChangeRecord::Row& row) const;
    bool passesRuntimeFilters(const std::unordered_map<std::string, std::string>& gk,
                               const std::vector<ViewFilter>& filters) const;
    /**
     * @brief Apply Row.
     * @param[in] row Input parameter.
     * @param[in] sign Input parameter.
     */
    void applyRow(const ChangeRecord::Row& row, int sign);
    /**
     * @brief Prune Empty Group.
     * @param[in] gk Input parameter.
     */
    void pruneEmptyGroup(const GroupKey& gk);
};

// ============================================================================
// IncrementalViewManager
// ============================================================================

class IncrementalViewManager {
public:
    IncrementalViewManager();
    ~IncrementalViewManager();

    /**
     * @brief Create View.
     * @param[in] def Input parameter.
     * @return True when the operation succeeds.
     */
    bool createView(const ViewDefinition& def);

    /**
     * @brief Drop View.
     * @param[in] name Input parameter.
     * @return True when the operation succeeds.
     */
    bool dropView(const std::string& name);

    /**
     * @brief Has View.
     * @param[in] name Input parameter.
     * @return True when the operation succeeds.
     */
    bool hasView(const std::string& name) const;

    /**
     * @brief Get View.
     * @param[in] name Input parameter.
     * @return Return value.
     */
    std::shared_ptr<IncrementalView> getView(const std::string& name) const;

    /**
     * @brief List Views.
     * @return Return value.
     */
    std::vector<std::string> listViews() const;

    /**
     * @brief Apply Change.
     * @param[in] change Input parameter.
     */
    void applyChange(const ChangeRecord& change);

    /**
     * @brief Apply Changes.
     * @param[in] changes Input parameter.
     */
    void applyChanges(const std::vector<ChangeRecord>& changes);

    ViewQueryResult query(
        const std::string& view_name,
        const std::vector<ViewFilter>& filters = {},
        int64_t limit  = 0,
        int64_t offset = 0
    ) const;

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
 * @brief Field Value To Str.
 * @param[in] v Input parameter.
 * @return Return value.
 */
std::string fieldValueToStr(const FieldValue& v);

/**
 * @brief View Agg Func To String.
 * @param[in] f Input parameter.
 * @return Pointer to the result.
 * @details Implements viewAggFuncToString without additional internal calls.
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
