/**
 * @file task_audit_event.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "scheduler/task_audit_event.h"
#include <random>
#include <sstream>
#include <iomanip>
#include <openssl/sha.h>

namespace themis {
namespace scheduler {

// UUID generation
std::string generateUUID() {
    // GAP-019: Use std::random_device directly for cryptographic-quality randomness.
    // Audit event UUIDs must be unguessable to prevent enumeration attacks.
    std::random_device rd;

    uint64_t high = (static_cast<uint64_t>(rd()) << 32) | rd();
    uint64_t low  = (static_cast<uint64_t>(rd()) << 32) | rd();

    // Set version to 4 (random UUID)
    high = (high & 0xFFFFFFFFFFFF0FFFULL) | 0x0000000000004000ULL;
    // Set variant to RFC 4122
    low = (low & 0x3FFFFFFFFFFFFFFFULL) | 0x8000000000000000ULL;
    
    std::ostringstream oss;
    oss << std::hex << std::setfill('0')
        << std::setw(8) << ((high >> 32) & 0xFFFFFFFF) << "-"
        << std::setw(4) << ((high >> 16) & 0xFFFF) << "-"
        << std::setw(4) << (high & 0xFFFF) << "-"
        << std::setw(4) << ((low >> 48) & 0xFFFF) << "-"
        << std::setw(12) << (low & 0xFFFFFFFFFFFFULL);
    
    return oss.str();
}

// Data masking for GDPR compliance
std::string maskSensitiveData(const std::string& data, const std::string& mask_type) {
    if (data.empty()) {
        return data;
    }
    
    if (mask_type == "full") {
        return "***REDACTED***";
    } else if (mask_type == "hash") {
        // SHA-256 hash for pseudonymization
        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256(reinterpret_cast<const unsigned char*>(data.c_str()), data.length(), hash);
        
        std::ostringstream oss;
        for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
            oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
        }
        return oss.str();
    } else { // partial masking
        if (data.length() <= 4) {
            return "***";
        }
        // Show first and last 2 characters
        return data.substr(0, 2) + "***" + data.substr(data.length() - 2);
    }
}

// Event type conversions
std::string taskEventTypeToString([[maybe_unused]] TaskEventType type) {
    switch (type) {
        case TaskEventType::TASK_REGISTERED: return "TASK_REGISTERED";
        case TaskEventType::TASK_UNREGISTERED: return "TASK_UNREGISTERED";
        case TaskEventType::TASK_ENABLED: return "TASK_ENABLED";
        case TaskEventType::TASK_DISABLED: return "TASK_DISABLED";
        case TaskEventType::TASK_UPDATED: return "TASK_UPDATED";
        case TaskEventType::TASK_STARTED: return "TASK_STARTED";
        case TaskEventType::TASK_COMPLETED: return "TASK_COMPLETED";
        case TaskEventType::TASK_FAILED: return "TASK_FAILED";
        case TaskEventType::TASK_TIMEOUT: return "TASK_TIMEOUT";
        case TaskEventType::TASK_RETRY: return "TASK_RETRY";
        case TaskEventType::TASK_QUEUED: return "TASK_QUEUED";
        case TaskEventType::TASK_DEQUEUED: return "TASK_DEQUEUED";
        case TaskEventType::MANUAL_EXECUTION: return "MANUAL_EXECUTION";
        case TaskEventType::CRON_TRIGGERED: return "CRON_TRIGGERED";
        case TaskEventType::CDC_TRIGGERED: return "CDC_TRIGGERED";
        case TaskEventType::INTERVAL_TRIGGERED: return "INTERVAL_TRIGGERED";
        case TaskEventType::WEBHOOK_TRIGGERED: return "WEBHOOK_TRIGGERED";
        default: return "UNKNOWN";
    }
}

std::string taskSecurityEventTypeToString([[maybe_unused]] TaskSecurityEventType type) {
    switch (type) {
        case TaskSecurityEventType::RATE_LIMIT_EXCEEDED: return "RATE_LIMIT_EXCEEDED";
        case TaskSecurityEventType::RESOURCE_LIMIT_EXCEEDED: return "RESOURCE_LIMIT_EXCEEDED";
        case TaskSecurityEventType::CRON_INJECTION_DETECTED: return "CRON_INJECTION_DETECTED";
        case TaskSecurityEventType::AQL_INJECTION_DETECTED: return "AQL_INJECTION_DETECTED";
        case TaskSecurityEventType::UNAUTHORIZED_ACCESS: return "UNAUTHORIZED_ACCESS";
        case TaskSecurityEventType::INVALID_CONFIGURATION: return "INVALID_CONFIGURATION";
        case TaskSecurityEventType::EXCESSIVE_FAILURES: return "EXCESSIVE_FAILURES";
        case TaskSecurityEventType::ANOMALY_DETECTED: return "ANOMALY_DETECTED";
        case TaskSecurityEventType::PRIVILEGE_ESCALATION: return "PRIVILEGE_ESCALATION";
        case TaskSecurityEventType::SUSPICIOUS_PATTERN: return "SUSPICIOUS_PATTERN";
        default: return "UNKNOWN";
    }
}

TaskEventType taskEventTypeFromString([[maybe_unused]] const std::string& s) {
    if (s == "TASK_REGISTERED") {
      return TaskEventType::TASK_REGISTERED;
    }
    if (s == "TASK_UNREGISTERED") {
      return TaskEventType::TASK_UNREGISTERED;
    }
    if (s == "TASK_ENABLED") {
      return TaskEventType::TASK_ENABLED;
    }
    if (s == "TASK_DISABLED") {
      return TaskEventType::TASK_DISABLED;
    }
    if (s == "TASK_UPDATED") {
      return TaskEventType::TASK_UPDATED;
    }
    if (s == "TASK_STARTED") {
      return TaskEventType::TASK_STARTED;
    }
    if (s == "TASK_COMPLETED") {
      return TaskEventType::TASK_COMPLETED;
    }
    if (s == "TASK_FAILED") {
      return TaskEventType::TASK_FAILED;
    }
    if (s == "TASK_TIMEOUT") {
      return TaskEventType::TASK_TIMEOUT;
    }
    if (s == "TASK_RETRY") {
      return TaskEventType::TASK_RETRY;
    }
    if (s == "TASK_QUEUED") {
      return TaskEventType::TASK_QUEUED;
    }
    if (s == "TASK_DEQUEUED") {
      return TaskEventType::TASK_DEQUEUED;
    }
    if (s == "MANUAL_EXECUTION") {
      return TaskEventType::MANUAL_EXECUTION;
    }
    if (s == "CRON_TRIGGERED") {
      return TaskEventType::CRON_TRIGGERED;
    }
    if (s == "CDC_TRIGGERED") {
      return TaskEventType::CDC_TRIGGERED;
    }
    if (s == "INTERVAL_TRIGGERED") {
      return TaskEventType::INTERVAL_TRIGGERED;
    }
    if (s == "WEBHOOK_TRIGGERED") {
      return TaskEventType::WEBHOOK_TRIGGERED;
    }
    return TaskEventType::TASK_COMPLETED; // default for unknown values
}


nlohmann::json TaskAuditEvent::toJson([[maybe_unused]] bool gdpr_mode) const {
    nlohmann::json j;
    
    // Standard audit fields
    j["uuid"] = uuid;
    j["timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(
        timestamp.time_since_epoch()).count();
    j["duration_ms"] = duration_ms;
    
    // Task information
    j["task_id"] = task_id;
    j["task_name"] = task_name;
    j["task_description"] = task_description;
    
    // Event classification
    j["event_type"] = taskEventTypeToString([[maybe_unused]] event_type);
    j["trigger_type"] = trigger_type;
    
    // Actor information (with GDPR masking if enabled)
    if (gdpr_mode) {
        j["user_id"] = maskSensitiveData(user_id, "partial");
        j["ip_address"] = maskSensitiveData(ip_address, "partial");
        j["session_id"] = maskSensitiveData(session_id, "hash");
    } else {
        j["user_id"] = user_id;
        j["ip_address"] = ip_address;
        j["session_id"] = session_id;
    }
    
    if (tenant_id) {
        j["tenant_id"] = *tenant_id;
    }
    
    // Result information
    j["success"] = success;
    if (error_message) {
        j["error_message"] = *error_message;
    }
    if (error_type) {
        j["error_type"] = *error_type;
    }
    if (retry_count) {
        j["retry_count"] = *retry_count;
    }
    
    // Resource usage
    j["resource_usage"] = resource_usage.toJson();
    
    // Anomaly metrics
    j["anomaly_metrics"] = anomaly_metrics.toJson();
    
    // Additional metadata
    j["metadata"] = metadata;
    
    return j;
}

std::string TaskAuditEvent::toCEF() const {
    // CEF Format: CEF:Version|Device Vendor|Device Product|Device Version|Signature ID|Name|Severity|Extension
    std::ostringstream cef;
    
    cef << "CEF:0|ThemisDB|TaskScheduler|1.5.0|";
    cef << taskEventTypeToString(event_type) << "|";
    cef << "Task Execution Event|";
    
    // Severity: 0-3 (Low), 4-6 (Medium), 7-8 (High), 9-10 (Critical)
    int severity = success ? 3 : 6;
    if (!success && error_type && *error_type == "TIMEOUT") {
        severity = 7;
    }
    cef << severity << "|";
    
    // Extensions (key=value pairs)
    cef << "taskId=" << task_id << " ";
    cef << "taskName=" << task_name << " ";
    cef << "triggerType=" << trigger_type << " ";
    cef << "outcome=" << (success ? "success" : "failure") << " ";
    cef << "durationMs=" << duration_ms << " ";
    cef << "cpuTimeMs=" << resource_usage.cpu_time_ms << " ";
    cef << "memoryBytes=" << resource_usage.memory_bytes << " ";
    
    if (anomaly_metrics.is_anomalous) {
        cef << "anomalyScore=" << anomaly_metrics.overall_score << " ";
        cef << "anomaly=" << anomaly_metrics.description << " ";
    }
    
    if (error_message) {
        cef << "msg=" << *error_message << " ";
    }
    
    return cef.str();
}

nlohmann::json TaskAuditEvent::toSplunkHEC() const {
    nlohmann::json event;
    
    event["time"] = std::chrono::duration_cast<std::chrono::seconds>(
        timestamp.time_since_epoch()).count();
    
    event["host"] = "themisdb";
    event["source"] = "task_scheduler";
    event["sourcetype"] = "themis:task:audit";
    
    // Index based on event type
    if (anomaly_metrics.is_anomalous || !success) {
        event["index"] = "themis_security";
    } else {
        event["index"] = "themis_audit";
    }
    
    // Event data
    event["event"] = toJson([[maybe_unused]] false);
    
    return event;
}

nlohmann::json TaskAuditEvent::toElasticECS() const {
    nlohmann::json ecs;
    
    // ECS base fields
    ecs["@timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(
        timestamp.time_since_epoch()).count();
    
    // Event category
    ecs["event"]["kind"] = "event";
    ecs["event"]["category"] = "process";
    ecs["event"]["type"] = success ? "info" : "error";
    ecs["event"]["outcome"] = success ? "success" : "failure";
    ecs["event"]["duration"] = static_cast<int64_t>([[maybe_unused]] duration_ms * 1000000); // nanoseconds
    
    // Service/observer
    ecs["observer"]["name"] = "themisdb";
    ecs["observer"]["type"] = "database";
    ecs["observer"]["product"] = "ThemisDB TaskScheduler";
    ecs["observer"]["version"] = "1.5.0";
    
    // User information
    ecs["user"]["id"] = user_id;
    
    // Network information
    ecs["source"]["ip"] = ip_address;
    
    // Process/task information
    ecs["process"]["name"] = task_name;
    ecs["process"]["command_line"] = task_description;
    
    // Custom fields
    ecs["themis"]["task"]["id"] = task_id;
    ecs["themis"]["task"]["trigger_type"] = trigger_type;
    ecs["themis"]["task"]["event_type"] = taskEventTypeToString([[maybe_unused]] event_type);
    ecs["themis"]["task"]["resource_usage"] = resource_usage.toJson();
    ecs["themis"]["task"]["anomaly_metrics"] = anomaly_metrics.toJson();
    
    if (error_message) {
        ecs["error"]["message"] = *error_message;
        if (error_type) {
            ecs["error"]["type"] = *error_type;
        }
    }
    
    return ecs;
}

// TaskSecurityEvent implementations
nlohmann::json TaskSecurityEvent::toJson() const {
    nlohmann::json j;
    
    j["uuid"] = uuid;
    j["timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(
        timestamp.time_since_epoch()).count();
    
    if (!task_id.empty()) {
        j["task_id"] = task_id;
        j["task_name"] = task_name;
    }
    
    j["event_type"] = taskSecurityEventTypeToString([[maybe_unused]] event_type);
    j["severity"] = severity;
    
    j["user_id"] = user_id;
    j["ip_address"] = ip_address;
    j["session_id"] = session_id;
    
    j["violation_type"] = violation_type;
    j["description"] = description;
    j["details"] = details;
    
    if (policy_id) {
        j["policy_id"] = *policy_id;
    }
    if (rule_id) {
        j["rule_id"] = *rule_id;
    }
    
    j["blocked"] = blocked;
    j["action_taken"] = action_taken;
    
    return j;
}

std::string TaskSecurityEvent::toCEF() const {
    std::ostringstream cef;
    
    cef << "CEF:0|ThemisDB|TaskScheduler|1.5.0|";
    cef << taskSecurityEventTypeToString(event_type) << "|";
    cef << "Task Security Event|";
    
    // Map severity to CEF severity (0-10)
    int cef_severity = 5; // default medium
    if (severity == "LOW") {
      cef_severity = 3;
    }
    else if (severity == "MEDIUM") cef_severity = 5;
    else if (severity == "HIGH") cef_severity = 8;
    else if (severity == "CRITICAL") cef_severity = 10;
    
    cef << cef_severity << "|";
    
    // Extensions
    cef << "act=" << action_taken << " ";
    cef << "outcome=" << (blocked ? "blocked" : "allowed") << " ";
    
    if (!task_id.empty()) {
        cef << "taskId=" << task_id << " ";
    }
    
    cef << "msg=" << description << " ";
    
    return cef.str();
}

} // namespace scheduler
} // namespace themis

