/**
 * @file llm_model_audit_logger.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "utils/audit_logger.h"
#include <string>
#include <chrono>
#include <memory>
#include <ostream>
#include <optional>
#include <vector>
#include <nlohmann/json.hpp>

namespace themis {
namespace llm {

using json = nlohmann::json;

enum class LLMModelAuditEventType {
    // Inference Events (CRITICAL for traceability)
    INFERENCE_STARTED,      // Model inference started
    INFERENCE_COMPLETED,    // Model inference completed
    INFERENCE_FAILED,       // Model inference failed
    
    // Model Lifecycle Events
    MODEL_LOADED,           // Model loaded into memory
    MODEL_UNLOADED,         // Model unloaded from memory
    MODEL_SWITCHED,         // Switched from one model to another
    
    // CRUD Operations
    MODEL_REGISTERED,       // New model registered in database
    MODEL_UPDATED,          // Model metadata updated
    MODEL_DELETED,          // Model deleted
    MODEL_IMPORTED,         // Model imported from external source
    MODEL_EXPORTED,         // Model exported
    
    // Quantization Operations
    MODEL_QUANTIZED,        // Model quantized
    QUANTIZATION_FAILED,    // Quantization failed
    
    // Fine-tuning Operations
    FINETUNING_STARTED,     // Fine-tuning started
    FINETUNING_COMPLETED,   // Fine-tuning completed
    FINETUNING_FAILED,      // Fine-tuning failed
    
    // Security Events
    MODEL_ENCRYPTED,        // Model encrypted
    MODEL_SIGNED,           // Digital signature created
    SIGNATURE_VERIFIED,     // Signature verified successfully
    SIGNATURE_FAILED,       // Signature verification failed
    CHECKSUM_MISMATCH,      // File checksum mismatch
    
    // Deployment Events
    MODEL_DEPLOYED,         // Model deployed to production
    MODEL_UNDEPLOYED,       // Model removed from production
    DEPLOYMENT_FAILED,      // Deployment failed
    
    // Performance Events
    PERFORMANCE_DEGRADED,   // Performance below threshold
    OOM_ERROR,              // Out of memory error
    CACHE_HIT,              // Model cache hit
    CACHE_MISS,             // Model cache miss

    // Safety / Policy Events (Q1)
    PROMPT_BLOCKED,         // Prompt blocked by PromptPolicy (hard block rule)
    PROMPT_REDACTED         // Prompt content redacted by PromptPolicy
};

struct LLMModelInferenceAudit {
    /**
     * @brief LLMModel Inference Audit.
     * @return Return value.
     */
    virtual ~LLMModelInferenceAudit() = default;
    // Timestamps
    std::chrono::system_clock::time_point timestamp;
    std::chrono::milliseconds duration_ms;
    
    // Request identification
    std::string request_id;
    std::string session_id;
    std::string user_id;
    
    // Model identification (CRITICAL)
    std::string model_id;              // Base model ID
    std::string model_version;         // Model version
    std::string model_checksum;        // SHA256 hash
    std::string quantization;          // Quantization level
    
    // Optional: LoRA adapter if used
    std::string lora_adapter_id;       // LoRA adapter (if any)
    std::string lora_version;          // LoRA version (if any)
    
    // Inference details
    std::string prompt;
    std::string response;
    int input_tokens = 0;
    int output_tokens = 0;
    
    // Quality metrics
    float confidence_score = 0.0f;
    float perplexity = 0.0f;
    bool hallucination_detected = false;
    
    // Configuration
    float temperature = 0.7f;
    float top_p = 0.9f;
    int max_tokens = 512;
    
    // Resource usage
    size_t vram_used_mb = 0;
    size_t ram_used_mb = 0;
    float tokens_per_second = 0.0f;
    
    // Result
    bool success = true;
    std::string error_message;
    
    // Additional context
    json metadata;
    
    json toJSON() const {
        auto ts = std::chrono::system_clock::to_time_t(timestamp);
        
        return json{
            {"timestamp", ts},
            {"duration_ms", duration_ms.count()},
            {"request_id", request_id},
            {"session_id", session_id},
            {"user_id", user_id},
            
            // Model identification (CRITICAL)
            {"model_id", model_id},
            {"model_version", model_version},
            {"model_checksum", model_checksum},
            {"quantization", quantization},
            
            // LoRA if used
            {"lora_adapter_id", lora_adapter_id},
            {"lora_version", lora_version},
            
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
            
            // Resources
            {"vram_used_mb", vram_used_mb},
            {"ram_used_mb", ram_used_mb},
            {"tokens_per_second", tokens_per_second},
            
            // Result
            {"success", success},
            {"error_message", error_message},
            
            {"metadata", metadata}
        };
    }
};

class LLMModelAuditLogger {
public:
    class Impl;

    explicit LLMModelAuditLogger(const utils::AuditLoggerConfig& config = utils::AuditLoggerConfig{});
    ~LLMModelAuditLogger();
    
    /**
     * @brief Log Inference.
     * @param[in] audit Input parameter.
     */
    void logInference(const LLMModelInferenceAudit& audit);
    
    void logEvent(
        LLMModelAuditEventType event_type,
        const std::string& model_id,
        const json& details = json::object()
    );
    
    void logModelLifecycle(
        LLMModelAuditEventType event_type,
        const std::string& model_id,
        const std::string& version = "",
        const json& metadata = json::object()
    );
    
    void logFineTuning(
        LLMModelAuditEventType event_type,
        const std::string& model_id,
        const std::string& base_model_id,
        int num_samples,
        float final_loss = 0.0f,
        const json& hyperparameters = json::object()
    );
    
    void logDeployment(
        LLMModelAuditEventType event_type,
        const std::string& model_id,
        const std::string& deployment_target,
        const json& config = json::object()
    );

    /**
     * @brief Log Policy Violation.
     * @param[in] model_id Identifier of the model.
     * @param[in] request_id Identifier of the request.
     * @param[in] rule_name Name of the retention policy.
     * @param[in] reason Input parameter.
     * @param[in] was_blocked Input parameter.
     */
    void logPolicyViolation(
        const std::string& model_id,
        const std::string& request_id,
        const std::string& rule_name,
        const std::string& reason,
        bool was_blocked
    );
    
    std::vector<json> queryLogs(
        const std::string& model_id,
        std::optional<std::chrono::system_clock::time_point> start_time = std::nullopt,
        std::optional<std::chrono::system_clock::time_point> end_time = std::nullopt
    );
    
    std::vector<LLMModelInferenceAudit> getInferenceHistory(
        const std::string& model_id,
        int limit = 100
    );
    
    /**
     * @brief Get Model Stats.
     * @param[in] model_id Identifier of the model.
     * @return Return value.
     */
    json getModelStats(const std::string& model_id);
    
    /**
     * @brief Set Enabled.
     * @param[in] enabled Input parameter.
     */
    void setEnabled(bool enabled);
    
    /**
     * @brief Flush.
     */
    void flush();

    size_t exportAnalytics(
        std::ostream& out_stream,
        const std::string& model_id = "",
        std::optional<std::chrono::system_clock::time_point> start_time = std::nullopt,
        std::optional<std::chrono::system_clock::time_point> end_time = std::nullopt
    );

private:
    std::unique_ptr<Impl> impl_;
};

/**
 * @brief Generate Model Request Id.
 * @return Return value.
 */
std::string generateModelRequestId();

/**
 * @brief Compute Model Checksum.
 * @param[in] file_path Path to the file.
 * @return Return value.
 */
std::string computeModelChecksum(const std::string& file_path);

} // namespace llm
} // namespace themis
