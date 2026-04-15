/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            data_lineage.cpp                                   ║
  Version:         0.0.14                                             ║
  Last Modified:   2026-04-15 18:07:51                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     222                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "governance/data_lineage.h"
#include "utils/audit_logger.h"
#include "utils/logger.h"
#include "observability/metrics_collector.h"

#include <algorithm>
#include <chrono>
#include <sstream>

namespace themis {
namespace governance {

// ─── Helpers ────────────────────────────────────────────────────────────────

std::string lineageEventTypeToString(LineageEventType type) {
    switch (type) {
        case LineageEventType::INGESTION:       return "INGESTION";
        case LineageEventType::ENRICHMENT:      return "ENRICHMENT";
        case LineageEventType::ANONYMIZATION:   return "ANONYMIZATION";
        case LineageEventType::TRANSFORMATION:  return "TRANSFORMATION";
        case LineageEventType::QUERY:           return "QUERY";
        case LineageEventType::EXPORT:          return "EXPORT";
        case LineageEventType::DELETION:        return "DELETION";
        case LineageEventType::MODEL_TRAINING:  return "MODEL_TRAINING";
    }
    return "UNKNOWN";
}

// ─── LineageEvent ────────────────────────────────────────────────────────────

nlohmann::json LineageEvent::toJson() const {
    nlohmann::json j;
    j["event_id"]       = event_id;
    j["dataset_id"]     = dataset_id;
    j["event_type"]     = lineageEventTypeToString(event_type);
    j["timestamp_ms"]   = timestamp_ms;
    j["performed_by"]   = performed_by;
    j["operation"]      = operation;
    if (!input_schema.empty())   j["input_schema"]  = input_schema;
    if (!output_schema.empty())  j["output_schema"] = output_schema;
    if (!parent_event_id.empty()) j["parent_event_id"] = parent_event_id;
    if (!metadata.is_null() && !metadata.empty()) j["metadata"] = metadata;
    return j;
}

// ─── LineageRecord ───────────────────────────────────────────────────────────

nlohmann::json LineageRecord::toJson() const {
    nlohmann::json j;
    j["dataset_id"] = dataset_id;
    j["event_count"] = events.size();
    nlohmann::json events_arr = nlohmann::json::array();
    for (const auto& e : events) {
        events_arr.push_back(e.toJson());
    }
    j["events"] = std::move(events_arr);
    return j;
}

// ─── DataLineageTracker ──────────────────────────────────────────────────────

void DataLineageTracker::setAuditLogger(std::shared_ptr<themis::utils::AuditLogger> logger) {
    std::lock_guard<std::mutex> lock(mutex_);
    audit_logger_ = std::move(logger);
}

std::string DataLineageTracker::assignEventId() {
    // Generates a simple monotonic ID; callers may supply their own UUID.
    uint64_t seq = next_event_seq_.fetch_add(1, std::memory_order_relaxed);
    std::ostringstream oss;
    oss << "lineage-" << seq;
    return oss.str();
}

void DataLineageTracker::recordEvent(LineageEvent event) {
    // Assign a timestamp if the caller left it at zero
    if (event.timestamp_ms == 0) {
        event.timestamp_ms = static_cast<int64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count());
    }

    // Assign an event_id if the caller left it empty
    if (event.event_id.empty()) {
        event.event_id = assignEventId();
    }

    const std::string event_type_str = lineageEventTypeToString(event.event_type);

    std::shared_ptr<themis::utils::AuditLogger> audit_log;
    {
        std::lock_guard<std::mutex> lock(mutex_);

        // Append to primary store (chronological order is maintained because
        // recordEvent() is the only mutation path and timestamps increase)
        lineage_store_[event.dataset_id].push_back(event);

        // Maintain secondary index for O(1) parent/child traversal
        event_index_[event.event_id] = event;

        audit_log = audit_logger_;
    }

    THEMIS_INFO("DataLineageTracker: recorded {} event '{}' for dataset '{}'",
        event_type_str, event.event_id, event.dataset_id);

    // Emit Prometheus counter
    observability::MetricsCollector::getInstance().addCounter(
        "governance_lineage_events_total", 1, {{"event_type", event_type_str}});

    // Forward to audit trail (append-only, outside the mutex to avoid deadlock)
    if (audit_log) {
        nlohmann::json audit_entry = {
            {"event_type",      "data_lineage"},
            {"lineage_event",   event.toJson()},
            {"timestamp",       event.timestamp_ms}
        };
        audit_log->logEvent(audit_entry);
    }
}

LineageRecord DataLineageTracker::getLineage(const std::string& dataset_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    LineageRecord record;
    record.dataset_id = dataset_id;
    auto it = lineage_store_.find(dataset_id);
    if (it != lineage_store_.end()) {
        record.events = it->second;
        // Ensure chronological order
        std::sort(record.events.begin(), record.events.end(),
            [](const LineageEvent& a, const LineageEvent& b) {
                return a.timestamp_ms < b.timestamp_ms;
            });
    }
    return record;
}

std::vector<LineageEvent> DataLineageTracker::getUpstreamLineage(
        const std::string& event_id) const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<LineageEvent> chain;
    std::string current_id = event_id;

    // Walk the parent chain; guard against cycles with a depth limit
    constexpr int kMaxDepth = 1024;
    for (int depth = 0; depth < kMaxDepth; ++depth) {
        auto it = event_index_.find(current_id);
        if (it == event_index_.end()) break;
        chain.push_back(it->second);
        if (it->second.parent_event_id.empty()) break;
        current_id = it->second.parent_event_id;
    }

    // Reverse so the result is root → requested event
    std::reverse(chain.begin(), chain.end());
    return chain;
}

std::vector<LineageEvent> DataLineageTracker::getDownstreamLineage(
        const std::string& event_id) const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<LineageEvent> result;

    // BFS over the event index to collect all transitively derived events
    std::vector<std::string> frontier{event_id};
    while (!frontier.empty()) {
        std::vector<std::string> next_frontier;
        for (const auto& [eid, ev] : event_index_) {
            if (ev.parent_event_id.empty()) continue;
            for (const auto& parent : frontier) {
                if (ev.parent_event_id == parent) {
                    result.push_back(ev);
                    next_frontier.push_back(eid);
                    break;
                }
            }
        }
        frontier = std::move(next_frontier);
    }

    std::sort(result.begin(), result.end(),
        [](const LineageEvent& a, const LineageEvent& b) {
            return a.timestamp_ms < b.timestamp_ms;
        });
    return result;
}

nlohmann::json DataLineageTracker::exportLineageAsJson(const std::string& dataset_id) const {
    return getLineage(dataset_id).toJson();
}

size_t DataLineageTracker::totalEventCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return event_index_.size();
}

} // namespace governance
} // namespace themis
