/**
 * @file ai_decision_auditor.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

struct AIDecisionAudit {
    /**
     * @brief AIDecision Audit.
     * @return Return value.
     */
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
    /**
     * @brief To Json.
     * @return Return value.
     */
    json toJson() const;
    /**
     * @brief From Json.
     * @param[in] j Input parameter.
     * @return Return value.
     */
    static AIDecisionAudit fromJson(const json& j);
};

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
    
    explicit AIDecisionAuditor(
        rocksdb::TransactionDB* db,
        rocksdb::ColumnFamilyHandle* cf = nullptr,
        std::shared_ptr<VCCPKIClient> pki_client = nullptr
    );
    
    ~AIDecisionAuditor() = default;
    
    /**
     * @brief Log Decision.
     * @param[in] audit Input parameter.
     * @return Return value.
     */
    AIDecisionAudit logDecision(AIDecisionAudit audit);
    
    /**
     * @brief Generate Explanation.
     * @param[in] decision_id Identifier of the decision.
     * @return Return value.
     */
    std::string generateExplanation(const std::string& decision_id);
    
    /**
     * @brief Query Audit Log.
     * @param[in] filter Input parameter.
     * @return Return value.
     */
    std::vector<AIDecisionAudit> queryAuditLog(const QueryFilter& filter);
    
    /**
     * @brief Flag For Review.
     * @param[in] decision_id Identifier of the decision.
     * @param[in] reason Input parameter.
     * @return True when the operation succeeds.
     */
    bool flagForReview(const std::string& decision_id, const std::string& reason);
    
    /**
     * @brief Record Override.
     * @param[in] decision_id Identifier of the decision.
     * @param[in] override_reason Input parameter.
     * @param[in] reviewer_id Identifier of the reviewer.
     * @return True when the operation succeeds.
     */
    bool recordOverride(
        const std::string& decision_id,
        const std::string& override_reason,
        const std::string& reviewer_id
    );
    
    /**
     * @brief Get Decision.
     * @param[in] decision_id Identifier of the decision.
     * @return Return value.
     */
    std::optional<AIDecisionAudit> getDecision(const std::string& decision_id) const;
    
    /**
     * @brief Verify Integrity.
     * @param[in] decision_id Identifier of the decision.
     * @return True when the operation succeeds.
     */
    bool verifyIntegrity(const std::string& decision_id) const;
    
    /**
     * @brief Export For Compliance.
     * @param[in] output_path Path to the output.
     * @param[in] filter Input parameter.
     * @return True when the operation succeeds.
     */
    bool exportForCompliance(const std::string& output_path, const QueryFilter& filter);
    
    struct Stats {
        size_t total_decisions = 0;
        size_t flagged_for_review = 0;
        size_t human_overrides = 0;
        float avg_confidence = 0.0f;
        int64_t avg_latency_ms = 0;
    };
    
    /**
     * @brief Get Stats.
     * @return Return value.
     */
    Stats getStats() const;

private:
    rocksdb::TransactionDB* db_;
    rocksdb::ColumnFamilyHandle* cf_;
    std::shared_ptr<VCCPKIClient> pki_client_;
    
    static constexpr const char* KEY_PREFIX = "ai_decision:";
    
    /**
     * @brief Make Key.
     * @param[in] id Input parameter.
     * @return Return value.
     */
    std::string makeKey(const std::string& id) const;
    /**
     * @brief Generate Id.
     * @return Return value.
     */
    std::string generateId() const;
    /**
     * @brief Sign Decision.
     * @param[in] audit Input parameter.
     * @return Return value.
     */
    std::string signDecision(const AIDecisionAudit& audit);
    /**
     * @brief Verify Signature.
     * @param[in] audit Input parameter.
     * @return True when the operation succeeds.
     */
    bool verifySignature(const AIDecisionAudit& audit) const;
};

} // namespace llm
} // namespace themis
