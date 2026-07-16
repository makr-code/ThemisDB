// Copyright 2025 ThemisDB
// Licensed under MIT License

/**
 * @file continuous_batch_scheduler.h
 * @brief Continuous Batch Scheduler for LLM Inference in Converged Storage-Inference
 * @version 1.9.0-beta
 * @note Maturity: PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=0; TODO=0, Stub=0, Unimpl=0, Mock=0, Sim=0, Debt=0
 * @note Implements vLLM-style continuous batching for per-shard inference
 */

#pragma once

#include <vector>
#include <queue>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>
#include <optional>
#include <unordered_map>
#include <functional>
#include <nlohmann/json.hpp>

namespace themisdb {
namespace sharding {

// Forward declarations
class KVCacheManager;
class InferenceEngineEnhanced;

/**
 * @brief Request scheduling priority levels
 */
enum class SchedulingPriority {
    REALTIME,      // Highest priority, immediate execution (e.g., interactive queries)
    INTERACTIVE,  // High priority, low latency (e.g., chat applications)
    BATCH,        // Normal priority, throughput-optimized
    BACKGROUND    // Lowest priority, best-effort (e.g., analytics)
};

/**
 * @brief Request state in the scheduling pipeline
 */
enum class RequestState {
    PENDING,      // Waiting to be scheduled
    PREFILLING,   // In prefill phase (generating first tokens)
    DECODING,     // In decode phase (generating subsequent tokens)
    COMPLETED,    // Request fully processed
    CANCELLED,    // Request was cancelled
    FAILED        // Request failed
};

/**
 * @brief Statistics for a single request
 */
struct RequestStats {
    int64_t request_id;
    int64_t user_id;
    std::string model_id;
    std::string lora_adapter_id;
    
    // Timing metrics
    std::chrono::steady_clock::time_point enqueue_time;
    std::chrono::steady_clock::time_point start_time;
    std::chrono::steady_clock::time_point first_token_time;
    std::chrono::steady_clock::time_point end_time;
    
    // Token metrics
    uint32_t prompt_tokens = 0;
    uint32_t generated_tokens = 0;
    uint32_t accepted_tokens = 0; // For speculative decoding
    
    // Performance metrics
    double time_to_first_token_ms = 0.0;
    double tokens_per_second = 0.0;
    
    // State tracking
    RequestState state = RequestState::PENDING;
    SchedulingPriority priority = SchedulingPriority::INTERACTIVE;
    
    nlohmann::json toJson() const {
        auto now = std::chrono::steady_clock::now();
        auto duration_ms = [&](auto start) {
            return std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
        };
        
        return {
            {"request_id", request_id},
            {"user_id", user_id},
            {"model_id", model_id},
            {"lora_adapter_id", lora_adapter_id},
            {"prompt_tokens", prompt_tokens},
            {"generated_tokens", generated_tokens},
            {"accepted_tokens", accepted_tokens},
            {"time_to_first_token_ms", time_to_first_token_ms},
            {"tokens_per_second", tokens_per_second},
            {"state", static_cast<int>(state)},
            {"priority", static_cast<int>(priority)},
            {"wait_time_ms", duration_ms(enqueue_time)},
            {"total_time_ms", duration_ms(start_time)}
        };
    }
};

/**
 * @brief Batch scheduling statistics
 */
struct BatchStats {
    // Batch sizes
    uint32_t current_batch_size = 0;
    uint32_t max_batch_size = 256;  // Default max batch size
    uint32_t max_tokens_per_batch = 8192;  // Default max tokens
    
    // Performance metrics
    double avg_tokens_per_second = 0.0;
    double avg_time_to_first_token_ms = 0.0;
    double throughput_tokens_per_second = 0.0;
    
    // Queue metrics
    uint32_t pending_requests = 0;
    uint32_t in_progress_requests = 0;
    uint32_t completed_requests = 0;
    uint32_t cancelled_requests = 0;
    uint32_t failed_requests = 0;
    
    // Timing metrics
    double p50_latency_ms = 0.0;
    double p95_latency_ms = 0.0;
    double p99_latency_ms = 0.0;
    
    nlohmann::json toJson() const {
        return {
            {"current_batch_size", current_batch_size},
            {"max_batch_size", max_batch_size},
            {"max_tokens_per_batch", max_tokens_per_batch},
            {"avg_tokens_per_second", avg_tokens_per_second},
            {"avg_time_to_first_token_ms", avg_time_to_first_token_ms},
            {"throughput_tokens_per_second", throughput_tokens_per_second},
            {"pending_requests", pending_requests},
            {"in_progress_requests", in_progress_requests},
            {"completed_requests", completed_requests},
            {"cancelled_requests", cancelled_requests},
            {"failed_requests", failed_requests},
            {"p50_latency_ms", p50_latency_ms},
            {"p95_latency_ms", p95_latency_ms},
            {"p99_latency_ms", p99_latency_ms}
        };
    }
};

/**
 * @brief Configuration for ContinuousBatchScheduler
 */
struct ContinuousBatchSchedulerConfig {
    // Batch size limits (from vLLM paper)
    uint32_t max_batch_size = 256;
    uint32_t max_tokens_per_batch = 8192;
    uint32_t chunked_prefill_size = 512;  // Tokens per chunk for prefill
    
    // Scheduling parameters
    uint32_t max_pending_requests = 10000;
    std::chrono::milliseconds request_timeout{30000};  // 30 seconds
    
    // Priority weights (higher = more priority)
    double realtime_weight = 100.0;
    double interactive_weight = 10.0;
    double batch_weight = 1.0;
    double background_weight = 0.1;
    
    // Preemption settings
    bool enable_preemption = true;
    std::chrono::milliseconds preemption_check_interval{100};
    
    // Speculative decoding
    bool enable_speculative_decoding = false;
    double speculative_acceptance_threshold = 0.65;  // Min acceptance rate
    uint32_t max_speculative_tokens = 16;  // Max tokens to speculate ahead
    
    // Chunked prefill (from Sarathi-Serve)
    bool enable_chunked_prefill = true;
    
    // Adaptive batching
    bool enable_adaptive_batching = true;
    double target_utilization = 0.9;  // Target GPU utilization
    
    bool isValid() const {
        return max_batch_size > 0 &&
               max_tokens_per_batch > 0 &&
               chunked_prefill_size > 0 &&
               max_pending_requests > 0 &&
               request_timeout.count() > 0;
    }
    
    nlohmann::json toJson() const {
        return {
            {"max_batch_size", max_batch_size},
            {"max_tokens_per_batch", max_tokens_per_batch},
            {"chunked_prefill_size", chunked_prefill_size},
            {"max_pending_requests", max_pending_requests},
            {"request_timeout_ms", request_timeout.count()},
            {"realtime_weight", realtime_weight},
            {"interactive_weight", interactive_weight},
            {"batch_weight", batch_weight},
            {"background_weight", background_weight},
            {"enable_preemption", enable_preemption},
            {"enable_speculative_decoding", enable_speculative_decoding},
            {"speculative_acceptance_threshold", speculative_acceptance_threshold},
            {"max_speculative_tokens", max_speculative_tokens},
            {"enable_chunked_prefill", enable_chunked_prefill},
            {"enable_adaptive_batching", enable_adaptive_batching},
            {"target_utilization", target_utilization}
        };
    }
};

/**
 * @brief Continuous Batch Scheduler for LLM Inference
 * 
 * Implements vLLM-style continuous batching with the following features:
 * - Iteration-level scheduling (Orca-style)
 * - Chunked prefill (Sarathi-Serve-style)
 * - Request priority queues
 * - Adaptive batch sizing
 * - Preemption support
 * - Statistics tracking
 * 
 * This scheduler is designed for per-shard inference in a converged
 * storage-retrieval-inference architecture.
 */
class ContinuousBatchScheduler {
public:
    /**
     * @brief Callback for request completion
     */
    using RequestCompletionCallback = std::function<void(
        int64_t request_id,
        const std::vector<int>& output_token_ids,
        const RequestStats& stats
    )>;
    
    /**
     * @brief Callback for token generation
     */
    using TokenGenerationCallback = std::function<bool(
        int64_t request_id,
        const std::vector<int>& input_token_ids,
        std::vector<int>& output_token_ids,
        bool is_prefill
    )>;
    
    /**
     * @brief Construct ContinuousBatchScheduler
     * @param config Scheduler configuration
     * @param kv_cache_manager KV cache manager for token state
     */
    explicit ContinuousBatchScheduler(
        const ContinuousBatchSchedulerConfig& config,
        KVCacheManager* kv_cache_manager = nullptr
    );
    
    ~ContinuousBatchScheduler();
    
    // Delete copy constructors and assignment operators
    ContinuousBatchScheduler(const ContinuousBatchScheduler&) = delete;
    ContinuousBatchScheduler& operator=(const ContinuousBatchScheduler&) = delete;
    
    // ========================================================================
    // Scheduling API
    // ========================================================================
    
    /**
     * @brief Submit a new inference request
     * @param request_id Unique request identifier
     * @param user_id User identifier
     * @param model_id Model identifier
     * @param lora_adapter_id LoRA adapter identifier (empty for base model)
     * @param input_token_ids Input token IDs
     * @param priority Request priority
     * @param callback Completion callback
     * @return true if request was accepted, false if queue is full
     */
    bool submitRequest(
        int64_t request_id,
        int64_t user_id,
        const std::string& model_id,
        const std::string& lora_adapter_id,
        const std::vector<int>& input_token_ids,
        SchedulingPriority priority,
        RequestCompletionCallback callback
    );
    
    /**
     * @brief Cancel a pending request
     * @param request_id Request identifier to cancel
     * @return true if request was found and cancelled
     */
    bool cancelRequest(int64_t request_id);
    
    /**
     * @brief Process the next batch of requests
     * This is called iteratively by the inference loop
     */
    void processNextBatch();
    
    /**
     * @brief Check if there are requests ready to process
     */
    bool hasPendingRequests() const;
    
    /**
     * @brief Get current batch statistics
     */
    BatchStats getCurrentStats() const;
    
    // ========================================================================
    // Configuration and Control
    // ========================================================================
    
    /**
     * @brief Update scheduler configuration
     * @param config New configuration
     */
    void updateConfig(const ContinuousBatchSchedulerConfig& config);
    
    /**
     * @brief Get current configuration
     */
    const ContinuousBatchSchedulerConfig& getConfig() const;
    
    /**
     * @brief Start the scheduler
     */
    void start();
    
    /**
     * @brief Stop the scheduler
     */
    void stop();
    
    /**
     * @brief Pause scheduling (for maintenance)
     */
    void pause();
    
    /**
     * @brief Resume scheduling
     */
    void resume();
    
    /**
     * @brief Check if scheduler is running
     */
    bool isRunning() const;
    
    // ========================================================================
    // Priority Management
    // ========================================================================
    
    /**
     * @brief Update request priority
     * @param request_id Request identifier
     * @param priority New priority
     * @return true if request was found and updated
     */
    bool updateRequestPriority(int64_t request_id, SchedulingPriority priority);
    
    /**
     * @brief Get current request count by priority
     */
    std::unordered_map<SchedulingPriority, uint32_t> getRequestCountByPriority() const;
    
    // ========================================================================
    // Speculative Decoding Support
    // ========================================================================
    
    /**
     * @brief Enable speculative decoding
     * @param draft_model_callback Callback for draft model generation
     * @param target_model_callback Callback for target model verification
     */
    void enableSpeculativeDecoding(
        TokenGenerationCallback draft_model_callback,
        TokenGenerationCallback target_model_callback
    );
    
    /**
     * @brief Disable speculative decoding
     */
    void disableSpeculativeDecoding();
    
    /**
     * @brief Check if speculative decoding is enabled
     */
    bool isSpeculativeDecodingEnabled() const;
    
    // ========================================================================
    // KV Cache Integration
    // ========================================================================
    
    /**
     * @brief Set KV cache manager
     * @param kv_cache_manager KV cache manager
     */
    void setKVCacheManager(KVCacheManager* kv_cache_manager);
    
    /**
     * @brief Get KV cache manager
     */
    KVCacheManager* getKVCacheManager();
    
    /**
     * @brief Clear KV cache for a specific request
     * @param request_id Request identifier
     */
    void clearKVCache(int64_t request_id);
    
    // ========================================================================
    // Metrics and Monitoring
    // ========================================================================
    
    /**
     * @brief Get detailed statistics as JSON
     */
    nlohmann::json getStatsJson() const;
    
    /**
     * @brief Reset statistics
     */
    void resetStats();
    
    /**
     * @brief Get request statistics by ID
     * @param request_id Request identifier
     * @return RequestStats or nullopt if not found
     */
    std::optional<RequestStats> getRequestStats(int64_t request_id) const;
    
    /**
     * @brief Register metrics callback
     */
    void onMetricsUpdate(std::function<void(const BatchStats&)> callback);
    
private:
    // ========================================================================
    // Internal Types
    // ========================================================================
    
    struct Request {
        RequestStats stats;
        std::vector<int> input_token_ids;
        RequestCompletionCallback callback;
        
        // KV cache state
        uint32_t kv_cache_start_pos = 0;
        uint32_t kv_cache_end_pos = 0;
        
        // Speculative decoding state
        std::vector<int> draft_token_ids;
        uint32_t accepted_draft_tokens = 0;
        bool speculative_active = false;
        
        // Chunked prefill state
        uint32_t current_chunk = 0;
        uint32_t chunks_processed = 0;
    };
    
    // ========================================================================
    // Internal State
    // ========================================================================
    
    ContinuousBatchSchedulerConfig config_;
    KVCacheManager* kv_cache_manager_ = nullptr;
    
    // Request queues by priority
    std::vector<std::queue<Request>> priority_queues_;
    
    // In-progress requests
    std::unordered_map<int64_t, Request> in_progress_requests_;
    
    // Completed requests (recent history)
    std::unordered_map<int64_t, RequestStats> completed_requests_;
    
    // Current batch
    std::vector<int64_t> current_batch_request_ids_;
    
    // Scheduling state
    std::atomic<bool> running_{false};
    std::atomic<bool> paused_{false};
    
    // Statistics
    mutable std::mutex stats_mutex_;
    BatchStats current_stats_;
    std::vector<double> latency_history_ms_;
    
    // Token generation callbacks
    TokenGenerationCallback draft_model_callback_;
    TokenGenerationCallback target_model_callback_;
    
    // Metrics callback
    std::function<void(const BatchStats&)> metrics_callback_;
    
    // Synchronization
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    
    // ========================================================================
    // Internal Methods
    // ========================================================================
    
    /**
     * @brief Get the index for a priority level
     */
    size_t getPriorityQueueIndex(SchedulingPriority priority) const;
    
    /**
     * @brief Select requests for the next batch
     */
    std::vector<int64_t> selectNextBatch();
    
    /**
     * @brief Process prefill phase for a request
     */
    bool processPrefill(Request& request);
    
    /**
     * @brief Process decode phase for a request
     */
    bool processDecode(Request& request);
    
    /**
     * @brief Process chunked prefill
     */
    bool processChunkedPrefill(Request& request);
    
    /**
     * @brief Process speculative decoding
     */
    bool processSpeculativeDecoding(Request& request);
    
    /**
     * @brief Check if preemption is needed
     */
    void checkPreemption();
    
    /**
     * @brief Update request state
     */
    void updateRequestState(int64_t request_id, RequestState state);
    
    /**
     * @brief Update statistics
     */
    void updateStats(const Request& request);
    
    /**
     * @brief Notify metrics callback
     */
    void notifyMetricsCallback();
};

} // namespace sharding
} // namespace themisdb
