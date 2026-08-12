/**
 * @file data_lineage.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "governance/data_lineage.h"

#include <algorithm>
#include <chrono>
#include <sstream>

#include "governance/governance_diagnostics.h"
#include "observability/metrics_collector.h"
#include "utils/audit_logger.h"
#include "utils/logger.h"

namespace themis {
namespace governance {

// ─── Helpers ────────────────────────────────────────────────────────────────

std::string lineageEventTypeToString(LineageEventType type) {
    switch (type) {
        case LineageEventType::INGESTION:
            return "INGESTION";
        case LineageEventType::ENRICHMENT:
            return "ENRICHMENT";
        case LineageEventType::ANONYMIZATION:
            return "ANONYMIZATION";
        case LineageEventType::TRANSFORMATION:
            return "TRANSFORMATION";
        case LineageEventType::QUERY:
            return "QUERY";
        case LineageEventType::EXPORT:
            return "EXPORT";
        case LineageEventType::DELETION:
            return "DELETION";
        case LineageEventType::MODEL_TRAINING:
            return "MODEL_TRAINING";
    }
    return "UNKNOWN";
}

// ─── LineageEvent ────────────────────────────────────────────────────────────

nlohmann::json LineageEvent::toJson() const {
    nlohmann::json j;
    j["event_id"]     = event_id;
    j["dataset_id"]   = dataset_id;
    j["event_type"]   = lineageEventTypeToString(event_type);
    j["timestamp_ms"] = timestamp_ms;
    j["performed_by"] = performed_by;
    j["operation"]    = operation;
    if (!input_schema.empty()) {
        j["input_schema"] = input_schema;
    }
    if (!output_schema.empty()) {
        j["output_schema"] = output_schema;
    }
    if (!parent_event_id.empty()) {
        j["parent_event_id"] = parent_event_id;
    }
    if (!metadata.is_null() && !metadata.empty()) {
        j["metadata"] = metadata;
    }
    return j;
}

// ─── LineageRecord ───────────────────────────────────────────────────────────

nlohmann::json LineageRecord::toJson() const {
    nlohmann::json j;
    j["dataset_id"]           = dataset_id;
    j["event_count"]          = events.size();
    nlohmann::json events_arr = nlohmann::json::array();
    for (const auto &e : events) {
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

LineageRecord DataLineageTracker::getLineage(const std::string &dataset_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    LineageRecord record;
    record.dataset_id = dataset_id;
    auto it           = lineage_store_.find(dataset_id);
    if (it != lineage_store_.end()) {
        record.events = it->second;
        // Ensure chronological order
        std::sort(record.events.begin(), record.events.end(),
                  [](const LineageEvent &a, const LineageEvent &b) { return a.timestamp_ms < b.timestamp_ms; });
    }
    return record;
}

std::vector<LineageEvent> DataLineageTracker::getUpstreamLineage(const std::string &event_id) const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<LineageEvent> chain;
    std::string current_id = event_id;

    // Walk the parent chain; guard against cycles with a depth limit
    constexpr int kMaxDepth = 1024;
    for (int depth = 0; depth < kMaxDepth; ++depth) {
        auto it = event_index_.find(current_id);
        if (it == event_index_.end()) {
            break;
        }
        chain.push_back(it->second);
        if (it->second.parent_event_id.empty()) {
            break;
        }
        current_id = it->second.parent_event_id;
    }

    // Reverse so the result is root → requested event
    std::reverse(chain.begin(), chain.end());
    return chain;
}

std::vector<LineageEvent> DataLineageTracker::getDownstreamLineage(const std::string &event_id) const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<LineageEvent> result;

    // BFS over the event index to collect all transitively derived events
    std::vector<std::string> frontier{event_id};
    while (!frontier.empty()) {
        std::vector<std::string> next_frontier;
        for (const auto &[eid, ev] : event_index_) {
            if (ev.parent_event_id.empty()) {
                continue;
            }
            for (const auto &parent : frontier) {
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
              [](const LineageEvent &a, const LineageEvent &b) { return a.timestamp_ms < b.timestamp_ms; });
    return result;
}

nlohmann::json DataLineageTracker::exportLineageAsJson(const std::string &dataset_id) const {
    return getLineage(dataset_id).toJson();
}

size_t DataLineageTracker::totalEventCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return event_index_.size();
}

// ─── Phase 2C: Lineage Backpressure Implementation ────────────────────────────

std::string LineageRecordResult::getErrorName() const {
    switch (error) {
        case LineageError::kSuccess:
            return "SUCCESS";
        case LineageError::kAuditLoggerFailure:
            return "AUDIT_LOGGER_FAILURE";
        case LineageError::kSizeLimitExceeded:
            return "SIZE_LIMIT_EXCEEDED";
        case LineageError::kMemoryPressure:
            return "MEMORY_PRESSURE";
        case LineageError::kCircuitBreakerOpen:
            return "CIRCUIT_BREAKER_OPEN";
        case LineageError::kEventSequenceViolation:
            return "EVENT_SEQUENCE_VIOLATION";
    }
    return "UNKNOWN";
}

CircuitBreakerState DataLineageTracker::getCircuitBreakerState() const {
    std::lock_guard<std::mutex> lock(cb_mutex_);
    return circuit_breaker_state_;
}

LineageStatistics DataLineageTracker::getStatistics() const {
    LineageStatistics stats;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stats.total_events = event_index_.size();
        stats.total_datasets = lineage_store_.size();
    }
    {
        std::lock_guard<std::mutex> lock(cb_mutex_);
        stats.circuit_breaker_state = circuit_breaker_state_;
    }
    stats.last_error_code = last_error_code_.load(std::memory_order_relaxed);
    stats.timestamp_ms = static_cast<int64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
    return stats;
}

void DataLineageTracker::recordAuditSuccess() {
    std::lock_guard<std::mutex> lock(cb_mutex_);
    consecutive_failures_.store(0, std::memory_order_relaxed);
    if (circuit_breaker_state_ == CircuitBreakerState::HALF_OPEN) {
        circuit_breaker_state_ = CircuitBreakerState::CLOSED;
        THEMIS_INFO("DataLineageTracker: circuit breaker HALF_OPEN → CLOSED (recovery successful)");
    }
}

void DataLineageTracker::recordAuditFailure() {
    std::lock_guard<std::mutex> lock(cb_mutex_);
    int32_t failures = consecutive_failures_.fetch_add(1, std::memory_order_relaxed) + 1;
    
    if (circuit_breaker_state_ == CircuitBreakerState::CLOSED && failures >= cb_failure_threshold_) {
        circuit_breaker_state_ = CircuitBreakerState::OPEN;
        last_open_time_ms_ = static_cast<int64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count());
        
        THEMIS_WARN("DataLineageTracker: circuit breaker CLOSED → OPEN after {} failures", failures);
        
        // Emit diagnostic
        auto& agg = getGlobalDiagnosticAggregator();
        GovernanceDiagnostic diag;
        diag.code = GovDiagnosticCode::kLineageCircuitBreakerOpen;
        diag.component = "lineage_tracker";
        diag.description = "Circuit breaker opened due to audit logger failures";
        diag.timestamp_ms = last_open_time_ms_;
        diag.context["consecutive_failures"] = std::to_string(failures);
        diag.context["threshold"] = std::to_string(cb_failure_threshold_);
        diag.remediation_steps = {
            "Check audit logger service health and connectivity",
            "Verify audit logger has sufficient capacity",
            "Review recent error logs for root cause",
            "Consider increasing circuit breaker recovery timeout if needed"
        };
        agg.recordDiagnostic(diag);
    }
}

void DataLineageTracker::updateCircuitBreakerState() {
    std::lock_guard<std::mutex> lock(cb_mutex_);
    
    if (circuit_breaker_state_ == CircuitBreakerState::OPEN) {
        int64_t now = static_cast<int64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count());
        
        if (now - last_open_time_ms_ >= cb_recovery_window_ms_) {
            circuit_breaker_state_ = CircuitBreakerState::HALF_OPEN;
            THEMIS_INFO("DataLineageTracker: circuit breaker OPEN → HALF_OPEN (recovery window elapsed)");
        }
    }
}

LineageRecordResult DataLineageTracker::checkAndEnforceSizeLimits() {
    // This is called under mutex_, no additional locking needed
    size_t total_events = event_index_.size();
    size_t total_datasets = lineage_store_.size();
    
    // Check total event limit
    if (total_events >= max_total_events_) {
        last_error_code_.store(static_cast<int32_t>(LineageError::kSizeLimitExceeded), 
                               std::memory_order_relaxed);
        LineageRecordResult result;
        result.error = LineageError::kSizeLimitExceeded;
        result.error_message = "Total events limit exceeded; FIFO eviction in progress";
        result.event_count = static_cast<int32_t>(total_events);
        
        // FIFO eviction: find oldest event across all datasets
        if (!lineage_store_.empty() && !lineage_store_.begin()->second.empty()) {
            auto oldest_it = lineage_store_.begin();
            int64_t oldest_time = oldest_it->second[0].timestamp_ms;
            std::string oldest_dataset = oldest_it->first;
            
            for (auto& [ds_id, events] : lineage_store_) {
                if (!events.empty() && events[0].timestamp_ms < oldest_time) {
                    oldest_time = events[0].timestamp_ms;
                    oldest_dataset = ds_id;
                }
            }
            
            auto& events = lineage_store_[oldest_dataset];
            if (!events.empty()) {
                const auto& removed = events[0];
                event_index_.erase(removed.event_id);
                events.erase(events.begin());
                THEMIS_DEBUG("DataLineageTracker: FIFO evicted oldest event '{}' from dataset '{}'",
                           removed.event_id, oldest_dataset);
            }
        }
        
        // Emit diagnostic
        auto& agg = getGlobalDiagnosticAggregator();
        GovernanceDiagnostic diag;
        diag.code = GovDiagnosticCode::kLineageSizeLimitExceeded;
        diag.component = "lineage_tracker";
        diag.description = "Total lineage events exceeded limit; FIFO eviction active";
        diag.timestamp_ms = static_cast<int64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count());
        diag.context["total_events"] = std::to_string(total_events);
        diag.context["limit"] = std::to_string(max_total_events_);
        diag.context["total_datasets"] = std::to_string(total_datasets);
        agg.recordDiagnostic(diag);
        
        return result;
    }
    
    // Check per-dataset limit (not enforced at record time to allow initial growth,
    // but noted for monitoring)
    for (const auto& [ds_id, events] : lineage_store_) {
        if (events.size() >= max_events_per_dataset_) {
            last_error_code_.store(static_cast<int32_t>(LineageError::kSizeLimitExceeded), 
                                   std::memory_order_relaxed);
            
            // Remove oldest event from this dataset
            if (!events.empty()) {
                const auto& removed = events[0];
                event_index_.erase(removed.event_id);
                lineage_store_[ds_id].erase(lineage_store_[ds_id].begin());
                THEMIS_DEBUG("DataLineageTracker: FIFO evicted oldest event '{}' from dataset '{}'",
                           removed.event_id, ds_id);
            }
        }
    }
    
    LineageRecordResult result;
    result.error = LineageError::kSuccess;
    result.event_count = static_cast<int32_t>(total_events);
    return result;
}

LineageRecordResult DataLineageTracker::recordEvent(LineageEvent event) {
    // Validate event before any recording
    if (event.dataset_id.empty()) {
        last_error_code_.store(static_cast<int32_t>(LineageError::kEventSequenceViolation), 
                               std::memory_order_relaxed);
        LineageRecordResult result;
        result.error = LineageError::kEventSequenceViolation;
        result.error_message = "dataset_id must not be empty";
        return result;
    }
    
    // Check and potentially recover circuit breaker
    updateCircuitBreakerState();
    
    // Assign timestamps and IDs
    if (event.timestamp_ms == 0) {
        event.timestamp_ms = static_cast<int64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count());
    }
    if (event.event_id.empty()) {
        event.event_id = assignEventId();
    }
    
    const std::string event_type_str = lineageEventTypeToString(event.event_type);
    
    std::shared_ptr<themis::utils::AuditLogger> audit_log;
    LineageRecordResult result;
    result.error = LineageError::kSuccess;
    result.event_count = 1;
    result.generated_at_ms = event.timestamp_ms;
    
    CircuitBreakerState cb_state;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Enforce size limits before recording
        auto size_check = checkAndEnforceSizeLimits();
        if (size_check.error != LineageError::kSuccess) {
            result.error = size_check.error;
            result.error_message = size_check.error_message;
            return result;
        }
        
        // Record event (append-only, never modify)
        lineage_store_[event.dataset_id].push_back(event);
        event_index_[event.event_id] = event;
        result.event_count = static_cast<int32_t>(event_index_.size());
        
        audit_log = audit_logger_;
        
        std::lock_guard<std::mutex> cb_lock(cb_mutex_);
        cb_state = circuit_breaker_state_;
    }
    
    THEMIS_DEBUG("DataLineageTracker: recorded {} event '{}' for dataset '{}'", 
                event_type_str, event.event_id, event.dataset_id);
    
    // Emit Prometheus counter
    observability::MetricsCollector::getInstance().addCounter(
        "governance_lineage_events_total", 1, {{"event_type", event_type_str}});
    
    // Forward to audit trail if circuit breaker is not OPEN
    if (audit_log && cb_state != CircuitBreakerState::OPEN) {
        try {
            nlohmann::json audit_entry = {
                {"event_type", "data_lineage"},
                {"lineage_event", event.toJson()},
                {"timestamp", event.timestamp_ms}
            };
            audit_log->logEvent(audit_entry);
            recordAuditSuccess();
        } catch (const std::exception& e) {
            THEMIS_WARN("DataLineageTracker: audit logger failed: {}", e.what());
            recordAuditFailure();
            last_error_code_.store(static_cast<int32_t>(LineageError::kAuditLoggerFailure), 
                                   std::memory_order_relaxed);
            result.error = LineageError::kAuditLoggerFailure;
            result.error_message = std::string("Audit logger failed: ") + e.what();
        }
    } else if (cb_state == CircuitBreakerState::OPEN) {
        // Record event locally but circuit breaker is open
        result.error = LineageError::kCircuitBreakerOpen;
        result.error_message = "Event recorded locally; audit logger unavailable (circuit breaker open)";
    }
    
    return result;
}

LineageRecordResult DataLineageTracker::pruneOldEvents(const std::string& dataset_id, 
                                                       int32_t keep_count) {
    LineageRecordResult result;
    result.error = LineageError::kSuccess;
    
    if (keep_count < 0) {
        result.error = LineageError::kEventSequenceViolation;
        result.error_message = "keep_count must be non-negative";
        return result;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = lineage_store_.find(dataset_id);
    if (it == lineage_store_.end()) {
        result.error = LineageError::kSuccess;  // No-op for unknown dataset
        result.event_count = 0;
        return result;
    }
    
    auto& events = it->second;
    if (events.size() <= static_cast<size_t>(keep_count)) {
        result.event_count = static_cast<int32_t>(events.size());
        return result;  // Nothing to prune
    }
    
    // Remove oldest events, keeping only keep_count most recent
    size_t to_remove = events.size() - keep_count;
    for (size_t i = 0; i < to_remove; ++i) {
        const auto& removed = events[0];
        event_index_.erase(removed.event_id);
        events.erase(events.begin());
    }
    
    result.event_count = static_cast<int32_t>(events.size());
    
    THEMIS_INFO("DataLineageTracker: pruned {} events from dataset '{}', retained {}",
               to_remove, dataset_id, keep_count);
    
    // Emit diagnostic for GC
    auto& agg = getGlobalDiagnosticAggregator();
    GovernanceDiagnostic diag;
    diag.code = GovDiagnosticCode::kLineageBackpressure;
    diag.component = "lineage_tracker";
    diag.description = "Manual garbage collection: pruned old events";
    diag.timestamp_ms = static_cast<int64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
    diag.context["dataset_id"] = dataset_id;
    diag.context["removed_count"] = std::to_string(to_remove);
    diag.context["retained_count"] = std::to_string(keep_count);
    agg.recordDiagnostic(diag);
    
    return result;
}

} // namespace governance
} // namespace themis
