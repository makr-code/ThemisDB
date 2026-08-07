/**
 * @file task_audit_manager.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=10, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: task_audit_manager.cpp | Version: 0.0.47 | Last Modified: 2026-05-31 12:17:24
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 684
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=12, L=0
 * PR History (last 5): none
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#include "scheduler/task_audit_manager.h"
#include "utils/logger.h"
#include <fstream>
#include <algorithm>
#include <unordered_set>
#include <stdexcept>
#include <openssl/hmac.h>
#include <openssl/sha.h>
#include <iomanip>
#include <sstream>

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
    std::unique_lock<std::shared_mutex> lock(mutex_);
    
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
    std::unique_lock<std::shared_mutex> lock(mutex_);
    
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
        try {
            audit_logger_->logSecurityEvent(
                utils::SecurityEventType::CUSTOM_EVENT,
                event.user_id,
                event.task_id,
                event.toJson()
            );
        } catch (const std::exception& e) {
            THEMIS_ERROR("Failed to log security event to audit logger: {}", e.what());
        }
    }
}

void TaskAuditManager::cacheAuditEvent(const TaskAuditEvent& event) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    
    recent_audit_events_.push_back(event);
    
    // Limit cache size
    if (recent_audit_events_.size() > MAX_CACHE_SIZE) {
        recent_audit_events_.pop_front();
    }
}

void TaskAuditManager::cacheSecurityEvent(const TaskSecurityEvent& event) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    
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
    std::shared_lock<std::shared_mutex> lock(mutex_);
    
    std::vector<TaskAuditEvent> results;
    
    // Query from in-memory cache first
    for (const auto& event : recent_audit_events_) {
        if (matchesQuery(event, params)) {
            results.push_back(event);
        }
    }
    
    // Always load from file to include events that have been evicted from the cache.
    // loadEventsFromFile() skips UUIDs already present in recent_audit_events_ to
    // prevent duplicates.
    auto file_results = loadEventsFromFile(config_.audit_log_path, params);
    results.insert(results.end(), file_results.begin(), file_results.end());
    
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
    
    // Build a set of UUIDs already in the results to avoid duplicates with cache
    std::unordered_set<std::string> seen_uuids;
    for (const auto& cached : recent_audit_events_) {
        seen_uuids.insert(cached.uuid);
    }
    
    // Read at most enough events to satisfy the pagination request, bounded by
    // max_query_results.  Reading only params.offset + params.limit matching
    // events ensures pagination cannot silently truncate when offset is large.
    const size_t read_limit = std::min(params.offset + params.limit, config_.max_query_results);
    
    try {
        std::ifstream ifs(file_path);
        if (!ifs.is_open()) {
            return results;
        }
        
        std::string line;
        while (std::getline(ifs, line) && results.size() < read_limit) {
            if (line.empty()) continue;
            try {
                auto j = nlohmann::json::parse(line);
                
                TaskAuditEvent event;
                event.uuid = j.value("uuid", "");
                
                // Skip events already present in the in-memory cache
                if (!event.uuid.empty() && seen_uuids.count(event.uuid)) {
                    continue;
                }
                
                event.task_id = j.value("task_id", "");
                event.task_name = j.value("task_name", "");
                event.task_description = j.value("task_description", "");
                
                if (j.contains("timestamp")) {
                    auto ts_ms = j["timestamp"].get<int64_t>();
                    event.timestamp = std::chrono::system_clock::time_point(
                        std::chrono::milliseconds(ts_ms));
                }
                
                event.duration_ms = j.value("duration_ms", 0.0);
                event.success = j.value("success", false);
                
                if (j.contains("event_type")) {
                    event.event_type = taskEventTypeFromString(j["event_type"].get<std::string>());
                }
                event.trigger_type = j.value("trigger_type", "");
                
                event.user_id = j.value("user_id", "");
                event.ip_address = j.value("ip_address", "");
                event.session_id = j.value("session_id", "");
                if (j.contains("tenant_id") && !j["tenant_id"].is_null()) {
                    event.tenant_id = j["tenant_id"].get<std::string>();
                }
                
                if (j.contains("error_message") && !j["error_message"].is_null()) {
                    event.error_message = j["error_message"].get<std::string>();
                }
                if (j.contains("error_type") && !j["error_type"].is_null()) {
                    event.error_type = j["error_type"].get<std::string>();
                }
                if (j.contains("retry_count") && !j["retry_count"].is_null()) {
                    event.retry_count = j["retry_count"].get<int>();
                }
                
                if (j.contains("resource_usage")) {
                    const auto& ru = j["resource_usage"];
                    event.resource_usage.cpu_time_ms = ru.value("cpu_time_ms", 0.0);
                    event.resource_usage.memory_bytes = ru.value("memory_bytes", uint64_t{0});
                    event.resource_usage.io_read_bytes = ru.value("io_read_bytes", uint64_t{0});
                    event.resource_usage.io_write_bytes = ru.value("io_write_bytes", uint64_t{0});
                    event.resource_usage.execution_time_ms = ru.value("execution_time_ms", 0.0);
                    event.resource_usage.result_rows = ru.value("result_rows", uint64_t{0});
                    event.resource_usage.affected_rows = ru.value("affected_rows", uint64_t{0});
                }
                
                if (j.contains("anomaly_metrics")) {
                    const auto& am = j["anomaly_metrics"];
                    event.anomaly_metrics.frequency_score = am.value("frequency_score", 0.0);
                    event.anomaly_metrics.pattern_score = am.value("pattern_score", 0.0);
                    event.anomaly_metrics.resource_score = am.value("resource_score", 0.0);
                    event.anomaly_metrics.failure_rate_score = am.value("failure_rate_score", 0.0);
                    event.anomaly_metrics.overall_score = am.value("overall_score", 0.0);
                    event.anomaly_metrics.is_anomalous = am.value("is_anomalous", false);
                    event.anomaly_metrics.description = am.value("description", "");
                }
                
                if (j.contains("metadata") && j["metadata"].is_object()) {
                    event.metadata = j["metadata"];
                }
                
                // Check if matches query; insert UUID before move so it's always valid
                if (matchesQuery(event, params)) {
                    seen_uuids.insert(event.uuid);
                    results.push_back(std::move(event));
                }
                
            } catch (...) {
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
    std::shared_lock<std::shared_mutex> lock(mutex_);
    
    std::vector<TaskSecurityEvent> results;
    
    // Query from in-memory cache
    for (const auto& event : recent_security_events_) {
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
    
    // Load from file to include events that have been evicted from the cache.
    // loadSecurityEventsFromFile() deduplicates against the in-memory results.
    auto file_results = loadSecurityEventsFromFile(config_.security_log_path, params);
    results.insert(results.end(), file_results.begin(), file_results.end());
    
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

std::vector<TaskSecurityEvent> TaskAuditManager::loadSecurityEventsFromFile(
    const std::string& file_path,
    const AuditQueryParams& params) const {
    
    std::vector<TaskSecurityEvent> results;
    
    // Build a set of UUIDs already in the cache to avoid duplicates
    std::unordered_set<std::string> seen_uuids;
    for (const auto& cached : recent_security_events_) {
        seen_uuids.insert(cached.uuid);
    }
    
    // Read at most enough events to satisfy the pagination request, bounded by
    // max_query_results.
    const size_t read_limit = std::min(params.offset + params.limit, config_.max_query_results);
    
    try {
        std::ifstream ifs(file_path);
        if (!ifs.is_open()) {
            return results;
        }
        
        std::string line;
        while (std::getline(ifs, line) && results.size() < read_limit) {
            if (line.empty()) continue;
            try {
                auto j = nlohmann::json::parse(line);
                
                TaskSecurityEvent event;
                event.uuid = j.value("uuid", "");
                
                // Skip events already in cache
                if (!event.uuid.empty() && seen_uuids.count(event.uuid)) {
                    continue;
                }
                
                if (j.contains("timestamp")) {
                    auto ts_ms = j["timestamp"].get<int64_t>();
                    event.timestamp = std::chrono::system_clock::time_point(
                        std::chrono::milliseconds(ts_ms));
                }
                
                event.task_id = j.value("task_id", "");
                event.task_name = j.value("task_name", "");
                event.severity = j.value("severity", "");
                event.user_id = j.value("user_id", "");
                event.ip_address = j.value("ip_address", "");
                event.session_id = j.value("session_id", "");
                event.violation_type = j.value("violation_type", "");
                event.description = j.value("description", "");
                event.blocked = j.value("blocked", false);
                event.action_taken = j.value("action_taken", "");
                
                if (j.contains("details") && j["details"].is_object()) {
                    event.details = j["details"];
                }
                if (j.contains("policy_id") && !j["policy_id"].is_null()) {
                    event.policy_id = j["policy_id"].get<std::string>();
                }
                if (j.contains("rule_id") && !j["rule_id"].is_null()) {
                    event.rule_id = j["rule_id"].get<std::string>();
                }
                
                // Apply filters
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
                
                // Insert UUID before move so it's always valid
                if (matches) {
                    seen_uuids.insert(event.uuid);
                    results.push_back(std::move(event));
                }
                
            } catch (...) {
                // Skip malformed lines
                continue;
            }
        }
    } catch (const std::exception& e) {
        THEMIS_ERROR("Failed to load security events from {}: {}", file_path, e.what());
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
            THEMIS_ERROR(
                "[TaskAuditManager::exportAuditEvents] "
                "code={} msg='cannot open output file' context={{output_path='{}', errno={}}}",
                static_cast<int>(SchedulerError::kInternalError),
                output_path, errno);
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
    std::shared_lock<std::shared_mutex> lock(mutex_);
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
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return config_;
}

void TaskAuditManager::updateConfig(const TaskAuditConfig& config) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
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
    std::shared_lock<std::shared_mutex> lock(mutex_);
    if (anomaly_detector_) {
        return anomaly_detector_->exportStatistics();
    }
    return nlohmann::json::object();
}

void TaskAuditManager::importAnomalyStatistics(const nlohmann::json& data) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    if (anomaly_detector_) {
        anomaly_detector_->importStatistics(data);
    }
}

// GAP 1 FIX: Immutable audit log enforcement with HMAC
std::string TaskAuditManager::generateAuditEntryHMAC(const TaskAuditEvent& event) const {
    if (!config_.enable_audit_hmac) {
        return "";
    }
    
    // Serialize event data for HMAC calculation
    // Use deterministic JSON encoding (sorted keys)
    nlohmann::json j = event.toJson(false);  // Don't mask for HMAC calculation
    std::string data = j.dump();
    
    // Calculate HMAC-SHA256
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hash_len = 0;
    
    HMAC(EVP_sha256(),
         config_.audit_hmac_key.data(), config_.audit_hmac_key.size(),
         reinterpret_cast<const unsigned char*>(data.data()), data.size(),
         hash, &hash_len);
    
    // Convert to hex string
    std::stringstream ss;
    for (unsigned int i = 0; i < hash_len; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }
    
    return ss.str();
}

bool TaskAuditManager::verifyAuditEntryIntegrity(const TaskAuditEvent& event, 
                                                 const std::string& stored_hmac) const {
    if (!config_.enable_audit_hmac || stored_hmac.empty()) {
        return true;  // HMAC verification disabled or not stored
    }
    
    std::string calculated_hmac = generateAuditEntryHMAC(event);
    
    if (calculated_hmac != stored_hmac) {
        THEMIS_ERROR("TaskAuditManager: HMAC verification failed for event {} "
                    "(stored={}, calculated={})",
                    event.uuid, stored_hmac.substr(0, 16) + "...",
                    calculated_hmac.substr(0, 16) + "...");
        return false;
    }
    
    return true;
}

// GAP 2 FIX: Retention policy enforcement
size_t TaskAuditManager::enforceRetentionPolicy() {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    
    if (config_.audit_retention_days.count() <= 0) {
        return 0;  // No retention policy configured
    }
    
    auto cutoff_time = std::chrono::system_clock::now() - config_.audit_retention_days;
    size_t removed_count = 0;
    
    try {
        std::vector<std::string> lines_to_keep;
        
        // Read all audit log entries
        {
            std::ifstream ifs(config_.audit_log_path);
            if (!ifs.is_open()) {
                return 0;
            }
            
            std::string line;
            while (std::getline(ifs, line)) {
                if (line.empty()) continue;
                
                try {
                    auto j = nlohmann::json::parse(line);
                    
                    if (j.contains("timestamp")) {
                        auto ts_ms = j["timestamp"].get<int64_t>();
                        auto event_time = std::chrono::system_clock::time_point(
                            std::chrono::milliseconds(ts_ms));
                        
                        // Keep entries newer than cutoff
                        if (event_time >= cutoff_time) {
                            lines_to_keep.push_back(line);
                        } else {
                            removed_count++;
                            
                            // Archive old entries if enabled
                            if (config_.enable_archival) {
                                try {
                                    std::ofstream archive_ofs(config_.archive_path, std::ios::app);
                                    if (archive_ofs.is_open()) {
                                        archive_ofs << line << "\n";
                                    }
                                } catch (...) {
                                    THEMIS_WARN("Failed to archive entry to {}", config_.archive_path);
                                }
                            }
                        }
                    } else {
                        lines_to_keep.push_back(line);
                    }
                } catch (...) {
                    // Keep malformed entries (don't delete)
                    lines_to_keep.push_back(line);
                }
            }
        }
        
        // Rewrite audit log with only retained entries
        {
            std::ofstream ofs(config_.audit_log_path, std::ios::trunc);
            for (const auto& line : lines_to_keep) {
                ofs << line << "\n";
            }
        }
        
        THEMIS_INFO("TaskAuditManager: retention policy enforced (removed={}, kept={})",
                   removed_count, lines_to_keep.size());
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("TaskAuditManager: failed to enforce retention policy: {}", e.what());
    }
    
    return removed_count;
}

// GAP 3 FIX: Corruption detection and recovery
size_t TaskAuditManager::detectAndRecoverCorruption() {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    
    size_t corruption_count = 0;
    std::vector<std::string> valid_lines;
    
    try {
        std::ifstream ifs(config_.audit_log_path);
        if (!ifs.is_open()) {
            return 0;
        }
        
        std::string line;
        size_t line_num = 0;
        
        while (std::getline(ifs, line)) {
            line_num++;
            
            if (line.empty()) continue;
            
            try {
                auto j = nlohmann::json::parse(line);
                
                // Basic structural validation
                if (!j.contains("uuid") || !j.contains("timestamp") || !j.contains("task_id")) {
                    THEMIS_WARN("TaskAuditManager: malformed audit entry detected at line {} "
                               "(missing required fields)", line_num);
                    corruption_count++;
                    
                    if (!config_.enable_corruption_recovery) {
                        valid_lines.push_back(line);  // Keep as-is if recovery disabled
                    }
                    continue;
                }
                
                // Validate timestamp format
                if (!j["timestamp"].is_number()) {
                    THEMIS_WARN("TaskAuditManager: corrupted timestamp at line {} (not a number)",
                               line_num);
                    corruption_count++;
                    
                    if (!config_.enable_corruption_recovery) {
                        valid_lines.push_back(line);
                    }
                    continue;
                }
                
                // Entry is valid, keep it
                valid_lines.push_back(line);
                
            } catch (const std::exception& e) {
                corruption_count++;
                THEMIS_WARN("TaskAuditManager: JSON parsing failed at line {} ({})",
                           line_num, e.what());
                
                if (!config_.enable_corruption_recovery) {
                    // Keep the line as-is
                    valid_lines.push_back(line);
                }
                // else: skip corrupted entries when recovery is enabled
            }
        }
        
        // Rewrite audit log with recovered entries
        if (corruption_count > 0 && config_.enable_corruption_recovery) {
            std::ofstream ofs(config_.audit_log_path, std::ios::trunc);
            for (const auto& valid_line : valid_lines) {
                ofs << valid_line << "\n";
            }
            
            THEMIS_INFO("TaskAuditManager: corruption recovery completed "
                       "(corruption_count={}, recovered={})",
                       corruption_count, valid_lines.size());
        }
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("TaskAuditManager: failed to detect/recover corruption: {}", e.what());
    }
    
    return corruption_count;
}

} // namespace scheduler
} // namespace themis