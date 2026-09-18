/**
 * @file materialized_cte.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

enum class CTEChangeType {
    INSERT,
    UPDATE,  ///< requires both before_row and after_row
    DELETE
};

struct CTEDataChange {
    CTEChangeType  type       = CTEChangeType::INSERT;
    std::string    collection;
    nlohmann::json before_row;  ///< Pre-change row (UPDATE / DELETE)
    nlohmann::json after_row;   ///< Post-change row (INSERT / UPDATE)
};

// ============================================================================
// View definition
// ============================================================================

enum class CTEAggFunc {
    COUNT,          ///< COUNT(*) or COUNT(field)
    SUM,            ///< SUM(field)
    AVG,            ///< AVG(field) — maintained via SUM / COUNT
    MIN,            ///< MIN(field) — correct removal via sorted multiset
    MAX,            ///< MAX(field) — correct removal via sorted multiset
    COUNT_DISTINCT  ///< COUNT(DISTINCT field) — reference-counted
};

struct CTEAggSpec {
    std::string output_name;   ///< Field name in the result row
    CTEAggFunc  func;
    std::string source_field;  ///< Source field (empty = COUNT(*))
};

struct CTEBaseFilter {
    enum class Op { EQ, NE, LT, LE, GT, GE, IS_NULL, IS_NOT_NULL };
    std::string    field;
    Op             op    = Op::EQ;
    nlohmann::json value;  ///< Reference value for comparison (unused for IS_NULL / IS_NOT_NULL)
};

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

struct MaterializedCTERow {
    nlohmann::json data;
};

struct MaterializedCTEResult {
    std::vector<MaterializedCTERow>       rows;
    int64_t                               total_rows  = 0;
    bool                                  is_stale    = false;
    std::chrono::system_clock::time_point last_update;
};

// ============================================================================
// MaterializedCTEView
// ============================================================================

class MaterializedCTEView {
public:
    /**
     * @brief Materialized CTEView.
     * @param[in] def Input parameter.
     * @return Return value.
     */
    explicit MaterializedCTEView(const MaterializedCTEDef& def);
    ~MaterializedCTEView();

    MaterializedCTEView(const MaterializedCTEView&)            = delete;
    MaterializedCTEView& operator=(const MaterializedCTEView&) = delete;

    /**
     * @brief Apply Change.
     * @param[in] change Input parameter.
     * @return True when the operation succeeds.
     */
    bool applyChange(const CTEDataChange& change);

    /**
     * @brief Apply Changes.
     * @param[in] changes Input parameter.
     * @return Return value.
     */
    int applyChanges(const std::vector<CTEDataChange>& changes);

    MaterializedCTEResult query(int64_t limit = 0, int64_t offset = 0) const;

    const MaterializedCTEDef& definition() const { return def_; }

    /**
     * @brief Is Dirty.
     * @return True when the operation succeeds.
     */
    bool isDirty() const;

    /**
     * @brief Is Stale.
     * @return True when the operation succeeds.
     */
    bool isStale() const;

    /**
     * @brief Group Count.
     * @return Return value.
     */
    int64_t groupCount() const;

    /**
     * @brief Change Count.
     * @return Return value.
     */
    uint64_t changeCount() const;

    /**
     * @brief Clear.
     */
    void clear();

private:
    MaterializedCTEDef                                        def_;
    std::unique_ptr<themisdb::analytics::IncrementalView>    view_;

    /**
     * @brief ---- type-conversion helpers ----
     * @param[in] def Input parameter.
     * @return Return value.
     */

    static themisdb::analytics::ViewDefinition buildViewDef(
        const MaterializedCTEDef& def);

    /**
     * @brief To View Agg Func.
     * @param[in] f Input parameter.
     * @return Return value.
     */
    static themisdb::analytics::ViewAggFunc toViewAggFunc(CTEAggFunc f);

    /**
     * @brief To View Filter Op.
     * @param[in] op Input parameter.
     * @return Return value.
     */
    static themisdb::analytics::ViewFilter::Op toViewFilterOp(
        CTEBaseFilter::Op op);

    /**
     * @brief Json To Row.
     * @param[in] json_row Input parameter.
     * @return Return value.
     */
    static themisdb::analytics::ChangeRecord::Row jsonToRow(
        const nlohmann::json& json_row);

    /**
     * @brief To Change Record.
     * @param[in] change Input parameter.
     * @return Return value.
     */
    static themisdb::analytics::ChangeRecord toChangeRecord(
        const CTEDataChange& change);

    /**
     * @brief From View Query Result.
     * @param[in] vqr Input parameter.
     * @param[in] def Input parameter.
     * @return Return value.
     */
    static MaterializedCTEResult fromViewQueryResult(
        const themisdb::analytics::ViewQueryResult& vqr,
        const MaterializedCTEDef& def);
};

// ============================================================================
// MaterializedCTERegistry
// ============================================================================

class MaterializedCTERegistry {
public:
    MaterializedCTERegistry();
    ~MaterializedCTERegistry();

    /**
     * @brief Register CTE.
     * @param[in] def Input parameter.
     * @return True when the operation succeeds.
     */
    bool registerCTE(const MaterializedCTEDef& def);

    /**
     * @brief Unregister CTE.
     * @param[in] name Input parameter.
     * @return True when the operation succeeds.
     */
    bool unregisterCTE(const std::string& name);

    /**
     * @brief Has CTE.
     * @param[in] name Input parameter.
     * @return True when the operation succeeds.
     */
    bool hasCTE(const std::string& name) const;

    /**
     * @brief List CTEs.
     * @return Return value.
     */
    std::vector<std::string> listCTEs() const;

    /**
     * @brief Get View.
     * @param[in] name Input parameter.
     * @return Return value.
     */
    std::shared_ptr<MaterializedCTEView> getView(const std::string& name) const;

    /**
     * @brief Apply Change.
     * @param[in] change Input parameter.
     */
    void applyChange(const CTEDataChange& change);

    /**
     * @brief Apply Changes.
     * @param[in] changes Input parameter.
     */
    void applyChanges(const std::vector<CTEDataChange>& changes);

    MaterializedCTEResult query(
        const std::string& cte_name,
        int64_t limit  = 0,
        int64_t offset = 0
    ) const;

    uint64_t totalChanges() const { return total_changes_.load(); }

private:
    mutable std::shared_mutex registry_mutex_;
    std::unordered_map<std::string, std::shared_ptr<MaterializedCTEView>> views_;
    std::atomic<uint64_t> total_changes_{0};
};

} // namespace query
} // namespace themis
