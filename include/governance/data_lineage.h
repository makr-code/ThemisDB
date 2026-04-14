/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            data_lineage.h                                     ║
  Version:         0.0.8                                              ║
  Last Modified:   2026-04-14 06:51:47                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     183                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • c9b77cb477  2026-02-25  feat(governance): implement AI/ML model governance with t... ║
    • 2baf9f8604  2026-02-25  fix(governance): code audit fixes for data_lineage.h ║
    • 5a3c435795  2026-02-25  feat(governance): implement data lineage tracking for gov... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <atomic>
#include <cstdint>
#include <nlohmann/json.hpp>

namespace themis {
namespace utils {
    class AuditLogger;
}

namespace governance {

/// Types of operations that generate a lineage event
enum class LineageEventType {
    INGESTION,        ///< Data first entered the system
    ENRICHMENT,       ///< Data was augmented with additional attributes
    ANONYMIZATION,    ///< PII/PHI was anonymized or pseudonymized
    TRANSFORMATION,   ///< Schema or structural change
    QUERY,            ///< Data was accessed via a read query
    EXPORT,           ///< Data was exported outside the system
    DELETION,         ///< Data (or a version of it) was deleted
    MODEL_TRAINING    ///< Data was used as training data for an AI/ML model
};

/// Convert LineageEventType to a string label (used in metrics and JSON)
std::string lineageEventTypeToString(LineageEventType type);

/// A single step in the lifecycle of a governed dataset
struct LineageEvent {
    std::string event_id;           ///< Unique event identifier (UUID or sequence)
    std::string dataset_id;         ///< The governed dataset this event belongs to
    LineageEventType event_type{LineageEventType::INGESTION};  ///< Kind of operation
    int64_t timestamp_ms{0};           ///< Unix epoch time in milliseconds

    std::string performed_by;       ///< User or service that triggered the operation
    std::string operation;          ///< Free-form operation description
    std::string input_schema;       ///< Schema / shape of input data (optional)
    std::string output_schema;      ///< Schema / shape of output data (optional)

    /// Parent event ID that this event was derived from (empty = root event)
    std::string parent_event_id;

    /// Arbitrary metadata (classification, compliance flags, tool names, etc.)
    nlohmann::json metadata;

    /// Serialize to JSON
    nlohmann::json toJson() const;
};

/// Full lineage record for a single governed dataset
struct LineageRecord {
    std::string dataset_id;
    std::vector<LineageEvent> events;  ///< Ordered chronologically (oldest first)

    /// Serialize to JSON — includes all events and derived fields
    nlohmann::json toJson() const;
};

/**
 * @brief Tracks the data lineage of governed datasets.
 *
 * DataLineageTracker is the single point of recording and querying lineage events
 * for every governed dataset in the system. It is designed to be thread-safe and
 * append-only: existing events are never modified or deleted.
 *
 * Integration points
 * ------------------
 * - AuditLogger: every recorded event is also written to the audit trail so that
 *   the immutable audit log always reflects the full lineage history.
 * - MetricsCollector: a Prometheus counter `governance_lineage_events_total`
 *   (label: `event_type`) is incremented for each recorded event.
 *
 * Design constraints (from FUTURE_ENHANCEMENTS.md)
 * -------------------------------------------------
 * - Append-only: recordEvent() may never modify or delete an existing entry.
 * - All public methods are thread-safe.
 */
class DataLineageTracker {
public:
    DataLineageTracker() = default;

    /// Attach an audit logger; each recorded event will be forwarded to it
    void setAuditLogger(std::shared_ptr<themis::utils::AuditLogger> logger);

    /**
     * @brief Record a new lineage event for a governed dataset.
     *
     * The event is appended to the in-memory store and, if an audit logger is
     * configured, forwarded to the audit trail.
     *
     * @param event  Fully populated LineageEvent. event.event_id must be unique;
     *               if empty a monotonic sequence ID is assigned automatically.
     */
    void recordEvent(LineageEvent event);

    /**
     * @brief Return all lineage events for a dataset, ordered by timestamp.
     *
     * @param dataset_id  The governed dataset identifier.
     * @return            LineageRecord with all events; empty record if unknown.
     */
    LineageRecord getLineage(const std::string& dataset_id) const;

    /**
     * @brief Return the upstream (ancestor) chain for a given event.
     *
     * Follows parent_event_id links upward until the root is reached.
     *
     * @param event_id  Starting event.
     * @return          Events ordered from root to the given event (inclusive).
     */
    std::vector<LineageEvent> getUpstreamLineage(const std::string& event_id) const;

    /**
     * @brief Return all downstream events derived from a given event.
     *
     * Returns every event whose parent_event_id equals event_id, transitively.
     *
     * @param event_id  Starting event.
     * @return          Events ordered by timestamp (oldest first).
     */
    std::vector<LineageEvent> getDownstreamLineage(const std::string& event_id) const;

    /**
     * @brief Serialize the full lineage record for a dataset to JSON.
     *
     * @param dataset_id  The governed dataset identifier.
     * @return            JSON object conforming to the LineageRecord schema.
     */
    nlohmann::json exportLineageAsJson(const std::string& dataset_id) const;

    /// Return the total number of events recorded across all datasets
    size_t totalEventCount() const;

private:
    mutable std::mutex mutex_;

    /// Primary store: dataset_id → ordered list of events
    std::unordered_map<std::string, std::vector<LineageEvent>> lineage_store_;

    /// Secondary index: event_id → pointer into lineage_store_ (for O(1) parent lookup)
    std::unordered_map<std::string, LineageEvent> event_index_;

    std::shared_ptr<themis::utils::AuditLogger> audit_logger_;
    std::atomic<uint64_t> next_event_seq_{1};

    /// Assign a unique event_id if the caller left it empty
    std::string assignEventId();
};

} // namespace governance
} // namespace themis
