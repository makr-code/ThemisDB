/**
 * @file ai_decision_auditor.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "llm/llm_interaction_store.h"
#include "utils/audit_logger.h"
#include <string>
#include <vector>
#include <optional>
#include <chrono>
#include <memory>
#include <nlohmann/json.hpp>

// Forward declarations
namespace themis {
    class VCCPKIClient;
}

namespace themis {
namespace llm {

using json = nlohmann::json;

/**
 * @brief AI Decision Audit Entry
 * 
 * Comprehensive audit record for AI-driven decisions with full explainability.
 * Complies with EU AI Act, GDPR Article 22, and eIDAS requirements.
 */
struct AIDecisionAudit {
    virtual ~AIDecisionAudit() = default;
    // Identification
    std::string decision_id;           // Unique decision ID
    std::string user_id;               // User who triggered decision
    std::string session_id;            // Session context
    std::chrono::system_clock::time_point timestamp; // When decision was made

    // P1.3 — End-to-End Observability: W3C traceparent correlation fields.
    // Both fields are propagated from the originating HTTP request through
    // retrieval → prompt-build → inference → audit so that all log records
    // for a single user request share the same trace_id.
    //
    // Format:
    //   trace_id — 32 lower-case hex chars (128-bit)
    //   span_id  — 16 lower-case hex chars (64-bit)
    //
    // An empty string means the field was not provided by the caller.
    std::string trace_id;              ///< W3C traceparent trace-id (128-bit hex, 32 chars)
    std::string span_id;               ///< W3C traceparent parent-id (64-bit hex, 16 chars)
    
    // Input Context
    std::string query;                 // Original user query
    json context;                      // Additional context data
    
    // Model Information
    std::string model_name;            // e.g., "gpt-4", "llama-2"
    std::string model_version;         // Model version
    json model_params;                 // Temperature, top_p, etc.
    
    // Output
    std::string response;              // AI-generated response
    float confidence_score = 0.0f;     // Confidence (0.0-1.0)
    std::vector<std::string> alternatives; // Alternative responses
    
    // Explainability
    std::string explanation;           // Human-readable explanation
    std::vector<std::string> reasoning_steps; // Step-by-step reasoning
    json key_factors;                  // Factors influencing decision
    
    // Audit Trail
    std::string signature;             // Cryptographic signature
    bool requires_human_review = false; // Flag for low confidence
    std::string human_override;        // Override details (if any)
    std::string reviewer_id;           // Human reviewer ID
    
    // Performance metrics
    int64_t latency_ms = 0;            // Decision latency
    int token_count = 0;               // Tokens used
    
    // Serialization
    json toJson() const;
    static AIDecisionAudit fromJson(const json& j);
};

/**
 * @brief AI Decision Auditor
 * 
 * Provides comprehensive AI decision auditing and explainability features.
 * Extends existing LLMInteractionStore with compliance-focused capabilities.
 * 
 * Features:
 * - Complete logging of AI decisions with context
 * - Explanation generation for transparency
 * - Cryptographic signing for integrity
 * - Human oversight and override mechanisms
 * - Compliance reporting (EU AI Act, GDPR)
 */
class AIDecisionAuditor {
public:
    struct QueryFilter {
        std::optional<std::string> user_id;
        std::optional<std::chrono::system_clock::time_point> start_time;
        std::optional<std::chrono::system_clock::time_point> end_time;
        std::optional<float> min_confidence;
        std::optional<float> max_confidence;
        std::optional<bool> requires_review;
        size_t limit = 100;
    };
    
    /**
     * @brief Construct AI Decision Auditor
     * @param db RocksDB TransactionDB instance
     * @param cf Optional column family handle
     * @param pki_client PKI client for signing (optional)
     */
    explicit AIDecisionAuditor(
        rocksdb::TransactionDB* db,
        rocksdb::ColumnFamilyHandle* cf = nullptr,
        std::shared_ptr<VCCPKIClient> pki_client = nullptr
    );
    
    ~AIDecisionAuditor() = default;
    
    /**
     * @brief Log AI decision with full context
     * @param audit Complete audit entry
     * @return Stored audit with generated ID and signature
     */
    AIDecisionAudit logDecision(AIDecisionAudit audit);
    
    /**
     * @brief Generate explanation for a decision
     * @param decision_id Decision ID to explain
     * @return Human-readable explanation or empty if not found
     */
    std::string generateExplanation(const std::string& decision_id);
    
    /**
     * @brief Query audit log with filters
     * @param filter Query filters
     * @return Vector of matching audit entries
     */
    std::vector<AIDecisionAudit> queryAuditLog(const QueryFilter& filter);
    
    /**
     * @brief Flag decision for human review
     * @param decision_id Decision to flag
     * @param reason Reason for review
     * @return true if successful
     */
    bool flagForReview(const std::string& decision_id, const std::string& reason);
    
    /**
     * @brief Record human override of AI decision
     * @param decision_id Decision being overridden
     * @param override_reason Reason for override
     * @param reviewer_id ID of human reviewer
     * @return true if successful
     */
    bool recordOverride(
        const std::string& decision_id,
        const std::string& override_reason,
        const std::string& reviewer_id
    );
    
    /**
     * @brief Get decision by ID
     * @param decision_id Decision ID
     * @return Audit entry if found
     */
    std::optional<AIDecisionAudit> getDecision(const std::string& decision_id) const;
    
    /**
     * @brief Verify integrity of audit entry
     * @param decision_id Decision ID to verify
     * @return true if signature is valid
     */
    bool verifyIntegrity(const std::string& decision_id) const;
    
    /**
     * @brief Export audit log for compliance reporting
     * @param output_path Path to export file
     * @param filter Export filters
     * @return true if successful
     */
    bool exportForCompliance(const std::string& output_path, const QueryFilter& filter);
    
    /**
     * @brief Get audit statistics
     */
    struct Stats {
        size_t total_decisions = 0;
        size_t flagged_for_review = 0;
        size_t human_overrides = 0;
        float avg_confidence = 0.0f;
        int64_t avg_latency_ms = 0;
    };
    
    Stats getStats() const;

private:
    rocksdb::TransactionDB* db_;
    rocksdb::ColumnFamilyHandle* cf_;
    std::shared_ptr<VCCPKIClient> pki_client_;
    
    static constexpr const char* KEY_PREFIX = "ai_decision:";
    
    std::string makeKey(const std::string& id) const;
    std::string generateId() const;
    std::string signDecision(const AIDecisionAudit& audit);
    bool verifySignature(const AIDecisionAudit& audit) const;
};

} // namespace llm
} // namespace themis
