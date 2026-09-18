/**
 * @file data_lineage.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

/**
 * @brief Lineage Event Type To String.
 * @param[in] type Input parameter.
 * @return Return value.
 */
std::string lineageEventTypeToString(LineageEventType type);

struct LineageEvent {
    std::string event_id;           ///< Unique event identifier (UUID or sequence)
    std::string dataset_id;         ///< The governed dataset this event belongs to
    LineageEventType event_type{LineageEventType::INGESTION};  ///< Kind of operation
    int64_t timestamp_ms{0};           ///< Unix epoch time in milliseconds

    std::string performed_by;       ///< User or service that triggered the operation
    std::string operation;          ///< Free-form operation description
    std::string input_schema;       ///< Schema / shape of input data (optional)
    std::string output_schema;      ///< Schema / shape of output data (optional)

    std::string parent_event_id;

    nlohmann::json metadata;

    /**
     * @brief To Json.
     * @return Return value.
     */
    nlohmann::json toJson() const;
};

struct LineageRecord {
    std::string dataset_id;
    std::vector<LineageEvent> events;  ///< Ordered chronologically (oldest first)

    /**
     * @brief To Json.
     * @return Return value.
     */
    nlohmann::json toJson() const;
};

// ─── Phase 2C: Lineage Backpressure (Circuit Breaker & Size Limits) ──────────

enum class CircuitBreakerState : int32_t {
    CLOSED = 0,      ///< Normal operation, forwarding to audit
    OPEN = 1,        ///< Audit failing, recording locally only
    HALF_OPEN = 2,   ///< Attempting recovery, one request in flight
};

enum class LineageError : int32_t {
    kSuccess                = 7360,  ///< Operation succeeded
    kAuditLoggerFailure     = 7361,  ///< Audit logger encountered an error
    kSizeLimitExceeded      = 7362,  ///< Dataset or total size limit exceeded
    kMemoryPressure         = 7363,  ///< System memory pressure detected
    kCircuitBreakerOpen     = 7364,  ///< Circuit breaker is open, audit unavailable
    kEventSequenceViolation = 7365,  ///< Event ordering or consistency violation
};

struct LineageRecordResult {
    LineageError error = LineageError::kSuccess;
    std::string error_message;
    int32_t event_count = 0;
    int64_t generated_at_ms = 0;
    
    bool isSuccess() const { return error == LineageError::kSuccess; }
    
    /**
     * @brief Get Error Name.
     * @return Return value.
     */
    std::string getErrorName() const;
};

struct LineageStatistics {
    size_t total_events = 0;
    size_t total_datasets = 0;
    CircuitBreakerState circuit_breaker_state = CircuitBreakerState::CLOSED;
    int32_t last_error_code = 0;
    int64_t timestamp_ms = 0;
};

class DataLineageTracker {
public:
    DataLineageTracker() = default;

    /**
     * @brief Set Audit Logger.
     * @param[in] logger Input parameter.
     */
    void setAuditLogger(std::shared_ptr<themis::utils::AuditLogger> logger);

    /**
     * @brief Record Event.
     * @param[in] event Input parameter.
     * @return Return value.
     */
    LineageRecordResult recordEvent(LineageEvent event);

    /**
     * @brief Get Lineage.
     * @param[in] dataset_id Identifier of the dataset.
     * @return Return value.
     */
    LineageRecord getLineage(const std::string& dataset_id) const;

    /**
     * @brief Get Upstream Lineage.
     * @param[in] event_id Identifier of the event.
     * @return Return value.
     */
    std::vector<LineageEvent> getUpstreamLineage(const std::string& event_id) const;

    /**
     * @brief Get Downstream Lineage.
     * @param[in] event_id Identifier of the event.
     * @return Return value.
     */
    std::vector<LineageEvent> getDownstreamLineage(const std::string& event_id) const;

    /**
     * @brief Export Lineage As Json.
     * @param[in] dataset_id Identifier of the dataset.
     * @return Return value.
     */
    nlohmann::json exportLineageAsJson(const std::string& dataset_id) const;

    /**
     * @brief Total Event Count.
     * @return Return value.
     */
    size_t totalEventCount() const;

    /**
     * @brief ─── Phase 2C: Backpressure Configuration ──────────────────────────────
     * @param[in] limit Input parameter.
     * @details Implements setMaxEventsPerDataset without additional internal calls.
     */

    void setMaxEventsPerDataset(size_t limit) { max_events_per_dataset_ = limit; }

    /**
     * @brief Set Max Total Events.
     * @param[in] limit Input parameter.
     * @details Implements setMaxTotalEvents without additional internal calls.
     */
    void setMaxTotalEvents(size_t limit) { max_total_events_ = limit; }

    /**
     * @brief Set Circuit Breaker Threshold.
     * @param[in] failures Input parameter.
     * @details Implements setCircuitBreakerThreshold without additional internal calls.
     */
    void setCircuitBreakerThreshold(int32_t failures) { cb_failure_threshold_ = failures; }

    /**
     * @brief Set Circuit Breaker Recovery Window Ms.
     * @param[in] ms Input parameter.
     * @details Implements setCircuitBreakerRecoveryWindowMs without additional internal calls.
     */
    void setCircuitBreakerRecoveryWindowMs(int64_t ms) { cb_recovery_window_ms_ = ms; }

    /**
     * @brief ─── Phase 2C: Circuit Breaker & Statistics ────────────────────────────
     * @return Return value.
     */

    CircuitBreakerState getCircuitBreakerState() const;

    /**
     * @brief Return access control statistics.
     * @return Access control statistics.
     */
    LineageStatistics getStatistics() const;

    /**
     * @brief Prune Old Events.
     * @param[in] dataset_id Identifier of the dataset.
     * @param[in] keep_count Input parameter.
     * @return Return value.
     */
    LineageRecordResult pruneOldEvents(const std::string& dataset_id, int32_t keep_count);

    /**
     * @brief ─── Phase 2C: Internal Circuit Breaker Management ────────────────────
     */

    void recordAuditSuccess();

    /**
     * @brief Record Audit Failure.
     */
    void recordAuditFailure();

private:
    mutable std::mutex mutex_;

    std::unordered_map<std::string, std::vector<LineageEvent>> lineage_store_;

    std::unordered_map<std::string, LineageEvent> event_index_;

    std::shared_ptr<themis::utils::AuditLogger> audit_logger_;
    std::atomic<uint64_t> next_event_seq_{1};

    /**
     * @brief Assign Event Id.
     * @return Return value.
     */
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
    
    LineageRecordResult checkAndEnforceSizeLimits(const std::string& dataset_id = {});
    
    /**
     * @brief Update Circuit Breaker State.
     */
    void updateCircuitBreakerState();
};

} // namespace governance
} // namespace themis
