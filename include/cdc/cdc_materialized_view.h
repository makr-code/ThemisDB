/**
 * @file cdc_materialized_view.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * ThemisDB CDC-based Materialized View Maintenance
 *
 * Bridges CDC change events (Changefeed::ChangeEvent) to the analytics
 * incremental view maintenance engine (IncrementalViewManager), enabling
 * materialized views to be kept up-to-date automatically as CDC events
 * arrive.
 *
 * Copyright (c) 2025 ThemisDB Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "cdc/changefeed.h"
#include "analytics/incremental_view.h"

#include <atomic>
#include <memory>
#include <string>
#include <vector>

namespace themis {
namespace cdc {

class CDCMaterializedViewMaintainer {
public:
    CDCMaterializedViewMaintainer();
    ~CDCMaterializedViewMaintainer();

    CDCMaterializedViewMaintainer(const CDCMaterializedViewMaintainer&)            = delete;
    CDCMaterializedViewMaintainer& operator=(const CDCMaterializedViewMaintainer&) = delete;

    /**
     * @brief ---- view lifecycle ----
     * @param[in] def Input parameter.
     * @return True when the operation succeeds.
     */

    bool createView(const themisdb::analytics::ViewDefinition& def);

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
     * @brief List Views.
     * @return Return value.
     */
    std::vector<std::string> listViews() const;

    std::shared_ptr<themisdb::analytics::IncrementalView>
    getView(const std::string& name) const;

    /**
     * @brief ---- change ingestion ----
     * @param[in] event Input parameter.
     */

    void applyEvent(const Changefeed::ChangeEvent& event);

    /**
     * @brief Apply Events.
     * @param[in] events Input parameter.
     */
    void applyEvents(const std::vector<Changefeed::ChangeEvent>& events);

    // ---- query ----

    themisdb::analytics::ViewQueryResult query(
        const std::string& view_name,
        const std::vector<themisdb::analytics::ViewFilter>& filters = {},
        int64_t limit  = 0,
        int64_t offset = 0
    ) const;

    uint64_t totalEventsProcessed() const { return total_events_processed_.load(); }

private:
    themisdb::analytics::IncrementalViewManager view_manager_;
    std::atomic<uint64_t> total_events_processed_{0};

    static themisdb::analytics::ChangeRecord
    toChangeRecord(const Changefeed::ChangeEvent& event);

    /**
     * @brief Extract Collection.
     * @param[in] key Input parameter.
     * @return Return value.
     */
    static std::string extractCollection(const std::string& key);

    static themisdb::analytics::ChangeRecord::Row
    parseJsonRow(const std::string& json_str);
};

} // namespace cdc
} // namespace themis
