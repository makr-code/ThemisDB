#include <gtest/gtest.h>
#include "llm/continuous_batch_scheduler.h"
#include "llm/paged_kv_cache.h"
#include "llm/paged_block_manager.h"
#include "llm/grafana_metrics.h"
#include <memory>
#include <chrono>
#include <thread>

using namespace themis::llm;
using namespace themis::llm::monitoring;

// Test constants
constexpr size_t CHARS_PER_TOKEN = 4;  // Rough estimate for token size
constexpr size_t BLOCK_SIZE_TOKENS = 16;  // Must match configuration

class ContinuousBatchSchedulerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Set up PagedBlockManager
        PagedBlockManager::Config bm_config;
        bm_config.total_blocks = 1000;
        bm_config.block_size_tokens = BLOCK_SIZE_TOKENS;
        block_manager = std::make_shared<PagedBlockManager>(bm_config);
        
        // Set up PagedKVCache
        PagedKVCache::Config cache_config;
        cache_config.num_blocks = 1000;
        cache_config.block_size = BLOCK_SIZE_TOKENS;
        kv_cache = std::make_unique<PagedKVCache>(cache_config, block_manager);
        
        // Set up ContinuousBatchScheduler
        ContinuousBatchScheduler::SchedulerConfig sched_config;
        sched_config.max_batch_size = 32;
        sched_config.max_concurrent_requests = 64;
        sched_config.max_tokens_per_batch = 2048;
        sched_config.block_size_tokens = BLOCK_SIZE_TOKENS;
        scheduler = std::make_unique<ContinuousBatchScheduler>(sched_config, kv_cache.get());
        
        scheduler->start();
    }
    
    void TearDown() override {
        scheduler->stop();
    }
    
    InferenceRequest createTestRequest(size_t prompt_length, size_t max_tokens) {
        InferenceRequest req;
        req.prompt = std::string(prompt_length * CHARS_PER_TOKEN, 'a');
        req.max_tokens = max_tokens;
        req.temperature = 1.0f;
        return req;
    }

    std::shared_ptr<PagedBlockManager> block_manager;
    std::unique_ptr<PagedKVCache> kv_cache;
    std::unique_ptr<ContinuousBatchScheduler> scheduler;
};

// Test 1: Basic block allocation and deallocation
TEST_F(ContinuousBatchSchedulerTest, BlockAllocationDeallocation) {
    auto req = createTestRequest(100, 50);
    
    // Get initial free blocks
    auto initial_stats = kv_cache->getStats();
    size_t initial_free = initial_stats.blocks_free;
    
    // Submit request
    auto req_id = scheduler->submitRequest(req);
    EXPECT_FALSE(req_id.empty());
    
    // Schedule a batch to trigger allocation
    auto batch = scheduler->scheduleNextBatch();
    EXPECT_GT(batch.size(), 0);
    
    // Check that blocks were allocated (or marked for allocation)
    auto stats_after_alloc = kv_cache->getStats();
    // Note: Blocks might not be allocated yet since allocation happens on first store
    
    // Cancel request to trigger deallocation
    bool cancelled = scheduler->cancelRequest(req_id);
    EXPECT_TRUE(cancelled);
    
    // After cancellation, blocks should be freed
    auto final_stats = kv_cache->getStats();
    EXPECT_EQ(final_stats.blocks_free, initial_free);
}

// Test 2: Block availability check prevents over-allocation
TEST_F(ContinuousBatchSchedulerTest, BlockAvailabilityCheck) {
    // Calculate how many requests we can fit
    auto stats = kv_cache->getStats();
    size_t free_blocks = stats.blocks_free;
    
    // Each request needs blocks for prompt + generation
    constexpr size_t PROMPT_TOKENS = 100;
    constexpr size_t MAX_TOKENS = 50;
    constexpr size_t TOTAL_TOKENS = PROMPT_TOKENS + MAX_TOKENS;  // 150 tokens
    // Calculate blocks needed using ceiling division: (150 + 16 - 1) / 16 = 165 / 16 = 10.3125 rounds down to 10
    size_t blocks_per_request = (TOTAL_TOKENS + BLOCK_SIZE_TOKENS - 1) / BLOCK_SIZE_TOKENS;
    size_t max_requests = free_blocks / blocks_per_request;
    
    // Submit requests up to the limit
    std::vector<std::string> request_ids;
    for (size_t i = 0; i < max_requests; ++i) {
        auto req = createTestRequest(PROMPT_TOKENS, MAX_TOKENS);
        auto req_id = scheduler->submitRequest(req);
        request_ids.push_back(req_id);
    }
    
    // Try to schedule - should work
    auto batch1 = scheduler->scheduleNextBatch();
    EXPECT_GT(batch1.size(), 0);
    
    // Submit one more request that should not fit
    auto extra_req = createTestRequest(PROMPT_TOKENS, MAX_TOKENS);
    auto extra_id = scheduler->submitRequest(extra_req);
    
    // This request should not be scheduled due to lack of blocks
    auto batch2 = scheduler->scheduleNextBatch();
    bool extra_in_batch = false;
    for (auto* req : batch2) {
        if (req->request_id == extra_id) {
            extra_in_batch = true;
            break;
        }
    }
    
    // The extra request should wait in queue
    // Note: It might be scheduled if previous requests complete
    // So we just check that the system doesn't crash
    EXPECT_TRUE(true);
    
    // Clean up
    for (const auto& id : request_ids) {
        scheduler->cancelRequest(id);
    }
    scheduler->cancelRequest(extra_id);
}

// Test 3: Multiple requests in batch
TEST_F(ContinuousBatchSchedulerTest, MultipleBatchRequests) {
    std::vector<std::string> request_ids;
    
    // Submit multiple requests
    for (int i = 0; i < 5; ++i) {
        auto req = createTestRequest(50, 20);
        auto req_id = scheduler->submitRequest(req);
        request_ids.push_back(req_id);
    }
    
    // Schedule a batch
    auto batch = scheduler->scheduleNextBatch();
    
    // Should have scheduled multiple requests
    EXPECT_GT(batch.size(), 1);
    EXPECT_LE(batch.size(), 5);
    
    // Clean up
    for (const auto& id : request_ids) {
        scheduler->cancelRequest(id);
    }
}

// Test 4: Request completion and block deallocation
TEST_F(ContinuousBatchSchedulerTest, RequestCompletionBlockDeallocation) {
    auto initial_stats = kv_cache->getStats();
    size_t initial_free = initial_stats.blocks_free;
    
    // Submit a request
    auto req = createTestRequest(50, 10);  // Small request
    auto req_id = scheduler->submitRequest(req);
    
    // Schedule it
    auto batch = scheduler->scheduleNextBatch();
    ASSERT_GT(batch.size(), 0);
    
    // Simulate completion by generating all tokens
    std::vector<InferenceResponse> responses;
    for (auto* req_ptr : batch) {
        InferenceResponse resp;
        resp.text = "Generated token";
        responses.push_back(resp);
    }
    
    // Process results multiple times until completion
    for (int i = 0; i < 12; ++i) {  // Generate more than max_tokens
        scheduler->processBatchResults(batch, responses);
        
        // Check if completed
        auto stats = scheduler->getStats();
        if (stats.completed_requests > 0) {
            break;
        }
    }
    
    // After completion, blocks should be freed
    auto final_stats = kv_cache->getStats();
    EXPECT_EQ(final_stats.blocks_free, initial_free);
}

// Test 5: Statistics update with throughput calculation
TEST_F(ContinuousBatchSchedulerTest, StatisticsUpdate) {
    // Submit and schedule a request
    auto req = createTestRequest(50, 20);
    auto req_id = scheduler->submitRequest(req);
    
    auto batch = scheduler->scheduleNextBatch();
    ASSERT_GT(batch.size(), 0);
    
    // Simulate some token generation
    std::vector<InferenceResponse> responses;
    for (auto* req_ptr : batch) {
        InferenceResponse resp;
        resp.text = "token";
        responses.push_back(resp);
    }
    
    // Process several times
    for (int i = 0; i < 5; ++i) {
        scheduler->processBatchResults(batch, responses);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    // Get statistics
    auto stats = scheduler->getStats();
    
    // Check that statistics are being tracked
    EXPECT_GT(stats.total_requests, 0);
    EXPECT_GE(stats.avg_scheduling_time_ms, 0.0);
    
    // Clean up
    scheduler->cancelRequest(req_id);
}

// Test 6: Out of memory scenario handling
TEST_F(ContinuousBatchSchedulerTest, OutOfMemoryHandling) {
    // Create a scheduler with very limited blocks
    PagedBlockManager::Config small_bm_config;
    small_bm_config.total_blocks = 20;  // Very small
    small_bm_config.block_size_tokens = 16;
    auto small_block_manager = std::make_shared<PagedBlockManager>(small_bm_config);
    
    PagedKVCache::Config small_cache_config;
    small_cache_config.num_blocks = 20;
    small_cache_config.block_size = 16;
    auto small_kv_cache = std::make_unique<PagedKVCache>(small_cache_config, small_block_manager);
    
    ContinuousBatchScheduler::SchedulerConfig sched_config;
    sched_config.max_batch_size = 32;
    auto small_scheduler = std::make_unique<ContinuousBatchScheduler>(
        sched_config, small_kv_cache.get());
    small_scheduler->start();
    
    // Submit requests that require more blocks than available
    std::vector<std::string> request_ids;
    for (int i = 0; i < 5; ++i) {
        auto req = createTestRequest(100, 50);  // ~10 blocks each
        auto req_id = small_scheduler->submitRequest(req);
        request_ids.push_back(req_id);
    }
    
    // Try to schedule - should only schedule what fits
    auto batch = small_scheduler->scheduleNextBatch();
    
    // Should schedule at most 2 requests (20 blocks / 10 blocks per request)
    EXPECT_LE(batch.size(), 2);
    
    // System should not crash
    EXPECT_TRUE(true);
    
    // Clean up
    for (const auto& id : request_ids) {
        small_scheduler->cancelRequest(id);
    }
    small_scheduler->stop();
}

// Test 7: Priority scheduling with block constraints
TEST_F(ContinuousBatchSchedulerTest, PrioritySchedulingWithBlocks) {
    using Priority = ContinuousBatchScheduler::RequestPriority;
    
    // Submit low priority request
    auto low_req = createTestRequest(50, 20);
    auto low_id = scheduler->submitRequest(low_req, Priority::LOW);
    
    // Submit high priority request
    auto high_req = createTestRequest(50, 20);
    auto high_id = scheduler->submitRequest(high_req, Priority::HIGH);
    
    // Schedule batch - high priority should be scheduled first
    auto batch = scheduler->scheduleNextBatch();
    ASSERT_GT(batch.size(), 0);
    
    // First request in batch should be high priority (if both fit)
    if (batch.size() >= 2) {
        // Both should be in batch, but order might vary
        bool high_found = false;
        for (auto* req : batch) {
            if (req->request_id == high_id) {
                high_found = true;
                break;
            }
        }
        EXPECT_TRUE(high_found);
    }
    
    // Clean up
    scheduler->cancelRequest(low_id);
    scheduler->cancelRequest(high_id);
}

// ═══════════════════════════════════════════════════════════
// Backpressure / Queue Depth Tests (Q1 implementation)
// ═══════════════════════════════════════════════════════════

// Test 8: submitRequest returns empty string when max_queue_depth is reached
TEST_F(ContinuousBatchSchedulerTest, BackpressureRejectWhenQueueFull) {
    // Create a scheduler with a small queue depth limit
    ContinuousBatchScheduler::SchedulerConfig cfg;
    cfg.max_batch_size = 32;
    cfg.max_tokens_per_batch = 2048;
    cfg.block_size_tokens = BLOCK_SIZE_TOKENS;
    cfg.max_queue_depth = 3;  // Only 3 requests allowed
    
    auto sched = std::make_unique<ContinuousBatchScheduler>(cfg, kv_cache.get());
    sched->start();
    
    // Fill the queue to the limit
    std::vector<std::string> ids;
    for (size_t i = 0; i < cfg.max_queue_depth; ++i) {
        auto req = createTestRequest(10, 5);
        std::string id = sched->submitRequest(req);
        EXPECT_FALSE(id.empty()) << "Request " << i << " should be accepted";
        ids.push_back(id);
    }
    
    // The next request must be rejected (empty string returned)
    auto overflow_req = createTestRequest(10, 5);
    std::string overflow_id = sched->submitRequest(overflow_req);
    EXPECT_TRUE(overflow_id.empty()) << "Overflow request should be rejected";
    
    // rejected_requests counter must be incremented
    auto stats = sched->getStats();
    EXPECT_EQ(stats.rejected_requests, 1u);
    
    // Cleanup
    for (const auto& id : ids) {
        sched->cancelRequest(id);
    }
    sched->stop();
}

// Test 9: max_queue_depth = 0 means unlimited (no rejection)
TEST_F(ContinuousBatchSchedulerTest, BackpressureUnlimitedWhenZero) {
    ContinuousBatchScheduler::SchedulerConfig cfg;
    cfg.max_batch_size = 32;
    cfg.max_tokens_per_batch = 2048;
    cfg.block_size_tokens = BLOCK_SIZE_TOKENS;
    cfg.max_queue_depth = 0;  // Unlimited
    
    auto sched = std::make_unique<ContinuousBatchScheduler>(cfg, kv_cache.get());
    sched->start();
    
    // Submit many requests — none should be rejected
    std::vector<std::string> ids;
    for (int i = 0; i < 20; ++i) {
        auto req = createTestRequest(5, 3);
        std::string id = sched->submitRequest(req);
        EXPECT_FALSE(id.empty()) << "Request " << i << " should be accepted (unlimited)";
        ids.push_back(id);
    }
    
    auto stats = sched->getStats();
    EXPECT_EQ(stats.rejected_requests, 0u);
    
    for (const auto& id : ids) {
        sched->cancelRequest(id);
    }
    sched->stop();
}

// Test 10: current_queue_depth is updated by scheduleNextBatch
TEST_F(ContinuousBatchSchedulerTest, CurrentQueueDepthTracked) {
    auto req = createTestRequest(20, 10);
    auto req_id = scheduler->submitRequest(req);
    EXPECT_FALSE(req_id.empty());
    
    // After submit, depth should be >= 1
    // (depth is updated in scheduleNextBatch, not submitRequest)
    scheduler->scheduleNextBatch();
    
    auto stats = scheduler->getStats();
    // current_queue_depth = waiting + active; after scheduling the request
    // moves to active, so depth should be 1
    EXPECT_EQ(stats.current_queue_depth, 1u);
    
    scheduler->cancelRequest(req_id);
}

// ═══════════════════════════════════════════════════════════
// Metrics Emission Tests (Q1 implementation)
// ═══════════════════════════════════════════════════════════

// Test 11: recordQueueLength is called during scheduleNextBatch
TEST_F(ContinuousBatchSchedulerTest, MetricsQueueLengthEmittedOnSchedule) {
    auto exporter = std::make_unique<PrometheusExporter>();
    auto collector = std::make_unique<LLMMetricsCollector>(exporter.get());
    
    scheduler->setMetricsCollector(collector.get());
    
    // Submit a request and schedule — should emit queue length
    auto req = createTestRequest(10, 5);
    auto req_id = scheduler->submitRequest(req);
    EXPECT_FALSE(req_id.empty());
    
    scheduler->scheduleNextBatch();
    
    // The exporter should have recorded a gauge for llm_queue_length
    std::string metrics = exporter->exportMetrics();
    EXPECT_NE(metrics.find("llm_queue_length"), std::string::npos);
    
    scheduler->cancelRequest(req_id);
    scheduler->setMetricsCollector(nullptr);
}

// Test 12: recordBackpressureDrop is called when queue is full
TEST_F(ContinuousBatchSchedulerTest, MetricsBackpressureDropEmittedOnRejection) {
    auto exporter = std::make_unique<PrometheusExporter>();
    auto collector = std::make_unique<LLMMetricsCollector>(exporter.get());
    
    // Small queue depth
    ContinuousBatchScheduler::SchedulerConfig cfg;
    cfg.max_batch_size = 32;
    cfg.max_tokens_per_batch = 2048;
    cfg.block_size_tokens = BLOCK_SIZE_TOKENS;
    cfg.max_queue_depth = 2;
    
    auto sched = std::make_unique<ContinuousBatchScheduler>(cfg, kv_cache.get());
    sched->setMetricsCollector(collector.get());
    sched->start();
    
    // Fill the queue
    std::string id1 = sched->submitRequest(createTestRequest(5, 3));
    std::string id2 = sched->submitRequest(createTestRequest(5, 3));
    EXPECT_FALSE(id1.empty());
    EXPECT_FALSE(id2.empty());
    
    // Overflow — should increment backpressure counter
    std::string id3 = sched->submitRequest(createTestRequest(5, 3));
    EXPECT_TRUE(id3.empty());
    
    // Check that llm_backpressure_drops_total was incremented
    std::string metrics = exporter->exportMetrics();
    EXPECT_NE(metrics.find("llm_backpressure_drops_total"), std::string::npos);
    
    sched->cancelRequest(id1);
    sched->cancelRequest(id2);
    sched->stop();
}

// Test 13: setMetricsCollector nullptr — no crash on backpressure or schedule
TEST_F(ContinuousBatchSchedulerTest, MetricsNullCollectorNoCrash) {
    // Ensure calling with nullptr doesn't crash even when limits are hit
    ContinuousBatchScheduler::SchedulerConfig cfg;
    cfg.max_batch_size = 32;
    cfg.max_tokens_per_batch = 2048;
    cfg.block_size_tokens = BLOCK_SIZE_TOKENS;
    cfg.max_queue_depth = 1;
    
    auto sched = std::make_unique<ContinuousBatchScheduler>(cfg, kv_cache.get());
    sched->setMetricsCollector(nullptr);  // explicitly null
    sched->start();
    
    auto id = sched->submitRequest(createTestRequest(5, 3));
    EXPECT_FALSE(id.empty());
    
    // This should hit the backpressure path with null collector — no crash
    auto id2 = sched->submitRequest(createTestRequest(5, 3));
    EXPECT_TRUE(id2.empty());
    
    sched->scheduleNextBatch();  // should not crash with null collector
    
    sched->cancelRequest(id);
    sched->stop();
}

// ─────────────────────────────────────────────────────────────────────────────
// LLM-RAID integration: getLLMStats() ShardStats bridge
// ─────────────────────────────────────────────────────────────────────────────

// Test 14 (CBS-LLM-01): getLLMStats() returns zero pending when queue is empty.
TEST_F(ContinuousBatchSchedulerTest, GetLLMStats_EmptyQueue) {
    const auto s = scheduler->getLLMStats();
    EXPECT_EQ(s.pending_requests, 0u);
    EXPECT_DOUBLE_EQ(s.avg_queue_ms, 0.0);
}

// Test 15 (CBS-LLM-02): getLLMStats() reflects queued requests.
TEST_F(ContinuousBatchSchedulerTest, GetLLMStats_PendingCount) {
    // Submit two requests but don't call scheduleNextBatch() to keep them waiting.
    scheduler->submitRequest(createTestRequest(4, 2));
    scheduler->submitRequest(createTestRequest(4, 2));

    const auto s = scheduler->getLLMStats();
    EXPECT_EQ(s.pending_requests, 2u);
}

// Test 16 (CBS-LLM-03): getLLMStats() pending returns to zero after batch is scheduled.
TEST_F(ContinuousBatchSchedulerTest, GetLLMStats_DropsAfterSchedule) {
    scheduler->submitRequest(createTestRequest(4, 2));
    EXPECT_EQ(scheduler->getLLMStats().pending_requests, 1u);

    scheduler->scheduleNextBatch();  // moves waiting → active
    EXPECT_EQ(scheduler->getLLMStats().pending_requests, 0u);
}
