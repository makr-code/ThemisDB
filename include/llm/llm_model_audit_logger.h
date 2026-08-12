/**
 * @file llm_model_audit_logger.h
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

/**
 * @brief LLM Model Audit Event Types
 * 
 * Tracks all operations on LLM models for compliance and debugging.
 * Mirrors LoRA adapter audit logging for consistency.
 */
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

/**
 * @brief Audit record for LLM model inference
 * 
 * CRITICAL: Tracks which model generated which response.
 * Similar to LoRAInferenceAudit but for base models.
 */
struct LLMModelInferenceAudit {
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

/**
 * @brief LLM Model Audit Logger
 * 
 * Specialized audit logger for LLM model operations.
 * Complements LoRAAuditLogger for complete traceability.
 * 
 * Key difference:
 * - LoRAAuditLogger: Tracks LoRA adapter operations
 * - LLMModelAuditLogger: Tracks base model operations
 * - Both can be combined: Model + LoRA → Response
 */
class LLMModelAuditLogger {
public:
    class Impl;

    explicit LLMModelAuditLogger(const utils::AuditLoggerConfig& config = utils::AuditLoggerConfig{});
    ~LLMModelAuditLogger();
    
    /**
     * @brief Log model inference event (MOST IMPORTANT)
     * 
     * This logs the complete context of an LLM inference including:
     * - Base model used
     * - Optional LoRA adapter
     * - Prompt and response
     * - Quality metrics
     * - Resource usage
     * 
     * Essential for:
     * - Debugging quality issues
     * - Compliance audits
     * - Performance monitoring
     * - Cost tracking
     */
    void logInference(const LLMModelInferenceAudit& audit);
    
    /**
     * @brief Log generic model event
     */
    void logEvent(
        LLMModelAuditEventType event_type,
        const std::string& model_id,
        const json& details = json::object()
    );
    
    /**
     * @brief Log model lifecycle event
     */
    void logModelLifecycle(
        LLMModelAuditEventType event_type,
        const std::string& model_id,
        const std::string& version = "",
        const json& metadata = json::object()
    );
    
    /**
     * @brief Log fine-tuning event
     */
    void logFineTuning(
        LLMModelAuditEventType event_type,
        const std::string& model_id,
        const std::string& base_model_id,
        int num_samples,
        float final_loss = 0.0f,
        const json& hyperparameters = json::object()
    );
    
    /**
     * @brief Log deployment event
     */
    void logDeployment(
        LLMModelAuditEventType event_type,
        const std::string& model_id,
        const std::string& deployment_target,
        const json& config = json::object()
    );

    /**
     * @brief Log a PromptPolicy violation (PROMPT_BLOCKED or PROMPT_REDACTED).
     *
     * Called by the inference path after PromptPolicy::apply() returns a
     * non-trivial result so that operators can audit safety events alongside
     * model lifecycle events.
     *
     * @param model_id    Model ID that would have processed the prompt.
     * @param request_id  Request ID for cross-referencing inference logs.
     * @param rule_name   Policy rule that triggered (PolicyResult::rule_name).
     * @param reason      Human-readable reason (PolicyResult::reason).
     * @param was_blocked true → PROMPT_BLOCKED; false → PROMPT_REDACTED.
     */
    void logPolicyViolation(
        const std::string& model_id,
        const std::string& request_id,
        const std::string& rule_name,
        const std::string& reason,
        bool was_blocked
    );
    
    /**
     * @brief Query audit logs for specific model
     */
    std::vector<json> queryLogs(
        const std::string& model_id,
        std::optional<std::chrono::system_clock::time_point> start_time = std::nullopt,
        std::optional<std::chrono::system_clock::time_point> end_time = std::nullopt
    );
    
    /**
     * @brief Get inference history for model
     */
    std::vector<LLMModelInferenceAudit> getInferenceHistory(
        const std::string& model_id,
        int limit = 100
    );
    
    /**
     * @brief Get statistics for model
     */
    json getModelStats(const std::string& model_id);
    
    /**
     * @brief Enable/disable audit logging
     */
    void setEnabled(bool enabled);
    
    /**
     * @brief Flush logs to disk
     */
    void flush();

    /**
     * @brief Export audit events as a JSON-lines (JSONL) stream.
     *
     * Writes one JSON object per line to @p out_stream.  Each line is a
     * self-contained JSON object with the following top-level fields:
     *
     *   - timestamp_iso8601  (string, UTC)
     *   - event_type         (string, e.g. "INFERENCE_COMPLETED")
     *   - model_id           (string)
     *   - details            (object, event-specific payload)
     *
     * The format is compatible with standard SIEM ingestors and data
     * warehouse batch loaders (e.g., BigQuery, Snowflake, Splunk).
     *
     * @param out_stream  Output stream to write lines to.
     * @param model_id    Optional filter; empty string = all models.
     * @param start_time  Optional inclusive start of the query window.
     * @param end_time    Optional exclusive end of the query window.
     * @return Number of lines written.
     */
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
 * @brief Helper to generate unique request ID
 */
std::string generateModelRequestId();

/**
 * @brief Helper to compute SHA256 hash of model file
 */
std::string computeModelChecksum(const std::string& file_path);

} // namespace llm
} // namespace themis
