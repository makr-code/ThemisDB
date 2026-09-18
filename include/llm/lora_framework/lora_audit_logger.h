/**
 * @file lora_audit_logger.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

struct LoRAInferenceAudit {
    /**
     * @brief Lo RAInference Audit.
     * @return Return value.
     */
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

class LoRAAuditLogger {
public:
    explicit LoRAAuditLogger(const utils::AuditLoggerConfig& config = utils::AuditLoggerConfig{});
    ~LoRAAuditLogger();
    
    /**
     * @brief Log Inference.
     * @param[in] audit Input parameter.
     */
    void logInference(const LoRAInferenceAudit& audit);
    
    void logEvent(
        LoRAAuditEventType event_type,
        const std::string& adapter_id,
        const json& details = json::object()
    );
    
    void logAdapterLifecycle(
        LoRAAuditEventType event_type,
        const std::string& adapter_id,
        const std::string& version = "",
        const json& metadata = json::object()
    );
    
    void logTraining(
        LoRAAuditEventType event_type,
        const std::string& adapter_id,
        int num_samples,
        float final_loss = 0.0f,
        float validation_accuracy = 0.0f,
        const json& hyperparameters = json::object()
    );
    
    void logFeedback(
        LoRAAuditEventType event_type,
        const std::string& adapter_id,
        const std::string& question,
        const std::string& answer,
        const std::string& correction = "",
        const std::string& user_id = ""
    );
    
    void logVersioning(
        LoRAAuditEventType event_type,
        const std::string& adapter_id,
        const std::string& from_version,
        const std::string& to_version,
        const std::string& reason = ""
    );
    
    std::vector<json> queryLogs(
        const std::string& adapter_id,
        std::optional<std::chrono::system_clock::time_point> start_time = std::nullopt,
        std::optional<std::chrono::system_clock::time_point> end_time = std::nullopt
    );
    
    std::vector<LoRAInferenceAudit> getInferenceHistory(
        const std::string& adapter_id,
        int limit = 100
    );
    
    /**
     * @brief Get Adapter Stats.
     * @param[in] adapter_id Identifier of the adapter.
     * @return Return value.
     */
    json getAdapterStats(const std::string& adapter_id);
    
    /**
     * @brief Set Enabled.
     * @param[in] enabled Input parameter.
     */
    void setEnabled(bool enabled);
    
    /**
     * @brief Flush.
     */
    void flush();

    /**
     * @brief ── Provenance & Merkle-chain integration ────────────────────────────────
     * @param[in] mgr Input parameter.
     */

    void setProvenanceManager(std::shared_ptr<LoRAProvenanceManager> mgr);

    /**
     * @brief Log Provenance Attached.
     * @param[in] adapter_id Identifier of the adapter.
     * @param[in] record Input parameter.
     */
    void logProvenanceAttached(const std::string& adapter_id,
                                const LoRAProvenanceRecord& record);

    /**
     * @brief Log Snapshot Created.
     * @param[in] adapter_id Identifier of the adapter.
     * @param[in] snapshot Input parameter.
     */
    void logSnapshotCreated(const std::string& adapter_id,
                             const AdapterSnapshot& snapshot);

    /**
     * @brief Log Audit Chain Verified.
     * @param[in] adapter_id Identifier of the adapter.
     * @param[in] valid Input parameter.
     * @param[in] entry_count Input parameter.
     */
    void logAuditChainVerified(const std::string& adapter_id,
                                bool valid,
                                std::size_t entry_count);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * @brief Generate Request Id.
 * @return Return value.
 */
std::string generateRequestId();

/**
 * @brief Compute Adapter Hash.
 * @param[in] weights Input parameter.
 * @return Return value.
 */
std::string computeAdapterHash(const std::vector<uint8_t>& weights);

} // namespace lora
} // namespace llm
} // namespace themis
