/**
 * @file incremental_view.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once
// Incremental view maintenance for materialized views
#include <string>
#include <vector>
#include <chrono>

namespace themis { namespace query {

struct ViewDefinition {
    std::string view_id;
    std::string name;
    std::string query;
    bool is_materialized = true;
    std::chrono::milliseconds refresh_interval{0};
    std::vector<std::string> source_collections;
};

enum class ViewRefreshMode {
    IMMEDIATE,
    DEFERRED,
    ON_DEMAND,
};

struct ViewRefreshStats {
    std::chrono::system_clock::time_point last_refresh;
    double last_refresh_ms = 0.0;
    size_t rows_updated = 0;
    size_t rows_deleted = 0;
    size_t rows_inserted = 0;
    bool is_stale = false;
};

class IIncrementalViewMaintainer {
public:
    /**
     * @brief IIncremental View Maintainer.
     * @return Return value.
     */
    virtual ~IIncrementalViewMaintainer() = default;
    /**
     * @brief Create View.
     * @param[in] def Input parameter.
     * @return True when the operation succeeds.
     */
    virtual bool createView(const ViewDefinition& def) = 0;
    /**
     * @brief Drop View.
     * @param[in] view_id Identifier of the view.
     * @return True when the operation succeeds.
     */
    virtual bool dropView(const std::string& view_id) = 0;
    /**
     * @brief Refresh View.
     * @param[in] view_id Identifier of the view.
     * @return Return value.
     */
    virtual ViewRefreshStats refreshView(const std::string& view_id) = 0;
    /**
     * @brief Get Stats.
     * @param[in] view_id Identifier of the view.
     * @return Return value.
     */
    virtual ViewRefreshStats getStats(const std::string& view_id) const = 0;
    /**
     * @brief Set Refresh Mode.
     * @param[in] view_id Identifier of the view.
     * @param[in] mode Input parameter.
     * @return True when the operation succeeds.
     */
    virtual bool setRefreshMode(const std::string& view_id, ViewRefreshMode mode) = 0;
    /**
     * @brief List Views.
     * @return Return value.
     */
    virtual std::vector<ViewDefinition> listViews() const = 0;
    /**
     * @brief Is Stale.
     * @param[in] view_id Identifier of the view.
     * @return True when the operation succeeds.
     */
    virtual bool isStale(const std::string& view_id) const = 0;
};

}} // namespace themis::query
