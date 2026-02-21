/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            task_audit_manager.cpp                             ║
  Version:         0.0.15                                             ║
  Last Modified:   2026-02-21 17:07:39                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     533                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "scheduler/task_audit_manager.h"
#include "utils/logger.h"
#include <fstream>
#include <algorithm>

namespace themis {
namespace scheduler {

TaskAuditManager::TaskAuditManager(
    std::shared_ptr<utils::AuditLogger> audit_logger,
    const TaskAuditConfig& config)
    : audit_logger_(audit_logger), config_(config) {
    
    if (config_.enable_anomaly_detection) {
        anomaly_detector_ = std::make_unique<TaskAnomalyDetector>(config_.anomaly_config);
    }
    
    THEMIS_INFO("TaskAuditManager initialized (audit={}, security={}, anomaly={})",
                config_.enable_audit_logging,
                config_.enable_security_logging,
                config_.enable_anomaly_detection);
}

AnomalyMetrics TaskAuditManager::logAuditEvent(const TaskAuditEvent& event) {
    AnomalyMetrics anomaly_metrics;
    
    // Perform anomaly detection first (if enabled)
    if (config_.enable_anomaly_detection && anomaly_detector_) {
        anomaly_metrics = anomaly_detector_->recordExecution(event);
        
        // Create a mutable copy to add anomaly metrics
        TaskAuditEvent event_with_anomaly = event;
        event_with_anomaly.anomaly_metrics = anomaly_metrics;
        
        // Trigger callback if anomaly detected
        if (anomaly_metrics.is_anomalous && config_.on_anomaly_detected) {
            config_.on_anomaly_detected(event.task_id, anomaly_metrics);
        }
        
        // Log to audit logger
        if (config_.enable_audit_logging) {
            writeToAuditLog(event_with_anomaly);
            cacheAuditEvent(event_with_anomaly);
        }
        
        // Trigger audit event callback
        if (config_.on_audit_event) {
            config_.on_audit_event(event_with_anomaly);
        }
        
        return anomaly_metrics;
    }
    
    // No anomaly detection - just log the event
    if (config_.enable_audit_logging) {
        writeToAuditLog(event);
        cacheAuditEvent(event);
    }
    
    if (config_.on_audit_event) {
        config_.on_audit_event(event);
    }
    
    return anomaly_metrics;
}

void TaskAuditManager::logSecurityEvent(const TaskSecurityEvent& event) {
    if (!config_.enable_security_logging) {
        return;
    }
    
    writeToSecurityLog(event);
    cacheSecurityEvent(event);
    
    if (config_.on_security_event) {
        config_.on_security_event(event);
    }
    
    THEMIS_WARN("Security event logged: {} - {} (severity={})",
                taskSecurityEventTypeToString(event.event_type),
                event.description,
                event.severity);
}

void TaskAuditManager::writeToAuditLog(const TaskAuditEvent& event) {
    // Write to dedicated task audit log
    std::lock_guard<std::mutex> lock(mutex_);
    
    try {
        std::ofstream ofs(config_.audit_log_path, std::ios::app);
        if (ofs.is_open()) {
            ofs << event.toJson(config_.enable_gdpr_mode).dump() << "\n";
        }
    } catch (const std::exception& e) {
        THEMIS_ERROR("Failed to write audit event to {}: {}", 
                    config_.audit_log_path, e.what());
    }
    
    // Also log to main audit logger (for tamper-evident chain)
    if (audit_logger_) {
        nlohmann::json audit_entry;
        audit_entry["category"] = "TASK_EXECUTION";
        audit_entry["event"] = event.toJson(config_.enable_gdpr_mode);
        audit_logger_->logEvent(audit_entry);
    }
}

void TaskAuditManager::writeToSecurityLog(const TaskSecurityEvent& event) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    try {
        std::ofstream ofs(config_.security_log_path, std::ios::app);
        if (ofs.is_open()) {
            ofs << event.toJson().dump() << "\n";
        }
    } catch (const std::exception& e) {
        THEMIS_ERROR("Failed to write security event to {}: {}",
                    config_.security_log_path, e.what());
    }
    
    // Also log to main audit logger as security event
    if (audit_logger_) {
        audit_logger_->logSecurityEvent(
            utils::SecurityEventType::CUSTOM_EVENT,
            event.user_id,
            event.task_id,
            event.toJson()
        );
    }
}

void TaskAuditManager::cacheAuditEvent(const TaskAuditEvent& event) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    recent_audit_events_.push_back(event);
    
    // Limit cache size
    if (recent_audit_events_.size() > MAX_CACHE_SIZE) {
        recent_audit_events_.pop_front();
    }
}

void TaskAuditManager::cacheSecurityEvent(const TaskSecurityEvent& event) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    recent_security_events_.push_back(event);
    
    if (recent_security_events_.size() > MAX_CACHE_SIZE) {
        recent_security_events_.pop_front();
    }
}

bool TaskAuditManager::matchesQuery(const TaskAuditEvent& event, 
                                    const AuditQueryParams& params) const {
    // Time range filter
    if (params.start_time && event.timestamp < *params.start_time) {
        return false;
    }
    if (params.end_time && event.timestamp > *params.end_time) {
        return false;
    }
    
    // Task ID filter
    if (params.task_id && event.task_id != *params.task_id) {
        return false;
    }
    
    // User ID filter
    if (params.user_id && event.user_id != *params.user_id) {
        return false;
    }
    
    // Event type filter
    if (params.event_type && event.event_type != *params.event_type) {
        return false;
    }
    
    // Trigger type filter
    if (params.trigger_type && event.trigger_type != *params.trigger_type) {
        return false;
    }
    
    // Success filter
    if (params.success && event.success != *params.success) {
        return false;
    }
    
    // Anomalous only filter
    if (params.anomalous_only && *params.anomalous_only && !event.anomaly_metrics.is_anomalous) {
        return false;
    }
    
    return true;
}

std::vector<TaskAuditEvent> TaskAuditManager::queryAuditEvents(const AuditQueryParams& params) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<TaskAuditEvent> results;
    
    // Query from in-memory cache first
    for (const auto& event : recent_audit_events_) {
        if (matchesQuery(event, params)) {
            results.push_back(event);
        }
    }
    
    // If not enough results, load from file
    if (results.size() < params.limit) {
        auto file_results = loadEventsFromFile(config_.audit_log_path, params);
        results.insert(results.end(), file_results.begin(), file_results.end());
    }
    
    // Sort results
    switch (params.sort_by) {
        case AuditQueryParams::SortBy::TIMESTAMP_ASC:
            std::sort(results.begin(), results.end(),
                     [](const auto& a, const auto& b) { return a.timestamp < b.timestamp; });
            break;
        case AuditQueryParams::SortBy::TIMESTAMP_DESC:
            std::sort(results.begin(), results.end(),
                     [](const auto& a, const auto& b) { return a.timestamp > b.timestamp; });
            break;
        case AuditQueryParams::SortBy::DURATION_ASC:
            std::sort(results.begin(), results.end(),
                     [](const auto& a, const auto& b) { return a.duration_ms < b.duration_ms; });
            break;
        case AuditQueryParams::SortBy::DURATION_DESC:
            std::sort(results.begin(), results.end(),
                     [](const auto& a, const auto& b) { return a.duration_ms > b.duration_ms; });
            break;
        case AuditQueryParams::SortBy::ANOMALY_SCORE_DESC:
            std::sort(results.begin(), results.end(),
                     [](const auto& a, const auto& b) { 
                         return a.anomaly_metrics.overall_score > b.anomaly_metrics.overall_score; 
                     });
            break;
    }
    
    // Apply pagination
    if (results.size() > params.offset) {
        results.erase(results.begin(), results.begin() + params.offset);
    } else {
        results.clear();
    }
    
    if (results.size() > params.limit) {
        results.resize(params.limit);
    }
    
    // Respect max query results limit
    if (results.size() > config_.max_query_results) {
        results.resize(config_.max_query_results);
    }
    
    return results;
}

std::vector<TaskAuditEvent> TaskAuditManager::loadEventsFromFile(
    const std::string& file_path,
    const AuditQueryParams& params) const {
    
    std::vector<TaskAuditEvent> results;
    
    try {
        std::ifstream ifs(file_path);
        if (!ifs.is_open()) {
            return results;
        }
        
        std::string line;
        while (std::getline(ifs, line) && results.size() < config_.max_query_results) {
            try {
                auto j = nlohmann::json::parse(line);
                
                // Reconstruct TaskAuditEvent from JSON
                // Note: This is a simplified version. Full implementation would
                // need to parse all fields correctly.
                TaskAuditEvent event;
                event.uuid = j.value("uuid", "");
                event.task_id = j.value("task_id", "");
                event.task_name = j.value("task_name", "");
                
                if (j.contains("timestamp")) {
                    auto ts_ms = j["timestamp"].get<int64_t>();
                    event.timestamp = std::chrono::system_clock::time_point(
                        std::chrono::milliseconds(ts_ms));
                }
                
                event.success = j.value("success", false);
                
                // Check if matches query
                if (matchesQuery(event, params)) {
                    results.push_back(event);
                }
                
            } catch (const std::exception& e) {
                // Skip malformed lines
                continue;
            }
        }
    } catch (const std::exception& e) {
        THEMIS_ERROR("Failed to load events from {}: {}", file_path, e.what());
    }
    
    return results;
}

std::vector<TaskSecurityEvent> TaskAuditManager::querySecurityEvents(const AuditQueryParams& params) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<TaskSecurityEvent> results;
    
    // Query from in-memory cache
    for (const auto& event : recent_security_events_) {
        // Apply filters (simplified version)
        bool matches = true;
        
        if (params.start_time && event.timestamp < *params.start_time) {
            matches = false;
        }
        if (params.end_time && event.timestamp > *params.end_time) {
            matches = false;
        }
        if (params.task_id && event.task_id != *params.task_id) {
            matches = false;
        }
        if (params.user_id && event.user_id != *params.user_id) {
            matches = false;
        }
        
        if (matches) {
            results.push_back(event);
        }
    }
    
    // Apply pagination
    if (results.size() > params.offset) {
        results.erase(results.begin(), results.begin() + params.offset);
    } else {
        results.clear();
    }
    
    if (results.size() > params.limit) {
        results.resize(params.limit);
    }
    
    return results;
}

size_t TaskAuditManager::exportAuditEvents(const AuditQueryParams& params,
                                          const std::string& output_path,
                                          ExportFormat format) const {
    auto events = queryAuditEvents(params);
    
    if (events.empty()) {
        return 0;
    }
    
    try {
        std::ofstream ofs(output_path);
        if (!ofs.is_open()) {
            throw std::runtime_error("Cannot open output file: " + output_path);
        }
        
        switch (format) {
            case ExportFormat::JSON: {
                nlohmann::json j = nlohmann::json::array();
                for (const auto& event : events) {
                    j.push_back(event.toJson(config_.enable_gdpr_mode));
                }
                ofs << j.dump(2);
                break;
            }
            
            case ExportFormat::JSONL: {
                for (const auto& event : events) {
                    ofs << event.toJson(config_.enable_gdpr_mode).dump() << "\n";
                }
                break;
            }
            
            case ExportFormat::CEF: {
                for (const auto& event : events) {
                    ofs << event.toCEF() << "\n";
                }
                break;
            }
            
            case ExportFormat::CSV: {
                // CSV header
                ofs << "UUID,Timestamp,TaskID,TaskName,EventType,TriggerType,UserID,IPAddress,"
                    << "Success,Duration(ms),CPUTime(ms),Memory(bytes),AnomalyScore,ErrorMessage\n";
                
                // CSV rows
                for (const auto& event : events) {
                    auto ts = std::chrono::duration_cast<std::chrono::milliseconds>(
                        event.timestamp.time_since_epoch()).count();
                    
                    ofs << event.uuid << ","
                        << ts << ","
                        << event.task_id << ","
                        << event.task_name << ","
                        << taskEventTypeToString(event.event_type) << ","
                        << event.trigger_type << ","
                        << (config_.enable_gdpr_mode ? maskSensitiveData(event.user_id) : event.user_id) << ","
                        << (config_.enable_gdpr_mode ? maskSensitiveData(event.ip_address) : event.ip_address) << ","
                        << (event.success ? "true" : "false") << ","
                        << event.duration_ms << ","
                        << event.resource_usage.cpu_time_ms << ","
                        << event.resource_usage.memory_bytes << ","
                        << event.anomaly_metrics.overall_score << ","
                        << (event.error_message ? *event.error_message : "") << "\n";
                }
                break;
            }
        }
        
        THEMIS_INFO("Exported {} audit events to {} (format={})",
                   events.size(), output_path, static_cast<int>(format));
        
        return events.size();
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("Failed to export audit events: {}", e.what());
        return 0;
    }
}

std::optional<TaskStatistics> TaskAuditManager::getTaskStatistics(const std::string& task_id) const {
    if (!anomaly_detector_) {
        return std::nullopt;
    }
    
    return anomaly_detector_->getTaskStatistics(task_id);
}

std::map<std::string, TaskStatistics> TaskAuditManager::getAllStatistics() const {
    if (!anomaly_detector_) {
        return {};
    }
    
    return anomaly_detector_->getAllStatistics();
}

bool TaskAuditManager::hasAnomalies(const std::string& task_id) const {
    auto stats = getTaskStatistics(task_id);
    if (!stats) {
        return false;
    }
    
    // Check recent events for anomalies
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& event : recent_audit_events_) {
        if (event.task_id == task_id && event.anomaly_metrics.is_anomalous) {
            return true;
        }
    }
    
    return false;
}

void TaskAuditManager::resetTaskStatistics(const std::string& task_id) {
    if (anomaly_detector_) {
        anomaly_detector_->resetTaskStatistics(task_id);
    }
}

TaskAuditConfig TaskAuditManager::getConfig() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return config_;
}

void TaskAuditManager::updateConfig(const TaskAuditConfig& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    config_ = config;
    
    if (config_.enable_anomaly_detection && !anomaly_detector_) {
        anomaly_detector_ = std::make_unique<TaskAnomalyDetector>(config_.anomaly_config);
    } else if (config_.enable_anomaly_detection && anomaly_detector_) {
        anomaly_detector_->updateConfig(config_.anomaly_config);
    }
}

void TaskAuditManager::flush() {
    if (audit_logger_) {
        audit_logger_->flush();
    }
}

nlohmann::json TaskAuditManager::exportAnomalyStatistics() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (anomaly_detector_) {
        return anomaly_detector_->exportStatistics();
    }
    return nlohmann::json::object();
}

void TaskAuditManager::importAnomalyStatistics(const nlohmann::json& data) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (anomaly_detector_) {
        anomaly_detector_->importStatistics(data);
    }
}

} // namespace scheduler
} // namespace themis
