/**
 * @file task_audit_event.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include <string>
#include <chrono>
#include <optional>
#include <nlohmann/json.hpp>

namespace themis {
namespace scheduler {

/**
 * @brief Type of audit event for task execution
 */
enum class TaskEventType {
    TASK_REGISTERED,           // New task registered
    TASK_REGISTRATION_REJECTED,// Task registration rejected (conflicting ID)
    TASK_UNREGISTERED,         // Task removed
    TASK_ENABLED,              // Task enabled
    TASK_DISABLED,             // Task disabled
    TASK_UPDATED,              // Task configuration updated
    TASK_STARTED,              // Task execution started
    TASK_COMPLETED,            // Task execution completed successfully
    TASK_FAILED,               // Task execution failed
    TASK_TIMEOUT,              // Task execution timed out
    TASK_RETRY,                // Task retry attempt
    TASK_QUEUED,               // Task queued for execution
    TASK_DEQUEUED,             // Task dequeued and starting execution
    MANUAL_EXECUTION,          // Manual task execution triggered
    CRON_TRIGGERED,            // Cron schedule triggered task
    CDC_TRIGGERED,             // CDC event triggered task
    INTERVAL_TRIGGERED,        // Interval timer triggered task
    WEBHOOK_TRIGGERED          // Webhook event triggered task (future)
};

/**
 * @brief Type of security violation event
 */
enum class TaskSecurityEventType {
    RATE_LIMIT_EXCEEDED,       // Task execution rate limit exceeded
    RESOURCE_LIMIT_EXCEEDED,   // Resource limit exceeded (CPU, memory, etc.)
    CRON_INJECTION_DETECTED,   // Potential cron injection attack detected
    AQL_INJECTION_DETECTED,    // Potential AQL injection attack detected
    UNAUTHORIZED_ACCESS,       // Unauthorized task access attempt
    INVALID_CONFIGURATION,     // Invalid or suspicious task configuration
    EXCESSIVE_FAILURES,        // Abnormal failure rate detected
    ANOMALY_DETECTED,          // Behavioral anomaly detected
    PRIVILEGE_ESCALATION,      // Privilege escalation attempt
    SUSPICIOUS_PATTERN         // Suspicious execution pattern
};

/**
 * @brief Resource usage metrics for task execution
 */
struct TaskResourceUsage {
    double cpu_time_ms = 0.0;             // CPU time used (milliseconds)
    uint64_t memory_bytes = 0;             // Memory used (bytes)
    uint64_t io_read_bytes = 0;            // I/O read (bytes)
    uint64_t io_write_bytes = 0;           // I/O write (bytes)
    double execution_time_ms = 0.0;        // Total execution time (milliseconds)
    uint64_t result_rows = 0;              // Number of result rows
    uint64_t affected_rows = 0;            // Number of affected rows
    
    nlohmann::json toJson() const {
        return {
            {"cpu_time_ms", cpu_time_ms},
            {"memory_bytes", memory_bytes},
            {"io_read_bytes", io_read_bytes},
            {"io_write_bytes", io_write_bytes},
            {"execution_time_ms", execution_time_ms},
            {"result_rows", result_rows},
            {"affected_rows", affected_rows}
        };
    }
};

/**
 * @brief Anomaly detection metrics for task execution
 */
struct AnomalyMetrics {
    double frequency_score = 0.0;          // 0-1: frequency deviation from baseline
    double pattern_score = 0.0;            // 0-1: pattern deviation from baseline
    double resource_score = 0.0;           // 0-1: resource usage deviation
    double failure_rate_score = 0.0;       // 0-1: failure rate deviation
    double overall_score = 0.0;            // 0-1: combined anomaly score
    bool is_anomalous = false;             // true if overall score exceeds threshold
    std::string description;               // Human-readable anomaly description
    
    nlohmann::json toJson() const {
        return {
            {"frequency_score", frequency_score},
            {"pattern_score", pattern_score},
            {"resource_score", resource_score},
            {"failure_rate_score", failure_rate_score},
            {"overall_score", overall_score},
            {"is_anomalous", is_anomalous},
            {"description", description}
        };
    }
};

/**
 * @brief Comprehensive audit event for task execution
 * 
 * Captures all relevant information for compliance and security monitoring:
 * - Unique identification (UUID)
 * - Temporal data (timestamp, duration)
 * - Task context (ID, type, trigger)
 * - Actor information (user, IP, session)
 * - Result data (success/failure, error details)
 * - Resource metrics
 * - Anomaly detection scores
 */
struct TaskAuditEvent {
    // Unique identification
    std::string uuid;                                    // Unique event ID (UUID v4)
    
    // Temporal data
    std::chrono::system_clock::time_point timestamp;     // Event timestamp (UTC)
    double duration_ms = 0.0;                            // Event duration (for execution events)
    
    // Task identification
    std::string task_id;                                 // Task identifier
    std::string task_name;                               // Task name
    std::string task_description;                        // Task description
    
    // Event classification
    TaskEventType event_type;                            // Type of event
    std::string trigger_type;                            // "CRON", "CDC", "INTERVAL", "MANUAL", "WEBHOOK"
    
    // Actor information (GDPR-sensitive)
    std::string user_id;                                 // User/service account (may be masked)
    std::string ip_address;                              // Client IP address (may be masked)
    std::string session_id;                              // Session identifier
    std::optional<std::string> tenant_id;                // Multi-tenant identifier
    
    // Result information
    bool success = false;                                // Execution success/failure
    std::optional<std::string> error_message;            // Error message (if failed)
    std::optional<std::string> error_type;               // Error type/category
    std::optional<int> retry_count;                      // Retry attempt number
    
    // Resource usage
    TaskResourceUsage resource_usage;                    // Resource consumption metrics
    
    // Anomaly detection
    AnomalyMetrics anomaly_metrics;                      // Anomaly detection scores
    
    // Additional context
    nlohmann::json metadata;                             // Additional event-specific data
    
    /**
     * @brief Convert audit event to JSON format
     * @param gdpr_mode If true, applies GDPR-compliant masking to sensitive fields
     * @return JSON representation of the event
     */
    nlohmann::json toJson(bool gdpr_mode = false) const;
    
    /**
     * @brief Convert to CEF (Common Event Format) for SIEM integration
     * @return CEF-formatted string
     */
    std::string toCEF() const;
    
    /**
     * @brief Convert to Splunk HEC format
     * @return Splunk-compatible JSON
     */
    nlohmann::json toSplunkHEC() const;
    
    /**
     * @brief Convert to Elastic Common Schema (ECS) format
     * @return ECS-compatible JSON
     */
    nlohmann::json toElasticECS() const;
};

/**
 * @brief Security violation event for task execution
 * 
 * Specialized event for security-related incidents and policy violations
 */
struct TaskSecurityEvent {
    // Unique identification
    std::string uuid;                                    // Unique event ID
    
    // Temporal data
    std::chrono::system_clock::time_point timestamp;     // Event timestamp (UTC)
    
    // Task identification
    std::string task_id;                                 // Task identifier (if applicable)
    std::string task_name;                               // Task name (if applicable)
    
    // Security event classification
    TaskSecurityEventType event_type;                    // Type of security event
    std::string severity;                                // "LOW", "MEDIUM", "HIGH", "CRITICAL"
    
    // Actor information
    std::string user_id;                                 // User/service account
    std::string ip_address;                              // Client IP address
    std::string session_id;                              // Session identifier
    
    // Violation details
    std::string violation_type;                          // Type of violation
    std::string description;                             // Human-readable description
    nlohmann::json details;                              // Detailed violation data
    
    // Context
    std::optional<std::string> policy_id;                // Policy that was violated
    std::optional<std::string> rule_id;                  // Specific rule violated
    bool blocked = false;                                // Whether action was blocked
    
    // Response
    std::string action_taken;                            // Action taken in response
    
    /**
     * @brief Convert security event to JSON format
     * @return JSON representation of the event
     */
    nlohmann::json toJson() const;
    
    /**
     * @brief Convert to CEF format with security extensions
     * @return CEF-formatted string
     */
    std::string toCEF() const;
};

/**
 * @brief Generate UUID v4 for event identification
 * @return UUID string in standard format (xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx)
 */
std::string generateUUID();

/**
 * @brief Mask sensitive data for GDPR compliance
 * @param data Original data
 * @param mask_type Type of masking ("full", "partial", "hash")
 * @return Masked data
 */
std::string maskSensitiveData(const std::string& data, const std::string& mask_type = "partial");

/**
 * @brief Convert event type to string
 */
std::string taskEventTypeToString(TaskEventType type);

/**
 * @brief Convert security event type to string
 */
std::string taskSecurityEventTypeToString(TaskSecurityEventType type);

/**
 * @brief Parse event type from string (reverse of taskEventTypeToString)
 * @param s String representation of the event type
 * @return Corresponding TaskEventType, or TASK_COMPLETED as default for unknown values
 */
TaskEventType taskEventTypeFromString(const std::string& s);

} // namespace scheduler
} // namespace themis

