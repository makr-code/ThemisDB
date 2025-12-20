#include "llm/continuous_batch_scheduler.h"
#include <spdlog/spdlog.h>
#include <algorithm>

namespace themis {
namespace llm {

ContinuousBatchScheduler::ContinuousBatchScheduler(
    const SchedulerConfig& config,
    PagedKVCache* kv_cache
) : config_(config),
    kv_cache_(kv_cache),
    waiting_queue_(
        [](const std::shared_ptr<ScheduledRequest>& a,
           const std::shared_ptr<ScheduledRequest>& b) {
            // Higher priority first, then FIFO
            if (a->priority != b->priority) {
                return a->priority < b->priority;  // Max heap
            }
            return a->submitted_at > b->submitted_at;  // Earlier first
        }
    ) {
    
    spdlog::info("Continuous Batch Scheduler initialized (vLLM-style):");
    spdlog::info("  Max batch size: {}", config_.max_batch_size);
    spdlog::info("  Max concurrent: {}", config_.max_concurrent_requests);
    spdlog::info("  Preemption: {}", config_.enable_preemption ? "enabled" : "disabled");
    spdlog::info("  Priority scheduling: {}", config_.enable_priority_scheduling ? "enabled" : "disabled");
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
    
    auto scheduled = std::make_shared<ScheduledRequest>();
    scheduled->request_id = generateRequestId();
    scheduled->inference_request = request;
    scheduled->priority = priority;
    scheduled->state = RequestState::WAITING;
    scheduled->submitted_at = std::chrono::system_clock::now();
    scheduled->callback = callback;
    scheduled->sequence_id = next_sequence_id_++;
    
    // Estimate prompt tokens (simplified)
    scheduled->total_prompt_tokens = request.prompt.length() / 4;  // Rough estimate
    
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
                      [&](const auto& r) { return r->request_id == request_id; }),
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
    
    // First, continue active decode requests
    for (auto& req : active_requests_) {
        if (req->state == RequestState::DECODE) {
            if (canAddToBatch(req.get(), total_tokens)) {
                batch.push_back(req.get());
                total_tokens++;  // Each decode request adds 1 token
            }
        }
    }
    
    // Then, process waiting requests (prefill)
    while (!waiting_queue_.empty() && batch.size() < config_.max_batch_size) {
        auto req = waiting_queue_.top();
        waiting_queue_.pop();
        
        req->state = RequestState::PREFILL;
        req->started_at = std::chrono::system_clock::now();
        
        // Allocate KV cache blocks
        allocateKVCacheBlocks(req.get());
        
        // Check if we can add to batch
        size_t prefill_tokens = config_.enable_chunked_prefill 
            ? std::min(req->total_prompt_tokens, config_.prefill_chunk_size)
            : req->total_prompt_tokens;
        
        if (canAddToBatch(req.get(), total_tokens + prefill_tokens)) {
            batch.push_back(req.get());
            total_tokens += prefill_tokens;
            active_requests_.push_back(req);
        } else {
            // Put back in queue
            waiting_queue_.push(req);
            break;
        }
    }
    
    // Update stats
    stats_.current_batch_size = batch.size();
    stats_.max_batch_size_seen = std::max(stats_.max_batch_size_seen, batch.size());
    stats_.active_requests = active_requests_.size();
    
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
    
    if (batch.size() != responses.size()) {
        spdlog::error("Batch size mismatch: {} requests, {} responses",
                      batch.size(), responses.size());
        return;
    }
    
    for (size_t i = 0; i < batch.size(); ++i) {
        auto* req = batch[i];
        const auto& resp = responses[i];
        
        req->tokens_generated++;
        req->last_token_at = std::chrono::system_clock::now();
        
        // Check if request is complete
        bool is_complete = resp.finish_reason.has_value() || 
                          req->tokens_generated >= req->inference_request.max_tokens;
        
        if (is_complete) {
            req->state = RequestState::COMPLETED;
            
            // Free KV cache
            freeKVCacheBlocks(req);
            
            // Remove from active
            active_requests_.erase(
                std::remove_if(active_requests_.begin(), active_requests_.end(),
                              [req](const auto& r) { return r.get() == req; }),
                active_requests_.end()
            );
            
            // Call callback if provided
            if (req->callback) {
                req->callback(resp);
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
    
    updateStats();
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

bool ContinuousBatchScheduler::canAddToBatch(
    const ScheduledRequest* request,
    size_t current_batch_tokens
) const {
    // Check batch size limit
    // Note: batch already includes current requests
    
    // Check token budget
    size_t tokens_needed = (request->state == RequestState::PREFILL)
        ? std::min(request->total_prompt_tokens, config_.prefill_chunk_size)
        : 1;  // Decode needs 1 token
    
    if (current_batch_tokens + tokens_needed > config_.max_tokens_per_batch) {
        return false;
    }
    
    // Check KV cache availability
    // TODO: Implement actual block availability check with PagedKVCache
    
    return true;
}

void ContinuousBatchScheduler::allocateKVCacheBlocks(ScheduledRequest* request) {
    if (!kv_cache_) {
        return;
    }
    
    // TODO: Implement actual block allocation with PagedKVCache
    // For now, placeholder
    
    // Estimate blocks needed
    size_t tokens = request->total_prompt_tokens + request->inference_request.max_tokens;
    size_t blocks_needed = (tokens + 15) / 16;  // 16 tokens per block
    
    request->allocated_blocks.resize(blocks_needed);
    for (size_t i = 0; i < blocks_needed; ++i) {
        request->allocated_blocks[i] = static_cast<int>(i);  // Placeholder
    }
    
    spdlog::debug("Allocated {} blocks for request {}",
                  blocks_needed, request->request_id);
}

void ContinuousBatchScheduler::freeKVCacheBlocks(ScheduledRequest* request) {
    if (!kv_cache_ || request->allocated_blocks.empty()) {
        return;
    }
    
    // TODO: Implement actual block deallocation with PagedKVCache
    
    spdlog::debug("Freed {} blocks for request {}",
                  request->allocated_blocks.size(), request->request_id);
    
    request->allocated_blocks.clear();
}

void ContinuousBatchScheduler::updateStats() {
    // Calculate average time to first token
    double total_ttft = 0.0;
    size_t ttft_count = 0;
    
    for (const auto& req : active_requests_) {
        if (req->tokens_generated > 0 && req->state == RequestState::DECODE) {
            auto ttft = std::chrono::duration_cast<std::chrono::milliseconds>(
                req->last_token_at - req->started_at
            ).count();
            total_ttft += ttft;
            ttft_count++;
        }
    }
    
    if (ttft_count > 0) {
        stats_.avg_time_to_first_token_ms = total_ttft / ttft_count;
    }
    
    // Calculate tokens per second
    // TODO: Implement more sophisticated throughput calculation
}

std::string ContinuousBatchScheduler::generateRequestId() {
    return "req_" + std::to_string(next_request_id_++);
}

} // namespace llm
} // namespace themis
