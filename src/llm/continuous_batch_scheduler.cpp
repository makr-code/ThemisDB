/**
 * @file continuous_batch_scheduler.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=13, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "llm/continuous_batch_scheduler.h"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <shared_mutex>
#include <chrono>

namespace themis {
namespace llm {

// Rough estimate of characters per token.  This is model-dependent (e.g. ~3.5
// for English prose in Llama-style tokenizers, 4 is a safe conservative
// upper bound for ASCII text).  A future improvement is to expose this in
// SchedulerConfig so operators can tune it per model.
static constexpr size_t CHARS_PER_TOKEN_ESTIMATE = 4;

static size_t ensureMinimumPrefillChunkSize([[maybe_unused]] size_t configured_size) {
    // Enforce a lower bound of 1 so that chunked prefill never stalls completely.
    return std::max<size_t>(1, configured_size);
}

ContinuousBatchScheduler::ContinuousBatchScheduler(
    const SchedulerConfig& config,
    PagedKVCache* kv_cache
) : config_(config),
    kv_cache_(kv_cache),
    metrics_collector_(nullptr),
    quota_manager_(nullptr),
    shard_load_cb_(),
    waiting_queue_(
        [](const std::shared_ptr<ScheduledRequest>& a,
           const std::shared_ptr<ScheduledRequest>& b) {
            // Higher priority first, then FIFO
            if (a->priority != b->priority) {
                return a->priority < b->priority;  // Max heap
            }
            return a->submitted_at > b->submitted_at;  // Earlier first
        }
    ),
    active_requests_(),
    preempted_requests_(),
    completed_requests_(),
    all_requests_(),
    mutex_(),
    cv_(),
    running_(false),
    stats_(),
    last_schedule_time_(),
    effective_prefill_chunk_size_(ensureMinimumPrefillChunkSize(config.prefill_chunk_size)) {
    
    spdlog::info("Continuous Batch Scheduler initialized (vLLM-style):");
    spdlog::info("  Max batch size: {}", config_.max_batch_size);
    spdlog::info("  Max concurrent: {}", config_.max_concurrent_requests);
    spdlog::info("  Preemption: {}", config_.enable_preemption ? "enabled" : "disabled");
    spdlog::info("  Priority scheduling: {}", config_.enable_priority_scheduling ? "enabled" : "disabled");
    stats_.adaptive_prefill_chunk_size_tokens = effective_prefill_chunk_size_;
}

ContinuousBatchScheduler::~ContinuousBatchScheduler() {
    stop();
}

std::string ContinuousBatchScheduler::submitRequest(
    const InferenceRequest& request,
    RequestPriority priority,
    std::function<void(const InferenceResponse&)> callback
) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Enforce maximum queue depth (backpressure).  Measure combined depth of
    // waiting queue + active requests so that the limit covers all in-flight
    // work, not just the waiting queue alone.
    if (config_.max_queue_depth > 0) {
        size_t current_depth = static_cast<int>(waiting_queue_.size()) + static_cast<int>(active_requests_.size()) ;
        if (current_depth >= config_.max_queue_depth) {
            stats_.rejected_requests++;
            if (metrics_collector_) {
                metrics_collector_->recordBackpressureDrop();
            }
            spdlog::warn("ContinuousBatchScheduler: queue full ({}/{}) — request rejected (backpressure)",
                         current_depth, config_.max_queue_depth);
            return {};  // Empty string signals rejection to caller
        }
    }

    // Per-user / per-model token quota check.  Estimated prompt tokens are
    // charged here (conservative pre-charge); actual tokens should also be
    // consumed via quota_manager_->consume() after the response is ready.
    if (quota_manager_) {
        const std::string& user_id  = request.request_id;  // best available key at ingestion
        const std::string& model_id = request.model_id;
        const size_t estimated = request.prompt.length() / CHARS_PER_TOKEN_ESTIMATE + request.max_tokens;
        auto qr = quota_manager_->check(user_id, model_id, estimated);
        if (!qr.allowed) {
            stats_.rejected_requests++;
            spdlog::warn("ContinuousBatchScheduler: quota exceeded — {}",
                         qr.reason);
            return {};
        }
        // Pre-charge the estimated tokens; callers should adjust via consume()
        quota_manager_->consume(user_id, model_id, estimated);
    }
    
    auto scheduled = std::make_shared<ScheduledRequest>();
    scheduled->request_id = generateRequestId();
    scheduled->inference_request = request;
    scheduled->priority = priority;
    scheduled->state = RequestState::WAITING;
    scheduled->submitted_at = std::chrono::system_clock::now();
    scheduled->callback = callback;
    scheduled->sequence_id = next_sequence_id_.fetch_add(1, std::memory_order_relaxed);
    
    // Estimate prompt tokens (simplified)
    scheduled->total_prompt_tokens = request.prompt.length() / CHARS_PER_TOKEN_ESTIMATE;
    
    // Add to queue and lookup
    waiting_queue_.push(scheduled);
    all_requests_[scheduled->request_id] = scheduled;
    
    stats_.total_requests++;
    
    spdlog::debug("Request submitted: {} (priority: {}, tokens: ~{})",
                  scheduled->request_id,
                  static_cast<int>(priority),
                  scheduled->total_prompt_tokens);
    
    // Wake up scheduler
    cv_.notify_one();

    // Notify shard router about updated queue depth.
    // Called inside the mutex — AdaptiveShardRouter's own lock is independent
    // so there is no circular lock ordering.
    if (shard_load_cb_) {
        shard_load_cb_(waiting_queue_.size(), stats_.avg_time_to_first_token_ms);
    }

    return scheduled->request_id;
}

bool ContinuousBatchScheduler::cancelRequest(const std::string& request_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = all_requests_.find(request_id);
    if (it == all_requests_.end()) {
        return false;
    }
    
    auto& request = it->second;
    
    // Free KV cache if allocated
    if (!request->allocated_blocks.empty()) {
        freeKVCacheBlocks(request.get());
    }
    
    request->state = RequestState::FAILED;
    
    // Remove from active list if present
    active_requests_.erase(
        std::remove_if(active_requests_.begin(), active_requests_.end(),
                      [&]([[maybe_unused]] const auto& r) { return r->request_id == request_id; }),
        active_requests_.end()
    );
    
    spdlog::info("Request cancelled: {}", request_id);
    
    all_requests_.erase(it);
    
    return true;
}

bool ContinuousBatchScheduler::reprioritizeRequest(
    const std::string& request_id,
    RequestPriority new_priority
) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = all_requests_.find(request_id);
    if (it == all_requests_.end()) {
        return false;
    }
    
    it->second->priority = new_priority;
    
    spdlog::debug("Request {} reprioritized to {}",
                  request_id, static_cast<int>(new_priority));
    
    return true;
}

void ContinuousBatchScheduler::start() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (running_) {
        return;
    }
    
    running_ = true;
    spdlog::info("Continuous Batch Scheduler started");
}

void ContinuousBatchScheduler::stop() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!running_) {
        return;
    }
    
    running_ = false;
    cv_.notify_all();
    
    spdlog::info("Continuous Batch Scheduler stopped");
}

bool ContinuousBatchScheduler::isRunning() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return running_;
}

std::vector<ContinuousBatchScheduler::ScheduledRequest*> 
ContinuousBatchScheduler::scheduleNextBatch() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto start_time = std::chrono::steady_clock::now();
    
    std::vector<ScheduledRequest*> batch;
    size_t total_tokens = 0;
    size_t reserved_blocks_in_batch = 0;
    
    // First, continue active decode requests
    for (auto& req : active_requests_) {
        if (req->state == RequestState::DECODE) {
            // Phase 3 n_ctx / KV-budget guard: ensure at least one free block
            // is available before scheduling a decode step.  A fully exhausted
            // KV cache must not enter the decode loop — it would cause a
            // silent decode failure or corrupt the KV state.
            // Note: if all blocks are held by active decode requests, they will
            // be freed as those requests complete, so the stall is self-healing.
            // Requests that stall here for longer than request_timeout_ms will
            // be cancelled by the engine's timeout monitor (enforced externally).
            if (kv_cache_) {
                const auto kv_stats = kv_cache_->getStats();
                if (kv_stats.blocks_free == 0) {
                    spdlog::warn("KV cache exhausted; skipping decode for request {}",
                                 req->request_id);
                    stats_.kv_budget_exhausted_count++;
                    continue;
                }
            }
            if (canAddToBatch(req.get(), total_tokens, reserved_blocks_in_batch)) {
                batch.push_back(req.get());
                total_tokens++;  // Each decode request adds 1 token
            }
        }
    }
    
    // Then, process waiting requests (prefill)
    while (!waiting_queue_.empty() && static_cast<int>(batch.size()) < config_.max_batch_size) {
        auto req = waiting_queue_.top();
        waiting_queue_.pop();
        
        req->state = RequestState::PREFILL;
        req->started_at = std::chrono::system_clock::now();
        
        // Allocate KV cache blocks
        allocateKVCacheBlocks(req.get());
        
        // Check if we can add to batch
        size_t prefill_tokens = config_.enable_chunked_prefill
            ? std::min(req->total_prompt_tokens, effective_prefill_chunk_size_)
            : req->total_prompt_tokens;
        
        if (canAddToBatch(req.get(), total_tokens + prefill_tokens, reserved_blocks_in_batch)) {
            batch.push_back(req.get());
            total_tokens += prefill_tokens;
            reserved_blocks_in_batch += req-> static_cast<int>(allocated_blocks.size());
            active_requests_.push_back(req);
        } else {
            // Put back in queue
            waiting_queue_.push(req);
            break;
        }
    }
    
    // Update stats
    stats_.current_batch_size = batch.size();
    stats_.max_batch_size_seen = std::max(stats_.max_batch_size_seen,static_cast<int>(batch.size()));
    stats_.active_requests = active_requests_.size();
    stats_.current_queue_depth = static_cast<int>(waiting_queue_.size()) + static_cast<int>(active_requests_.size()) ;
    
    // Emit queue-length metric so Prometheus/Grafana can visualise scheduler
    // pressure in real time.  Called under the scheduler lock so the value is
    // consistent with stats_.current_queue_depth.
    if (metrics_collector_) {
        metrics_collector_->recordQueueLength(stats_.current_queue_depth);
    }
    
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time
    ).count();
    
    stats_.avg_scheduling_time_ms = 
        (stats_.avg_scheduling_time_ms * 0.9) + (duration * 0.1);  // EMA
    
    spdlog::debug("Scheduled batch: {} requests ({} tokens, {} ms)",
                  batch.size(), total_tokens, duration);
    
    return batch;
}

void ContinuousBatchScheduler::processBatchResults(
    const std::vector<ScheduledRequest*>& batch,
    const std::vector<InferenceResponse>& responses
) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (static_cast<int>(batch.size()) != static_cast<int>(responses.size())) {
        spdlog::error("Batch size mismatch: {} requests, {} responses",
                      batch.size(),static_cast<int>(responses.size()));
        return;
    }
    
    bool saw_decode_error = false;
    std::vector<ScheduledRequest*> to_retry;

    for (size_t i = 0; i <static_cast<int>(batch.size()); ++i) {
        auto* req = batch[i];
        const auto& resp = responses[i];
        const bool decode_failed = !resp.error_message.empty();
        if (decode_failed) {
            saw_decode_error = true;
            if (config_.enable_adaptive_batch_retry) {
                freeKVCacheBlocks(req);
                req->state = RequestState::WAITING;
                req->tokens_generated = 0;
                to_retry.push_back(req);
                continue;
            }

            req->state = RequestState::FAILED;
            freeKVCacheBlocks(req);
            active_requests_.erase(
                std::remove_if(active_requests_.begin(), active_requests_.end(),
                              [req](const auto& r) { return r && r.get() == req; }),
                active_requests_.end()
            );
            if ([[maybe_unused]] req->callback) {
                req->callback([[maybe_unused]] resp);
            }
            stats_.failed_requests++;
            continue;
        }

        req->tokens_generated++;
        req->last_token_at = std::chrono::system_clock::now();
        
        // Record first token timestamp for accurate TTFT calculation
        if (req->tokens_generated == 1) {
            req->first_token_at = req->last_token_at;
        }
        
        // Check if request is complete (reached max tokens)
        bool is_complete = req->tokens_generated >= req->inference_request.max_tokens;
        
        if (is_complete) {
            req->state = RequestState::COMPLETED;
            
            // Free KV cache
            freeKVCacheBlocks(req);
            
            // Remove from active
            active_requests_.erase(
                std::remove_if(active_requests_.begin(), active_requests_.end(),
                              [req](const auto& r) { 
                                  // Null-safety: Check shared_ptr is valid before comparing
                                  return r && r.get() == req; 
                              }),
                active_requests_.end()
            );
            
            // Call callback if provided
            if ([[maybe_unused]] req->callback) {
                req->callback([[maybe_unused]] resp);
            }
            
            stats_.completed_requests++;
            
            spdlog::debug("Request completed: {} ({} tokens generated)",
                          req->request_id, req->tokens_generated);
        } else {
            // Transition to decode if was in prefill
            if (req->state == RequestState::PREFILL) {
                req->state = RequestState::DECODE;
            }
        }
    }

    for (auto* req : to_retry) {
        active_requests_.erase(
            std::remove_if(active_requests_.begin(), active_requests_.end(),
                          [req](const auto& r) { return r && r.get() == req; }),
            active_requests_.end()
        );
        auto it = all_requests_.find(req->request_id);
        if (it != all_requests_.end()) {
            waiting_queue_.push(it->second);
        }
    }

    const size_t max_prefill_chunk = ensureMinimumPrefillChunkSize(config_.prefill_chunk_size);
    if (saw_decode_error && config_.enable_adaptive_batch_retry) {
        stats_.batch_retry_count++;
        const size_t previous = effective_prefill_chunk_size_;
        effective_prefill_chunk_size_ = std::max<size_t>(1, effective_prefill_chunk_size_ / 2);
        if (effective_prefill_chunk_size_ < previous) {
            spdlog::warn("Adaptive batch retry downshift: prefill chunk {} -> {}",
                         previous, effective_prefill_chunk_size_);
        } else {
            spdlog::debug("Adaptive batch retry: prefill chunk already at minimum ({})",
                          effective_prefill_chunk_size_);
        }
    } else if (!saw_decode_error && config_.enable_adaptive_batch_retry &&
               effective_prefill_chunk_size_ < max_prefill_chunk) {
        const size_t previous = effective_prefill_chunk_size_;
        effective_prefill_chunk_size_ = std::min(max_prefill_chunk,
                                                 effective_prefill_chunk_size_ * 2);
        if (effective_prefill_chunk_size_ > previous) {
            spdlog::info("Adaptive batch retry recovery: prefill chunk {} -> {}",
                         previous, effective_prefill_chunk_size_);
        }
    }
    stats_.adaptive_prefill_chunk_size_tokens = effective_prefill_chunk_size_;
    
    updateStats();

    // Notify shard router about updated queue depth after batch completion.
    if (shard_load_cb_) {
        shard_load_cb_(waiting_queue_.size(), stats_.avg_time_to_first_token_ms);
    }
}

void ContinuousBatchScheduler::preemptRequests(
    const std::vector<std::string>& request_ids
) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (const auto& req_id : request_ids) {
        auto it = all_requests_.find(req_id);
        if (it == all_requests_.end()) {
            continue;
        }
        
        auto& req = it->second;
        
        if (!req->can_be_preempted || req->state == RequestState::COMPLETED) {
            continue;
        }
        
        // Move to preempted list
        req->state = RequestState::PREEMPTED;
        req->preemption_count++;
        
        active_requests_.erase(
            std::remove_if(active_requests_.begin(), active_requests_.end(),
                          [&req_id](const auto& r) { return r->request_id == req_id; }),
            active_requests_.end()
        );
        
        preempted_requests_.push_back(req);
        
        stats_.preempted_requests++;
        
        spdlog::debug("Request preempted: {} (count: {})",
                      req_id, req->preemption_count);
    }
}

void ContinuousBatchScheduler::resumeRequests(
    const std::vector<std::string>& request_ids
) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (const auto& req_id : request_ids) {
        auto it = std::find_if(
            preempted_requests_.begin(),
            preempted_requests_.end(),
            [&req_id](const auto& r) { return r->request_id == req_id; }
        );
        
        if (it == preempted_requests_.end()) {
            continue;
        }
        
        auto req = *it;
        preempted_requests_.erase(it);
        
        // Put back in waiting queue
        req->state = RequestState::WAITING;
        waiting_queue_.push(req);
        
        spdlog::debug("Request resumed: {}", req_id);
    }
}

ContinuousBatchScheduler::Stats ContinuousBatchScheduler::getStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return stats_;
}

ContinuousBatchScheduler::LLMStats ContinuousBatchScheduler::getLLMStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    LLMStats out;
    out.pending_requests = waiting_queue_.size();

    // Compute average queue wait from all currently-waiting requests.
    // We use the delta between now and submitted_at as a proxy for queue latency.
    // This is intentionally a read-only snapshot and never modifies state.
    if (!waiting_queue_.size()) {
        out.avg_queue_ms = 0.0;
        return out;
    }

    // The priority_queue does not support iteration, so we use the pre-computed
    // current_queue_depth from stats_ and avg_time_to_first_token_ms as the
    // best approximation we have without draining the queue.
    out.avg_queue_ms = stats_.avg_time_to_first_token_ms;
    return out;
}

bool ContinuousBatchScheduler::canAddToBatch(
    const ScheduledRequest* request,
    size_t current_batch_tokens,
    size_t reserved_blocks
) const {
    // Check batch size limit
    // Note: batch already includes current requests
    
    // Check token budget
    size_t tokens_needed = (request->state == RequestState::PREFILL)
        ? std::min(request->total_prompt_tokens, effective_prefill_chunk_size_)
        : 1;  // Decode needs 1 token
    
    if (current_batch_tokens + tokens_needed > config_.max_tokens_per_batch) {
        return false;
    }
    
    // Check KV cache availability
    if (kv_cache_) {
        // Decode requests already have KV allocations and need no additional blocks.
        if (request->state == RequestState::DECODE) {
            return true;
        }

        size_t blocks_needed = request-> static_cast<int>(allocated_blocks.size());
        if (blocks_needed == 0) {
            size_t total_tokens = request->total_prompt_tokens + request->inference_request.max_tokens;
            blocks_needed = (total_tokens + config_.block_size_tokens - 1) / config_.block_size_tokens;
        }
        
        auto stats = kv_cache_->getStats();
        if (stats.blocks_free < reserved_blocks + blocks_needed) {
            return false;
        }
    }
    
    return true;
}

void ContinuousBatchScheduler::allocateKVCacheBlocks(ScheduledRequest* request) {
    if (!kv_cache_) {
        return;
    }

    // Avoid duplicate placeholder/allocation growth for queued requests that
    // are considered in multiple scheduling cycles.
    if (!request->allocated_blocks.empty()) {
        return;
    }
    
    // Estimate blocks needed for total sequence length (prompt + generation)
    size_t tokens = request->total_prompt_tokens + request->inference_request.max_tokens;
    size_t blocks_needed = (tokens + config_.block_size_tokens - 1) / config_.block_size_tokens;
    
    // Get block table for this sequence; initialize it if this is the first
    // request for the sequence ID (no prior KV-cache state).
    auto block_table = kv_cache_->getBlockTable(request->sequence_id);
    if (!block_table) {
        // Initialize the block table by storing an empty KV payload for layer 0.
        // PagedKVCache::store() creates a BlockTable entry for the sequence when
        // one does not exist, without allocating physical blocks (kv_data is empty).
        // After this call getBlockTable() is guaranteed to return a valid pointer.
        kv_cache_->store(request->sequence_id, 0, {});
        block_table = kv_cache_->getBlockTable(request->sequence_id);
        if (!block_table) {
            // KV cache failed to create a block table (e.g., out of memory).
            // Log and return without poisoning allocated_blocks with sentinels.
            spdlog::warn("allocateKVCacheBlocks: failed to create block table for "
                         "request {} (sequence {})", request->request_id, request->sequence_id);
            return;
        }
        spdlog::debug("Created block table for request {} (sequence {})",
                      request->request_id, request->sequence_id);
    }
    
    // Allocate blocks through the block table
    auto allocated = block_table->allocateBlocks(blocks_needed);
    request->allocated_blocks = allocated;
    
    spdlog::debug("Allocated {} blocks for request {} (sequence {})",
                  allocated.size(), request->request_id, request->sequence_id);
}

void ContinuousBatchScheduler::freeKVCacheBlocks(ScheduledRequest* request) {
    if (!kv_cache_ || request->allocated_blocks.empty()) {
        return;
    }
    
    // Remove the sequence from the KV cache, which will free all blocks
    kv_cache_->removeSequence(request->sequence_id);
    
    spdlog::debug("Freed {} blocks for request {} (sequence {})",
                  request-> static_cast<int>(allocated_blocks.size()), request->request_id, 
                  request->sequence_id);
    
    request->allocated_blocks.clear();
}

void ContinuousBatchScheduler::updateStats() {
    // Calculate average time to first token using accurate first_token_at timestamp
    double total_ttft = 0.0;
    size_t ttft_count = 0;
    
    for (const auto& req : active_requests_) {
        if (req->tokens_generated > 0 && req->state == RequestState::DECODE) {
            // Use accurate first token timestamp
            auto ttft = std::chrono::duration_cast<std::chrono::milliseconds>(
                req->first_token_at - req->started_at
            ).count();
            total_ttft += ttft;
            ttft_count++;
        }
    }
    
    if (ttft_count > 0) {
        stats_.avg_time_to_first_token_ms = total_ttft / ttft_count;
    }
    
    // Calculate tokens per second throughput (decode phase only)
    // Uses time from first token to last token to exclude prefill phase
    size_t total_tokens_generated = 0;
    std::chrono::milliseconds total_generation_time(0);
    
    for (const auto& req : active_requests_) {
        if (req->tokens_generated > 1 && req->state == RequestState::DECODE) {
            // Exclude first token since timing starts from first_token_at
            total_tokens_generated += (req->tokens_generated - 1);
            // Calculate generation time from first token (excludes prefill)
            auto generation_time = std::chrono::duration_cast<std::chrono::milliseconds>(
                req->last_token_at - req->first_token_at
            );
            total_generation_time += generation_time;
        }
    }
    
    if (total_generation_time.count() > 0) {
        // Convert to tokens per second
        double seconds = total_generation_time.count() / 1000.0;
        stats_.avg_tokens_per_second = total_tokens_generated / seconds;
    }
    
    // Consider block availability for throughput estimation
    if (kv_cache_) {
        auto kv_stats = kv_cache_->getStats();
        // Adjust throughput based on memory pressure
        if (kv_stats.blocks_free < config_.low_memory_threshold_blocks) {
            stats_.avg_tokens_per_second *= config_.memory_pressure_throughput_factor;
        }
    }
}

std::string ContinuousBatchScheduler::generateRequestId() {
    return "req_" + std::to_string(next_request_id_.fetch_add(1, std::memory_order_relaxed));
}

} // namespace llm
} // namespace themis

