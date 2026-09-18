/**
 * @file materialized_view.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

enum class DeltaOp {
    INSERT,  ///< A new row was inserted into a base table.
    UPDATE,  ///< An existing row was modified in a base table.
    DELETE   ///< A row was removed from a base table.
};

// ============================================================================
// MaterializedView
// ============================================================================

class MaterializedView {
public:
    // =========================================================================
    // Supporting types
    // =========================================================================

    enum class RefreshStrategy {
        IMMEDIATE,  ///< Apply every base-table delta in-place immediately.
        DEFERRED,   ///< Mark stale on delta; recompute on next access.
        PERIODIC,   ///< Mark stale on delta; external scheduler refreshes.
        MANUAL      ///< Only refresh on explicit user call to refresh().
    };

    struct Definition {
        std::string name;

        std::string query_aql;

        RefreshStrategy strategy = RefreshStrategy::DEFERRED;

        std::chrono::milliseconds staleness_tolerance{60000};  // default 60 s

        std::vector<std::string> base_tables;
    };

    struct ViewStats {
        uint64_t full_refreshes = 0;

        uint64_t incremental_updates = 0;

        uint64_t query_hits = 0;

        uint64_t delta_inserts = 0;  ///< Rows appended via INSERT delta.
        uint64_t delta_deletes = 0;  ///< Rows removed via DELETE delta.
        uint64_t delta_updates = 0;  ///< Rows replaced via UPDATE delta.

        size_t current_row_count = 0;

        std::chrono::system_clock::time_point last_refresh{};

        bool is_stale = true;
    };

    struct Config {
        size_t max_rows = 1'000'000;

        Config() = default;
    };

    // =========================================================================
    // Construction / factory
    // =========================================================================

    /**
     * @brief Create.
     * @param[in] def Input parameter.
     * @return Return value.
     */
    static Result<std::shared_ptr<MaterializedView>> create(
        const Definition& def);

    /**
     * @brief Create.
     * @param[in] def Input parameter.
     * @param[in] config Input parameter.
     * @return Return value.
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

    Result<void> refresh(bool incremental = true,
                         std::vector<nlohmann::json> new_rows = {});

    /**
     * @brief Mark Stale.
     */
    void markStale();

    /**
     * @brief Is Stale.
     * @return True when the operation succeeds.
     */
    bool isStale() const;

    // =========================================================================
    // Incremental delta maintenance
    // =========================================================================

    /**
     * @brief Apply Delta.
     * @param[in] op Input parameter.
     * @param[in] entity Input parameter.
     */
    void applyDelta(DeltaOp op, const BaseEntity& entity);

    /**
     * @brief Apply Delta Json.
     * @param[in] op Input parameter.
     * @param[in] row Input parameter.
     */
    void applyDeltaJson(DeltaOp op, const nlohmann::json& row);

    /**
     * @brief Apply Aggregate Delta.
     * @param[in] op Input parameter.
     * @param[in] entity Input parameter.
     * @param[in] field_name Name of the field.
     * @param[in,out] aggregate_ref Input/output parameter.
     */
    static void applyAggregateDelta(DeltaOp              op,
                                    const BaseEntity&    entity,
                                    const std::string&   field_name,
                                    double&              aggregate_ref);

    // =========================================================================
    // Query access
    // =========================================================================

    /**
     * @brief Get Rows.
     * @return Return value.
     */
    std::vector<nlohmann::json> getRows() const;

    std::vector<nlohmann::json> queryRows(
        const std::string&   filter_field  = "",
        const nlohmann::json& filter_value = nlohmann::json{}) const;

    // =========================================================================
    // Query rewriting
    // =========================================================================

    /**
     * @brief Can Rewrite.
     * @param[in] query_aql Input parameter.
     * @param[in] view Input parameter.
     * @return True when the operation succeeds.
     */
    static bool canRewrite(const std::string&      query_aql,
                           const MaterializedView& view);

    /**
     * @brief Can Rewrite.
     * @param[in] parsed_query Input parameter.
     * @param[in] view Input parameter.
     * @return True when the operation succeeds.
     */
    static bool canRewrite(const query::Query&     parsed_query,
                           const MaterializedView& view);

    // =========================================================================
    // Accessors
    // =========================================================================

    /**
     * @brief Get Definition.
     * @return Return value.
     */
    const Definition& getDefinition()  const;
    /**
     * @brief Get Name.
     * @return Return value.
     */
    const std::string& getName()       const;
    /**
     * @brief Get Stats.
     * @return Return value.
     */
    ViewStats          getStats()      const;

    /**
     * @brief Get Last Refresh.
     * @return Return value.
     */
    std::chrono::system_clock::time_point getLastRefresh() const;

private:
    /**
     * @brief Materialized View.
     * @param[in] def Input parameter.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit MaterializedView(const Definition& def, Config config);

    /**
     * @brief ------------------------------------------------------------------------- Internal helpers (caller must hold mutex_) -------------------------------------------------------------------------
     * @return True when the operation succeeds.
     */

    bool isStaleByAge_locked() const;

    /**
     * @brief Apply Insert locked.
     * @param[in] row Input parameter.
     */
    void applyInsert_locked(const nlohmann::json& row);

    /**
     * @brief Apply Delete locked.
     * @param[in] row Input parameter.
     */
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
     * @brief Register View.
     * @param[in] view Input parameter.
     * @return Return value.
     */
    Result<void> registerView(std::shared_ptr<MaterializedView> view);

    /**
     * @brief Get View.
     * @param[in] name Input parameter.
     * @return Return value.
     */
    std::shared_ptr<MaterializedView> getView(const std::string& name) const;

    /**
     * @brief Remove View.
     * @param[in] name Input parameter.
     * @return True when the operation succeeds.
     */
    bool removeView(const std::string& name);

    /**
     * @brief List Views.
     * @return Return value.
     */
    std::vector<std::string> listViews() const;

    // =========================================================================
    // Delta propagation (BaseEntity overloads)
    // =========================================================================

    /**
     * @brief On Insert.
     * @param[in] table Input parameter.
     * @param[in] entity Input parameter.
     */
    void onInsert(const std::string& table, const BaseEntity& entity);

    /**
     * @brief On Delete.
     * @param[in] table Input parameter.
     * @param[in] entity Input parameter.
     */
    void onDelete(const std::string& table, const BaseEntity& entity);

    /**
     * @brief On Update.
     * @param[in] table Input parameter.
     * @param[in] entity Input parameter.
     */
    void onUpdate(const std::string& table, const BaseEntity& entity);

    /**
     * @brief ========================================================================= Delta propagation (JSON overloads — for tests and lightweight callers) =========================================================================
     * @param[in] table Input parameter.
     * @param[in] row Input parameter.
     */

    void onInsertJson(const std::string& table, const nlohmann::json& row);

    /**
     * @brief On Delete Json.
     * @param[in] table Input parameter.
     * @param[in] row Input parameter.
     */
    void onDeleteJson(const std::string& table, const nlohmann::json& row);

    /**
     * @brief On Update Json.
     * @param[in] table Input parameter.
     * @param[in] row Input parameter.
     */
    void onUpdateJson(const std::string& table, const nlohmann::json& row);

    // =========================================================================
    // Query rewriting
    // =========================================================================

    /**
     * @brief Try Rewrite.
     * @param[in] query_aql Input parameter.
     * @return Return value.
     */
    std::shared_ptr<MaterializedView> tryRewrite(
        const std::string& query_aql) const;

    // =========================================================================
    // Maintenance helpers
    // =========================================================================

    /**
     * @brief Refresh Stale.
     * @return Return value.
     */
    size_t refreshStale();

private:
    /**
     * @brief Propagate Delta Json locked.
     * @param[in] table Input parameter.
     * @param[in] op Input parameter.
     * @param[in] row Input parameter.
     */
    void propagateDeltaJson_locked(const std::string&   table,
                                   DeltaOp              op,
                                   const nlohmann::json& row);

    mutable std::mutex mutex_;

    std::unordered_map<std::string, std::shared_ptr<MaterializedView>> views_;

    std::unordered_map<std::string, std::vector<std::string>> table_index_;
};

}  // namespace query
}  // namespace themis
