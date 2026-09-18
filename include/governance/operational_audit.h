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
     * @brief To Json.
     * @return Return value.
     */
    nlohmann::json toJson() const;
    
    /**
     * @brief From Json.
     * @param[in] j Input parameter.
     * @return Return value.
     */
    static OperationalEvent fromJson(const nlohmann::json& j);
};

struct THEMIS_SECURITY_API ComplianceEvidence {
    std::string evidence_id;                  // Unique evidence identifier
    std::string requirement_id;               // Associated compliance requirement
    std::string requirement_type;             // Requirement category (e.g., EU_AI_ACT_13)
    std::string evidence_type;                // "audit_log", "policy_snapshot", "approval", etc.
    int64_t collected_at_ms = 0;              // When evidence was collected
    std::string description;                  // Human-readable description
    std::string source_event_id;              // Associated operational event
    std::string fingerprint;                  // Hash of evidence for integrity
    std::string data_summary;                 // Actual evidence summary or JSON payload
    int64_t retention_until_ms = 0;           // Retention deadline
    std::string audit_classification;         // Classification (REGULATORY, EU_REGULATED, etc.)
    nlohmann::json metadata;                  // Additional metadata

    // Compatibility aliases for older callers/tests
    std::string event_id;                     // Associated operational event
    std::string compliance_requirement;       // Legacy alias for requirement_id
    int64_t evidence_timestamp_ms = 0;        // Legacy alias for collected_at_ms
    std::string evidence_description;         // Legacy alias for description
    std::string evidence_content;             // Legacy alias for data_summary
    std::vector<std::string> related_events;  // Other related events
    bool is_retained = true;                  // Whether to retain long-term
    
    /**
     * @brief To Json.
     * @return Return value.
     */
    nlohmann::json toJson() const;
    
    /**
     * @brief From Json.
     * @param[in] j Input parameter.
     * @return Return value.
     */
    static ComplianceEvidence fromJson(const nlohmann::json& j);
};

// ============================================================================
// Event Logging and Correlation
// ============================================================================

class THEMIS_SECURITY_API OperationalAuditLogger {
public:
    explicit OperationalAuditLogger(size_t max_events = 100000);
    ~OperationalAuditLogger();
    
    // Non-copyable, non-movable
    OperationalAuditLogger(const OperationalAuditLogger&) = delete;
    OperationalAuditLogger& operator=(const OperationalAuditLogger&) = delete;
    OperationalAuditLogger(OperationalAuditLogger&&) = delete;
    OperationalAuditLogger& operator=(OperationalAuditLogger&&) = delete;
    
    /**
     * @brief Log Event.
     * @param[in] event Input parameter.
     * @return Return value.
     */
    OperationalEvent logEvent(const OperationalEvent& event);
    
    OperationalEvent logPolicyEvaluation(
        const std::string& correlation_id,
        const std::string& policy_id,
        const std::string& decision,
        const std::string& actor_id,
        const std::unordered_map<std::string, std::string>& context = {}
    );
    
    OperationalEvent logComplianceCheck(
        const std::string& correlation_id,
        const std::string& check_name,
        const std::string& result,
        const std::string& actor_id,
        const std::unordered_map<std::string, std::string>& context = {}
    );
    
    OperationalEvent logDataGovernanceOp(
        const std::string& correlation_id,
        const std::string& operation_name,
        const std::string& resource_id,
        bool success,
        const std::string& actor_id,
        const std::unordered_map<std::string, std::string>& context = {}
    );
    
    OperationalEvent logPolicyLifecycle(
        const std::string& correlation_id,
        const std::string& policy_id,
        const std::string& lifecycle_event,
        const std::string& actor_id,
        const std::unordered_map<std::string, std::string>& context = {}
    );
    
    /**
     * @brief Link Causality Relationship.
     * @param[in] parent_event_id Identifier of the parent event.
     * @param[in] child_event_id Identifier of the child event.
     */
    void linkCausalityRelationship(
        const std::string& parent_event_id,
        const std::string& child_event_id
    );
    
    /**
     * @brief Get Event By Id.
     * @param[in] event_id Identifier of the event.
     * @return Return value.
     */
    std::optional<OperationalEvent> getEventById(const std::string& event_id) const;
    
    /**
     * @brief Query Events By Correlation Id.
     * @param[in] correlation_id Identifier of the correlation.
     * @return Return value.
     */
    std::vector<OperationalEvent> queryEventsByCorrelationId(
        const std::string& correlation_id
    ) const;
    
    std::vector<OperationalEvent> queryEventsByTimeRange(
        int64_t start_ms,
        int64_t end_ms,
        const std::optional<OperationalEventType>& event_type = std::nullopt
    ) const;
    
    /**
     * @brief Query Events By Actor.
     * @param[in] actor_id Identifier of the actor.
     * @return Return value.
     */
    std::vector<OperationalEvent> queryEventsByActor(
        const std::string& actor_id
    ) const;
    
    /**
     * @brief Query Events By Module.
     * @param[in] module_name Name of the module.
     * @return Return value.
     */
    std::vector<OperationalEvent> queryEventsByModule(
        const std::string& module_name
    ) const;
    
    /**
     * @brief Query Events By Resource.
     * @param[in] resource_id Identifier of the resource.
     * @return Return value.
     */
    std::vector<OperationalEvent> queryEventsByResource(
        const std::string& resource_id
    ) const;
    
    /**
     * @brief Get Causality Chain.
     * @param[in] event_id Identifier of the event.
     * @return Return value.
     */
    std::vector<OperationalEvent> getCausalityChain(
        const std::string& event_id
    ) const;
    
    /**
     * @brief Get Triggered Events.
     * @param[in] parent_event_id Identifier of the parent event.
     * @return Return value.
     */
    std::vector<OperationalEvent> getTriggeredEvents(
        const std::string& parent_event_id
    ) const;
    
    nlohmann::json exportEvents(
        int64_t start_ms = 0,
        int64_t end_ms = 0,
        const std::optional<OperationalEventType>& event_type = std::nullopt,
        bool compress = false
    ) const;
    
    /**
     * @brief Get Event Statistics.
     * @return Return value.
     */
    nlohmann::json getEventStatistics() const;
    
    /**
     * @brief Get Performance Metrics.
     * @return Return value.
     */
    nlohmann::json getPerformanceMetrics() const;
    
    /**
     * @brief Get Total Event Count.
     * @return Return value.
     */
    size_t getTotalEventCount() const;
    
    /**
     * @brief Clear.
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
     * @brief Generate Event Id.
     * @return Return value.
     */
    std::string generateEventId() const;
    
    /**
     * @brief Get Next Sequence Number.
     * @return Return value.
     */
    int64_t getNextSequenceNumber() const;
};

// ============================================================================
// Event Correlation and Aggregation
// ============================================================================

struct THEMIS_SECURITY_API CorrelationGroup {
    std::string correlation_id;               // Correlation ID for this group
    int64_t created_at_ms = 0;               // When group was created
    int64_t last_updated_ms = 0;             // When last event was added
    std::vector<std::string> event_ids;       // Event IDs in this group
    std::vector<std::string> causality_chain; // Causality ordering
    
    /**
     * @brief To Json.
     * @return Return value.
     */
    nlohmann::json toJson() const;
};

class THEMIS_SECURITY_API EventCorrelationEngine {
public:
    /**
     * @brief Event Correlation Engine.
     * @param[in] audit_logger Input parameter.
     * @return Return value.
     */
    explicit EventCorrelationEngine(
        std::shared_ptr<OperationalAuditLogger> audit_logger
    );
    
    ~EventCorrelationEngine();
    
    // Non-copyable
    EventCorrelationEngine(const EventCorrelationEngine&) = delete;
    EventCorrelationEngine& operator=(const EventCorrelationEngine&) = delete;
    
    std::string createCorrelation(
        const std::string& operation_name,
        const std::string& actor_id,
        const std::unordered_map<std::string, std::string>& initial_context = {}
    );
    
    /**
     * @brief Get Correlation Group.
     * @param[in] correlation_id Identifier of the correlation.
     * @return Return value.
     */
    std::optional<CorrelationGroup> getCorrelationGroup(
        const std::string& correlation_id
    ) const;
    
    /**
     * @brief Query Correlations By Time Range.
     * @param[in] start_ms Input parameter.
     * @param[in] end_ms Input parameter.
     * @return Return value.
     */
    std::vector<CorrelationGroup> queryCorrelationsByTimeRange(
        int64_t start_ms,
        int64_t end_ms
    ) const;
    
    /**
     * @brief Query Correlations By Actor.
     * @param[in] actor_id Identifier of the actor.
     * @return Return value.
     */
    std::vector<CorrelationGroup> queryCorrelationsByActor(
        const std::string& actor_id
    ) const;
    
    /**
     * @brief Get Correlation Latency.
     * @param[in] correlation_id Identifier of the correlation.
     * @return Return value.
     */
    int64_t getCorrelationLatency(const std::string& correlation_id) const;
    
    /**
     * @brief Get Correlation Latency Stats.
     * @return Return value.
     */
    nlohmann::json getCorrelationLatencyStats() const;
    
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
     * @brief Generate Correlation Id.
     * @return Return value.
     */
    std::string generateCorrelationId() const;
};

// ============================================================================
// Compliance Evidence Collection
// ============================================================================

class THEMIS_SECURITY_API ComplianceEvidenceCollector {
public:
    /**
     * @brief Compliance Evidence Collector.
     * @param[in] audit_logger Input parameter.
     * @return Return value.
     */
    explicit ComplianceEvidenceCollector(
        std::shared_ptr<OperationalAuditLogger> audit_logger
    );
    
    ~ComplianceEvidenceCollector();
    
    // Non-copyable
    ComplianceEvidenceCollector(const ComplianceEvidenceCollector&) = delete;
    ComplianceEvidenceCollector& operator=(const ComplianceEvidenceCollector&) = delete;
    
    std::vector<ComplianceEvidence> collectEvidence(
        const std::string& requirement,
        const std::string& actor_id,
        const std::unordered_map<std::string, std::string>& context = {}
    );
    
    /**
     * @brief Record Evidence.
     * @param[in] evidence Input parameter.
     * @return Return value.
     */
    ComplianceEvidence recordEvidence(const ComplianceEvidence& evidence);
    
    /**
     * @brief Link Evidence To Event.
     * @param[in] evidence_id Identifier of the evidence.
     * @param[in] event_id Identifier of the event.
     */
    void linkEvidenceToEvent(
        const std::string& evidence_id,
        const std::string& event_id
    );
    
    /**
     * @brief Link Evidence To Events.
     * @param[in] evidence_id Identifier of the evidence.
     * @param[in] event_ids Input parameter.
     */
    void linkEvidenceToEvents(
        const std::string& evidence_id,
        const std::vector<std::string>& event_ids
    );
    
    /**
     * @brief Get Evidence By Requirement.
     * @param[in] requirement Input parameter.
     * @return Return value.
     */
    std::vector<ComplianceEvidence> getEvidenceByRequirement(
        const std::string& requirement
    ) const;
    
    /**
     * @brief Get Evidence By Event.
     * @param[in] event_id Identifier of the event.
     * @return Return value.
     */
    std::vector<ComplianceEvidence> getEvidenceByEvent(
        const std::string& event_id
    ) const;
    
    /**
     * @brief Get Evidence By Time Range.
     * @param[in] start_ms Input parameter.
     * @param[in] end_ms Input parameter.
     * @return Return value.
     */
    std::vector<ComplianceEvidence> getEvidenceByTimeRange(
        int64_t start_ms,
        int64_t end_ms
    ) const;
    
    nlohmann::json exportEvidenceForAudit(
        const std::optional<std::string>& requirement = std::nullopt,
        int64_t start_ms = 0,
        int64_t end_ms = 0
    ) const;
    
    nlohmann::json generateEvidenceReport(
        const std::vector<std::string>& requirements = {}
    ) const;
    
    /**
     * @brief Get Evidence Statistics.
     * @return Return value.
     */
    nlohmann::json getEvidenceStatistics() const;
    
    /**
     * @brief Get Total Evidence Count.
     * @return Return value.
     */
    size_t getTotalEvidenceCount() const;
    
    /**
     * @brief Clear.
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
     * @brief Generate Evidence Id.
     * @return Return value.
     */
    std::string generateEvidenceId() const;
    
    /**
     * @brief Compute Evidence Fingerprint.
     * @param[in] content Input parameter.
     * @return Return value.
     */
    std::string computeEvidenceFingerprint(const std::string& content) const;
};

/**
 * @brief Get Global Audit Logger.
 * @return Return value.
 */
THEMIS_SECURITY_API OperationalAuditLogger& getGlobalAuditLogger();

/**
 * @brief Get Global Correlation Engine.
 * @return Return value.
 */
THEMIS_SECURITY_API EventCorrelationEngine& getGlobalCorrelationEngine();

/**
 * @brief Get Global Evidence Collector.
 * @return Return value.
 */
THEMIS_SECURITY_API ComplianceEvidenceCollector& getGlobalEvidenceCollector();

} // namespace themis::governance
