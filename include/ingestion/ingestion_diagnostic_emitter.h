/**
 * @file ingestion_diagnostic_emitter.h
 * @brief Diagnostic emitter for operator-visible incident categorization and troubleshooting.
 *
 * Phase 3 (Error Handling & Edge Cases) — Operator Diagnostics
 *
 * Provides:
 * - High-cardinality error categorization
 * - Operator-visible incident taxonomy
 * - Diagnostic hints and remediation suggestions
 * - Distributed incident tracking
 * - Runbook linkage
 *
 * @see src/ingestion/ROADMAP.md — Phase 3 item
 */

#pragma once

#include "ingestion/ingestion_error_contract.h"

#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace themis {
namespace ingestion {

// ============================================================================
// Incident categorization and severity
// ============================================================================

/**
 * @brief Severity level for diagnostic incidents.
 */
enum class DiagnosticSeverity {
    INFO,        ///< Informational (normal operation)
    WARNING,     ///< Warning (degraded but operational)
    ALERT,       ///< Alert (operational but concerning)
    CRITICAL,    ///< Critical (service impact)
    EMERGENCY    ///< Emergency (complete failure)
};

/**
 * @brief Incident category for high-cardinality error grouping.
 *
 * Allows grouping of related errors for pattern analysis and alerting.
 */
enum class IncidentCategory {
    // Connectivity issues
    NETWORK_CONNECTIVITY,    ///< DNS, connection refused, timeout
    API_RATE_LIMITING,       ///< HTTP 429, rate limit headers
    API_AUTHENTICATION,      ///< HTTP 401, 403, expired tokens
    API_SERVICE_DEGRADATION, ///< HTTP 5xx, service unavailable
    NETWORK_TIMEOUT,         ///< Read/write timeouts

    // Data validation issues
    SCHEMA_VIOLATION,        ///< Invalid schema, type mismatch
    SEMANTIC_VIOLATION,      ///< Business logic violation
    DUPLICATE_DETECTION,     ///< Duplicate key, idempotency violation
    TYPE_COERCION_FAILURE,   ///< Cannot convert value to required type

    // Quality issues
    QUALITY_THRESHOLD_MISS,  ///< Document below quality threshold
    QUALITY_INFERENCE_FAILURE, ///< ML model inference failed
    QUALITY_TIMEOUT,         ///< Quality check timed out

    // Resource issues
    MEMORY_PRESSURE,         ///< Memory usage high
    QUEUE_SATURATION,        ///< Queue full, backpressure
    CONNECTION_POOL_EXHAUSTED, ///< All connections in use
    DISK_SPACE_LOW,          ///< Disk full or near full

    // System issues
    PERMISSION_DENIED,       ///< File/database access denied
    CONFIGURATION_ERROR,     ///< Invalid configuration
    INTERNAL_ERROR,          ///< Unexpected system error

    // Workflow issues
    WORKFLOW_STEP_FAILURE,   ///< Workflow step execution failed
    WORKFLOW_TIMEOUT,        ///< Workflow exceeded timeout
    ADAPTER_INCOMPATIBILITY  ///< Adapter not compatible with plugin
};

/**
 * @brief Detailed diagnostic information about an incident.
 */
struct DiagnosticIncident {
    IncidentCategory category;
    DiagnosticSeverity severity = DiagnosticSeverity::INFO;
    std::string incident_id;     ///< Unique incident identifier
    std::string title;           ///< Short description
    std::string description;     ///< Detailed description

    ErrorContext error_context;

    // Incident tracking
    std::chrono::system_clock::time_point detected_at;
    std::vector<std::string> affected_items;  ///< Items/documents affected
    std::size_t occurrence_count = 0;         ///< How many times seen recently

    // Remediation hints
    std::string remediation_hint;  ///< Suggested operator action
    std::string runbook_link;      ///< Link to operational runbook

    // Observable metrics
    std::map<std::string, double> metrics;    ///< Key metrics (e.g., "latency_ms", "queue_depth")

    /**
     * @brief Serialize incident to JSON.
     */
    std::string toJson() const;
};

// ============================================================================
// Diagnostic listener callback
// ============================================================================

/**
 * @brief Callback invoked when a diagnostic incident is detected.
 *
 * Implementations can log to metrics systems, alert operators,
 * update dashboards, etc.
 */
using DiagnosticListener = std::function<void(const DiagnosticIncident&)>;

// ============================================================================
// Diagnostic emitter with listener pattern
// ============================================================================

/**
 * @brief Emits diagnostic incidents for operator visibility.
 *
 * Uses listener pattern (observer) for decoupled notification of
 * diagnostic events. Multiple listeners can be registered.
 *
 * Thread-safe for multi-threaded ingestion pipelines.
 *
 * Example usage:
 * @code
 * DiagnosticEmitter emitter;
 *
 * // Register listener for all incidents
 * emitter.onIncident([](const DiagnosticIncident& incident) {
 *     std::cout << "Incident: " << incident.title << std::endl;
 *     std::cout << "Severity: " << static_cast<int>(incident.severity) << std::endl;
 *     std::cout << "Remedy: " << incident.remediation_hint << std::endl;
 *     std::cout << "Runbook: " << incident.runbook_link << std::endl;
 * });
 *
 * // Emit incidents
 * DiagnosticIncident incident;
 * incident.category = IncidentCategory::NETWORK_CONNECTIVITY;
 * incident.severity = DiagnosticSeverity::ALERT;
 * incident.title = "API connection timeout";
 * incident.remediation_hint = "Check network connectivity and API health";
 * incident.runbook_link = "https://wiki/ops/api-timeout";
 * emitter.emit(incident);
 * @endcode
 */
class DiagnosticEmitter {
public:
    /**
     * @brief Construct a diagnostic emitter.
     */
    DiagnosticEmitter() = default;

    ~DiagnosticEmitter() = default;

    // Delete copy/move
    DiagnosticEmitter(const DiagnosticEmitter&) = delete;
    DiagnosticEmitter& operator=(const DiagnosticEmitter&) = delete;

    // ── Listener registration ───────────────────────────────────────────────

    /**
     * @brief Register a listener for all incident notifications.
     *
     * Multiple listeners can be registered and will all be invoked
     * when an incident is emitted.
     *
     * @param listener Callback function to invoke on incidents
     * @return Listener ID (can be used to unregister)
     */
    int registerListener(DiagnosticListener listener) {
        std::lock_guard<std::mutex> lock(mutex_);
        const int id = next_listener_id_++;
        listeners_[id] = listener;
        return id;
    }

    /**
     * @brief Unregister a previously registered listener.
     * @param listener_id ID returned by registerListener()
     * @return true if the listener was found and removed
     */
    bool unregisterListener(int listener_id) {
        std::lock_guard<std::mutex> lock(mutex_);
        return listeners_.erase(listener_id) > 0;
    }

    /**
     * @brief Clear all registered listeners.
     */
    void clearListeners() {
        std::lock_guard<std::mutex> lock(mutex_);
        listeners_.clear();
    }

    /**
     * @brief Get the number of registered listeners.
     */
    std::size_t getListenerCount() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return listeners_.size();
    }

    // ── Incident emission ───────────────────────────────────────────────────

    /**
     * @brief Emit a diagnostic incident.
     *
     * Invokes all registered listeners with the incident details.
     * Thread-safe; can be called from multiple threads.
     *
     * @param incident Diagnostic incident to emit
     */
    void emit(DiagnosticIncident incident) {
        std::lock_guard<std::mutex> lock(mutex_);

        // Generate unique incident ID if not already set
        if (incident.incident_id.empty()) {
            incident.incident_id = generateIncidentId();
        }

        // Set timestamp if not already set
        if (incident.detected_at == std::chrono::system_clock::time_point{}) {
            incident.detected_at = std::chrono::system_clock::now();
        }

        // Notify all listeners
        for (const auto& kv : listeners_) {
            try {
                kv.second(incident);
            } catch (const std::exception& e) {
                // Listener threw; catch and continue with other listeners
                // In production, this would be logged
            }
        }

        // Track incident frequency
        auto key = static_cast<int>(incident.category);
        incident_counts_[key]++;
    }

    /**
     * @brief Emit an incident based on an error context.
     *
     * Converts an ErrorContext into a DiagnosticIncident and emits it.
     *
     * @param error_context Error context to convert
     * @return Generated incident ID
     */
    std::string emitFromError(const ErrorContext& error_context) {
        DiagnosticIncident incident;
        incident.error_context = error_context;

        // Categorize based on error code
        incident.category = categorizeError(error_context.error_code);
        incident.severity = calculateSeverity(incident.category);
        incident.title = getErrorMessage(error_context.error_code);
        incident.description = error_context.error_message;
        incident.remediation_hint = getRemediationHint(incident.category);
        incident.runbook_link = getRunbookLink(incident.category);

        emit(incident);
        return incident.incident_id;
    }

    // ── Incident history and statistics ────────────────────────────────────

    /**
     * @brief Get statistics for incidents in a category.
     * @param category Incident category
     * @return Number of incidents in this category since emitter creation
     */
    std::size_t getIncidentCount(IncidentCategory category) const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto key = static_cast<int>(category);
        auto it = incident_counts_.find(key);
        return (it != incident_counts_.end()) ? it->second : 0;
    }

    /**
     * @brief Get total incident count across all categories.
     */
    std::size_t getTotalIncidentCount() const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::size_t total = 0;
        for (const auto& kv : incident_counts_) {
            total += kv.second;
        }
        return total;
    }

    /**
     * @brief Reset all incident counters.
     */
    void resetCounters() {
        std::lock_guard<std::mutex> lock(mutex_);
        incident_counts_.clear();
    }

private:
    mutable std::mutex mutex_;
    int next_listener_id_ = 0;
    std::map<int, DiagnosticListener> listeners_;
    std::map<int, std::size_t> incident_counts_;

    /**
     * @brief Generate a unique incident identifier.
     */
    std::string generateIncidentId() {
        static std::atomic<uint64_t> counter(0);
        return "ING-" + std::to_string(counter++);
    }

    /**
     * @brief Categorize an error code into an incident category.
     */
    IncidentCategory categorizeError(IngestionErrorCode code) {
        // Map error codes to incident categories
        switch (code) {
            case IngestionErrorCode::HTTP_TIMEOUT:
            case IngestionErrorCode::HTTP_CONNECTION_REFUSED:
            case IngestionErrorCode::HTTP_DNS_RESOLUTION_FAILED:
                return IncidentCategory::NETWORK_CONNECTIVITY;

            case IngestionErrorCode::HTTP_RATE_LIMITED:
                return IncidentCategory::API_RATE_LIMITING;

            case IngestionErrorCode::HTTP_UNAUTHORIZED:
            case IngestionErrorCode::AUTH_FAILED:
            case IngestionErrorCode::AUTH_EXPIRED:
                return IncidentCategory::API_AUTHENTICATION;

            case IngestionErrorCode::HTTP_SERVER_ERROR:
            case IngestionErrorCode::SOURCE_UNAVAILABLE:
                return IncidentCategory::API_SERVICE_DEGRADATION;

            case IngestionErrorCode::SCHEMA_INVALID:
            case IngestionErrorCode::VALIDATION_FAILED:
                return IncidentCategory::SCHEMA_VIOLATION;

            case IngestionErrorCode::QUALITY_THRESHOLD_FAILED:
                return IncidentCategory::QUALITY_THRESHOLD_MISS;

            case IngestionErrorCode::QUALITY_CHECK_TIMEOUT:
                return IncidentCategory::QUALITY_TIMEOUT;

            case IngestionErrorCode::MEMORY_EXHAUSTION:
                return IncidentCategory::MEMORY_PRESSURE;

            case IngestionErrorCode::QUEUE_SATURATED:
            case IngestionErrorCode::BUFFER_FULL:
                return IncidentCategory::QUEUE_SATURATION;

            default:
                return IncidentCategory::INTERNAL_ERROR;
        }
    }

    /**
     * @brief Calculate severity level for an incident category.
     */
    DiagnosticSeverity calculateSeverity(IncidentCategory category) {
        switch (category) {
            case IncidentCategory::MEMORY_PRESSURE:
            case IncidentCategory::QUEUE_SATURATION:
            case IncidentCategory::CONNECTION_POOL_EXHAUSTED:
                return DiagnosticSeverity::ALERT;

            case IncidentCategory::API_SERVICE_DEGRADATION:
            case IncidentCategory::DISK_SPACE_LOW:
                return DiagnosticSeverity::CRITICAL;

            case IncidentCategory::NETWORK_CONNECTIVITY:
            case IncidentCategory::API_RATE_LIMITING:
                return DiagnosticSeverity::WARNING;

            default:
                return DiagnosticSeverity::INFO;
        }
    }

    /**
     * @brief Get remediation hint for an incident category.
     */
    std::string getRemediationHint(IncidentCategory category) {
        switch (category) {
            case IncidentCategory::NETWORK_CONNECTIVITY:
                return "Check network connectivity and DNS resolution";

            case IncidentCategory::API_RATE_LIMITING:
                return "Reduce ingestion rate or request API rate limit increase";

            case IncidentCategory::API_AUTHENTICATION:
                return "Verify API credentials and token expiration";

            case IncidentCategory::API_SERVICE_DEGRADATION:
                return "Check API service status and retry with backoff";

            case IncidentCategory::MEMORY_PRESSURE:
                return "Reduce concurrent ingestion tasks or increase system memory";

            case IncidentCategory::QUEUE_SATURATION:
                return "Increase queue capacity or reduce input rate via backpressure";

            case IncidentCategory::SCHEMA_VIOLATION:
                return "Validate source data format against schema; check transformation logic";

            case IncidentCategory::QUALITY_THRESHOLD_MISS:
                return "Improve source data quality or adjust quality thresholds";

            default:
                return "Consult logs and runbooks for details";
        }
    }

    /**
     * @brief Get runbook link for an incident category.
     */
    std::string getRunbookLink(IncidentCategory category) {
        switch (category) {
            case IncidentCategory::NETWORK_CONNECTIVITY:
                return "https://wiki/ops/network-troubleshooting";

            case IncidentCategory::API_RATE_LIMITING:
                return "https://wiki/ops/api-rate-limits";

            case IncidentCategory::API_AUTHENTICATION:
                return "https://wiki/ops/auth-troubleshooting";

            case IncidentCategory::MEMORY_PRESSURE:
                return "https://wiki/ops/memory-management";

            case IncidentCategory::QUEUE_SATURATION:
                return "https://wiki/ops/backpressure-management";

            default:
                return "";
        }
    }
};

}  // namespace ingestion
}  // namespace themis

#endif  // THEMISDB_INCLUDE_INGESTION_DIAGNOSTIC_EMITTER_H
