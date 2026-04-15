/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            task_audit_manager.h                               ║
  Version:         0.0.45                                             ║
  Last Modified:   2026-04-15 07:09:01                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     270                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • e963d4e9ba  2026-04-14  fix(concurrency): eliminate deadlocks, blocking I/O under... ║
    • 71d99c4f28  2026-04-14  fix(concurrency): eliminate deadlocks, blocking I/O under... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file task_audit_manager.h
 * @brief Central manager for task scheduler audit logging and anomaly detection
 * 
 * Integrates:
 * - Audit event logging with AuditLogger
 * - Anomaly detection with TaskAnomalyDetector
 * - Security event reporting
 * - Query and export interfaces
 * - GDPR-compliant data handling
 */

#pragma once

#include "scheduler/task_audit_event.h"
#include "scheduler/task_anomaly_detector.h"
#include "utils/audit_logger.h"
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <vector>
#include <chrono>
#include <functional>

namespace themis {
namespace scheduler {

/**
 * @brief Configuration for task audit manager
 */
struct TaskAuditConfig {
    // Audit logging
    bool enable_audit_logging = true;
    std::string audit_log_path = "data/logs/task_audit.jsonl";
    bool enable_gdpr_mode = false;              // Enable GDPR-compliant masking
    
    // Security event logging
    bool enable_security_logging = true;
    std::string security_log_path = "data/logs/task_security.jsonl";
    
    // Anomaly detection
    bool enable_anomaly_detection = true;
    AnomalyDetectorConfig anomaly_config;
    
    // Export/query
    size_t max_query_results = 1000;           // Maximum results per query
    bool enable_export_api = true;
    
    // Callbacks (optional)
    std::function<void(const TaskAuditEvent&)> on_audit_event;
    std::function<void(const TaskSecurityEvent&)> on_security_event;
    std::function<void(const std::string& task_id, const AnomalyMetrics&)> on_anomaly_detected;
};

/**
 * @brief Query parameters for audit event search
 */
struct AuditQueryParams {
    // Time range
    std::optional<std::chrono::system_clock::time_point> start_time;
    std::optional<std::chrono::system_clock::time_point> end_time;
    
    // Filters
    std::optional<std::string> task_id;
    std::optional<std::string> user_id;
    std::optional<TaskEventType> event_type;
    std::optional<std::string> trigger_type;
    std::optional<bool> success;               // Filter by success/failure
    std::optional<bool> anomalous_only;        // Only return anomalous events
    
    // Pagination
    size_t offset = 0;
    size_t limit = 100;
    
    // Sorting
    enum class SortBy {
        TIMESTAMP_ASC,
        TIMESTAMP_DESC,
        DURATION_ASC,
        DURATION_DESC,
        ANOMALY_SCORE_DESC
    } sort_by = SortBy::TIMESTAMP_DESC;
};

/**
 * @brief Export format options
 */
enum class ExportFormat {
    JSON,           // Standard JSON array
    JSONL,          // JSON Lines format
    CEF,            // Common Event Format
    CSV             // Comma-separated values
};

/**
 * @brief Central manager for task scheduler auditing
 * 
 * Responsibilities:
 * - Log all task execution events with structured data
 * - Detect and report anomalies in task behavior
 * - Report security violations separately
 * - Provide query interface for audit events
 * - Support multiple export formats (JSON, CEF, CSV, etc.)
 * - Handle GDPR-compliant data masking
 * - Integrate with existing AuditLogger infrastructure
 */
class TaskAuditManager {
public:
    /**
     * @brief Construct audit manager
     * @param audit_logger Shared audit logger instance (for tamper-evident logging)
     * @param config Audit manager configuration
     */
    explicit TaskAuditManager(
        std::shared_ptr<utils::AuditLogger> audit_logger,
        const TaskAuditConfig& config = TaskAuditConfig()
    );
    
    /**
     * @brief Log a task audit event
     * @param event Audit event to log
     * @return Anomaly metrics (if anomaly detection enabled)
     */
    AnomalyMetrics logAuditEvent(const TaskAuditEvent& event);
    
    /**
     * @brief Log a security violation event
     * @param event Security event to log
     */
    void logSecurityEvent(const TaskSecurityEvent& event);
    
    /**
     * @brief Query audit events with filters
     * @param params Query parameters
     * @return Vector of matching audit events
     */
    std::vector<TaskAuditEvent> queryAuditEvents(const AuditQueryParams& params) const;
    
    /**
     * @brief Query security events with filters
     * @param params Query parameters (similar structure)
     * @return Vector of matching security events
     */
    std::vector<TaskSecurityEvent> querySecurityEvents(const AuditQueryParams& params) const;
    
    /**
     * @brief Export audit events to file
     * @param params Query parameters for filtering
     * @param output_path Output file path
     * @param format Export format
     * @return Number of events exported
     */
    size_t exportAuditEvents(const AuditQueryParams& params,
                            const std::string& output_path,
                            ExportFormat format) const;
    
    /**
     * @brief Get task execution statistics
     * @param task_id Task identifier
     * @return Task statistics (if available)
     */
    std::optional<TaskStatistics> getTaskStatistics(const std::string& task_id) const;
    
    /**
     * @brief Get all task statistics
     * @return Map of task_id -> statistics
     */
    std::map<std::string, TaskStatistics> getAllStatistics() const;
    
    /**
     * @brief Check if a task has anomalous behavior
     * @param task_id Task identifier
     * @return true if recent executions show anomalies
     */
    bool hasAnomalies(const std::string& task_id) const;
    
    /**
     * @brief Reset statistics for a task
     * @param task_id Task identifier
     */
    void resetTaskStatistics(const std::string& task_id);
    
    /**
     * @brief Get current configuration
     */
    TaskAuditConfig getConfig() const;
    
    /**
     * @brief Update configuration
     */
    void updateConfig(const TaskAuditConfig& config);
    
    /**
     * @brief Flush all logs to disk
     */
    void flush();
    
    /**
     * @brief Get audit log file path
     */
    std::string getAuditLogPath() const { return config_.audit_log_path; }
    
    /**
     * @brief Get security log file path
     */
    std::string getSecurityLogPath() const { return config_.security_log_path; }

    /**
     * @brief Export anomaly detector statistics to JSON for persistence
     * @return JSON object with all task statistics and configuration
     */
    nlohmann::json exportAnomalyStatistics() const;

    /**
     * @brief Import anomaly detector statistics from JSON (restored after restart)
     * @param data Previously exported statistics JSON
     */
    void importAnomalyStatistics(const nlohmann::json& data);

private:
    std::shared_ptr<utils::AuditLogger> audit_logger_;
    TaskAuditConfig config_;
    std::unique_ptr<TaskAnomalyDetector> anomaly_detector_;
    
    mutable std::shared_mutex mutex_;
    
    // In-memory cache for recent events (for querying)
    std::deque<TaskAuditEvent> recent_audit_events_;
    std::deque<TaskSecurityEvent> recent_security_events_;
    static constexpr size_t MAX_CACHE_SIZE = 10000;
    
    // Helper methods
    void cacheAuditEvent(const TaskAuditEvent& event);
    void cacheSecurityEvent(const TaskSecurityEvent& event);
    void writeToAuditLog(const TaskAuditEvent& event);
    void writeToSecurityLog(const TaskSecurityEvent& event);
    bool matchesQuery(const TaskAuditEvent& event, const AuditQueryParams& params) const;
    std::vector<TaskAuditEvent> loadEventsFromFile(const std::string& file_path,
                                                   const AuditQueryParams& params) const;
    std::vector<TaskSecurityEvent> loadSecurityEventsFromFile(const std::string& file_path,
                                                              const AuditQueryParams& params) const;
};

} // namespace scheduler
} // namespace themis
