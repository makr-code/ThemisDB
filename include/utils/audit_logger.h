/**
 * @file audit_logger.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: audit_logger.h | Version: 0.0.47
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
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
    EXPORT_DENIED,          ///< Export request rejected by PolicyEngine (EXP-001)
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
    
    // Sharding Events
    SHARD_SPLIT,            ///< A hot shard was split into two shards (load-based splitting)
    SHARD_MERGE,            ///< Two shards were merged into one (load-based merging)
    SHARD_LIVE_MIGRATION_STARTED,  ///< Dual-write live migration initiated
    SHARD_LIVE_MIGRATION_COMPLETED, ///< Dual-write live migration completed with atomic cutover
    SHARD_LIVE_MIGRATION_FAILED,   ///< Dual-write live migration failed

    // AI Safety Layer Events (ASL-12)
    // Docs: docs/de/security/ai_safety/AI_SAFETY_AUDIT_TRAIL.md
    AI_TOOL_CALL,              ///< AI agent invoked an MCP tool
    AI_APPROVAL_REQUIRED,      ///< HILG gate held op for operator approval
    AI_OPERATION_EXECUTED,     ///< Operator approved; operation executed
    AI_SNAPSHOT_CREATED,       ///< Pre-operation snapshot created (ASL-8)
    AI_OPERATION_DENIED,       ///< Operator denied a pending operation
    AI_OPERATION_EXPIRED,      ///< Pending approval TTL expired without action
    AI_ROLLBACK_EXECUTED,      ///< Operator triggered DB rollback to snapshot (ASL-10)
    AI_CLEANUP_EXECUTED,       ///< Snapshot cleanup job ran (ASL-11)

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

    // Loss-protection settings
    bool enable_fsync = false;          ///< fdatasync after each write for crash durability
    uint64_t max_file_size_bytes = 0;   ///< rotate when file reaches this size (0 = disabled)
    size_t max_rotated_files = 5;       ///< number of rotated log files to keep
    std::string secondary_log_path;     ///< mirror path for redundancy (empty = disabled)
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
    /**
     * @brief Log an audit event with automatic formatting and chaining
     * 
     * Logs an event to the audit trail. If hash chaining is enabled, the event 
     * is cryptographically linked to the previous event. If encryption is enabled,
     * the event is encrypted before writing.
     * 
     * @param event JSON object containing event data
     * 
     * @return void (failure is logged, not signaled - see @error_contract below)
     * 
     * @error_contract
     * - If buffer would overflow: logs ERR_AUDIT_BUFFER_OVERFLOW and truncates event
     * - If write fails: logs ERR_AUDIT_LOG_WRITE_FAILED and retries with fallback to stderr
     * - If serialization fails: logs ERR_AUDIT_SERIALIZATION_FAILED and uses simplified format
     * - If encryption fails: logs ERR_AUDIT_SERVICE_DEGRADED and switches to unencrypted mode
     * - Service degradation: if external audit service unreachable, continues with local logging
     * 
     * @bounded_resources
     * - Buffer capacity: cfg.max_buffer_size (default: 1GB)
     * - Event size: Individual events capped at 10MB
     * - Queue depth: max cfg.max_queued_events entries
     * 
     * @thread_safety Thread-safe via internal mutex (file_mu_)
     * @performance O(n) where n is event JSON size; async batch writes if configured
     * 
     * @see ErrorCode for error taxonomy
     * @see logSecurityEvent() for security-specific event logging
     */
    void logEvent(const nlohmann::json& event);
     
    /**
     * @brief Log a security event with structured data
     * 
     * Logs a high-level security event (authentication, authorization, key management, etc.)
     * with automatic timestamp, user context, and resource tagging.
     * 
     * @param event_type Security event type (enum)
     * @param user_id User/service account performing the action
     * @param resource Resource being accessed (e.g., entity key, API endpoint)
     * @param details Additional event-specific details (optional)
     * 
     * @return void (see @error_contract below)
     * 
     * @error_contract
     * - If event_type invalid: logs ERR_AUDIT_FORMAT_INVALID and skips event
     * - If user_id or resource empty: logs ERR_AUDIT_SERIALIZATION_FAILED with warning
     * - If details JSON too large: logs ERR_AUDIT_BUFFER_OVERFLOW and truncates
     * - All error paths fall through to logEvent() error handling
     * 
     * @thread_safety Thread-safe
     * @performance O(1) for event type resolution; delegates to logEvent() for write
     * 
     * @see SecurityEventType for valid event types
     * @see logEvent() for write operation details
     */
    void logSecurityEvent(
        SecurityEventType event_type,
        const std::string& user_id,
        const std::string& resource,
        const nlohmann::json& details = {}
    );
     
    /**
     * @brief Verify integrity of audit log hash chain
     * 
     * If hash chaining is enabled, verifies that the cryptographic chain linking
     * audit events is intact. Detects tampering or log rotation issues.
     * 
     * @return true if chain is valid or chaining disabled; false if tampering detected
     * 
     * @error_contract
     * - If chain file missing/unreadable: returns false (logs ERR_AUDIT_LOG_WRITE_FAILED)
     * - If hash verification fails: returns false (logs ERR_AUDIT_SERVICE_DEGRADED warning)
     * - If internal error: returns false (logs ERR_AUDIT_CLEANUP_FAILED context)
     * 
     * @thread_safety Thread-safe
     * @performance O(n) where n is log file size; consider caching in production
     * 
     * @note Should be called before critical operations to detect tampering
     * @see ErrorCode::ERR_AUDIT_ROTATION_FAILED for related rotation errors
     */
    bool verifyChainIntegrity();
     
    /**
     * @brief Flush audit log to disk
     * 
     * Flushes pending audit events to disk. If fsync is enabled in config,
     * ensures data is durably written to storage.
     * 
     * @return void (see @error_contract below)
     * 
     * @error_contract
     * - If disk full: logs ERR_AUDIT_DISK_FULL and triggers rotation/cleanup
     * - If write permission denied: logs ERR_AUDIT_PERMISSION_DENIED
     * - If fsync fails: logs ERR_AUDIT_SERVICE_DEGRADED (non-fatal)
     * - All failures logged but flush continues with what it can
     * 
     * @thread_safety Thread-safe
     * @performance O(pending_events); blocks until flush complete if fsync enabled
     * 
     * @note Critical for audit durability; should be called periodically
     * @see ErrorCode for diagnostics
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
    void rotateLogIfNeeded(); ///< rotate primary log when max_file_size_bytes is reached (file_mu_ must be held)
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

// ---------------------------------------------------------------------------
// HashChainAuditWriter
// ---------------------------------------------------------------------------

/**
 * @brief Configuration for HashChainAuditWriter.
 */
struct HashChainAuditWriterConfig {
    std::string log_path        = "data/logs/audit_chain.jsonl";
    std::string chain_head_path = "data/logs/audit_chain_head.bin";
    bool        fsync_on_write  = true;   ///< fdatasync the head file after each write
    uint64_t    checkpoint_interval = 1;  ///< persist chain head every N writes (1 = every write)
};

/**
 * @brief Standalone tamper-evident audit record writer with SHA-256 hash chain.
 *
 * Each entry is stored as a JSON line with two extra fields injected:
 *   "chain_seq"   — monotonically-increasing sequence number (uint64)
 *   "prev_hash"   — SHA-256 hex digest of the previous log record's hash input
 *
 * The running chain head is persisted to `chain_head_path` after every write so
 * that AuditLogVerifier::verify_chain() can verify from any checkpoint without
 * replaying the whole log.
 *
 * Initial chain seed: SHA-256 of a caller-supplied seed string (typically
 * derived via HKDF from the cluster root key so chain heads are cluster-specific).
 *
 * v1.5.0: Initial implementation (Phase 3 FUTURE_ENHANCEMENTS.md).
 */
class HashChainAuditWriter {
public:
    /**
     * @param cfg        Writer configuration.
     * @param chain_seed Hex string used to initialise the genesis hash.
     *                   Should be HKDF-derived from the cluster root key.
     *                   Defaults to 64 zeros (all-zero genesis) when empty.
     */
    explicit HashChainAuditWriter(HashChainAuditWriterConfig cfg = {},
                                  const std::string& chain_seed = "");

    ~HashChainAuditWriter();

    /**
     * @brief Append a JSON record to the chain log.
     *
     * Injects `chain_seq` and `prev_hash` into @p record, writes the
     * augmented record as a JSON line, then updates and persists the chain head.
     *
     * Thread-safe.
     */
    void write(nlohmann::json record);

    /**
     * @brief Returns the current chain head hash (hex-SHA-256).
     */
    std::string headHash() const;

    /**
     * @brief Returns the number of entries written since construction.
     */
    uint64_t sequenceNumber() const;

private:
    HashChainAuditWriterConfig cfg_;
    mutable std::mutex         mu_;
    std::ofstream              log_stream_;
    std::fstream               chain_head_stream_;
    std::string                last_hash_;
    uint64_t                   seq_{0};

    static std::vector<uint8_t> sha256(const std::vector<uint8_t>& data);
    static std::string bytesToHex(const std::vector<uint8_t>& data);

    void saveChainHead();
    void loadOrInitChainHead(const std::string& chain_seed);
};

// ---------------------------------------------------------------------------
// AuditLogVerifier
// ---------------------------------------------------------------------------

/**
 * @brief Result of a hash-chain verification pass.
 */
struct AuditVerifyResult {
    bool     ok            = true;     ///< true when chain is intact
    uint64_t entries_ok    = 0;        ///< number of entries with valid chain links
    uint64_t entries_total = 0;        ///< total non-empty lines inspected
    uint64_t first_bad_seq = UINT64_MAX; ///< sequence number of first tampered entry (if any)
    std::string error_message;         ///< human-readable description of first failure
};

/**
 * @brief Standalone audit log chain verifier.
 *
 * Replays the hash chain in a log file written by HashChainAuditWriter and
 * reports the first entry where the `prev_hash` field does not match the
 * expected running hash.
 *
 * Usage:
 * @code
 *   AuditLogVerifier verifier;
 *   auto result = verifier.verify_chain("data/logs/audit_chain.jsonl");
 *   if (!result.ok) { ... }
 * @endcode
 *
 * v1.5.0: Initial implementation (Phase 3 FUTURE_ENHANCEMENTS.md).
 */
class AuditLogVerifier {
public:
    AuditLogVerifier() = default;

    /**
     * @brief Replay the hash chain in @p log_path and verify each link.
     *
     * @param log_path      Path to the JSONL log file.
     * @param genesis_hash  Expected genesis hash (64-char hex).  Use the
     *                      all-zero default when the log was written with
     *                      the default chain seed.
     * @return Verification result struct.
     */
    AuditVerifyResult verify_chain(const std::string& log_path,
                                   const std::string& genesis_hash = std::string(64, '0')) const;

private:
    static std::string computeEntryHash(const std::string& prev_hash,
                                        const nlohmann::json& entry);
};

} // namespace utils
} // namespace themis
