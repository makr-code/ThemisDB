/**
 * @file continuous_batch_scheduler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "llm/llm_plugin_interface.h"
#include "llm/paged_kv_cache.h"
#include "llm/grafana_metrics.h"
#include "llm/token_quota_manager.h"
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <functional>

namespace themis {
namespace llm {

class ContinuousBatchScheduler {
public:
    struct SchedulerConfig {
        size_t max_batch_size = 256;           // Max sequences in batch
        size_t max_concurrent_requests = 128;   // Max pending requests
        size_t max_tokens_per_batch = 8192;    // Total token budget
        
        // Backpressure: maximum combined waiting + active requests.
        // When the queue reaches this depth, submitRequest() returns an empty
        // string immediately instead of enqueuing the request.
        // 0 means unlimited (no backpressure).
        size_t max_queue_depth = 0;
        
        // Scheduling policy
        bool enable_preemption = true;
        bool enable_chunked_prefill = true;    // Chunk large prefills
        size_t prefill_chunk_size = 512;       // Tokens per prefill chunk
        bool enable_adaptive_batch_retry = false; // Retry decode failures with downshift
        
        // Priority scheduling
        bool enable_priority_scheduling = true;
        size_t high_priority_threshold = 10;
        
        // Performance
        size_t scheduling_overhead_ms = 5;     // Target scheduling time
        bool enable_continuous_batching = true;
        
        // Memory management (PagedAttention integration)
        size_t block_size_tokens = 16;         // Tokens per block (MUST match PagedKVCache)
        size_t low_memory_threshold_blocks = 10;  // Memory pressure trigger
        double memory_pressure_throughput_factor = 0.8;  // Throughput reduction (0.0-1.0)
    };
    // Backwards-compatibility alias: older callers used `ContinuousBatchScheduler::Config`
    using Config = SchedulerConfig;
    
    enum class RequestPriority {
        LOW = 0,
        NORMAL = 5,
        HIGH = 10,
        CRITICAL = 15
    };
    
    enum class RequestState {
        WAITING,       // In queue
        PREFILL,       // Processing prompt
        DECODE,        // Generating tokens
        PREEMPTED,     // Temporarily paused
        COMPLETED,     // Finished
        FAILED         // Error occurred
    };
    
    struct ScheduledRequest {
        std::string request_id;
        InferenceRequest inference_request;
        
        RequestPriority priority = RequestPriority::NORMAL;
        RequestState state = RequestState::WAITING;
        
        // Progress tracking
        size_t tokens_processed = 0;
        size_t tokens_generated = 0;
        size_t total_prompt_tokens = 0;
        
        // PagedAttention integration
        std::vector<int> allocated_blocks;  // Physical block IDs from PagedKVCache
        int sequence_id = -1;                // Unique sequence identifier
        
        // Timestamps for accurate metrics
        std::chrono::system_clock::time_point submitted_at;   // Request submission time
        std::chrono::system_clock::time_point started_at;     // Prefill start time
        std::chrono::system_clock::time_point first_token_at; // First token generated (for TTFT)
        std::chrono::system_clock::time_point last_token_at;
        
        // Callback
        std::function<void(const InferenceResponse&)> callback;
        
        // Preemption support
        bool can_be_preempted = true;
        size_t preemption_count = 0;
    };
    
    /**
     * @brief Continuous Batch Scheduler.
     * @param[in] config Input parameter.
     * @param[in,out] kv_cache Input/output parameter.
     * @return Return value.
     */
    explicit ContinuousBatchScheduler(
        const SchedulerConfig& config,
        PagedKVCache* kv_cache
    );
    
    ~ContinuousBatchScheduler();
    
    /**
     * @brief Attach a metrics collector for queue-length and backpressure-drop instrumentation.
     * @param[in,out] collector Input/output parameter.
     * @details May be called at any time after construction; safe to call nullptr to detach. Ownership is NOT transferred. Calls: lock().
     */
    void setMetricsCollector(monitoring::LLMMetricsCollector* collector) {
        std::lock_guard<std::mutex> lock(mutex_);
        metrics_collector_ = collector;
    }

    /**
     * @brief Attach a TokenQuotaManager for per-user/per-model token-per-minute enforcement.
     * @param[in,out] quota Input/output parameter.
     * @details submitRequest() will call check() and, on success, consume() on the manager. Pass nullptr to disable quota checks. Ownership is NOT transferred. Calls: lock().
     */
    void setQuotaManager(TokenQuotaManager* quota) {
        std::lock_guard<std::mutex> lock(mutex_);
        quota_manager_ = quota;
    }

    using ShardLoadCallback = std::function<void(size_t pending, double avg_queue_ms)>;

    /**
     * @brief Set Shard Load Callback.
     * @param[in] cb Input parameter.
     * @details Calls: lock(), std::move().
     */
    void setShardLoadCallback(ShardLoadCallback cb) {
        std::lock_guard<std::mutex> lock(mutex_);
        shard_load_cb_ = std::move(cb);
    }
    
    // Request submission
    std::string submitRequest(
        const InferenceRequest& request,
        RequestPriority priority = RequestPriority::NORMAL,
        std::function<void(const InferenceResponse&)> callback = nullptr
    );
    
    // Request management
    /**
     * @brief Cancel Request.
     * @param[in] request_id Identifier of the request.
     * @return True when the operation succeeds.
     */
    bool cancelRequest(const std::string& request_id);
    /**
     * @brief Reprioritize Request.
     * @param[in] request_id Identifier of the request.
     * @param[in] new_priority Input parameter.
     * @return True when the operation succeeds.
     */
    bool reprioritizeRequest(const std::string& request_id, RequestPriority new_priority);
    
    // Scheduler lifecycle
    /**
     * @brief Start.
     */
    void start();
    /**
     * @brief Stop.
     */
    void stop();
    /**
     * @brief Is Running.
     * @return True when the operation succeeds.
     */
    bool isRunning() const;
    
    /**
     * @brief Batch scheduling (main loop)
     * @return Return value.
     */
    std::vector<ScheduledRequest*> scheduleNextBatch();
    
    // Process batch results
    /**
     * @brief Process Batch Results.
     * @param[in] batch Input parameter.
     * @param[in] responses Input parameter.
     */
    void processBatchResults(
        const std::vector<ScheduledRequest*>& batch,
        const std::vector<InferenceResponse>& responses
    );
    
    // Preemption
    /**
     * @brief Preempt Requests.
     * @param[in] request_ids Input parameter.
     */
    void preemptRequests(const std::vector<std::string>& request_ids);
    /**
     * @brief Resume Requests.
     * @param[in] request_ids Input parameter.
     */
    void resumeRequests(const std::vector<std::string>& request_ids);
    
    // Statistics
    struct Stats {
        size_t total_requests = 0;
        size_t active_requests = 0;
        size_t completed_requests = 0;
        size_t failed_requests = 0;
        size_t preempted_requests = 0;
        // Requests shed by backpressure (queue depth limit reached)
        size_t rejected_requests = 0;
        
        double avg_scheduling_time_ms = 0.0;
        double avg_time_to_first_token_ms = 0.0;
        double avg_tokens_per_second = 0.0;
        
        size_t current_batch_size = 0;
        size_t max_batch_size_seen = 0;
        size_t batch_retry_count = 0;
        size_t adaptive_prefill_chunk_size_tokens = 0;
        // Current combined depth of waiting + active requests
        size_t current_queue_depth = 0;
        size_t kv_budget_exhausted_count = 0;
    };
    
    /**
     * @brief Get Stats.
     * @return Return value.
     */
    Stats getStats() const;

    struct LLMStats {
        size_t pending_requests = 0;
        double avg_queue_ms = 0.0;
    };

    /**
     * @brief Get LLMStats.
     * @return Return value.
     */
    LLMStats getLLMStats() const;
    
private:
    SchedulerConfig config_;
    PagedKVCache* kv_cache_;
    // Optional metrics collector — not owned, may be nullptr
    monitoring::LLMMetricsCollector* metrics_collector_ = nullptr;
    // Optional quota manager — not owned, may be nullptr
    TokenQuotaManager* quota_manager_ = nullptr;
    // Optional shard-load callback — fired on queue depth changes
    ShardLoadCallback shard_load_cb_;
    
    // Request queues by priority
    std::priority_queue<
        std::shared_ptr<ScheduledRequest>,
        std::vector<std::shared_ptr<ScheduledRequest>>,
        std::function<bool(const std::shared_ptr<ScheduledRequest>&,
                          const std::shared_ptr<ScheduledRequest>&)>
    > waiting_queue_;
    
    // Active requests (currently in batch)
    std::vector<std::shared_ptr<ScheduledRequest>> active_requests_;
    
    // Preempted requests
    std::vector<std::shared_ptr<ScheduledRequest>> preempted_requests_;
    
    // Completed requests (for stats)
    std::vector<std::shared_ptr<ScheduledRequest>> completed_requests_;
    
    // Request lookup
    std::unordered_map<std::string, std::shared_ptr<ScheduledRequest>> all_requests_;
    
    // LOCK HIERARCHY ENFORCEMENT (§3.3):
    // ┌─ mutex_ : std::mutex (exclusive access to all request queues and state)
    // └─ cv_ : std::condition_variable (paired with mutex_)
    // Note: No nested locks - external callbacks (metrics_collector_, shard_load_cb_)
    //       are invoked while holding mutex_ but must not acquire it themselves
    
    mutable std::mutex mutex_;
    
    std::condition_variable cv_;
    
    bool running_ = false;
    
    // Statistics (protected by mutex_)
    Stats stats_;
    std::chrono::system_clock::time_point last_schedule_time_;
    // Adaptive prefill chunk state; accessed only while holding mutex_ in
    // scheduleNextBatch() and processBatchResults().
    size_t effective_prefill_chunk_size_ = 0;
    
    // Internal helpers
    /**
     * @brief Can Add To Batch.
     * @param[in] request Input parameter.
     * @param[in] current_batch_tokens Input parameter.
     * @param[in] reserved_blocks Input parameter.
     * @return True when the operation succeeds.
     */
    bool canAddToBatch(const ScheduledRequest* request,
                      size_t current_batch_tokens,
                      size_t reserved_blocks) const;
    /**
     * @brief Allocate KVCache Blocks.
     * @param[in,out] request Input/output parameter.
     */
    void allocateKVCacheBlocks(ScheduledRequest* request);
    /**
     * @brief Free KVCache Blocks.
     * @param[in,out] request Input/output parameter.
     */
    void freeKVCacheBlocks(ScheduledRequest* request);
    /**
     * @brief Update Stats.
     */
    void updateStats();
    
    /**
     * @brief Generate Request Id.
     * @return Return value.
     */
    std::string generateRequestId();
    
    // Thread-safe counters using atomics (std::memory_order_relaxed)
    // Note: These are incremented during request submission under mutex_,
    // but making them atomic is defensive programming for future changes
    std::atomic<int> next_request_id_{0};
    std::atomic<int> next_sequence_id_{0};
};

} // namespace llm
} // namespace themis

// Backwards-compatibility: expose the nested RequestPriority as a namespace-level
// alias so older tests and callers can refer to `themis::llm::RequestPriority`.
namespace themis {
namespace llm {
using RequestPriority = ContinuousBatchScheduler::RequestPriority;
}
}

