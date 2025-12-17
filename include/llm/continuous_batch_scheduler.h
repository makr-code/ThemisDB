#pragma once

#include "llm/llm_plugin_interface.h"
#include "llm/paged_kv_cache.h"
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <functional>

namespace themis {
namespace llm {

/**
 * @brief Continuous Batching Scheduler (vLLM-style)
 * 
 * Week 7-9 Implementation: Dynamic batching with PagedAttention integration.
 * Supports 100+ concurrent requests with iterative scheduling and preemption.
 */
class ContinuousBatchScheduler {
public:
    struct SchedulerConfig {
        size_t max_batch_size = 256;           // Max sequences in batch
        size_t max_concurrent_requests = 128;   // Max pending requests
        size_t max_tokens_per_batch = 8192;    // Total token budget
        
        // Scheduling policy
        bool enable_preemption = true;
        bool enable_chunked_prefill = true;    // Chunk large prefills
        size_t prefill_chunk_size = 512;       // Tokens per prefill chunk
        
        // Priority scheduling
        bool enable_priority_scheduling = true;
        size_t high_priority_threshold = 10;
        
        // Performance
        size_t scheduling_overhead_ms = 5;     // Target scheduling time
        bool enable_continuous_batching = true;
    };
    
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
        std::vector<int> allocated_blocks;
        int sequence_id = -1;
        
        // Timestamps
        std::chrono::system_clock::time_point submitted_at;
        std::chrono::system_clock::time_point started_at;
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
        
        double avg_scheduling_time_ms = 0.0;
        double avg_time_to_first_token_ms = 0.0;
        double avg_tokens_per_second = 0.0;
        
        size_t current_batch_size = 0;
        size_t max_batch_size_seen = 0;
    };
    
    Stats getStats() const;
    
private:
    SchedulerConfig config_;
    PagedKVCache* kv_cache_;
    
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
    
    // Internal helpers
    bool canAddToBatch(const ScheduledRequest* request, size_t current_batch_tokens) const;
    void allocateKVCacheBlocks(ScheduledRequest* request);
    void freeKVCacheBlocks(ScheduledRequest* request);
    void updateStats();
    
    std::string generateRequestId();
    int next_request_id_ = 0;
    int next_sequence_id_ = 0;
};

} // namespace llm
} // namespace themis
