/**
 * @file data_lineage.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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

// ─── Phase 2C: Lineage Backpressure (Circuit Breaker & Size Limits) ──────────

/// Circuit breaker states for audit logger resilience
enum class CircuitBreakerState : int32_t {
    CLOSED = 0,      ///< Normal operation, forwarding to audit
    OPEN = 1,        ///< Audit failing, recording locally only
    HALF_OPEN = 2,   ///< Attempting recovery, one request in flight
};

/// Error codes for lineage backpressure (7360-7365 range)
enum class LineageError : int32_t {
    kSuccess                = 7360,  ///< Operation succeeded
    kAuditLoggerFailure     = 7361,  ///< Audit logger encountered an error
    kSizeLimitExceeded      = 7362,  ///< Dataset or total size limit exceeded
    kMemoryPressure         = 7363,  ///< System memory pressure detected
    kCircuitBreakerOpen     = 7364,  ///< Circuit breaker is open, audit unavailable
    kEventSequenceViolation = 7365,  ///< Event ordering or consistency violation
};

/// Structured result for lineage operations with error semantics
struct LineageRecordResult {
    LineageError error = LineageError::kSuccess;
    std::string error_message;
    int32_t event_count = 0;
    int64_t generated_at_ms = 0;
    
    /// @brief Check if operation succeeded
    bool isSuccess() const { return error == LineageError::kSuccess; }
    
    /// @brief Get human-readable error name
    std::string getErrorName() const;
};

/// Statistics snapshot for lineage tracking
struct LineageStatistics {
    size_t total_events = 0;
    size_t total_datasets = 0;
    CircuitBreakerState circuit_breaker_state = CircuitBreakerState::CLOSED;
    int32_t last_error_code = 0;
    int64_t timestamp_ms = 0;
};

/**
 * @brief Tracks the data lineage of governed datasets.
 *
 * DataLineageTracker is the single point of recording and querying lineage events
 * for every governed dataset in the system. It is designed to be thread-safe and
 * append-only: existing events are never modified or deleted.
 *
 * Phase 2C Enhancements (Backpressure & Resilience)
 * --------------------------------------------------
 * - Circuit breaker: automatically disengages audit forwarding when failures occur,
 *   recovers after configurable timeout, tracks consecutive failures
 * - Size limits: per-dataset and global limits with FIFO eviction of oldest events
 * - Error semantics: all operations return structured LineageRecordResult instead of void
 * - Statistics: real-time snapshot of tracking state for monitoring
 *
 * Integration points
 * ------------------
 * - AuditLogger: every recorded event is also written to the audit trail so that
 *   the immutable audit log always reflects the full lineage history.
 * - MetricsCollector: a Prometheus counter `governance_lineage_events_total`
 *   (label: `event_type`) is incremented for each recorded event.
 * - DiagnosticAggregator: circuit breaker state changes and errors emitted as diagnostics
 *
 * Design constraints (from FUTURE_ENHANCEMENTS.md)
 * -------------------------------------------------
 * - Append-only: recordEvent() may never modify or delete an existing entry.
 * - All public methods are thread-safe.
 * - No exceptions; all errors returned via LineageRecordResult.
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
     * configured, forwarded to the audit trail. With circuit breaker active,
     * events are still recorded locally but not forwarded to audit.
     *
     * @param event  Fully populated LineageEvent. event.event_id must be unique;
     *               if empty a monotonic sequence ID is assigned automatically.
     * @return       LineageRecordResult with error code and diagnostics
     *
     * @note Emits diagnostics on circuit breaker state changes, size limit violations
     */
    LineageRecordResult recordEvent(LineageEvent event);

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

    // ─── Phase 2C: Backpressure Configuration ──────────────────────────────

    /**
     * @brief Set the maximum events allowed per dataset.
     * 
     * When exceeded, oldest events (FIFO) are automatically pruned.
     * Default: 10000.
     * 
     * @param limit Maximum events per dataset
     */
    void setMaxEventsPerDataset(size_t limit) { max_events_per_dataset_ = limit; }

    /**
     * @brief Set the maximum total events across all datasets.
     * 
     * When exceeded, oldest events across all datasets are pruned.
     * Default: 1000000.
     * 
     * @param limit Maximum total events
     */
    void setMaxTotalEvents(size_t limit) { max_total_events_ = limit; }

    /**
     * @brief Set the number of consecutive failures before circuit breaker opens.
     * 
     * Default: 3.
     * 
     * @param failures Failure count threshold
     */
    void setCircuitBreakerThreshold(int32_t failures) { cb_failure_threshold_ = failures; }

    /**
     * @brief Set the milliseconds to wait before attempting recovery from OPEN state.
     * 
     * After this duration, circuit breaker transitions OPEN → HALF_OPEN.
     * Default: 30000 (30 seconds).
     * 
     * @param ms Recovery window in milliseconds
     */
    void setCircuitBreakerRecoveryWindowMs(int64_t ms) { cb_recovery_window_ms_ = ms; }

    // ─── Phase 2C: Circuit Breaker & Statistics ────────────────────────────

    /**
     * @brief Get the current circuit breaker state.
     * 
     * @return Current CircuitBreakerState (CLOSED, OPEN, or HALF_OPEN)
     */
    CircuitBreakerState getCircuitBreakerState() const;

    /**
     * @brief Get current statistics snapshot.
     * 
     * Thread-safe capture of total events, total datasets, circuit breaker state,
     * and last error code.
     * 
     * @return LineageStatistics snapshot
     */
    LineageStatistics getStatistics() const;

    /**
     * @brief Manually prune old events from a dataset, keeping only the most recent N.
     * 
     * Removes oldest (FIFO) events until only keep_count events remain.
     * This triggers garbage collection and records diagnostics.
     * 
     * @param dataset_id  Dataset to prune
     * @param keep_count  Number of most recent events to retain
     * @return            LineageRecordResult with operation status
     */
    LineageRecordResult pruneOldEvents(const std::string& dataset_id, int32_t keep_count);

    // ─── Phase 2C: Internal Circuit Breaker Management ────────────────────

    /**
     * @brief Record a successful audit logger operation.
     * 
     * Decrements failure counter; if HALF_OPEN, transitions to CLOSED.
     */
    void recordAuditSuccess();

    /**
     * @brief Record a failed audit logger operation.
     * 
     * Increments failure counter; if threshold exceeded, transitions CLOSED → OPEN.
     */
    void recordAuditFailure();

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

    // ─── Phase 2C: Circuit Breaker State ──────────────────────────────────
    
    CircuitBreakerState circuit_breaker_state_ = CircuitBreakerState::CLOSED;
    std::atomic<int32_t> consecutive_failures_{0};
    int32_t cb_failure_threshold_ = 3;
    int64_t cb_recovery_window_ms_ = 30000;
    int64_t last_open_time_ms_ = 0;
    mutable std::mutex cb_mutex_;

    // Size limits
    size_t max_events_per_dataset_ = 10000;
    size_t max_total_events_ = 1000000;

    // Statistics
    std::atomic<int32_t> last_error_code_{static_cast<int32_t>(LineageError::kSuccess)};

    // ─── Phase 2C: Helper Methods ──────────────────────────────────────────
    
    /// Check if event recording would violate size limits; emit diagnostics and trim if needed
    LineageRecordResult checkAndEnforceSizeLimits();
    
    /// Update circuit breaker state based on current conditions
    void updateCircuitBreakerState();
};

} // namespace governance
} // namespace themis
