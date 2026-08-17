/**
 * @file materialized_view.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <chrono>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include <nlohmann/json.hpp>

#include "utils/expected.h"

// Forward-declare BaseEntity to avoid pulling in all of storage/ here.
// Callers that invoke the BaseEntity overloads must include base_entity.h.
namespace themis { class BaseEntity; }

// Forward-declare query::Query for the ParsedQuery-based canRewrite overload.
namespace themis { namespace query { struct Query; } }

namespace themis {
namespace query {

/**
 * @brief Operation type applied to a base table.
 *
 * Used by incremental-maintenance delta processing to distinguish inserts,
 * updates, and deletes arriving from the storage layer.
 */
enum class DeltaOp {
    INSERT,  ///< A new row was inserted into a base table.
    UPDATE,  ///< An existing row was modified in a base table.
    DELETE   ///< A row was removed from a base table.
};

// ============================================================================
// MaterializedView
// ============================================================================

/**
 * @brief Pre-computed query result with incremental maintenance — v1.8.0
 *
 * A MaterializedView stores the result of an AQL query as a snapshot of
 * pre-computed rows.  Depending on the chosen RefreshStrategy the view is
 * kept fresh either immediately on every base-table change, lazily at query
 * time, on a periodic schedule, or only when the user explicitly calls
 * refresh().
 *
 * ### Features
 *  - View definition and creation
 *  - Automatic query rewriting detection (canRewrite)
 *  - Incremental maintenance via applyDelta / applyDeltaJson
 *  - Partial-refresh strategies (IMMEDIATE, DEFERRED, PERIODIC, MANUAL)
 *  - View staleness tracking (isStale / markStale)
 *
 * ### Thread Safety
 *  All public methods are thread-safe.
 *
 * ### Refresh Strategies
 *  - IMMEDIATE  : Every call to applyDelta applies the change in-place.
 *  - DEFERRED   : applyDelta marks the view stale; the next query call
 *                 (or an explicit refresh()) recomputes the snapshot.
 *  - PERIODIC   : The view is marked stale; an external scheduler calls
 *                 refresh() at the desired interval.
 *  - MANUAL     : applyDelta is a no-op; the user triggers refresh()
 *                 explicitly.
 *
 * ### Performance
 *  - Query speedup: 10–100× over re-executing the base aggregation query
 *    (depends on aggregation complexity and dataset size).
 *  - Insert overhead for IMMEDIATE maintenance: 5–20% over a raw insert.
 *
 * Example:
 * @code
 *   MaterializedView::Definition def;
 *   def.name               = "sales_by_region";
 *   def.query_aql          = "FOR s IN sales COLLECT r=s.region ...";
 *   def.strategy           = MaterializedView::RefreshStrategy::DEFERRED;
 *   def.staleness_tolerance = std::chrono::minutes(5);
 *   def.base_tables        = {"sales"};
 *
 *   auto view = MaterializedView::create(def).value();
 *   view->refresh({precomputed_rows});
 *
 *   // Check if a query can be served from the view
 *   bool rewrite = MaterializedView::canRewrite(
 *       "FOR r IN sales_by_region FILTER r.region=='EU' RETURN r", *view);
 *
 *   // Incremental update on insert
 *   view->applyDeltaJson(DeltaOp::INSERT, new_sale_row);
 * @endcode
 */
class MaterializedView {
public:
    // =========================================================================
    // Supporting types
    // =========================================================================

    /**
     * @brief How and when the view's snapshot is refreshed.
     */
    enum class RefreshStrategy {
        IMMEDIATE,  ///< Apply every base-table delta in-place immediately.
        DEFERRED,   ///< Mark stale on delta; recompute on next access.
        PERIODIC,   ///< Mark stale on delta; external scheduler refreshes.
        MANUAL      ///< Only refresh on explicit user call to refresh().
    };

    /**
     * @brief Immutable view definition supplied at creation time.
     */
    struct Definition {
        /// Logical name of the view (used for query rewriting).
        std::string name;

        /// AQL query whose result this view materialises.
        std::string query_aql;

        /// How the view is kept up-to-date after base-table changes.
        RefreshStrategy strategy = RefreshStrategy::DEFERRED;

        /**
         * @brief Maximum age of the snapshot before it is considered stale.
         *
         * Only honoured by DEFERRED and PERIODIC strategies.  Set to zero to
         * disable time-based staleness (require explicit markStale() calls).
         * Millisecond granularity allows sub-second tolerances in unit tests.
         */
        std::chrono::milliseconds staleness_tolerance{60000};  // default 60 s

        /**
         * @brief Base tables (collections) whose changes invalidate this view.
         *
         * The MaterializedViewRegistry uses this list to route delta events
         * to the correct views.
         */
        std::vector<std::string> base_tables;
    };

    /**
     * @brief Runtime statistics for monitoring and diagnostics.
     */
    struct ViewStats {
        /// Number of full (non-incremental) refreshes performed.
        uint64_t full_refreshes = 0;

        /// Number of incremental delta applications performed.
        uint64_t incremental_updates = 0;

        /// Number of times query rows have been retrieved from the view.
        uint64_t query_hits = 0;

        uint64_t delta_inserts = 0;  ///< Rows appended via INSERT delta.
        uint64_t delta_deletes = 0;  ///< Rows removed via DELETE delta.
        uint64_t delta_updates = 0;  ///< Rows replaced via UPDATE delta.

        /// Number of rows currently stored in the snapshot.
        size_t current_row_count = 0;

        /// Timestamp of the most recent successful refresh.
        std::chrono::system_clock::time_point last_refresh{};

        /// True when the snapshot is considered out-of-date.
        bool is_stale = true;
    };

    /**
     * @brief Tuning parameters for a single MaterializedView instance.
     */
    struct Config {
        /// Hard upper bound on the number of rows that may be stored.
        size_t max_rows = 1'000'000;

        Config() = default;
    };

    // =========================================================================
    // Construction / factory
    // =========================================================================

    /**
     * @brief Create a new MaterializedView from @p def.
     *
     * The returned view starts in the stale state — call refresh() to
     * populate its snapshot.
     *
     * @param def    View definition.
     * @return       Shared pointer to the new view, or an Error.
     */
    static Result<std::shared_ptr<MaterializedView>> create(
        const Definition& def);

    /**
     * @brief Create a new MaterializedView from @p def.
     *
     * The returned view starts in the stale state — call refresh() to
     * populate its snapshot.
     *
     * @param def    View definition.
     * @param config Optional tuning parameters.
     * @return       Shared pointer to the new view, or an Error.
     */
    static Result<std::shared_ptr<MaterializedView>> create(
        const Definition& def,
        Config            config);

    ~MaterializedView();

    // Non-copyable, movable.
    MaterializedView(const MaterializedView&)            = delete;
    MaterializedView& operator=(const MaterializedView&) = delete;
    MaterializedView(MaterializedView&&)                 noexcept = default;
    MaterializedView& operator=(MaterializedView&&)      noexcept = default;

    // =========================================================================
    // Refresh / staleness
    // =========================================================================

    /**
     * @brief Refresh the view snapshot.
     *
     * @param incremental  When true, apply pending row-level deltas on top of
     *                     the existing snapshot.  When false, replace the
     *                     snapshot entirely with @p new_rows.
     * @param new_rows     Replacement rows used for a full (non-incremental)
     *                     refresh.  Ignored when @p incremental is true.
     * @return             OkVoid() on success; Error on failure (e.g., row
     *                     count exceeds Config::max_rows).
     */
    Result<void> refresh(bool incremental = true,
                         std::vector<nlohmann::json> new_rows = {});

    /**
     * @brief Mark the view snapshot as stale.
     *
     * After this call isStale() returns true until the next successful
     * refresh().
     */
    void markStale();

    /**
     * @brief Return true when the snapshot is out-of-date.
     *
     * The snapshot is stale when:
     *  - markStale() has been called since the last refresh(), OR
     *  - staleness_tolerance > 0 and the snapshot age exceeds it.
     */
    bool isStale() const;

    // =========================================================================
    // Incremental delta maintenance
    // =========================================================================

    /**
     * @brief Apply a base-table change (described as a BaseEntity) to the view.
     *
     * The effect depends on the view's RefreshStrategy:
     *  - IMMEDIATE : the row-level change is applied immediately to rows_.
     *  - DEFERRED  : the view is marked stale (no row change).
     *  - PERIODIC  : the view is marked stale.
     *  - MANUAL    : no-op.
     *
     * @param op     Type of base-table change.
     * @param entity The inserted / updated / deleted entity.
     */
    void applyDelta(DeltaOp op, const BaseEntity& entity);

    /**
     * @brief Apply a base-table change (described as a JSON row) to the view.
     *
     * Equivalent to applyDelta() but accepts JSON rows directly — useful for
     * tests and callers that do not have a BaseEntity available.
     *
     * For IMMEDIATE strategy:
     *  - INSERT  appends @p row to the snapshot.
     *  - DELETE  removes all rows whose "_key" or "_id" field matches @p row.
     *  - UPDATE  removes matching rows and appends @p row.
     *
     * @param op  Type of base-table change.
     * @param row JSON representation of the changed row.
     */
    void applyDeltaJson(DeltaOp op, const nlohmann::json& row);

    /**
     * @brief Apply an aggregate-level delta for common SUM / COUNT patterns.
     *
     * This is a convenience helper for views that materialise a single
     * aggregate value (e.g. SUM(amount)).  The caller is responsible for
     * mapping the field name and current accumulator.
     *
     * @param op             Type of base-table change.
     * @param entity         The changed entity.
     * @param field_name     Name of the numeric field being aggregated.
     * @param aggregate_ref  Reference to the running aggregate accumulator.
     */
    static void applyAggregateDelta(DeltaOp              op,
                                    const BaseEntity&    entity,
                                    const std::string&   field_name,
                                    double&              aggregate_ref);

    // =========================================================================
    // Query access
    // =========================================================================

    /**
     * @brief Return all rows currently stored in the snapshot.
     *
     * Increments the query_hits counter.
     *
     * @return Copy of the internal row vector.
     */
    std::vector<nlohmann::json> getRows() const;

    /**
     * @brief Return rows matching a simple equality filter.
     *
     * Scans the snapshot and returns every row where
     * row[filter_field] == filter_value.  Pass an empty filter_field to
     * return all rows (equivalent to getRows()).
     *
     * @param filter_field  JSON field name to match on.
     * @param filter_value  Required JSON value for that field.
     * @return              Matching rows.
     */
    std::vector<nlohmann::json> queryRows(
        const std::string&   filter_field  = "",
        const nlohmann::json& filter_value = nlohmann::json{}) const;

    // =========================================================================
    // Query rewriting
    // =========================================================================

    /**
     * @brief Return true when @p query_aql can be served from @p view.
     *
     * A query can be rewritten to use the view when the query's primary FOR
     * clause iterates over the view's name as its collection — i.e. the view's
     * pre-computed rows are the intended data source.
     *
     * Example:
     * @code
     *   // View name: "sales_by_region"
     *   // Rewritable:  "FOR r IN sales_by_region FILTER ... RETURN r"
     *   // Not rewritable: "FOR s IN sales RETURN s"
     * @endcode
     *
     * @param query_aql  Raw AQL query string.
     * @param view       Candidate materialized view.
     * @return           true if the view can answer the query.
     */
    static bool canRewrite(const std::string&      query_aql,
                           const MaterializedView& view);

    /**
     * @brief Overload accepting a parsed query AST.
     *
     * Checks whether the primary FOR node's collection name matches the view
     * name.  Prefer this over the string overload when the AST is already
     * available (avoids re-parsing).
     *
     * @param parsed_query  Parsed AQL query AST.
     * @param view          Candidate materialized view.
     * @return              true if the view can answer the query.
     */
    static bool canRewrite(const query::Query&     parsed_query,
                           const MaterializedView& view);

    // =========================================================================
    // Accessors
    // =========================================================================

    const Definition& getDefinition()  const;
    const std::string& getName()       const;
    ViewStats          getStats()      const;

    /// Timestamp of the most recent successful refresh.
    std::chrono::system_clock::time_point getLastRefresh() const;

private:
    explicit MaterializedView(const Definition& def, Config config);

    // -------------------------------------------------------------------------
    // Internal helpers (caller must hold mutex_)
    // -------------------------------------------------------------------------

    /// True when the snapshot age exceeds staleness_tolerance (lock held).
    bool isStaleByAge_locked() const;

    /// Apply an INSERT delta row (lock held).
    void applyInsert_locked(const nlohmann::json& row);

    /// Apply a DELETE delta — removes rows matching the primary key (lock held).
    void applyDelete_locked(const nlohmann::json& row);

    // -------------------------------------------------------------------------
    // State
    // -------------------------------------------------------------------------

    Definition def_;
    Config     config_;

    mutable std::mutex         mutex_;
    std::vector<nlohmann::json> rows_;
    mutable ViewStats           stats_;
    bool                        stale_    = true;
};

// ============================================================================
// MaterializedViewRegistry
// ============================================================================

/**
 * @brief Registry that manages a collection of MaterializedView instances.
 *
 * The registry acts as the integration point between the storage layer and the
 * materialized-view subsystem.  Storage components call the onInsert /
 * onDelete / onUpdate methods; the registry forwards the delta to every view
 * that depends on the affected table.
 *
 * The registry also provides query-rewriting support via tryRewrite(), which
 * returns the first registered view whose name is referenced by the supplied
 * AQL query string.
 *
 * Thread Safety: all public methods are thread-safe.
 */
class MaterializedViewRegistry {
public:
    MaterializedViewRegistry()  = default;
    ~MaterializedViewRegistry() = default;

    // Non-copyable, movable.
    MaterializedViewRegistry(const MaterializedViewRegistry&)            = delete;
    MaterializedViewRegistry& operator=(const MaterializedViewRegistry&) = delete;
    MaterializedViewRegistry(MaterializedViewRegistry&&)                 noexcept = default;
    MaterializedViewRegistry& operator=(MaterializedViewRegistry&&)      noexcept = default;

    // =========================================================================
    // Registration
    // =========================================================================

    /**
     * @brief Register @p view with the registry.
     *
     * @return OkVoid() on success; error if a view with the same name already
     *         exists.
     */
    Result<void> registerView(std::shared_ptr<MaterializedView> view);

    /**
     * @brief Look up a view by @p name.
     * @return The view, or nullptr if not found.
     */
    std::shared_ptr<MaterializedView> getView(const std::string& name) const;

    /**
     * @brief Remove the view named @p name from the registry.
     * @return true if a view was removed; false if not found.
     */
    bool removeView(const std::string& name);

    /**
     * @brief Return the names of all registered views.
     */
    std::vector<std::string> listViews() const;

    // =========================================================================
    // Delta propagation (BaseEntity overloads)
    // =========================================================================

    /**
     * @brief Notify the registry that a row was inserted into @p table.
     *
     * Forwards the delta to every view that lists @p table in its
     * Definition::base_tables.
     */
    void onInsert(const std::string& table, const BaseEntity& entity);

    /** @brief Notify the registry that a row in @p table was deleted. */
    void onDelete(const std::string& table, const BaseEntity& entity);

    /** @brief Notify the registry that a row in @p table was updated. */
    void onUpdate(const std::string& table, const BaseEntity& entity);

    // =========================================================================
    // Delta propagation (JSON overloads — for tests and lightweight callers)
    // =========================================================================

    /** @brief JSON-based counterpart of onInsert(). */
    void onInsertJson(const std::string& table, const nlohmann::json& row);

    /** @brief JSON-based counterpart of onDelete(). */
    void onDeleteJson(const std::string& table, const nlohmann::json& row);

    /** @brief JSON-based counterpart of onUpdate(). */
    void onUpdateJson(const std::string& table, const nlohmann::json& row);

    // =========================================================================
    // Query rewriting
    // =========================================================================

    /**
     * @brief Find the first registered view that can rewrite @p query_aql.
     *
     * Iterates over all views and returns the first one for which
     * MaterializedView::canRewrite(query_aql, *view) is true.
     *
     * @param query_aql  Raw AQL query string.
     * @return           Matching view, or nullptr if none can rewrite.
     */
    std::shared_ptr<MaterializedView> tryRewrite(
        const std::string& query_aql) const;

    // =========================================================================
    // Maintenance helpers
    // =========================================================================

    /**
     * @brief Refresh every stale view that supports automatic refreshes
     *        (DEFERRED or PERIODIC strategy).
     *
     * @return Number of views refreshed.
     */
    size_t refreshStale();

private:
    /// Forward a JSON delta to every view that depends on @p table.
    void propagateDeltaJson_locked(const std::string&   table,
                                   DeltaOp              op,
                                   const nlohmann::json& row);

    mutable std::mutex mutex_;

    /// view_name → view
    std::unordered_map<std::string, std::shared_ptr<MaterializedView>> views_;

    /// table_name → list of view names that depend on this table
    std::unordered_map<std::string, std::vector<std::string>> table_index_;
};

}  // namespace query
}  // namespace themis
