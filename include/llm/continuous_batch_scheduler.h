/**
 * @file continuous_batch_scheduler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: continuous_batch_scheduler.h | Version: 0.0.47 | Last Modified: 2026-05-31 12:17:24
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 321
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * PR History (last 5): #5144 research: revise DB_NATIVE_... (2026-05-14) | #242 Complete PagedAttention int... (2026-03-11) | #1215 Fix thread-safety: atomic c... (2026-03-11)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
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

/**
 * @brief Continuous Batching Scheduler (vLLM-style) with PagedAttention Integration
 * 
 * Implements dynamic batching with efficient memory management through PagedAttention.
 * Supports 100+ concurrent requests with iterative scheduling, preemption, and
 * intelligent block management.
 * 
 * Key Features:
 * - Block-based KV cache allocation via PagedKVCache
 * - Memory-aware scheduling prevents OOM scenarios
 * - Accurate TTFT and TPS metrics with first_token_at tracking
 * - Configurable memory pressure handling
 * 
 * Thread Safety: All public methods are thread-safe via internal mutex.
 * 
 * @see docs/llm/PAGED_ATTENTION_INTEGRATION.md for detailed documentation
 */
class ContinuousBatchScheduler {
public:
    /**
     * @brief Configuration for the scheduler
     * 
     * Note: block_size_tokens MUST match PagedKVCache::Config::block_size
     * for correct availability calculations.
     */
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
    
    /**
     * @brief Represents a scheduled inference request with PagedAttention tracking
     * 
     * Tracks request lifecycle, allocated blocks, and timing metrics for
     * accurate TTFT and TPS calculation.
     */
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
    
    explicit ContinuousBatchScheduler(
        const SchedulerConfig& config,
        PagedKVCache* kv_cache
    );
    
    ~ContinuousBatchScheduler();
    
    // Attach a metrics collector for queue-length and backpressure-drop
    // instrumentation.  May be called at any time after construction; safe to
    // call nullptr to detach.  Ownership is NOT transferred.
    void setMetricsCollector(monitoring::LLMMetricsCollector* collector) {
        std::lock_guard<std::mutex> lock(mutex_);
        metrics_collector_ = collector;
    }

    // Attach a TokenQuotaManager for per-user/per-model token-per-minute
    // enforcement.  submitRequest() will call check() and, on success,
    // consume() on the manager.  Pass nullptr to disable quota checks.
    // Ownership is NOT transferred.
    void setQuotaManager(TokenQuotaManager* quota) {
        std::lock_guard<std::mutex> lock(mutex_);
        quota_manager_ = quota;
    }

    /**
     * @brief Callback type for shard load telemetry.
     *
     * Fired on every queue-depth change (submitRequest and processBatchResults)
     * so that the AdaptiveShardRouter can keep its LLM-load table up-to-date
     * for LEAST_LOADED routing decisions.
     *
     * @param pending     Current number of requests in the waiting queue.
     * @param avg_queue_ms  Estimated average queue wait time in milliseconds.
     */
    using ShardLoadCallback = std::function<void(size_t pending, double avg_queue_ms)>;

    /**
     * @brief Inject a shard-load callback.
     *
     * When set, the callback is invoked (while holding the internal mutex) at
     * the end of submitRequest() and processBatchResults() whenever the queue
     * depth changes.  Pass an empty std::function to detach.
     * Ownership of any captured state is the caller's responsibility.
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
    bool cancelRequest(const std::string& request_id);
    bool reprioritizeRequest(const std::string& request_id, RequestPriority new_priority);
    
    // Scheduler lifecycle
    void start();
    void stop();
    bool isRunning() const;
    
    // Batch scheduling (main loop)
    std::vector<ScheduledRequest*> scheduleNextBatch();
    
    // Process batch results
    void processBatchResults(
        const std::vector<ScheduledRequest*>& batch,
        const std::vector<InferenceResponse>& responses
    );
    
    // Preemption
    void preemptRequests(const std::vector<std::string>& request_ids);
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
        /// Times a decode step was skipped due to KV cache budget exhaustion
        /// (n_ctx / blocks_free == 0 guard, Phase 3).
        size_t kv_budget_exhausted_count = 0;
    };
    
    Stats getStats() const;

    /**
     * @brief Snapshot of LLM queue telemetry for ShardStats integration.
     *
     * Returned by getLLMStats() and intended to be forwarded into a
     * sharding::ShardStats struct so that the AdaptiveShardRouter can make
     * LLM-load-aware routing decisions.
     */
    struct LLMStats {
        /// Number of requests currently waiting in the scheduler queue.
        size_t pending_requests = 0;
        /// Moving average queue wait time in milliseconds, or 0.0 when idle.
        double avg_queue_ms = 0.0;
    };

    /**
     * @brief Return a point-in-time snapshot of LLM queue metrics.
     *
     * Thread-safe; acquires the internal scheduler mutex briefly.
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
    
    // Synchronization
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    bool running_ = false;
    
    // Statistics
    Stats stats_;
    std::chrono::system_clock::time_point last_schedule_time_;
    // Adaptive prefill chunk state; accessed only while holding mutex_ in
    // scheduleNextBatch() and processBatchResults().
    size_t effective_prefill_chunk_size_ = 0;
    
    // Internal helpers
    bool canAddToBatch(const ScheduledRequest* request,
                      size_t current_batch_tokens,
                      size_t reserved_blocks) const;
    void allocateKVCacheBlocks(ScheduledRequest* request);
    void freeKVCacheBlocks(ScheduledRequest* request);
    void updateStats();
    
    std::string generateRequestId();
    
    // Thread-safe counters using atomics
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

