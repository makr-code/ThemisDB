/**
 * @file cdc_materialized_view.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * ThemisDB CDC-based Materialized View Maintenance — Implementation
 *
 * Converts Changefeed::ChangeEvent records to analytics::ChangeRecord and
 * dispatches them to IncrementalViewManager for delta-based view maintenance.
 *
 * Copyright (c) 2025 ThemisDB Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include "cdc/cdc_materialized_view.h"

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

namespace themis {
namespace cdc {

CDCMaterializedViewMaintainer::CDCMaterializedViewMaintainer()  = default;
CDCMaterializedViewMaintainer::~CDCMaterializedViewMaintainer() = default;

// ============================================================================
// View lifecycle
// ============================================================================

bool CDCMaterializedViewMaintainer::createView(const themisdb::analytics::ViewDefinition &def) {
    return view_manager_.createView(def);
}

bool CDCMaterializedViewMaintainer::dropView(const std::string &name) {
    return view_manager_.dropView(name);
}

bool CDCMaterializedViewMaintainer::hasView(const std::string &name) const {
    return view_manager_.hasView(name);
}

std::vector<std::string> CDCMaterializedViewMaintainer::listViews() const {
    return view_manager_.listViews();
}

std::shared_ptr<themisdb::analytics::IncrementalView>
CDCMaterializedViewMaintainer::getView(const std::string &name) const {
    return view_manager_.getView(name);
}

// ============================================================================
// Change ingestion
// ============================================================================

void CDCMaterializedViewMaintainer::applyEvent(const Changefeed::ChangeEvent &event) {
    auto rec = toChangeRecord(event);
    if (rec.collection.empty()) {
        return; // TRANSACTION_* — skip
    }
    view_manager_.applyChange(rec);
    ++total_events_processed_;
}

void CDCMaterializedViewMaintainer::applyEvents(const std::vector<Changefeed::ChangeEvent> &events) {
    std::vector<themisdb::analytics::ChangeRecord> records = {};

    records.reserve(events.size());
    for (const auto &ev : events) {
        auto rec = toChangeRecord(ev);
        if (!rec.collection.empty()) {
            records.push_back(std::move(rec));
        }
    }
    if (!records.empty()) {
        view_manager_.applyChanges(records);
        total_events_processed_ += static_cast<uint64_t>(records.size());
    }
}

// ============================================================================
// Query
// ============================================================================

themisdb::analytics::ViewQueryResult
CDCMaterializedViewMaintainer::query(const std::string &view_name,
                                     const std::vector<themisdb::analytics::ViewFilter> &filters, int64_t limit,
                                     int64_t offset) const {
    return view_manager_.query(view_name, filters, limit, offset);
}

// ============================================================================
// Private helpers
// ============================================================================

std::string CDCMaterializedViewMaintainer::extractCollection(const std::string &key) {
    auto pos = key.find(':');
    if (pos == std::string::npos) {
        return key;
    }
    return key.substr(0, pos);
}

themisdb::analytics::ChangeRecord::Row CDCMaterializedViewMaintainer::parseJsonRow(const std::string &json_str) {
    themisdb::analytics::ChangeRecord::Row row;
    if (json_str.empty()) {
        return row;
    }
    try {
        auto j = nlohmann::json::parse(json_str);
        if (!j.is_object()) {
            return row;
        }
        for (auto it = j.begin(); it != j.end(); ++it) {
            const auto &v = it.value();
            if (v.is_null()) {
                row[it.key()] = themisdb::analytics::FieldValue{nullptr};
            } else if (v.is_boolean()) {
                row[it.key()] = themisdb::analytics::FieldValue{v.get<bool>()};
            } else if (v.is_number_integer()) {
                row[it.key()] = themisdb::analytics::FieldValue{v.get<int64_t>()};
            } else if (v.is_number_float()) {
                row[it.key()] = themisdb::analytics::FieldValue{v.get<double>()};
            } else if (v.is_string()) {
                row[it.key()] = themisdb::analytics::FieldValue{v.get<std::string>()};
            } else {
                row[it.key()] = themisdb::analytics::FieldValue{v.dump()};
            }
        }
    } catch (const nlohmann::json::parse_error &e) {
        spdlog::warn("CDCMaterializedViewMaintainer: failed to parse row JSON: {}", e.what());
    }
    return row;
}

themisdb::analytics::ChangeRecord CDCMaterializedViewMaintainer::toChangeRecord(const Changefeed::ChangeEvent &event) {
    themisdb::analytics::ChangeRecord rec;

    // Transaction lifecycle events carry no document data — skip them.
    if (event.type == Changefeed::ChangeEventType::EVENT_TRANSACTION_COMMIT
        || event.type == Changefeed::ChangeEventType::EVENT_TRANSACTION_ROLLBACK) {
        return rec; // empty collection signals "skip"
    }

    rec.collection  = extractCollection(event.key);
    rec.change_time = std::chrono::system_clock::time_point{std::chrono::milliseconds{event.timestamp_ms}};

    if (event.type == Changefeed::ChangeEventType::EVENT_DELETE) {
        rec.type = themisdb::analytics::ChangeType::DELETE;
        if (event.before_snapshot.has_value()) {
            rec.before_row = parseJsonRow(*event.before_snapshot);
        } else if (event.value.has_value()) {
            rec.before_row = parseJsonRow(*event.value);
        }
    } else {
        // EVENT_PUT — INSERT when there is no previous document, UPDATE otherwise.
        if (event.before_snapshot.has_value()) {
            rec.type       = themisdb::analytics::ChangeType::UPDATE;
            rec.before_row = parseJsonRow(*event.before_snapshot);
        } else {
            rec.type = themisdb::analytics::ChangeType::INSERT;
        }
        if (event.after_snapshot.has_value()) {
            rec.after_row = parseJsonRow(*event.after_snapshot);
        } else if (event.value.has_value()) {
            rec.after_row = parseJsonRow(*event.value);
        }
    }

    return rec;
}

} // namespace cdc
} // namespace themis
