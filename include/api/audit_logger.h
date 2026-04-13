/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            audit_logger.h                                     ║
  Version:         0.0.38                                             ║
  Last Modified:   2026-04-13 04:13:25                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     352                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • a629043ab2  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <string>
#include <chrono>
#include <vector>
#include <mutex>
#include <functional>
#include <sstream>
#include <iomanip>

namespace themis {
namespace graphql {

/**
 * @brief Audit log entry for GraphQL operations
 * 
 * Records important events for security, compliance, and debugging.
 */
struct AuditLogEntry {
    enum class EventType {
        QueryExecution,
        MutationExecution,
        SubscriptionCreated,
        AuthenticationAttempt,
        AuthorizationFailure,
        RateLimitExceeded,
        ValidationFailure,
        DeprecatedFeatureUsed
    };
    
    EventType event_type;
    std::string operation_name;
    std::string operation_type;
    std::string user_id;
    std::string tenant_id;
    std::string ip_address;
    std::chrono::system_clock::time_point timestamp;
    bool success;
    std::string error_message;
    std::string query_hash;
    size_t query_complexity = 0;
    
    // Optional metadata
    std::unordered_map<std::string, std::string> metadata;
    
    /**
     * @brief Convert event type to string
     */
    static std::string eventTypeToString(EventType type) {
        switch (type) {
            case EventType::QueryExecution: return "QueryExecution";
            case EventType::MutationExecution: return "MutationExecution";
            case EventType::SubscriptionCreated: return "SubscriptionCreated";
            case EventType::AuthenticationAttempt: return "AuthenticationAttempt";
            case EventType::AuthorizationFailure: return "AuthorizationFailure";
            case EventType::RateLimitExceeded: return "RateLimitExceeded";
            case EventType::ValidationFailure: return "ValidationFailure";
            case EventType::DeprecatedFeatureUsed: return "DeprecatedFeatureUsed";
            default: return "Unknown";
        }
    }
    
    /**
     * @brief Convert to JSON string for logging
     */
    std::string toJSON() const {
        std::ostringstream oss;
        oss << "{"
            << "\"event_type\":\"" << eventTypeToString(event_type) << "\","
            << "\"operation_name\":\"" << operation_name << "\","
            << "\"operation_type\":\"" << operation_type << "\","
            << "\"user_id\":\"" << user_id << "\","
            << "\"tenant_id\":\"" << tenant_id << "\","
            << "\"ip_address\":\"" << ip_address << "\","
            << "\"timestamp\":\"" << formatTimestamp() << "\","
            << "\"success\":" << (success ? "true" : "false") << ","
            << "\"error_message\":\"" << error_message << "\","
            << "\"query_hash\":\"" << query_hash << "\","
            << "\"query_complexity\":" << query_complexity;
        
        if (!metadata.empty()) {
            oss << ",\"metadata\":{";
            bool first = true;
            for (const auto& [key, value] : metadata) {
                if (!first) oss << ",";
                oss << "\"" << key << "\":\"" << value << "\"";
                first = false;
            }
            oss << "}";
        }
        
        oss << "}";
        return oss.str();
    }
    
private:
    std::string formatTimestamp() const {
        auto time_t = std::chrono::system_clock::to_time_t(timestamp);
        std::ostringstream oss;
        oss << std::put_time(std::gmtime(&time_t), "%Y-%m-%dT%H:%M:%SZ");
        return oss.str();
    }
};

/**
 * @brief Audit logger for GraphQL operations
 * 
 * Provides structured logging for security and compliance.
 */
class AuditLogger {
public:
    using LogHandler = std::function<void(const AuditLogEntry&)>;
    
    /**
     * @brief Log an audit entry
     */
    void log(const AuditLogEntry& entry) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Call all registered handlers
        for (const auto& handler : handlers_) {
            handler(entry);
        }
        
        // Keep in memory buffer (circular buffer)
        if (buffer_.size() >= buffer_capacity_) {
            buffer_.erase(buffer_.begin());
        }
        buffer_.push_back(entry);
        
        stats_.total_entries++;
        if (!entry.success) {
            stats_.failure_entries++;
        }
    }
    
    /**
     * @brief Register a log handler
     * 
     * Handlers are called for each log entry.
     * Example: write to file, send to monitoring system, etc.
     */
    void addHandler(const LogHandler& handler) {
        std::lock_guard<std::mutex> lock(mutex_);
        handlers_.push_back(handler);
    }
    
    /**
     * @brief Clear all handlers
     */
    void clearHandlers() {
        std::lock_guard<std::mutex> lock(mutex_);
        handlers_.clear();
    }
    
    /**
     * @brief Get recent log entries
     */
    std::vector<AuditLogEntry> getRecent(size_t count) const {
        std::lock_guard<std::mutex> lock(mutex_);
        
        size_t start = buffer_.size() > count ? buffer_.size() - count : 0;
        return std::vector<AuditLogEntry>(buffer_.begin() + start, buffer_.end());
    }
    
    /**
     * @brief Search log entries by user ID
     */
    std::vector<AuditLogEntry> searchByUser(const std::string& user_id) const {
        std::lock_guard<std::mutex> lock(mutex_);
        
        std::vector<AuditLogEntry> results;
        for (const auto& entry : buffer_) {
            if (entry.user_id == user_id) {
                results.push_back(entry);
            }
        }
        return results;
    }
    
    /**
     * @brief Search log entries by event type
     */
    std::vector<AuditLogEntry> searchByEventType(AuditLogEntry::EventType type) const {
        std::lock_guard<std::mutex> lock(mutex_);
        
        std::vector<AuditLogEntry> results;
        for (const auto& entry : buffer_) {
            if (entry.event_type == type) {
                results.push_back(entry);
            }
        }
        return results;
    }
    
    /**
     * @brief Clear the buffer
     */
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        buffer_.clear();
    }
    
    /**
     * @brief Set buffer capacity
     */
    void setBufferCapacity(size_t capacity) {
        std::lock_guard<std::mutex> lock(mutex_);
        buffer_capacity_ = capacity;
        
        // Trim if needed
        while (buffer_.size() > buffer_capacity_) {
            buffer_.erase(buffer_.begin());
        }
    }
    
    /**
     * @brief Get audit statistics
     */
    struct Stats {
        uint64_t total_entries = 0;
        uint64_t failure_entries = 0;
        
        double failureRate() const {
            return total_entries > 0 ? 
                static_cast<double>(failure_entries) / total_entries : 0.0;
        }
    };
    
    Stats getStats() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return stats_;
    }
    
    /**
     * @brief Singleton instance
     */
    static AuditLogger& instance() {
        static AuditLogger instance;
        return instance;
    }
    
private:
    AuditLogger() = default;
    
    mutable std::mutex mutex_;
    std::vector<LogHandler> handlers_;
    std::vector<AuditLogEntry> buffer_;
    size_t buffer_capacity_ = 1000;  // Keep last 1000 entries in memory
    Stats stats_;
};

/**
 * @brief Helper to create audit log entries
 */
class AuditLogBuilder {
public:
    AuditLogBuilder(AuditLogEntry::EventType type)
        : entry_{} {
        entry_.event_type = type;
        entry_.timestamp = std::chrono::system_clock::now();
        entry_.success = true;
    }
    
    AuditLogBuilder& operationName(const std::string& name) {
        entry_.operation_name = name;
        return *this;
    }
    
    AuditLogBuilder& operationType(const std::string& type) {
        entry_.operation_type = type;
        return *this;
    }
    
    AuditLogBuilder& user(const std::string& user_id) {
        entry_.user_id = user_id;
        return *this;
    }
    
    AuditLogBuilder& tenant(const std::string& tenant_id) {
        entry_.tenant_id = tenant_id;
        return *this;
    }
    
    AuditLogBuilder& ipAddress(const std::string& ip) {
        entry_.ip_address = ip;
        return *this;
    }
    
    AuditLogBuilder& success(bool succeeded) {
        entry_.success = succeeded;
        return *this;
    }
    
    AuditLogBuilder& error(const std::string& error_msg) {
        entry_.error_message = error_msg;
        entry_.success = false;
        return *this;
    }
    
    AuditLogBuilder& queryHash(const std::string& hash) {
        entry_.query_hash = hash;
        return *this;
    }
    
    AuditLogBuilder& complexity(size_t complexity) {
        entry_.query_complexity = complexity;
        return *this;
    }
    
    AuditLogBuilder& metadata(const std::string& key, const std::string& value) {
        entry_.metadata[key] = value;
        return *this;
    }
    
    void log() {
        AuditLogger::instance().log(entry_);
    }
    
    AuditLogEntry build() const {
        return entry_;
    }
    
private:
    AuditLogEntry entry_;
};

} // namespace graphql
} // namespace themis
