/**
 * @file cdc_materialized_view.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/**
 * @brief CDC-based materialized view maintainer.
 *
 * Bridges Changefeed::ChangeEvent objects to the analytics
 * IncrementalViewManager so that registered materialized views are
 * kept up-to-date incrementally as CDC events arrive.
 *
 * Collection derivation: the collection name is taken as the substring of
 * the event key before the first ':' character (e.g. "users:42" → "users").
 * If the key contains no ':', the full key is used as the collection name.
 *
 * Change-type mapping:
 *   EVENT_PUT  + no before_snapshot   →  INSERT
 *   EVENT_PUT  + has before_snapshot  →  UPDATE
 *   EVENT_DELETE                      →  DELETE
 *   TRANSACTION_COMMIT / _ROLLBACK    →  skipped (not counted)
 *
 * Row data:
 *   before_row is populated from before_snapshot (JSON object string).
 *   after_row  is populated from after_snapshot  (JSON object string).
 *   If a snapshot is absent, the event's value field is used as a fallback
 *   for the after_row (INSERT / UPDATE) or before_row (DELETE).
 *
 * Thread-safety: all public methods are thread-safe.
 */
class CDCMaterializedViewMaintainer {
public:
    CDCMaterializedViewMaintainer();
    ~CDCMaterializedViewMaintainer();

    CDCMaterializedViewMaintainer(const CDCMaterializedViewMaintainer&)            = delete;
    CDCMaterializedViewMaintainer& operator=(const CDCMaterializedViewMaintainer&) = delete;

    // ---- view lifecycle ----

    /**
     * Register a new materialized view for CDC-driven maintenance.
     * @return false if a view with the same name already exists.
     */
    bool createView(const themisdb::analytics::ViewDefinition& def);

    /**
     * Remove a materialized view by name.
     * @return false if the view is not found.
     */
    bool dropView(const std::string& name);

    /** Return true if a view with the given name exists. */
    bool hasView(const std::string& name) const;

    /** List all registered view names. */
    std::vector<std::string> listViews() const;

    /**
     * Get a view by name.
     * @return nullptr if not found.
     */
    std::shared_ptr<themisdb::analytics::IncrementalView>
    getView(const std::string& name) const;

    // ---- change ingestion ----

    /**
     * Apply a single CDC change event to all relevant views.
     *
     * TRANSACTION_COMMIT and TRANSACTION_ROLLBACK events are silently
     * skipped and do not increment totalEventsProcessed().
     */
    void applyEvent(const Changefeed::ChangeEvent& event);

    /**
     * Apply a batch of CDC change events.
     *
     * Equivalent to calling applyEvent() for each element but acquires
     * per-view locks once for the entire batch, making it more efficient
     * for high-throughput ingestion paths.
     */
    void applyEvents(const std::vector<Changefeed::ChangeEvent>& events);

    // ---- query ----

    /**
     * Query a materialized view by name.
     * @return empty ViewQueryResult if the view is not found.
     */
    themisdb::analytics::ViewQueryResult query(
        const std::string& view_name,
        const std::vector<themisdb::analytics::ViewFilter>& filters = {},
        int64_t limit  = 0,
        int64_t offset = 0
    ) const;

    /**
     * Total CDC events processed (TRANSACTION_* events not counted).
     */
    uint64_t totalEventsProcessed() const { return total_events_processed_.load(); }

private:
    themisdb::analytics::IncrementalViewManager view_manager_;
    std::atomic<uint64_t> total_events_processed_{0};

    /**
     * Convert a CDC ChangeEvent to an analytics ChangeRecord.
     * Returns a record with an empty collection string for events that
     * should be skipped (TRANSACTION_* types).
     */
    static themisdb::analytics::ChangeRecord
    toChangeRecord(const Changefeed::ChangeEvent& event);

    /**
     * Extract the collection name from an event key.
     * Returns the substring before the first ':', or the full key if
     * no ':' is present.
     */
    static std::string extractCollection(const std::string& key);

    /**
     * Parse a JSON object string into a ChangeRecord::Row.
     * Non-object or invalid JSON yields an empty row.
     */
    static themisdb::analytics::ChangeRecord::Row
    parseJsonRow(const std::string& json_str);
};

} // namespace cdc
} // namespace themis
