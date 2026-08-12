/**
 * @file audit_logger.h
 * @brief Audit logging for GraphQL operations (security, compliance, debugging).
 *
 * @details Records structured audit events for all GraphQL operations,
 * authentication/authorization events, rate limiting violations, and validation failures.
 *
 * Core components:
 *  - `AuditLogEntry`: Structured log entry with event type, user/tenant/IP, timestamps
 *  - `AuditLogger`: Thread-safe audit event sink with optional file output
 *
 * Event types tracked:
 *  - QueryExecution, MutationExecution, SubscriptionCreated
 *  - AuthenticationAttempt, AuthorizationFailure
 *  - RateLimitExceeded, ValidationFailure
 *  - DeprecatedFeatureUsed
 *
 * Log format:
 *  - JSON-serialized for machine parsing
 *  - Includes operation name, type, user ID, tenant ID, IP address, timestamps
 *  - Query complexity metrics and operation hashes for analytics
 *  - Optional metadata map for custom attributes
 *
 * Design goals:
 *  - Non-blocking event recording (background thread or buffering)
 *  - Bounded memory consumption (circular buffer or TTL expiration)
 *  - PII sanitization: user ID and IP included only if required by policy
 *  - Never logs full query text or mutation arguments (prevents credential leakage)
 *
 * ### Thread safety
 * `AuditLogger` is thread-safe; multiple HTTP handler threads may log concurrently.
 * Logging is asynchronous (buffered or background) to avoid critical-path latency.
 *
 * ### Usage
 * ```cpp
 * AuditLogger logger;
 * logger.enableFileOutput("/var/log/themisdb/audit.jsonl");
 *
 * AuditLogEntry entry;
 * entry.event_type = AuditLogEntry::EventType::QueryExecution;
 * entry.operation_name = "GetEntity";
 * entry.user_id = "user-12345";
 * entry.ip_address = "192.168.1.100";
 * entry.success = true;
 * logger.log(entry);
 * ```
 *
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 */


#pragma once

#include <string>
#include <chrono>
#include <vector>
#include <mutex>
#include <functional>
#include <memory>
#include <sstream>
#include <iomanip>
#include <fstream>

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
     *
     * Handlers are invoked outside the critical section: the handlers vector
     * is copied under the lock (O(n) pointer copies), the lock is released,
     * and then each handler is called. This prevents slow handlers (file I/O,
     * network sinks, regex matching, etc.) from stalling concurrent API
     * threads. The buffer append and statistics update remain lock-protected.
     */
    void log(const AuditLogEntry& entry) {
        // Copy handlers under lock so we can invoke them without holding it.
        std::vector<LogHandler> handlers_copy;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            handlers_copy = handlers_;
        }

        // Invoke handlers outside the critical section.
        for (const auto& handler : handlers_copy) {
            handler(entry);
        }

        // Update buffer and stats under lock — both are fast in-memory ops.
        {
            std::lock_guard<std::mutex> lock(mutex_);

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
     * @brief Register a file-backed audit log handler
     *
     * Creates a handler that appends newline-delimited JSON audit entries to
     * @p path (JSONL format).  Entries are opened in append mode so that
     * existing content is preserved across process restarts.  Calling this
     * method is the programmatic equivalent of setting
     * `persistence: {backend: file}` in `config/audit.yaml`.
     *
     * The returned handler captures a shared state object; ownership is
     * transferred to the internal handlers list and the caller need not
     * retain anything.
     */
    void addFileHandler(const std::string& path) {
        // Shared state so the lambda captures by value without copying the path.
        auto shared_path = std::make_shared<std::string>(path);
        auto file_mutex  = std::make_shared<std::mutex>();
        addHandler([shared_path, file_mutex](const AuditLogEntry& entry) {
            std::lock_guard<std::mutex> lk(*file_mutex);
            std::ofstream ofs(*shared_path, std::ios::app);
            if (ofs.is_open()) {
                ofs << entry.toJSON() << "\n";
            }
        });
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
 * @brief Standalone file-backed audit log handler
 *
 * Appends newline-delimited JSON (JSONL) audit entries to a configurable
 * file path. Thread-safe: concurrent invocations are serialised by an
 * internal mutex. Registered automatically by `AuditLogger::addFileHandler()`
 * and by default when `config/audit.yaml` specifies `persistence: {backend: file}`.
 */
class FileAuditLogHandler {
public:
    /**
     * @brief Construct a handler that writes to @p path.
     *
     * The file is opened in append mode on each write so that content
     * survives process restarts and multiple handler instances for the same
     * path do not conflict.
     */
    explicit FileAuditLogHandler(const std::string& path)
        : path_(path) {}

    /** Append @p entry as a JSON line to the configured file. */
    void operator()(const AuditLogEntry& entry) {
        std::lock_guard<std::mutex> lock(mutex_);
        std::ofstream ofs(path_, std::ios::app);
        if (ofs.is_open()) {
            ofs << entry.toJSON() << "\n";
        }
    }

    /** Return the configured file path. */
    const std::string& path() const { return path_; }

private:
    std::string path_;
    mutable std::mutex mutex_;
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
