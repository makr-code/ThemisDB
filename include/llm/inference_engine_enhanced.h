/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            inference_engine_enhanced.h                        ║
  Version:         0.0.23                                             ║
  Last Modified:   2026-02-21 19:42:52                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     268                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 03329d86d  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31e8b8df0  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 0d722b04c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "llm/inference_handle.h"
#include "llm/continuous_batch_scheduler.h"
#include "llm/paged_kv_cache.h"
#include "llm/llm_prefix_cache.h"
#include "llm/llm_plugin_interface.h"
#include <memory>
#include <vector>
#include <unordered_map>
#include <queue>
#include <chrono>
#include <atomic>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

/**
 * @file inference_engine_enhanced.h
 * @brief Enhanced LLM Inference Engine with P1 Features
 * 
 * Implements the P1 Enterprise Features:
 * 1. Context Caching (KV-Cache Reuse) with LRU eviction
 * 2. Batch Processing with dynamic sizing
 * 3. Request Queuing with priority and timeout
 * 4. Load Balancing with multi-model support
 * 
 * Acceptance Criteria:
 * - Context cache hit rate > 80%
 * - Batch processing improves throughput by > 2x
 * - Queue prevents request drops under load
 * - Load balancer distributes requests evenly
 */

namespace themis {
namespace llm {

/**
 * @brief Enhanced inference engine with context caching and load balancing
 */
class InferenceEngineEnhanced {
public:
    struct Config {
        // Context caching
        bool enable_context_caching = true;
        size_t max_cache_entries = 10000;
        double cache_similarity_threshold = 0.95;
        int cache_ttl_seconds = 7200;  // 2 hours
        
        // Batch processing
        bool enable_batch_processing = true;
        size_t min_batch_size = 1;
        size_t max_batch_size = 256;
        size_t batch_timeout_ms = 100;  // Wait up to 100ms to form batch
        size_t max_tokens_per_batch = 8192;
        
        // Request queuing
        size_t max_queue_size = 1000;
        size_t request_timeout_ms = 30000;  // 30 seconds
        bool enable_priority_scheduling = true;
        
        // Load balancing
        bool enable_load_balancing = true;
        enum class LoadBalanceStrategy {
            ROUND_ROBIN,
            LEAST_LOADED,
            RESPONSE_TIME_WEIGHTED
        };
        LoadBalanceStrategy load_balance_strategy = LoadBalanceStrategy::LEAST_LOADED;
        
        // Worker threads
        size_t num_worker_threads = 4;
    };
    
    /**
     * @brief Statistics for monitoring and optimization
     */
    struct Statistics {
        // Context caching metrics
        size_t cache_hits = 0;
        size_t cache_misses = 0;
        double cache_hit_rate = 0.0;
        size_t tokens_saved_by_cache = 0;
        
        // Batch processing metrics
        size_t total_batches = 0;
        double avg_batch_size = 0.0;
        size_t max_batch_size_seen = 0;
        double throughput_improvement = 0.0;
        
        // Queue metrics
        size_t total_requests = 0;
        size_t completed_requests = 0;
        size_t timed_out_requests = 0;
        size_t rejected_requests = 0;
        size_t current_queue_size = 0;
        
        // Load balancing metrics
        std::unordered_map<std::string, size_t> requests_per_model;
        std::unordered_map<std::string, double> avg_latency_per_model;
        double load_balance_fairness = 0.0;  // 1.0 = perfectly balanced
        
        // Overall performance
        double avg_latency_ms = 0.0;
        double p95_latency_ms = 0.0;
        double p99_latency_ms = 0.0;
        double tokens_per_second = 0.0;
    };
    
    /**
     * @brief Model information for load balancing
     */
    struct ModelInfo {
        std::string model_id;
        std::shared_ptr<ILLMPlugin> plugin;
        size_t active_requests = 0;
        double avg_response_time_ms = 0.0;
        size_t total_requests = 0;
        bool is_available = true;
    };
    
    /**
     * @brief Enhanced inference request with caching metadata
     */
    struct EnhancedInferenceRequest {
        InferenceRequest base_request;
        int priority = 0;
        std::chrono::milliseconds timeout{30000};
        bool allow_caching = true;
        std::string preferred_model_id;  // Optional model preference
        
        // For result tracking
        std::string request_id;
        std::chrono::steady_clock::time_point submitted_at;
    };
    
    explicit InferenceEngineEnhanced(const Config& config);
    ~InferenceEngineEnhanced();
    
    // Model management
    void registerModel(const std::string& model_id, std::shared_ptr<ILLMPlugin> plugin);
    void unregisterModel(const std::string& model_id);
    std::vector<std::string> getAvailableModels() const;
    
    // Inference submission
    InferenceHandle submit(const EnhancedInferenceRequest& request);
    std::string submitAsync(
        const EnhancedInferenceRequest& request,
        std::function<void(const InferenceResponse&)> callback
    );
    
    // Request management
    bool cancel(const std::string& request_id);
    bool reprioritize(const std::string& request_id, int new_priority);
    
    // Cache management
    void clearCache();
    void prewarmCache(const std::vector<std::string>& common_prompts);
    
    // Statistics and monitoring
    Statistics getStatistics() const;
    json getDetailedMetrics() const;
    
    // Lifecycle
    void start();
    void shutdown();
    bool isRunning() const;
    
private:
    Config config_;
    std::atomic<bool> running_{false};
    
    // Core components
    std::unique_ptr<LLMPrefixCache> prefix_cache_;
    std::shared_ptr<PagedKVCache> kv_cache_;
    std::shared_ptr<PagedBlockManager> block_manager_;
    std::unique_ptr<ContinuousBatchScheduler> batch_scheduler_;
    
    // Model registry for load balancing
    std::unordered_map<std::string, ModelInfo> models_;
    mutable std::mutex models_mutex_;
    std::atomic<size_t> round_robin_index_{0};
    
    // Request tracking
    struct TrackedRequest {
        EnhancedInferenceRequest request;
        std::chrono::steady_clock::time_point deadline;
        std::promise<InferenceResponse> promise;
        std::function<void(const InferenceResponse&)> callback;
    };
    std::unordered_map<std::string, std::shared_ptr<TrackedRequest>> tracked_requests_;
    mutable std::mutex requests_mutex_;
    
    // Statistics
    Statistics stats_;
    mutable std::mutex stats_mutex_;
    std::vector<double> latency_samples_;  // For percentile calculation
    
    // Worker threads for request processing
    std::vector<std::thread> worker_threads_;
    std::queue<std::shared_ptr<TrackedRequest>> request_queue_;
    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
    
    // Timeout monitoring thread
    std::thread timeout_thread_;
    
    // Internal methods
    void workerLoop(size_t worker_id);
    void timeoutMonitorLoop();
    void processBatch(const std::vector<std::shared_ptr<TrackedRequest>>& batch);
    
    // Cache integration
    std::optional<InferenceResponse> checkCache(const InferenceRequest& request);
    void updateCache(const InferenceRequest& request, const InferenceResponse& response);
    std::string generateCacheKey(const InferenceRequest& request);
    
    // Load balancing
    std::string selectModel(const EnhancedInferenceRequest& request);
    void updateModelStats(const std::string& model_id, double latency_ms, bool success);
    
    // Batch formation
    std::vector<std::shared_ptr<TrackedRequest>> formBatch();
    bool canAddToBatch(const std::shared_ptr<TrackedRequest>& req, size_t current_batch_tokens);
    
    // Timeout handling
    void checkAndHandleTimeouts();
    
    // Statistics updates
    void recordCacheHit(size_t tokens_saved);
    void recordCacheMiss();
    void recordBatchCompletion(size_t batch_size);
    void recordRequestCompletion(double latency_ms, const std::string& model_id);
    void recordRequestTimeout();
    
    // Helper methods
    std::string generateRequestId();
    std::atomic<uint64_t> request_counter_{0};
};

} // namespace llm
} // namespace themis
