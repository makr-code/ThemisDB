// Copyright 2025 ThemisDB
// Licensed under MIT License

/**
 * @file inference_engine_enhanced.h
 * @brief Enhanced Inference Engine for Converged Storage-Inference
 * @version 1.9.0-beta
 * @note Maturity: PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=0; TODO=0, Stub=0, Unimpl=0, Mock=0, Sim=0, Debt=0
 * @note Implements LLM inference engine with LoRA adapter support for per-shard execution
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <optional>
#include <functional>
#include <chrono>
#include <nlohmann/json.hpp>

namespace themisdb {
namespace sharding {

// Forward declarations
class ContinuousBatchScheduler;
class PagedKVCache;
class LoRAAdapter;

/**
 * @brief Inference engine mode
 */
enum class InferenceMode {
    SINGLE_TOKEN,      // Generate one token at a time
    BATCH,            // Batch generation
    STREAMING,        // Streaming generation
    SPECULATIVE       // Speculative decoding mode
};

/**
 * @brief Token generation result
 */
struct TokenGenerationResult {
    std::vector<int> token_ids;
    std::vector<float> logits;
    std::vector<float> probabilities;
    bool is_eos = false;  // End of sequence
    bool is_error = false;
    std::string error_message;
    
    // Performance metrics
    std::chrono::milliseconds generation_time;
    double tokens_per_second = 0.0;
    
    nlohmann::json toJson() const {
        nlohmann::json j;
        j["token_ids"] = token_ids;
        j["is_eos"] = is_eos;
        j["is_error"] = is_error;
        if (is_error) {
            j["error_message"] = error_message;
        }
        j["generation_time_ms"] = generation_time.count();
        j["tokens_per_second"] = tokens_per_second;
        return j;
    }
};

/**
 * @brief Model loading configuration
 */
struct ModelConfig {
    std::string model_id;
    std::string model_path;
    std::string model_type = "llama";  // llama, mistral, phi, etc.
    
    // Quantization
    std::string quantization = "Q4_K_M";  // Q4_0, Q4_K_M, Q5_0, Q5_K_M, Q6_K, Q8_0
    
    // Model parameters
    uint32_t max_context_length = 4096;
    uint32_t max_generated_tokens = 2048;
    double temperature = 0.7;
    double top_p = 0.9;
    double top_k = 40.0;
    double repeat_penalty = 1.1;
    
    // Memory settings
    bool use_gpu = true;
    int gpu_device = 0;
    size_t max_memory_bytes = 0;  // 0 = unlimited
    
    // LoRA settings
    bool enable_lora = true;
    std::string default_lora_path;
    
    // Performance settings
    uint32_t num_threads = 4;
    
    bool isValid() const {
        return !model_id.empty() && !model_path.empty();
    }
    
    nlohmann::json toJson() const {
        return {
            {"model_id", model_id},
            {"model_path", model_path},
            {"model_type", model_type},
            {"quantization", quantization},
            {"max_context_length", max_context_length},
            {"max_generated_tokens", max_generated_tokens},
            {"temperature", temperature},
            {"top_p", top_p},
            {"top_k", top_k},
            {"repeat_penalty", repeat_penalty},
            {"use_gpu", use_gpu},
            {"gpu_device", gpu_device},
            {"enable_lora", enable_lora},
            {"default_lora_path", default_lora_path},
            {"num_threads", num_threads}
        };
    }
};

/**
 * @brief LoRA adapter configuration
 */
struct LoRAConfig {
    std::string adapter_id;
    std::string adapter_path;
    std::string domain;  // LEGAL, MEDICAL, TRANSACTION, etc.
    
    // Adapter parameters
    uint32_t rank = 64;
    double scaling = 1.0;
    
    // Capability metadata (for gossip-driven routing)
    double accuracy_delta = 0.0;  // Accuracy improvement vs. base model
    double performance_delta_p99_ms = 0.0;  // P99 latency delta
    uint64_t last_trained_timestamp = 0;  // Timestamp of last training
    std::string training_dataset;
    uint64_t training_steps = 0;
    
    // Usage statistics
    uint64_t total_requests = 0;
    uint64_t total_tokens_generated = 0;
    double avg_acceptance_rate = 1.0;  // For speculative decoding
    
    bool isValid() const {
        return !adapter_id.empty() && !adapter_path.empty();
    }
    
    nlohmann::json toJson() const {
        return {
            {"adapter_id", adapter_id},
            {"adapter_path", adapter_path},
            {"domain", domain},
            {"rank", rank},
            {"scaling", scaling},
            {"accuracy_delta", accuracy_delta},
            {"performance_delta_p99_ms", performance_delta_p99_ms},
            {"last_trained_timestamp", last_trained_timestamp},
            {"training_dataset", training_dataset},
            {"training_steps", training_steps},
            {"total_requests", total_requests},
            {"total_tokens_generated", total_tokens_generated},
            {"avg_acceptance_rate", avg_acceptance_rate}
        };
    }
};

/**
 * @brief Inference statistics
 */
struct InferenceStats {
    // Token statistics
    uint64_t total_tokens_generated = 0;
    uint64_t total_prompt_tokens = 0;
    uint64_t total_prefill_tokens = 0;
    uint64_t total_decode_tokens = 0;
    
    // Request statistics
    uint64_t total_requests = 0;
    uint64_t completed_requests = 0;
    uint64_t failed_requests = 0;
    uint64_t cancelled_requests = 0;
    
    // Performance statistics
    double avg_tokens_per_second = 0.0;
    double avg_time_to_first_token_ms = 0.0;
    double avg_request_latency_ms = 0.0;
    
    // Per-phase statistics
    double avg_prefill_time_ms = 0.0;
    double avg_decode_time_ms = 0.0;
    
    // Quality statistics
    uint64_t total_speculative_acceptances = 0;
    uint64_t total_speculative_rejections = 0;
    double speculative_acceptance_rate = 1.0;
    
    // LoRA statistics
    std::unordered_map<std::string, uint64_t> adapter_usage_counts;
    
    double getSpeculativeAcceptanceRate() const {
        uint64_t total = total_speculative_acceptances + total_speculative_rejections;
        return total > 0 ? static_cast<double>(total_speculative_acceptances) / total : 1.0;
    }
    
    nlohmann::json toJson() const {
        nlohmann::json j;
        j["total_tokens_generated"] = total_tokens_generated;
        j["total_prompt_tokens"] = total_prompt_tokens;
        j["total_requests"] = total_requests;
        j["completed_requests"] = completed_requests;
        j["failed_requests"] = failed_requests;
        j["avg_tokens_per_second"] = avg_tokens_per_second;
        j["avg_time_to_first_token_ms"] = avg_time_to_first_token_ms;
        j["avg_request_latency_ms"] = avg_request_latency_ms;
        j["avg_prefill_time_ms"] = avg_prefill_time_ms;
        j["avg_decode_time_ms"] = avg_decode_time_ms;
        j["speculative_acceptance_rate"] = getSpeculativeAcceptanceRate();
        j["adapter_usage"] = adapter_usage_counts;
        return j;
    }
};

/**
 * @brief Enhanced Inference Engine for Converged Storage-Inference
 * 
 * Provides LLM inference capabilities integrated with:
 * - ContinuousBatchScheduler for request scheduling
 * - PagedKVCache for KV state management
 * - LoRA adapter support for domain-specific fine-tuning
 * - Speculative decoding support
 * - Streaming generation support
 * 
 * This engine is designed for per-shard inference in a converged
 * storage-retrieval-inference architecture.
 */
class InferenceEngineEnhanced {
public:
    /**
     * @brief Callback for token generation
     */
    using TokenCallback = std::function<void(int64_t request_id, const TokenGenerationResult&)>;
    
    /**
     * @brief Callback for request completion
     */
    using CompletionCallback = std::function<void(int64_t request_id, const std::string& output_text)>;
    
    /**
     * @brief Callback for adapter capability update
     */
    using CapabilityUpdateCallback = std::function<void(const LoRAConfig& adapter_config)>;
    
    /**
     * @brief Construct InferenceEngineEnhanced
     * @param model_config Model configuration
     */
    explicit InferenceEngineEnhanced(const ModelConfig& model_config);
    
    ~InferenceEngineEnhanced();
    
    // Delete copy constructors and assignment operators
    InferenceEngineEnhanced(const InferenceEngineEnhanced&) = delete;
    InferenceEngineEnhanced& operator=(const InferenceEngineEnhanced&) = delete;
    
    // ========================================================================
    // Lifecycle Management
    // ========================================================================
    
    /**
     * @brief Initialize the inference engine
     * @return true if initialization succeeded
     */
    bool initialize();
    
    /**
     * @brief Shutdown the inference engine
     */
    void shutdown() noexcept;
    
    /**
     * @brief Load a model
     * @param model_path Path to model
     * @return true if loading succeeded
     */
    bool loadModel(const std::string& model_path);
    
    /**
     * @brief Unload the current model
     */
    void unloadModel();
    
    /**
     * @brief Check if model is loaded
     */
    bool isModelLoaded() const;
    
    // ========================================================================
    // Inference API
    // ========================================================================
    
    /**
     * @brief Generate tokens for a request
     * @param request_id Request identifier
     * @param prompt_text Input prompt text
     * @param max_tokens Maximum tokens to generate
     * @param mode Inference mode
     * @param adapter_id LoRA adapter identifier (empty for base model)
     * @param token_callback Token generation callback
     * @param completion_callback Completion callback
     * @return true if generation started successfully
     */
    bool generate(
        int64_t request_id,
        const std::string& prompt_text,
        uint32_t max_tokens,
        InferenceMode mode = InferenceMode::BATCH,
        const std::string& adapter_id = "",
        TokenCallback token_callback = nullptr,
        CompletionCallback completion_callback = nullptr
    );
    
    /**
     * @brief Generate tokens from token IDs
     * @param request_id Request identifier
     * @param input_token_ids Input token IDs
     * @param max_tokens Maximum tokens to generate
     * @param is_prefill Whether this is prefill phase
     * @param output_token_ids Output token IDs
     * @return true if generation succeeded
     */
    bool generateTokens(
        int64_t request_id,
        const std::vector<int>& input_token_ids,
        uint32_t max_tokens,
        bool is_prefill,
        std::vector<int>& output_token_ids
    );
    
    /**
     * @brief Generate a single token
     * @param request_id Request identifier
     * @param input_token_ids Input token IDs
     * @param is_prefill Whether this is prefill phase
     * @return Generated token ID or -1 on error
     */
    int generateSingleToken(
        int64_t request_id,
        const std::vector<int>& input_token_ids,
        bool is_prefill
    );
    
    /**
     * @brief Cancel generation for a request
     * @param request_id Request identifier
     * @return true if cancelled successfully
     */
    bool cancelRequest(int64_t request_id);
    
    /**
     * @brief Check if a request is in progress
     * @param request_id Request identifier
     */
    bool isRequestInProgress(int64_t request_id) const;
    
    // ========================================================================
    // Streaming API
    // ========================================================================
    
    /**
     * @brief Start streaming generation
     * @param request_id Request identifier
     * @param prompt_text Input prompt text
     * @param max_tokens Maximum tokens to generate
     * @param adapter_id LoRA adapter identifier
     * @param token_callback Token callback (called for each generated token)
     * @param completion_callback Completion callback
     * @return true if streaming started successfully
     */
    bool startStreaming(
        int64_t request_id,
        const std::string& prompt_text,
        uint32_t max_tokens,
        const std::string& adapter_id = "",
        TokenCallback token_callback = nullptr,
        CompletionCallback completion_callback = nullptr
    );
    
    /**
     * @brief Stop streaming for a request
     * @param request_id Request identifier
     */
    void stopStreaming(int64_t request_id);
    
    // ========================================================================
    // Speculative Decoding API
    // ========================================================================
    
    /**
     * @brief Enable speculative decoding
     * @param draft_model_id Draft model identifier
     * @param target_model_id Target model identifier
     * @return true if speculative decoding is enabled
     */
    bool enableSpeculativeDecoding(
        const std::string& draft_model_id,
        const std::string& target_model_id
    );
    
    /**
     * @brief Disable speculative decoding
     */
    void disableSpeculativeDecoding();
    
    /**
     * @brief Check if speculative decoding is enabled
     */
    bool isSpeculativeDecodingEnabled() const;
    
    /**
     * @brief Generate draft tokens for speculative decoding
     * @param request_id Request identifier
     * @param input_token_ids Input token IDs
     * @param max_draft_tokens Maximum draft tokens to generate
     * @param draft_token_ids Output draft token IDs
     * @return true if draft generation succeeded
     */
    bool generateDraftTokens(
        int64_t request_id,
        const std::vector<int>& input_token_ids,
        uint32_t max_draft_tokens,
        std::vector<int>& draft_token_ids
    );
    
    /**
     * @brief Verify draft tokens with target model
     * @param request_id Request identifier
     * @param input_token_ids Input token IDs
     * @param draft_token_ids Draft token IDs to verify
     * @param verified_token_ids Output verified token IDs
     * @return Acceptance rate (0.0 to 1.0)
     */
    double verifyDraftTokens(
        int64_t request_id,
        const std::vector<int>& input_token_ids,
        const std::vector<int>& draft_token_ids,
        std::vector<int>& verified_token_ids
    );
    
    // ========================================================================
    // LoRA Adapter Management
    // ========================================================================
    
    /**
     * @brief Load a LoRA adapter
     * @param adapter_config Adapter configuration
     * @return true if loading succeeded
     */
    bool loadLoRAAdapter(const LoRAConfig& adapter_config);
    
    /**
     * @brief Unload a LoRA adapter
     * @param adapter_id Adapter identifier
     * @return true if unloading succeeded
     */
    bool unloadLoRAAdapter(const std::string& adapter_id);
    
    /**
     * @brief Get loaded adapter configuration
     * @param adapter_id Adapter identifier
     * @return Adapter configuration or nullopt if not found
     */
    std::optional<LoRAConfig> getAdapterConfig(const std::string& adapter_id) const;
    
    /**
     * @brief Get all loaded adapter configurations
     */
    std::vector<LoRAConfig> getAllAdapterConfigs() const;
    
    /**
     * @brief Set the active adapter for a domain
     * @param domain Domain identifier
     * @param adapter_id Adapter identifier
     * @return true if set successfully
     */
    bool setActiveAdapter(const std::string& domain, const std::string& adapter_id);
    
    /**
     * @brief Get the active adapter for a domain
     * @param domain Domain identifier
     * @return Adapter configuration or nullopt if not found
     */
    std::optional<LoRAConfig> getActiveAdapter(const std::string& domain) const;
    
    /**
     * @brief Update adapter capability metadata
     * @param adapter_id Adapter identifier
     * @param accuracy_delta Accuracy improvement
     * @param performance_delta_p99_ms P99 latency delta
     */
    void updateAdapterCapabilities(
        const std::string& adapter_id,
        double accuracy_delta,
        double performance_delta_p99_ms
    );
    
    // ========================================================================
    // KV Cache Integration
    // ========================================================================
    
    /**
     * @brief Set KV cache manager
     * @param kv_cache KV cache manager
     */
    void setKVCache(PagedKVCache* kv_cache);
    
    /**
     * @brief Get KV cache manager
     */
    PagedKVCache* getKVCache();
    
    /**
     * @brief Clear KV cache for a request
     * @param request_id Request identifier
     */
    void clearKVCache(int64_t request_id);
    
    // ========================================================================
    // Scheduler Integration
    // ========================================================================
    
    /**
     * @brief Set the scheduler
     * @param scheduler Continuous batch scheduler
     */
    void setScheduler(ContinuousBatchScheduler* scheduler);
    
    /**
     * @brief Get the scheduler
     */
    ContinuousBatchScheduler* getScheduler();
    
    // ========================================================================
    // Capability Announcement (Gossip)
    // ========================================================================
    
    /**
     * @brief Set capability update callback for gossip
     * @param callback Callback to receive capability updates
     */
    void setCapabilityUpdateCallback(CapabilityUpdateCallback callback);
    
    /**
     * @brief Broadcast adapter capabilities (for gossip)
     */
    void broadcastAdapterCapabilities();
    
    /**
     * @brief Get capability announcement for an adapter
     * @param adapter_id Adapter identifier
     * @return Capability announcement JSON
     */
    nlohmann::json getCapabilityAnnouncement(const std::string& adapter_id) const;
    
    // ========================================================================
    // Statistics and Monitoring
    // ========================================================================
    
    /**
     * @brief Get current inference statistics
     */
    InferenceStats getStats() const;
    
    /**
     * @brief Get detailed statistics as JSON
     */
    nlohmann::json getStatsJson() const;
    
    /**
     * @brief Reset statistics
     */
    void resetStats();
    
    /**
     * @brief Get model information
     */
    nlohmann::json getModelInfo() const;
    
    // ========================================================================
    // Configuration and Control
    // ========================================================================
    
    /**
     * @brief Update model configuration
     * @param config New configuration
     */
    void updateConfig(const ModelConfig& config);
    
    /**
     * @brief Get current configuration
     */
    const ModelConfig& getConfig() const;
    
    /**
     * @brief Check if engine is ready
     */
    bool isReady() const;
    
    /**
     * @brief Get engine status
     */
    std::string getStatusString() const;
    
private:
    // ========================================================================
    // Internal Types
    // ========================================================================
    
    struct RequestState {
        int64_t request_id;
        std::string prompt_text;
        std::vector<int> input_token_ids;
        std::vector<int> output_token_ids;
        
        // Generation state
        uint32_t tokens_generated = 0;
        uint32_t max_tokens = 0;
        InferenceMode mode;
        std::string adapter_id;
        
        // Timing
        std::chrono::steady_clock::time_point start_time;
        std::chrono::steady_clock::time_point last_token_time;
        
        // Callbacks
        TokenCallback token_callback;
        CompletionCallback completion_callback;
        
        // State
        bool is_streaming = false;
        bool is_cancelled = false;
        bool is_completed = false;
    };
    
    // ========================================================================
    // Internal State
    // ========================================================================
    
    ModelConfig model_config_;
    ContinuousBatchScheduler* scheduler_ = nullptr;
    PagedKVCache* kv_cache_ = nullptr;
    
    // Model state
    bool model_loaded_ = false;
    bool ready_ = false;
    
    // Speculative decoding
    bool speculative_decoding_enabled_ = false;
    std::string draft_model_id_;
    std::string target_model_id_;
    
    // Adapter management
    std::unordered_map<std::string, LoRAConfig> loaded_adapters_;
    std::unordered_map<std::string, std::string> domain_to_adapter_;
    
    // Request management
    std::unordered_map<int64_t, RequestState> active_requests_;
    
    // Statistics
    mutable std::mutex stats_mutex_;
    InferenceStats stats_;
    
    // Callbacks
    CapabilityUpdateCallback capability_update_callback_;
    
    // Synchronization
    mutable std::mutex mutex_;
    
    // ========================================================================
    // Internal Methods
    // ========================================================================
    
    /**
     * @brief Tokenize text to token IDs
     */
    std::vector<int> tokenize(const std::string& text) const;
    
    /**
     * @brief Detokenize token IDs to text
     */
    std::string detokenize(const std::vector<int>& token_ids) const;
    
    /**
     * @brief Apply sampling to logits
     */
    void applySampling(
        std::vector<float>& logits,
        double temperature,
        double top_p,
        double top_k,
        double repeat_penalty
    );
    
    /**
     * @brief Update statistics
     */
    void updateStats(const RequestState& request);
};

} // namespace sharding
} // namespace themisdb
