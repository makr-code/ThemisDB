/**
 * @file lora_audit_logger.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "utils/audit_logger.h"
#include "llm/lora_framework/lora_provenance.h"
#include <string>
#include <chrono>
#include <memory>
#include <nlohmann/json.hpp>

namespace themis {
namespace llm {
namespace lora {

using json = nlohmann::json;

/**
 * @brief LoRA-specific audit event types
 * 
 * Critical for compliance and quality assurance:
 * - Track which LLM generated which response
 * - Track which LoRA adapter was used
 * - Track training provenance
 * - Track feedback and corrections
 */
enum class LoRAAuditEventType {
    // Inference Events (CRITICAL for traceability)
    INFERENCE_STARTED,          // LLM inference started with LoRA
    INFERENCE_COMPLETED,        // LLM inference completed
    INFERENCE_FAILED,           // LLM inference failed
    
    // Adapter Lifecycle Events
    ADAPTER_LOADED,             // LoRA adapter loaded into memory
    ADAPTER_UNLOADED,           // LoRA adapter unloaded
    ADAPTER_SWITCHED,           // Switched from one adapter to another
    
    // Training Events
    TRAINING_STARTED,           // Training job started
    TRAINING_COMPLETED,         // Training completed successfully
    TRAINING_FAILED,            // Training failed
    TRAINING_DATA_ADDED,        // Training data added
    
    // Feedback Events
    FEEDBACK_POSITIVE,          // User provided positive feedback
    FEEDBACK_NEGATIVE,          // User provided negative feedback
    FEEDBACK_CORRECTION,        // User provided correction
    
    // Version Management
    VERSION_CREATED,            // New version created
    VERSION_SWITCHED,           // Switched to different version
    VERSION_ROLLED_BACK,        // Rolled back to previous version
    
    // CRUD Operations
    ADAPTER_CREATED,            // New adapter created
    ADAPTER_UPDATED,            // Adapter updated/retrained
    ADAPTER_DELETED,            // Adapter deleted
    ADAPTER_IMPORTED,           // Adapter imported from external source
    ADAPTER_EXPORTED,           // Adapter exported
    
    // Metadata Events
    METADATA_UPDATED,           // Adapter metadata modified
    HYPERPARAMETERS_CHANGED,    // Training hyperparameters changed
    
    // Security Events
    ADAPTER_ENCRYPTED,          // Adapter encrypted
    ADAPTER_SIGNED,             // Digital signature created
    SIGNATURE_VERIFIED,         // Signature verified successfully
    SIGNATURE_FAILED,           // Signature verification failed
    
    // Quality Events
    ACCURACY_THRESHOLD_VIOLATED, // Validation accuracy below threshold
    ROLLBACK_TRIGGERED,         // Automatic rollback triggered
    
    // System Events
    CACHE_HIT,                  // Adapter cache hit
    CACHE_MISS,                 // Adapter cache miss
    CACHE_EVICTION,             // Adapter evicted from cache

    // Provenance & Audit-Chain Events
    PROVENANCE_ATTACHED,        // Cryptographic provenance record attached
    PROVENANCE_VERIFIED,        // Provenance record verified against stored hashes
    SNAPSHOT_CREATED,           // MVCC snapshot of adapter state created
    AUDIT_CHAIN_VERIFIED,       // Merkle audit chain integrity confirmed
    AUDIT_CHAIN_TAMPERED        // Merkle audit chain integrity check FAILED
};

/**
 * @brief Audit record for LoRA inference
 * 
 * This is THE most critical audit record - it tracks exactly
 * which LLM with which LoRA adapter generated which response.
 */
struct LoRAInferenceAudit {
    virtual ~LoRAInferenceAudit() = default;
    // Timestamps
    std::chrono::system_clock::time_point timestamp;
    std::chrono::milliseconds duration_ms;
    
    // Request identification
    std::string request_id;             // Unique request ID
    std::string session_id;             // User session ID
    std::string user_id;                // User who made request
    
    // Model identification (CRITICAL)
    std::string base_model_id;          // Base LLM model (e.g., "llama-2-7b")
    std::string base_model_version;     // Base model version
    std::string adapter_id;             // LoRA adapter ID (e.g., "themis_help_lora")
    std::string adapter_version;        // LoRA adapter version (e.g., "v2.1")
    std::string adapter_hash;           // SHA256 hash of adapter weights
    
    // Inference details
    std::string prompt;                 // Input prompt (may be truncated)
    std::string response;               // Generated response (may be truncated)
    int input_tokens = 0;               // Number of input tokens
    int output_tokens = 0;              // Number of output tokens
    
    // Quality metrics
    float confidence_score = 0.0f;      // Model confidence (0-1)
    float perplexity = 0.0f;            // Response perplexity
    bool hallucination_detected = false; // Hallucination flag
    
    // Configuration
    float temperature = 0.7f;           // Generation temperature
    float top_p = 0.9f;                 // Top-p sampling
    int max_tokens = 512;               // Max tokens to generate
    float lora_scaling = 1.0f;          // LoRA scaling factor
    
    // Result
    bool success = true;
    std::string error_message;
    
    // Additional context
    json metadata;                      // Additional metadata
    
    json toJSON() const {
        auto ts = std::chrono::system_clock::to_time_t(timestamp);
        
        return json{
            {"timestamp", ts},
            {"duration_ms", duration_ms.count()},
            {"request_id", request_id},
            {"session_id", session_id},
            {"user_id", user_id},
            
            // Model identification (CRITICAL)
            {"base_model_id", base_model_id},
            {"base_model_version", base_model_version},
            {"adapter_id", adapter_id},
            {"adapter_version", adapter_version},
            {"adapter_hash", adapter_hash},
            
            // Inference details
            {"prompt", prompt},
            {"response", response},
            {"input_tokens", input_tokens},
            {"output_tokens", output_tokens},
            
            // Quality
            {"confidence_score", confidence_score},
            {"perplexity", perplexity},
            {"hallucination_detected", hallucination_detected},
            
            // Config
            {"temperature", temperature},
            {"top_p", top_p},
            {"max_tokens", max_tokens},
            {"lora_scaling", lora_scaling},
            
            // Result
            {"success", success},
            {"error_message", error_message},
            
            {"metadata", metadata}
        };
    }
};

/**
 * @brief LoRA Audit Logger
 * 
 * Specialized audit logger for LoRA operations with focus on:
 * 1. Inference traceability (which LLM + which LoRA = which response)
 * 2. Training provenance
 * 3. Quality assurance
 * 4. Compliance (GDPR, SOC2, etc.)
 */
class LoRAAuditLogger {
public:
    explicit LoRAAuditLogger(const utils::AuditLoggerConfig& config = utils::AuditLoggerConfig{});
    ~LoRAAuditLogger();
    
    /**
     * @brief Log inference event (MOST IMPORTANT)
     * 
     * This logs the complete context of an LLM inference including:
     * - Base model used
     * - LoRA adapter used
     * - Prompt and response
     * - Quality metrics
     * - Configuration
     * 
     * Essential for:
     * - Debugging quality issues
     * - Compliance audits
     * - A/B testing analysis
     * - Model performance tracking
     */
    void logInference(const LoRAInferenceAudit& audit);
    
    /**
     * @brief Log generic LoRA event
     */
    void logEvent(
        LoRAAuditEventType event_type,
        const std::string& adapter_id,
        const json& details = json::object()
    );
    
    /**
     * @brief Log adapter lifecycle event
     */
    void logAdapterLifecycle(
        LoRAAuditEventType event_type,
        const std::string& adapter_id,
        const std::string& version = "",
        const json& metadata = json::object()
    );
    
    /**
     * @brief Log training event
     */
    void logTraining(
        LoRAAuditEventType event_type,
        const std::string& adapter_id,
        int num_samples,
        float final_loss = 0.0f,
        float validation_accuracy = 0.0f,
        const json& hyperparameters = json::object()
    );
    
    /**
     * @brief Log feedback event
     */
    void logFeedback(
        LoRAAuditEventType event_type,
        const std::string& adapter_id,
        const std::string& question,
        const std::string& answer,
        const std::string& correction = "",
        const std::string& user_id = ""
    );
    
    /**
     * @brief Log version management event
     */
    void logVersioning(
        LoRAAuditEventType event_type,
        const std::string& adapter_id,
        const std::string& from_version,
        const std::string& to_version,
        const std::string& reason = ""
    );
    
    /**
     * @brief Query audit logs for specific adapter
     * 
     * @param adapter_id Adapter identifier
     * @param start_time Start of time range
     * @param end_time End of time range
     * @return Vector of audit records
     */
    std::vector<json> queryLogs(
        const std::string& adapter_id,
        std::optional<std::chrono::system_clock::time_point> start_time = std::nullopt,
        std::optional<std::chrono::system_clock::time_point> end_time = std::nullopt
    );
    
    /**
     * @brief Get inference history for adapter
     * 
     * Returns all inferences made with a specific adapter.
     * Critical for quality analysis and debugging.
     */
    std::vector<LoRAInferenceAudit> getInferenceHistory(
        const std::string& adapter_id,
        int limit = 100
    );
    
    /**
     * @brief Get statistics for adapter
     */
    json getAdapterStats(const std::string& adapter_id);
    
    /**
     * @brief Enable/disable audit logging
     */
    void setEnabled(bool enabled);
    
    /**
     * @brief Flush logs to disk
     */
    void flush();

    // ── Provenance & Merkle-chain integration ────────────────────────────────

    /**
     * @brief Wire a LoRAProvenanceManager so that every logInference() call
     *        also appends an InferenceAuditEntry to the cryptographic Merkle chain.
     *
     * Must be called before the first logInference() for chain-based audit to work.
     * Passing nullptr disconnects the provenance manager.
     */
    void setProvenanceManager(std::shared_ptr<LoRAProvenanceManager> mgr);

    /**
     * @brief Log that a provenance record was attached to an adapter.
     *
     * Emits a PROVENANCE_ATTACHED event and, if a provenance manager is set,
     * records the attachment in the Merkle chain metadata.
     */
    void logProvenanceAttached(const std::string& adapter_id,
                                const LoRAProvenanceRecord& record);

    /**
     * @brief Log that an MVCC snapshot was created.
     *
     * Emits a SNAPSHOT_CREATED event containing snapshot_id, version, and
     * weights_hash.
     */
    void logSnapshotCreated(const std::string& adapter_id,
                             const AdapterSnapshot& snapshot);

    /**
     * @brief Log the result of a Merkle audit-chain verification.
     *
     * Emits AUDIT_CHAIN_VERIFIED or AUDIT_CHAIN_TAMPERED based on @p valid,
     * and includes the entry count.
     */
    void logAuditChainVerified(const std::string& adapter_id,
                                bool valid,
                                std::size_t entry_count);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * @brief Helper function to create unique request ID
 */
std::string generateRequestId();

/**
 * @brief Helper to compute SHA256 hash of adapter weights
 */
std::string computeAdapterHash(const std::vector<uint8_t>& weights);

} // namespace lora
} // namespace llm
} // namespace themis
