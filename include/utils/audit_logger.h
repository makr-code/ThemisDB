/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            audit_logger.h                                     ║
  Version:         0.0.13                                             ║
  Last Modified:   2026-02-21 16:34:27                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     407                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <string>
#include <memory>
#include <vector>
#include <fstream>
#include <mutex>
#include <optional>
#include <nlohmann/json.hpp>

#include "security/encryption.h"
#include "utils/pki_client.h"

namespace themis {
namespace utils {

/**
 * @brief Security event types for audit logging
 */
enum class SecurityEventType {
    // Authentication & Authorization
    LOGIN_SUCCESS,
    LOGIN_FAILED,
    LOGOUT,
    TOKEN_CREATED,
    TOKEN_REVOKED,
    UNAUTHORIZED_ACCESS,
    PERMISSION_DENIED,
    
    // MFA Events (Phase 3)
    MFA_ENROLLED,
    MFA_ENABLED,
    MFA_DISABLED,
    MFA_TOTP_SUCCESS,
    MFA_TOTP_FAILED,
    MFA_RECOVERY_CODE_USED,
    MFA_RECOVERY_CODES_REGENERATED,
    MFA_BACKUP_CODES_VIEWED,
    
    // Privilege Escalation
    PRIVILEGE_ESCALATION_ATTEMPT,
    ROLE_CHANGED,
    SCOPE_GRANTED,
    SCOPE_REVOKED,
    
    // Key Management
    KEY_CREATED,
    KEY_ROTATED,
    KEY_DELETED,
    KEY_ACCESS,
    
    // HSM Operations (Hardware Security Module)
    HSM_INITIALIZED,
    HSM_FINALIZED,
    HSM_KEY_GENERATED,
    HSM_KEY_IMPORTED,
    HSM_CERT_IMPORTED,
    HSM_SIGN_OPERATION,
    HSM_VERIFY_OPERATION,
    HSM_SIGN_FAILED,
    HSM_VERIFY_FAILED,
    HSM_SESSION_OPENED,
    HSM_SESSION_CLOSED,
    HSM_LOGIN_SUCCESS,
    HSM_LOGIN_FAILED,
    HSM_KEY_ACCESS,
    
    // Data Access
    DATA_READ,
    DATA_WRITE,
    DATA_DELETE,
    BULK_EXPORT,
    BULK_IMPORT,            ///< Large-scale data import started (e.g. pg_dump)
    BULK_IMPORT_COMPLETED,  ///< Large-scale data import finished; includes stats payload
    
    // Graph & Vector Operations (Phase 1 - Knowledge Graph Protection)
    GRAPH_TRAVERSAL,        // BFS/DFS traversal operations
    BULK_NODE_ACCESS,       // Large-scale node queries
    BULK_EDGE_ACCESS,       // Large-scale edge queries
    EMBEDDING_QUERY,        // Vector embedding queries
    EMBEDDING_EXPORT,       // Vector embedding downloads
    GRAPH_EXPORT,           // Full graph exports
    TEMPORAL_QUERY,         // Historical graph queries
    
    // GPU/VRAM Security (Phase 2)
    VRAM_ALLOCATED,
    VRAM_DEALLOCATED,
    VRAM_SECURE_CLEAR,
    GPU_MEMORY_EXHAUSTION,
    
    // PII Operations
    PII_ACCESSED,
    PII_REVEALED,
    PII_ERASED,
    
    // Configuration Changes
    CONFIG_CHANGED,
    POLICY_UPDATED,
    ENCRYPTION_SCHEMA_CHANGED,
    
    // Security Incidents
    BRUTE_FORCE_DETECTED,
    RATE_LIMIT_EXCEEDED,
    SUSPICIOUS_ACTIVITY,
    INTEGRITY_VIOLATION,
    
    // Binary Integrity (Phase 5)
    BINARY_SIGNATURE_VERIFIED,
    BINARY_SIGNATURE_FAILED,
    MANIFEST_UPDATED,
    
    // System Events
    SERVER_STARTED,
    SERVER_STOPPED,
    BACKUP_CREATED,
    RESTORE_COMPLETED,
    
    // Task Scheduler Events (SIEM Integration)
    TASK_REGISTERED,
    TASK_UNREGISTERED,
    TASK_ENABLED,
    TASK_DISABLED,
    TASK_UPDATED,
    TASK_EXECUTED_SUCCESS,
    TASK_EXECUTED_FAILURE,
    TASK_CRON_TRIGGERED,
    TASK_CDC_TRIGGERED,
    TASK_MANUAL_TRIGGERED,
    TASK_TIMEOUT,
    TASK_RESOURCE_LIMIT_EXCEEDED,
    TASK_ANOMALY_DETECTED,
    
    // Generic
    CUSTOM_EVENT
};

struct AuditLoggerConfig {
    bool enabled = true;
    bool encrypt_then_sign = true;
    std::string log_path = "data/logs/audit.jsonl"; // JSON Lines sink
    std::string key_id = "saga_log";               // logical key id for log encryption
    
    // Hash chain for tamper-proofing
    bool enable_hash_chain = true;
    std::string chain_state_file = "data/logs/audit_chain.json";
    
    // SIEM integration
    bool enable_siem = false;
    std::string siem_type = "syslog"; // "syslog", "splunk", or "elastic"
    std::string siem_format = "json";  // "json", "cef", or "syslog"
    std::string siem_host = "localhost";
    uint16_t siem_port = 514; // Default: syslog (514). Configure based on siem_type: Splunk HEC (8088), Elastic (9200)
    std::string splunk_token; // Splunk HEC token
    std::string elastic_index = "themisdb-audit"; // Elasticsearch index
    /// Path to a CA bundle file (PEM) used for TLS verification when
    /// siem_type is "splunk".  If empty, libcurl uses its system default
    /// bundle.  Set this explicitly in production to pin the expected CA.
    std::string siem_ca_bundle_path;
    
    // Task Scheduler SIEM settings
    bool enable_task_scheduler_audit = true;
    bool enable_anomaly_detection = true;
    double anomaly_threshold = 2.0; // Standard deviations from baseline
};

// Minimal Audit Logger supporting Encrypt-then-Sign batches (single-entry for now)
class AuditLogger {
public:
    AuditLogger(std::shared_ptr<themis::FieldEncryption> enc,
                std::shared_ptr<VCCPKIClient> pki,
                AuditLoggerConfig cfg);

    // Log a generic data access/audit event; if encrypt_then_sign is enabled,
    // encrypts the canonical JSON with FieldEncryption, computes SHA-256 over
    // ciphertext (iv|ciphertext|tag), obtains a signature from PKI client, and
    // appends a JSON record to log_path.
    void logEvent(const nlohmann::json& event);
    
    /**
     * @brief Log a security event with structured data
     * @param event_type Security event type
     * @param user_id User/service account performing the action
     * @param resource Resource being accessed (e.g., entity key, API endpoint)
     * @param details Additional event-specific details
     */
    void logSecurityEvent(
        SecurityEventType event_type,
        const std::string& user_id,
        const std::string& resource,
        const nlohmann::json& details = {}
    );
    
    /**
     * @brief Verify integrity of audit log hash chain
     * @return true if chain is valid, false if tampering detected
     */
    bool verifyChainIntegrity();
    
    /**
     * @brief Flush audit log to disk
     */
    void flush();
    
    /**
     * @brief Get current hash chain state
     * @return JSON with last_hash, entry_count, last_timestamp
     */
    nlohmann::json getChainState() const;
    
    /**
     * @brief Structure representing an audit log entry with metadata
     */
    struct AuditLogEntry {
        uint64_t entry_number;                              // Sequential entry number
        std::chrono::system_clock::time_point timestamp;    // Entry timestamp
        nlohmann::json record;                              // Full JSON record
    };
    
    /**
     * @brief Enumerate all audit log entries with their timestamps
     * @return Vector of audit log entries
     */
    std::vector<AuditLogEntry> enumerateEntries() const;
    
    /**
     * @brief Archive audit log entries older than specified timestamp
     * @param older_than Archive entries created before this timestamp
     * @param archive_path Path to archive file (appends to existing archive)
     * @return Number of entries archived
     */
    size_t archiveOldEntries(std::chrono::system_clock::time_point older_than, 
                             const std::string& archive_path);
    
    /**
     * @brief Purge audit log entries older than specified timestamp
     * @param older_than Purge entries created before this timestamp
     * @return Number of entries purged
     */
    size_t purgeOldEntries(std::chrono::system_clock::time_point older_than);

    // -----------------------------------------------------------------------
    // Phase 3 – Audit Search & Compliance Reporting
    // -----------------------------------------------------------------------

    /**
     * @brief Search criteria for audit log queries.
     */
    struct SearchQuery {
        /// Only return entries at or after this timestamp (optional).
        std::optional<std::chrono::system_clock::time_point> from;
        /// Only return entries before this timestamp (optional).
        std::optional<std::chrono::system_clock::time_point> to;
        /// Filter by user_id field (empty = any).
        std::string user_id;
        /// Filter by action/event type keyword (empty = any).
        std::string action;
        /// Filter by resource field prefix (empty = any).
        std::string resource_prefix;
        /// Maximum number of results to return (0 = unlimited).
        size_t max_results = 0;
    };

    /**
     * @brief Search audit log entries matching the given criteria.
     *
     * Reads the log file sequentially and returns every plaintext record
     * that satisfies all specified filters.  Encrypted entries are included
     * as-is (payload.type == "ciphertext") without attempting decryption.
     *
     * @param query  Search parameters.
     * @return       Matching entries in chronological order.
     */
    std::vector<AuditLogEntry> searchEntries(const SearchQuery& query) const;

    /**
     * @brief Compliance report for a given time window.
     */
    struct ComplianceReport {
        std::chrono::system_clock::time_point from;
        std::chrono::system_clock::time_point to;
        uint64_t total_events = 0;
        uint64_t security_events = 0;
        uint64_t data_access_events = 0;
        uint64_t authentication_events = 0;
        uint64_t key_management_events = 0;
        uint64_t pii_events = 0;
        bool chain_intact = false;
        nlohmann::json event_counts_by_type;   ///< map<string, int>
        nlohmann::json top_users;              ///< map<user_id, count>
    };

    /**
     * @brief Generate a compliance report for the specified time window.
     *
     * Aggregates event counts, identifies top users, and verifies chain
     * integrity across the window.
     *
     * @param from  Start of the reporting period.
     * @param to    End of the reporting period.
     * @return      Populated ComplianceReport.
     */
    ComplianceReport generateComplianceReport(
        std::chrono::system_clock::time_point from,
        std::chrono::system_clock::time_point to);
    
    /**
     * @brief Get the configured log file path
     */
    std::string getLogPath() const { return cfg_.log_path; }
    
    /**
     * @brief Log a task scheduler event with resource metrics and anomaly detection
     * @param event_type Task scheduler event type
     * @param task_id Task identifier
     * @param user_id User/service account executing the task
     * @param details Event details including resource consumption and metrics
     */
    void logTaskSchedulerEvent(
        SecurityEventType event_type,
        const std::string& task_id,
        const std::string& user_id,
        const nlohmann::json& details = {}
    );
    
    /**
     * @brief Calculate anomaly score for a task execution
     * @param task_id Task identifier
     * @param execution_time_ms Execution time in milliseconds
     * @param resource_usage Resource consumption metrics
     * @return Anomaly score (0.0 = normal, >2.0 = anomalous)
     */
    double calculateAnomalyScore(
        const std::string& task_id,
        double execution_time_ms,
        const nlohmann::json& resource_usage
    );

private:
    std::shared_ptr<themis::FieldEncryption> enc_;
    std::shared_ptr<VCCPKIClient> pki_;
    AuditLoggerConfig cfg_;

    mutable std::mutex file_mu_;
    
    // Hash chain state (for tamper-proofing)
    std::string last_hash_;
    uint64_t entry_count_ = 0;
    std::chrono::system_clock::time_point last_timestamp_;
    mutable std::mutex chain_mu_;
    
    // Task scheduler baseline metrics for anomaly detection
    struct TaskBaseline {
        double avg_execution_time_ms = 0.0;
        double stddev_execution_time_ms = 0.0;
        size_t execution_count = 0;
        std::chrono::system_clock::time_point last_execution;
        double avg_frequency_seconds = 0.0; // Average time between executions
    };
    std::map<std::string, TaskBaseline> task_baselines_;
    mutable std::mutex baselines_mu_;

    static std::vector<uint8_t> sha256(const std::vector<uint8_t>& data);
    void appendJsonLine(const nlohmann::json& j);
    void forwardToSiem(const nlohmann::json& event);
    void loadChainState();
    void saveChainState();
    std::string computeEntryHash(const nlohmann::json& entry) const;
    static std::string securityEventTypeToString(SecurityEventType type);
    
    // SIEM format converters
    std::string formatAsJson(const nlohmann::json& event) const;
    std::string formatAsCef(const nlohmann::json& event, SecurityEventType event_type) const;
    std::string formatAsSyslog(const nlohmann::json& event, SecurityEventType event_type) const;
    
    // Anomaly detection helpers
    void updateTaskBaseline(const std::string& task_id, double execution_time_ms);
    double calculateZScore(double value, double mean, double stddev) const;
};

} // namespace utils
} // namespace themis
