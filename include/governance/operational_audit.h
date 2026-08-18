/**
 * @file operational_audit.h
 * @brief Operational audit and evidence collection for governance module
 * @version 0.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Critical Path 5: Operational Audit & Evidence Collection
 * 
 * Provides:
 * - Operational event logging with structured schema
 * - Correlation IDs for tracking related events across modules
 * - Event tracing and causality tracking
 * - Automated compliance evidence collection
 * - Evidence linking to compliance requirements
 * - Performance monitoring (target: <5% overhead)
 * 
 * Test Gates:
 * - GOV-Observ-01 to GOV-Observ-06: Operational audit validation
 * - Benchmark: Event logging overhead <5% of operation latency
 * - Benchmark: Correlation queries ≤100ms for typical timeframes
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <memory>
#include <mutex>
#include <chrono>
#include <optional>
#include <nlohmann/json.hpp>
#include "themis/export.h"

namespace themis::governance {

// ============================================================================
// Event Schema and Correlation
// ============================================================================

/**
 * @enum OperationalEventType
 * @brief Classification of operational events
 */
enum class OperationalEventType : int32_t {
    // Policy evaluation events
    POLICY_EVALUATION_PERMIT      = 5010,  // Policy evaluation: permit
    POLICY_EVALUATION_DENY        = 5011,  // Policy evaluation: deny
    POLICY_EVALUATION_ERROR       = 5012,  // Policy evaluation: error
    
    // Compliance check events
    COMPLIANCE_CHECK_PASS         = 5020,  // Compliance check: pass
    COMPLIANCE_CHECK_FAIL         = 5021,  // Compliance check: fail
    COMPLIANCE_CHECK_ERROR        = 5022,  // Compliance check: error
    
    // Data governance operations
    DATA_MASKING_APPLIED          = 5030,  // Data masking applied
    DATA_LINEAGE_RECORDED         = 5031,  // Data lineage recorded
    DATA_LINEAGE_ERROR            = 5032,  // Data lineage error
    
    // Policy lifecycle events
    POLICY_CREATED                = 5040,  // Policy created
    POLICY_UPDATED                = 5041,  // Policy updated
    POLICY_DELETED                = 5042,  // Policy deleted
    POLICY_ACTIVATED              = 5043,  // Policy activated
    POLICY_DEPRECATED             = 5044,  // Policy deprecated
    
    // Access control events
    ACCESS_GRANTED                = 5050,  // Access granted
    ACCESS_DENIED                 = 5051,  // Access denied
    ACCESS_REVOKED                = 5052,  // Access revoked
    
    // System events
    AUDIT_LOG_FAILURE             = 5060,  // Audit logging failed
    CORRELATION_CREATED           = 5061,  // Correlation ID created
    EVIDENCE_COLLECTED            = 5062,  // Evidence collected
};

/**
 * @struct OperationalEvent
 * @brief Structured operational event for audit logging
 */
struct THEMIS_SECURITY_API OperationalEvent {
    // Identifiers and timing
    std::string event_id;                     // Unique event identifier (UUID)
    std::string correlation_id;               // Correlation ID for related events
    std::string causality_parent_id;          // Parent event that triggered this
    OperationalEventType event_type;          // Type of event
    
    // Temporal information
    int64_t timestamp_ms = 0;                 // When event occurred (Unix milliseconds)
    int64_t sequence_number = 0;              // Sequential ordering (for causality)
    
    // Actor and context
    std::string actor_id;                     // User/service performing action
    std::string actor_type;                   // "human", "service", "system"
    std::string module_name;                  // Module emitting event (e.g., "policy_engine")
    std::string operation_name;               // Operation being audited
    
    // Event details
    std::string resource_id;                  // Resource being operated on
    std::string resource_type;                // Type of resource (policy, rule, data, etc.)
    std::string action;                       // Action taken (read, write, evaluate, etc.)
    std::string result;                       // Result: "success", "failure", "error", "unknown"
    
    // Classification and compliance
    std::string classification;               // Data classification (offen, vs-nfd, geheim, etc.)
    std::vector<std::string> compliance_tags; // Linked compliance requirements
    
    // Performance metrics
    int64_t operation_duration_us = 0;        // Operation duration in microseconds
    int64_t logging_duration_us = 0;          // Time spent logging this event
    
    // Additional context
    std::unordered_map<std::string, std::string> context; // Key-value context
    std::string error_message;                // Error details if result == "error"
    
    // Payload and evidence
    std::string event_payload;                // JSON-serialized event details
    std::vector<std::string> evidence_ids;    // IDs of collected evidence items
    
    /**
     * @brief Serialize to JSON
     */
    nlohmann::json toJson() const;
    
    /**
     * @brief Deserialize from JSON
     */
    static OperationalEvent fromJson(const nlohmann::json& j);
};

/**
 * @struct ComplianceEvidence
 * @brief Evidence collected for compliance demonstrations
 */
struct THEMIS_SECURITY_API ComplianceEvidence {
    std::string evidence_id;                  // Unique evidence identifier
    std::string event_id;                     // Associated operational event
    std::string compliance_requirement;       // EU AI Act §13, SOC 2 CC, ISO 27001 A.1, etc.
    std::string evidence_type;                // "audit_log", "policy_snapshot", "approval", etc.
    int64_t collected_at_ms = 0;             // When evidence was collected
    int64_t evidence_timestamp_ms = 0;        // What the evidence proves (timestamp)
    
    // Evidence content
    std::string evidence_description;         // Human-readable description
    std::string evidence_content;             // Actual evidence (JSON or serialized)
    std::string fingerprint;                  // Hash of evidence for integrity
    
    // Linkage and retention
    std::vector<std::string> related_events;  // Other related events
    bool is_retained = true;                  // Whether to retain long-term
    int64_t retention_until_ms = 0;          // Retention deadline
    
    /**
     * @brief Serialize to JSON
     */
    nlohmann::json toJson() const;
    
    /**
     * @brief Deserialize from JSON
     */
    static ComplianceEvidence fromJson(const nlohmann::json& j);
};

// ============================================================================
// Event Logging and Correlation
// ============================================================================

/**
 * @class OperationalAuditLogger
 * @brief Thread-safe operational event logging with correlation tracking
 * 
 * Records all governance operations with:
 * - Structured event schema
 * - Correlation IDs for related events
 * - Causality tracking (parent/child relationships)
 * - Performance overhead monitoring (<5% target)
 * - Compliance evidence collection
 */
class THEMIS_SECURITY_API OperationalAuditLogger {
public:
    /**
     * @brief Create audit logger
     * @param max_events Maximum events to retain in memory (default: 100,000)
     */
    explicit OperationalAuditLogger(size_t max_events = 100000);
    ~OperationalAuditLogger();
    
    // Non-copyable, non-movable
    OperationalAuditLogger(const OperationalAuditLogger&) = delete;
    OperationalAuditLogger& operator=(const OperationalAuditLogger&) = delete;
    OperationalAuditLogger(OperationalAuditLogger&&) = delete;
    OperationalAuditLogger& operator=(OperationalAuditLogger&&) = delete;
    
    /**
     * @brief Log an operational event
     * 
     * Thread-safe. Automatically populates event_id, timestamp_ms, and
     * sequence_number if not already set.
     * 
     * @param event Event to log
     * @return The logged event (with auto-populated fields)
     */
    OperationalEvent logEvent(const OperationalEvent& event);
    
    /**
     * @brief Log a policy evaluation event
     * 
     * Convenience method for policy evaluation decisions.
     * 
     * @param correlation_id Correlation ID for related events
     * @param policy_id Policy being evaluated
     * @param decision "permit" or "deny"
     * @param actor_id User performing evaluation
     * @param context Additional context
     * @return The logged event
     */
    OperationalEvent logPolicyEvaluation(
        const std::string& correlation_id,
        const std::string& policy_id,
        const std::string& decision,
        const std::string& actor_id,
        const std::unordered_map<std::string, std::string>& context = {}
    );
    
    /**
     * @brief Log a compliance check event
     * 
     * @param correlation_id Correlation ID
     * @param check_name Name of compliance check
     * @param result "pass" or "fail"
     * @param actor_id User performing check
     * @param context Additional context
     * @return The logged event
     */
    OperationalEvent logComplianceCheck(
        const std::string& correlation_id,
        const std::string& check_name,
        const std::string& result,
        const std::string& actor_id,
        const std::unordered_map<std::string, std::string>& context = {}
    );
    
    /**
     * @brief Log a data governance operation
     * 
     * @param correlation_id Correlation ID
     * @param operation_name Name of operation (e.g., "data_masking", "lineage_update")
     * @param resource_id Resource being operated on
     * @param success true if successful
     * @param actor_id Actor performing operation
     * @param context Additional context
     * @return The logged event
     */
    OperationalEvent logDataGovernanceOp(
        const std::string& correlation_id,
        const std::string& operation_name,
        const std::string& resource_id,
        bool success,
        const std::string& actor_id,
        const std::unordered_map<std::string, std::string>& context = {}
    );
    
    /**
     * @brief Log a policy lifecycle event
     * 
     * @param correlation_id Correlation ID
     * @param policy_id Policy being operated on
     * @param lifecycle_event "create", "update", "delete", "activate", "deprecate"
     * @param actor_id Actor performing operation
     * @param context Additional context
     * @return The logged event
     */
    OperationalEvent logPolicyLifecycle(
        const std::string& correlation_id,
        const std::string& policy_id,
        const std::string& lifecycle_event,
        const std::string& actor_id,
        const std::unordered_map<std::string, std::string>& context = {}
    );
    
    /**
     * @brief Link a parent event to a child event (for causality tracking)
     * 
     * @param parent_event_id Event that triggered the child event
     * @param child_event_id Event that was triggered
     */
    void linkCausalityRelationship(
        const std::string& parent_event_id,
        const std::string& child_event_id
    );
    
    /**
     * @brief Get event by ID
     * 
     * @param event_id Event identifier
     * @return Event if found, nullopt otherwise
     */
    std::optional<OperationalEvent> getEventById(const std::string& event_id) const;
    
    /**
     * @brief Query events by correlation ID
     * 
     * Returns all events in a correlation group.
     * 
     * @param correlation_id Correlation ID
     * @return Vector of events (ordered by sequence number)
     */
    std::vector<OperationalEvent> queryEventsByCorrelationId(
        const std::string& correlation_id
    ) const;
    
    /**
     * @brief Query events in time range
     * 
     * @param start_ms Start time (Unix milliseconds, 0 = no lower bound)
     * @param end_ms End time (Unix milliseconds, 0 = no upper bound)
     * @param event_type Optional: filter by event type
     * @return Matching events (ordered by timestamp)
     */
    std::vector<OperationalEvent> queryEventsByTimeRange(
        int64_t start_ms,
        int64_t end_ms,
        const std::optional<OperationalEventType>& event_type = std::nullopt
    ) const;
    
    /**
     * @brief Query events by actor
     * 
     * @param actor_id Actor ID to query
     * @return Events performed by actor (ordered by timestamp, most recent first)
     */
    std::vector<OperationalEvent> queryEventsByActor(
        const std::string& actor_id
    ) const;
    
    /**
     * @brief Query events by module
     * 
     * @param module_name Module name (e.g., "policy_engine")
     * @return Events from module (ordered by timestamp)
     */
    std::vector<OperationalEvent> queryEventsByModule(
        const std::string& module_name
    ) const;
    
    /**
     * @brief Query events by resource
     * 
     * @param resource_id Resource ID
     * @return Events affecting resource (ordered by timestamp)
     */
    std::vector<OperationalEvent> queryEventsByResource(
        const std::string& resource_id
    ) const;
    
    /**
     * @brief Get causality chain for an event
     * 
     * Returns parent event, grandparent, etc. leading up to this event.
     * 
     * @param event_id Event to trace
     * @return Vector of events in causality chain (most recent to oldest)
     */
    std::vector<OperationalEvent> getCausalityChain(
        const std::string& event_id
    ) const;
    
    /**
     * @brief Get events triggered by a parent event
     * 
     * @param parent_event_id Parent event ID
     * @return All events where causality_parent_id == parent_event_id
     */
    std::vector<OperationalEvent> getTriggeredEvents(
        const std::string& parent_event_id
    ) const;
    
    /**
     * @brief Export events as JSON
     * 
     * @param start_ms Optional: filter by start time
     * @param end_ms Optional: filter by end time
     * @param event_type Optional: filter by type
     * @param compress Whether to compress output (default: false)
     * @return JSON array of events
     */
    nlohmann::json exportEvents(
        int64_t start_ms = 0,
        int64_t end_ms = 0,
        const std::optional<OperationalEventType>& event_type = std::nullopt,
        bool compress = false
    ) const;
    
    /**
     * @brief Get event statistics
     * 
     * @return JSON with event counts by type, module, result, etc.
     */
    nlohmann::json getEventStatistics() const;
    
    /**
     * @brief Get performance metrics
     * 
     * @return JSON with logging overhead, latencies, etc.
     */
    nlohmann::json getPerformanceMetrics() const;
    
    /**
     * @brief Get total event count
     */
    size_t getTotalEventCount() const;
    
    /**
     * @brief Clear all logged events (for testing)
     */
    void clear();

private:
    mutable std::mutex mutex_;
    std::vector<OperationalEvent> events_;
    size_t max_events_;
    
    // Performance tracking
    struct PerformanceMetrics {
        std::vector<int64_t> logging_times_us;
        std::vector<int64_t> query_times_us;
        int64_t total_operations = 0;
        int64_t total_query_operations = 0;
    } metrics_;
    
    // Causality tracking (event_id -> vector of child event_ids)
    std::unordered_map<std::string, std::vector<std::string>> causality_map_;
    
    /**
     * @brief Generate unique event ID (UUID)
     */
    std::string generateEventId() const;
    
    /**
     * @brief Get next sequence number
     */
    int64_t getNextSequenceNumber() const;
};

// ============================================================================
// Event Correlation and Aggregation
// ============================================================================

/**
 * @struct CorrelationGroup
 * @brief Group of correlated events
 */
struct THEMIS_SECURITY_API CorrelationGroup {
    std::string correlation_id;               // Correlation ID for this group
    int64_t created_at_ms = 0;               // When group was created
    int64_t last_updated_ms = 0;             // When last event was added
    std::vector<std::string> event_ids;       // Event IDs in this group
    std::vector<std::string> causality_chain; // Causality ordering
    
    /**
     * @brief Serialize to JSON
     */
    nlohmann::json toJson() const;
};

/**
 * @class EventCorrelationEngine
 * @brief Links related events across modules and time boundaries
 * 
 * Features:
 * - Automatic correlation ID propagation
 * - Causality tracking (which event triggered which)
 * - Time-range correlation queries
 * - Event aggregation and batching
 * - Performance-optimized correlation lookup
 */
class THEMIS_SECURITY_API EventCorrelationEngine {
public:
    /**
     * @brief Create correlation engine
     * @param audit_logger Audit logger to correlate
     */
    explicit EventCorrelationEngine(
        std::shared_ptr<OperationalAuditLogger> audit_logger
    );
    
    ~EventCorrelationEngine();
    
    // Non-copyable
    EventCorrelationEngine(const EventCorrelationEngine&) = delete;
    EventCorrelationEngine& operator=(const EventCorrelationEngine&) = delete;
    
    /**
     * @brief Create new correlation group
     * 
     * @param operation_name Name of operation being tracked
     * @param actor_id Actor performing operation
     * @param initial_context Optional context
     * @return Correlation ID for new group
     */
    std::string createCorrelation(
        const std::string& operation_name,
        const std::string& actor_id,
        const std::unordered_map<std::string, std::string>& initial_context = {}
    );
    
    /**
     * @brief Get correlation group
     * 
     * @param correlation_id Correlation ID
     * @return CorrelationGroup if found
     */
    std::optional<CorrelationGroup> getCorrelationGroup(
        const std::string& correlation_id
    ) const;
    
    /**
     * @brief Query correlations by time range
     * 
     * Returns all correlation groups that had activity in the time range.
     * 
     * @param start_ms Start time (Unix milliseconds)
     * @param end_ms End time (Unix milliseconds)
     * @return Matching correlation groups
     */
    std::vector<CorrelationGroup> queryCorrelationsByTimeRange(
        int64_t start_ms,
        int64_t end_ms
    ) const;
    
    /**
     * @brief Query correlations by actor
     * 
     * @param actor_id Actor ID
     * @return Correlation groups initiated by actor
     */
    std::vector<CorrelationGroup> queryCorrelationsByActor(
        const std::string& actor_id
    ) const;
    
    /**
     * @brief Get correlation latency (time from first to last event)
     * 
     * @param correlation_id Correlation ID
     * @return Latency in milliseconds, 0 if not found
     */
    int64_t getCorrelationLatency(const std::string& correlation_id) const;
    
    /**
     * @brief Get statistics on correlation latencies
     * 
     * @return JSON with p50, p95, p99 latencies
     */
    nlohmann::json getCorrelationLatencyStats() const;
    
    /**
     * @brief Export correlation data
     * 
     * @param correlation_id Optional: export single correlation
     * @return JSON export
     */
    nlohmann::json exportCorrelations(
        const std::optional<std::string>& correlation_id = std::nullopt
    ) const;

private:
    mutable std::mutex mutex_;
    std::shared_ptr<OperationalAuditLogger> audit_logger_;
    
    // Correlation groups by ID
    std::unordered_map<std::string, CorrelationGroup> correlations_;
    
    // Actor -> correlation IDs for quick lookup
    std::unordered_map<std::string, std::vector<std::string>> actor_correlations_;
    
    // Time-based index for range queries
    std::vector<std::pair<int64_t, std::string>> timeline_;
    
    /**
     * @brief Generate correlation ID (UUID)
     */
    std::string generateCorrelationId() const;
};

// ============================================================================
// Compliance Evidence Collection
// ============================================================================

/**
 * @class ComplianceEvidenceCollector
 * @brief Automated evidence collection for compliance demonstrations
 * 
 * Features:
 * - Links evidence to compliance requirements (EU AI Act, SOC 2, ISO 27001)
 * - Automated evidence collection from operational events
 * - Evidence fingerprinting for integrity verification
 * - Evidence retention policy enforcement
 * - Export for regulatory audits
 */
class THEMIS_SECURITY_API ComplianceEvidenceCollector {
public:
    /**
     * @brief Create evidence collector
     * @param audit_logger Audit logger to collect evidence from
     */
    explicit ComplianceEvidenceCollector(
        std::shared_ptr<OperationalAuditLogger> audit_logger
    );
    
    ~ComplianceEvidenceCollector();
    
    // Non-copyable
    ComplianceEvidenceCollector(const ComplianceEvidenceCollector&) = delete;
    ComplianceEvidenceCollector& operator=(const ComplianceEvidenceCollector&) = delete;
    
    /**
     * @brief Collect evidence for a compliance requirement
     * 
     * Automatically collects evidence from related events based on
     * compliance requirement type.
     * 
     * @param requirement Compliance requirement (e.g., "EU_AI_ACT_13", "SOC2_CC7.2")
     * @param actor_id Actor initiating collection
     * @param context Optional additional context
     * @return Collected evidence items
     */
    std::vector<ComplianceEvidence> collectEvidence(
        const std::string& requirement,
        const std::string& actor_id,
        const std::unordered_map<std::string, std::string>& context = {}
    );
    
    /**
     * @brief Record manually collected evidence
     * 
     * @param evidence Evidence item to record
     * @return The recorded evidence (with auto-populated fields)
     */
    ComplianceEvidence recordEvidence(const ComplianceEvidence& evidence);
    
    /**
     * @brief Link evidence to an event
     * 
     * @param evidence_id Evidence ID
     * @param event_id Event ID to link
     */
    void linkEvidenceToEvent(
        const std::string& evidence_id,
        const std::string& event_id
    );
    
    /**
     * @brief Link evidence to multiple events
     * 
     * @param evidence_id Evidence ID
     * @param event_ids Event IDs to link
     */
    void linkEvidenceToEvents(
        const std::string& evidence_id,
        const std::vector<std::string>& event_ids
    );
    
    /**
     * @brief Get evidence by requirement
     * 
     * @param requirement Compliance requirement
     * @return Evidence items for this requirement
     */
    std::vector<ComplianceEvidence> getEvidenceByRequirement(
        const std::string& requirement
    ) const;
    
    /**
     * @brief Get evidence by event
     * 
     * @param event_id Event ID
     * @return Evidence items linked to this event
     */
    std::vector<ComplianceEvidence> getEvidenceByEvent(
        const std::string& event_id
    ) const;
    
    /**
     * @brief Get evidence by time range
     * 
     * @param start_ms Start time (Unix milliseconds)
     * @param end_ms End time (Unix milliseconds)
     * @return Evidence items collected in range
     */
    std::vector<ComplianceEvidence> getEvidenceByTimeRange(
        int64_t start_ms,
        int64_t end_ms
    ) const;
    
    /**
     * @brief Export evidence for audit
     * 
     * Returns evidence in structured format suitable for regulatory submission.
     * 
     * @param requirement Optional: filter by requirement
     * @param start_ms Optional: filter by start time
     * @param end_ms Optional: filter by end time
     * @return JSON export
     */
    nlohmann::json exportEvidenceForAudit(
        const std::optional<std::string>& requirement = std::nullopt,
        int64_t start_ms = 0,
        int64_t end_ms = 0
    ) const;
    
    /**
     * @brief Generate evidence report
     * 
     * Creates compliance report with summary of evidence collected.
     * 
     * @param requirements Optional: specific requirements to report on
     * @return JSON report
     */
    nlohmann::json generateEvidenceReport(
        const std::vector<std::string>& requirements = {}
    ) const;
    
    /**
     * @brief Get evidence statistics
     * 
     * @return JSON with counts by requirement, retention status, etc.
     */
    nlohmann::json getEvidenceStatistics() const;
    
    /**
     * @brief Get total evidence count
     */
    size_t getTotalEvidenceCount() const;
    
    /**
     * @brief Clear all evidence (for testing)
     */
    void clear();

private:
    mutable std::mutex mutex_;
    std::shared_ptr<OperationalAuditLogger> audit_logger_;
    std::vector<ComplianceEvidence> evidence_;
    
    // Evidence ID -> event IDs for linking
    std::unordered_map<std::string, std::vector<std::string>> evidence_event_links_;
    
    // Requirement -> evidence IDs
    std::unordered_map<std::string, std::vector<std::string>> requirement_evidence_map_;
    
    /**
     * @brief Generate evidence ID (UUID)
     */
    std::string generateEvidenceId() const;
    
    /**
     * @brief Compute fingerprint of evidence for integrity
     */
    std::string computeEvidenceFingerprint(const std::string& content) const;
};

/**
 * @brief Get process-global operational audit logger
 */
THEMIS_SECURITY_API OperationalAuditLogger& getGlobalAuditLogger();

/**
 * @brief Get process-global event correlation engine
 */
THEMIS_SECURITY_API EventCorrelationEngine& getGlobalCorrelationEngine();

/**
 * @brief Get process-global evidence collector
 */
THEMIS_SECURITY_API ComplianceEvidenceCollector& getGlobalEvidenceCollector();

} // namespace themis::governance
