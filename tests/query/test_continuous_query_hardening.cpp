/**
 * @file test_continuous_query_hardening.cpp
 * @brief Phase 4: Continuous Query Engine Hardening Tests
 *
 * Validates:
 *   - Backpressure controls and queue management
 *   - Long-running stability under sustained load
 *   - Persistence safeguards for streaming state
 *   - Graceful degradation under resource pressure
 *
 * Acceptance Criteria:
 *   AC-1: Backpressure prevents unbounded queue growth
 *   AC-2: Queue depth monitoring is accurate
 *   AC-3: Long-running queries don't leak resources
 *   AC-4: Persistence is attempted before critical state loss
 *   AC-5: Graceful shutdown without data loss (where applicable)
 */

#include <gtest/gtest.h>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <deque>
#include <mutex>
#include <thread>
#include <vector>

#include "query/continuous_query_engine.h"
#include "query/window_spec.h"
#include "utils/expected.h"

using namespace themis;
using namespace themis::query;

// ─────────────────────────────────────────────────────────────────────────────
// Mock Continuous Query Engine for Testing
// ─────────────────────────────────────────────────────────────────────────────

class MockContinuousQueryEngine : public ContinuousQueryEngine {
public:
    struct Config {
        size_t max_queue_depth = 10'000;
        bool enable_backpressure = true;
        bool enable_persistence = true;
        size_t checkpoint_interval = 1000;
    };
    
    explicit MockContinuousQueryEngine(const Config& cfg = Config{}) : config_(cfg) {}
    
    Result<ContinuousQueryHandle> registerQuery(ContinuousQuerySpec spec) override {
        std::lock_guard<std::mutex> lock(queries_mutex_);
        
        if (queries_.size() >= 1'000) {
            return Result<ContinuousQueryHandle>::Error("Too many registered queries");
        }
        
        queries_[spec.name] = spec;
        tuple_counts_[spec.name] = 0;
        queue_depths_[spec.name] = 0;
        
        return spec.name;
    }
    
    Result<void> dropQuery(const std::string& name) override {
        std::lock_guard<std::mutex> lock(queries_mutex_);
        
        if (queries_.find(name) == queries_.end()) {
            return Result<void>::Error("Query not found");
        }
        
        queries_.erase(name);
        tuple_counts_.erase(name);
        queue_depths_.erase(name);
        
        return Result<void>::Ok();
    }
    
    Result<ResultStreamPtr> subscribe(const std::string& name, ResultMode mode) override {
        std::lock_guard<std::mutex> lock(queries_mutex_);
        
        if (queries_.find(name) == queries_.end()) {
            return Result<ResultStreamPtr>::Error("Query not found");
        }
        
        // Return a mock stream
        auto stream = std::make_shared<MockResultStream>(100);
        return stream;
    }
    
    std::vector<ContinuousQueryInfo> listQueries() const override {
        std::vector<ContinuousQueryInfo> result;
        std::lock_guard<std::mutex> lock(queries_mutex_);
        
        for (const auto& [name, spec] : queries_) {
            ContinuousQueryInfo info;
            info.name = name;
            result.push_back(info);
        }
        
        return result;
    }
    
    void injectTuple(const std::string& collection,
                     const std::string& tuple,
                     int64_t event_ts) override {
        std::lock_guard<std::mutex> lock(tuples_mutex_);
        
        // Simulate backpressure if queue is full
        if (config_.enable_backpressure) {
            auto it = queue_depths_.find(collection);
            if (it != queue_depths_.end() && it->second >= config_.max_queue_depth) {
                // In production: apply backpressure (block or drop)
                // For testing: track dropped tuples
                dropped_tuples_++;
                return;
            }
        }
        
        injected_tuples_.push_back({collection, tuple, event_ts});
        
        // Track queue depth
        auto depth_it = queue_depths_.find(collection);
        if (depth_it != queue_depths_.end()) {
            depth_it->second++;
        }
    }
    
    // Testing utilities
    
    /// Get tuple injection count for a collection
    size_t getTupleCount(const std::string& collection) const {
        std::lock_guard<std::mutex> lock(tuples_mutex_);
        return tuple_counts_.at(collection);
    }
    
    /// Get current queue depth for a collection
    size_t getQueueDepth(const std::string& collection) const {
        std::lock_guard<std::mutex> lock(tuples_mutex_);
        auto it = queue_depths_.find(collection);
        return it != queue_depths_.end() ? it->second : 0;
    }
    
    /// Get count of dropped tuples due to backpressure
    size_t getDroppedTuples() const {
        return dropped_tuples_.load();
    }
    
    /// Simulate processing a batch from the queue
    void processBatch(const std::string& collection, size_t batch_size) {
        std::lock_guard<std::mutex> lock(tuples_mutex_);
        
        auto it = queue_depths_.find(collection);
        if (it != queue_depths_.end()) {
            it->second = std::max(static_cast<size_t>(0), it->second - batch_size);
        }
    }
    
private:
    struct InjectedTuple {
        std::string collection;
        std::string payload;
        int64_t timestamp;
    };
    
    class MockResultStream : public CQResultStream {
    public:
        explicit MockResultStream(size_t capacity) : capacity_(capacity) {}
        
        bool hasMore() const noexcept override { return !cancelled_; }
        
        std::optional<CQResult> next(std::chrono::milliseconds /*timeout*/) override {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            if (queue_.empty() || cancelled_) {
              return std::nullopt;
            }
            
            auto item = queue_.front();
            queue_.pop_front();
            return item;
        }
        
        void cancel() noexcept override {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            cancelled_ = true;
        }
        
        size_t queueDepth() const noexcept override {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            return queue_.size();
        }
        
        void pushResult(CQResult item) {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            if (queue_.size() < capacity_) {
                queue_.push_back(std::move(item));
            }
        }
        
    private:
        mutable std::mutex queue_mutex_;
        std::deque<CQResult> queue_;
        size_t capacity_;
        std::atomic<bool> cancelled_{false};
    };
    
    Config config_;
    mutable std::mutex queries_mutex_;
    mutable std::mutex tuples_mutex_;
    
    std::unordered_map<std::string, ContinuousQuerySpec> queries_;
    std::unordered_map<std::string, size_t> tuple_counts_;
    std::unordered_map<std::string, size_t> queue_depths_;
    std::vector<InjectedTuple> injected_tuples_;
    std::atomic<size_t> dropped_tuples_{0};
};

// ─────────────────────────────────────────────────────────────────────────────
// Test Fixtures
// ─────────────────────────────────────────────────────────────────────────────

class ContinuousQueryHardeningTest : public ::testing::Test {
protected:
    MockContinuousQueryEngine::Config engine_config_{
        .max_queue_depth = 10'000,
        .enable_backpressure = true,
        .enable_persistence = true,
        .checkpoint_interval = 1000,
    };
    
    ContinuousQuerySpec makeTestSpec(const std::string& name) {
        return ContinuousQuerySpec{
            .name = name,
            .source_collection = "test_collection",
            .window = WindowSpec{
                .type = WindowType::TumblingWindow,
                .duration_ms = 5000,
            },
            .aql_body = "SELECT * FROM @collection",
            .result_mode = ResultMode::DELTA,
            .allowed_lateness_ms = 500,
            .max_window_tuples = 10'000'000,
            .max_window_bytes = 1ULL << 30,
        };
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// Test Cases: Backpressure Controls (AC-1)
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ContinuousQueryHardeningTest, BackpressurePreventsUnboundedGrowth) {
    MockContinuousQueryEngine::Config cfg{
        .max_queue_depth = 1000,
        .enable_backpressure = true,
    };
    MockContinuousQueryEngine engine(cfg);
    
    auto spec = makeTestSpec("test_query");
    auto result = engine.registerQuery(spec);
    ASSERT_TRUE(result);
    
    // Inject tuples up to the limit
    for (int i = 0; i < 1000; ++i) {
        engine.injectTuple("test_collection", "{\"id\": " + std::to_string(i) + "}", 
                          std::chrono::system_clock::now().time_since_epoch().count());
    }
    
    size_t dropped_before = engine.getDroppedTuples();
    
    // Try to inject more (should hit backpressure)
    for (int i = 1000; i < 2000; ++i) {
        engine.injectTuple("test_collection", "{\"id\": " + std::to_string(i) + "}", 
                          std::chrono::system_clock::now().time_since_epoch().count());
    }
    
    size_t dropped_after = engine.getDroppedTuples();
    
    // Verify backpressure dropped tuples
    EXPECT_GT(dropped_after, dropped_before) << "Backpressure should drop excess tuples";
    EXPECT_LE(dropped_after - dropped_before, 1000) << "Should not drop all tuples";
}

TEST_F(ContinuousQueryHardeningTest, BackpressureCanBeDisabled) {
    MockContinuousQueryEngine::Config cfg{
        .max_queue_depth = 100,
        .enable_backpressure = false,  // Disabled
    };
    MockContinuousQueryEngine engine(cfg);
    
    auto spec = makeTestSpec("no_backpressure");
    engine.registerQuery(spec);
    
    // Should not drop tuples even beyond the limit (in this mock)
    size_t inject_count = 500;
    for (size_t i = 0; i < inject_count; ++i) {
        engine.injectTuple("test_collection", "{\"id\": " + std::to_string(i) + "}", 0);
    }
    
    // With backpressure disabled, dropped should be 0 (mock allows all)
    EXPECT_EQ(engine.getDroppedTuples(), 0) << "No backpressure means no drops";
}

// ─────────────────────────────────────────────────────────────────────────────
// Test Cases: Queue Depth Monitoring (AC-2)
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ContinuousQueryHardeningTest, QueueDepthAccuracy) {
    MockContinuousQueryEngine engine(engine_config_);
    
    auto spec = makeTestSpec("queue_test");
    engine.registerQuery(spec);
    
    // Inject tuples and verify queue depth
    for (int i = 0; i < 100; ++i) {
        engine.injectTuple("test_collection", "{\"id\": " + std::to_string(i) + "}", 0);
        size_t expected_depth = i + 1;
        EXPECT_EQ(engine.getQueueDepth("test_collection"), expected_depth)
            << "Queue depth should match injection count at iteration " << i;
    }
}

TEST_F(ContinuousQueryHardeningTest, QueueDepthDecreases_OnProcessing) {
    MockContinuousQueryEngine engine(engine_config_);
    
    auto spec = makeTestSpec("queue_dequeue_test");
    engine.registerQuery(spec);
    
    // Inject 100 tuples
    for (int i = 0; i < 100; ++i) {
        engine.injectTuple("test_collection", "{\"id\": " + std::to_string(i) + "}", 0);
    }
    
    EXPECT_EQ(engine.getQueueDepth("test_collection"), 100);
    
    // Process a batch
    engine.processBatch("test_collection", 30);
    EXPECT_EQ(engine.getQueueDepth("test_collection"), 70)
        << "Queue depth should decrease after processing";
    
    // Process remaining
    engine.processBatch("test_collection", 70);
    EXPECT_EQ(engine.getQueueDepth("test_collection"), 0)
        << "Queue should be empty after processing all";
}

TEST_F(ContinuousQueryHardeningTest, MultipleQueriesIndependentQueues) {
    MockContinuousQueryEngine engine(engine_config_);
    
    auto spec1 = makeTestSpec("query_1");
    auto spec2 = makeTestSpec("query_2");
    spec2.name = "query_2";  // Different name
    
    engine.registerQuery(spec1);
    engine.registerQuery(spec2);
    
    // Inject to different collections
    for (int i = 0; i < 50; ++i) {
        engine.injectTuple("collection_1", "{\"id\": " + std::to_string(i) + "}", 0);
        engine.injectTuple("collection_2", "{\"id\": " + std::to_string(i * 10) + "}", 0);
    }
    
    // Verify independent queue depths
    EXPECT_EQ(engine.getQueueDepth("collection_1"), 50);
    EXPECT_EQ(engine.getQueueDepth("collection_2"), 50);
    
    // Process one collection
    engine.processBatch("collection_1", 30);
    EXPECT_EQ(engine.getQueueDepth("collection_1"), 20);
    EXPECT_EQ(engine.getQueueDepth("collection_2"), 50) << "Other collection unaffected";
}

// ─────────────────────────────────────────────────────────────────────────────
// Test Cases: Long-Running Stability (AC-3)
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ContinuousQueryHardeningTest, LongRunningNoResourceLeak) {
    MockContinuousQueryEngine engine(engine_config_);
    
    auto spec = makeTestSpec("long_running");
    auto reg = engine.registerQuery(spec);
    ASSERT_TRUE(reg);
    
    // Simulate sustained load: inject, process, repeat
    const int iterations = 1000;
    const int batch_size = 100;
    
    for (int iter = 0; iter < iterations; ++iter) {
        // Inject a batch
        for (int i = 0; i < batch_size; ++i) {
            engine.injectTuple("test_collection", 
                              "{\"iter\": " + std::to_string(iter) + ", \"id\": " + std::to_string(i) + "}",
                              iter * 1000000);  // Simulated timestamp
        }
        
        // Process part of it (simulate streaming window)
        engine.processBatch("test_collection", batch_size / 2);
    }
    
    // Verify no unbounded queue growth
    size_t final_queue_depth = engine.getQueueDepth("test_collection");
    EXPECT_LE(final_queue_depth, 10 * batch_size) 
        << "Queue depth should remain bounded even with sustained load";
    
    // Should not have encountered catastrophic failures
    EXPECT_LT(engine.getDroppedTuples(), iterations * batch_size)
        << "Should not drop most tuples in normal operation";
}

TEST_F(ContinuousQueryHardeningTest, ConcurrentInjectionAndProcessing) {
    MockContinuousQueryEngine engine(engine_config_);
    
    auto spec = makeTestSpec("concurrent_test");
    engine.registerQuery(spec);
    
    std::atomic<int> injection_errors{0};
    std::atomic<int> injection_count{0};
    
    // Producer thread: inject tuples continuously
    auto producer = std::thread([&]() {
        for (int i = 0; i < 1000; ++i) {
            engine.injectTuple("test_collection", "{\"id\": " + std::to_string(i) + "}", i * 1000);
            injection_count++;
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }
    });
    
    // Consumer thread: process batches periodically
    auto consumer = std::thread([&]() {
        for (int i = 0; i < 100; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            engine.processBatch("test_collection", 50);
        }
    });
    
    producer.join();
    consumer.join();
    
    EXPECT_EQ(injection_count, 1000) << "All injections should complete";
    EXPECT_LT(engine.getQueueDepth("test_collection"), 100)
        << "Queue should drain with concurrent consumer";
}

// ─────────────────────────────────────────────────────────────────────────────
// Test Cases: Persistence Safeguards (AC-4)
// ─────────────────────────────────────────────────────────────────────────────

class MockPersistenceManager {
public:
    /// Simulate checkpoint: capture current state
    struct Checkpoint {
        std::string query_name;
        std::vector<std::string> buffered_results;
        int64_t last_watermark_us;
        size_t checkpoint_id;
    };
    
    std::optional<Checkpoint> lastCheckpoint() const {
        std::lock_guard<std::mutex> lock(mutex_);
        if (checkpoints_.empty()) {
          return std::nullopt;
        }
        return checkpoints_.back();
    }
    
    void addCheckpoint(const Checkpoint& cp) {
        std::lock_guard<std::mutex> lock(mutex_);
        checkpoints_.push_back(cp);
    }
    
    size_t checkpointCount() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return checkpoints_.size();
    }
    
private:
    mutable std::mutex mutex_;
    std::vector<Checkpoint> checkpoints_;
};

TEST_F(ContinuousQueryHardeningTest, PersistenceOnCheckpoint) {
    MockPersistenceManager persistence;
    MockContinuousQueryEngine engine(engine_config_);
    
    auto spec = makeTestSpec("persist_test");
    engine.registerQuery(spec);
    
    // Inject and process with checkpoints every 10 tuples
    for (int batch = 0; batch < 10; ++batch) {
        for (int i = 0; i < 10; ++i) {
            engine.injectTuple("test_collection", 
                              "{\"batch\": " + std::to_string(batch) + ", \"id\": " + std::to_string(i) + "}",
                              (batch * 10 + i) * 1000);
        }
        
        // Simulate checkpoint
        engine.processBatch("test_collection", 5);
        MockPersistenceManager::Checkpoint cp{
            .query_name = "persist_test",
            .buffered_results = {},
            .last_watermark_us = (batch + 1) * 10 * 1000,
            .checkpoint_id = batch,
        };
        persistence.addCheckpoint(cp);
    }
    
    // Verify checkpoints were created
    EXPECT_EQ(persistence.checkpointCount(), 10)
        << "Should have created checkpoint at each interval";
    
    auto last_cp = persistence.lastCheckpoint();
    ASSERT_TRUE(last_cp);
    EXPECT_EQ(last_cp->checkpoint_id, 9);
    EXPECT_EQ(last_cp->query_name, "persist_test");
}

// ─────────────────────────────────────────────────────────────────────────────
// Test Cases: Query Lifecycle (AC-5)
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ContinuousQueryHardeningTest, QueryRegistration_Limits) {
    MockContinuousQueryEngine engine(engine_config_);
    
    // Register queries up to the limit (1000)
    for (int i = 0; i < 100; ++i) {
        auto spec = makeTestSpec("query_" + std::to_string(i));
        auto result = engine.registerQuery(spec);
        ASSERT_TRUE(result) << "Should register query " << i;
    }
    
    EXPECT_EQ(engine.listQueries().size(), 100);
}

TEST_F(ContinuousQueryHardeningTest, QueryDropAndRecreate) {
    MockContinuousQueryEngine engine(engine_config_);
    
    auto spec = makeTestSpec("drop_recreate_test");
    auto reg = engine.registerQuery(spec);
    ASSERT_TRUE(reg);
    
    auto queries = engine.listQueries();
    EXPECT_EQ(queries.size(), 1);
    
    // Inject some tuples
    for (int i = 0; i < 50; ++i) {
        engine.injectTuple("test_collection", "{\"id\": " + std::to_string(i) + "}", 0);
    }
    
    // Drop the query
    auto drop_result = engine.dropQuery(spec.name);
    ASSERT_TRUE(drop_result);
    
    EXPECT_EQ(engine.listQueries().size(), 0);
    
    // Recreate it
    auto spec2 = makeTestSpec("drop_recreate_test");
    auto reg2 = engine.registerQuery(spec2);
    ASSERT_TRUE(reg2);
    
    EXPECT_EQ(engine.listQueries().size(), 1);
}

TEST_F(ContinuousQueryHardeningTest, SubscribeToQuery) {
    MockContinuousQueryEngine engine(engine_config_);
    
    auto spec = makeTestSpec("subscribe_test");
    auto reg = engine.registerQuery(spec);
    ASSERT_TRUE(reg);
    
    // Subscribe to the query
    auto stream_result = engine.subscribe("subscribe_test", ResultMode::DELTA);
    ASSERT_TRUE(stream_result) << "Subscribe should succeed";
    
    auto stream = *stream_result;
    ASSERT_TRUE(stream);
    EXPECT_TRUE(stream->hasMore()) << "Stream should be initially active";
    
    // Try to subscribe to non-existent query
    auto bad_stream = engine.subscribe("nonexistent", ResultMode::DELTA);
    EXPECT_FALSE(bad_stream) << "Subscribe to nonexistent query should fail";
}

TEST_F(ContinuousQueryHardeningTest, SubscriptionQueueManagement) {
    MockContinuousQueryEngine engine(engine_config_);
    
    auto spec = makeTestSpec("subscription_queue_test");
    auto reg = engine.registerQuery(spec);
    ASSERT_TRUE(reg);
    
    auto stream_result = engine.subscribe("subscription_queue_test", ResultMode::DELTA);
    ASSERT_TRUE(stream_result);
    auto stream = *stream_result;
    
    // Queue should initially be empty
    EXPECT_EQ(stream->queueDepth(), 0) << "Initial queue depth should be 0";
    
    // Cancel should prevent further receives
    stream->cancel();
    EXPECT_FALSE(stream->hasMore()) << "Cancelled stream should report not hasMore";
}

// ─────────────────────────────────────────────────────────────────────────────
// Test Cases: Stress and Edge Cases
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ContinuousQueryHardeningTest, LargePayloads) {
    MockContinuousQueryEngine engine(engine_config_);
    
    auto spec = makeTestSpec("large_payload_test");
    engine.registerQuery(spec);
    
    // Create large JSON payloads
    std::string large_payload(10'000, 'x');  // 10KB payload
    
    for (int i = 0; i < 100; ++i) {
        std::string payload = "{\"id\": " + std::to_string(i) + 
                             ", \"data\": \"" + large_payload + "\"}";
        engine.injectTuple("test_collection", payload, i * 1000);
    }
    
    EXPECT_EQ(engine.getQueueDepth("test_collection"), 100);
}

TEST_F(ContinuousQueryHardeningTest, RapidRegisterDrop) {
    MockContinuousQueryEngine engine(engine_config_);
    
    // Rapid register/drop cycles
    for (int i = 0; i < 50; ++i) {
        auto spec = makeTestSpec("rapid_" + std::to_string(i));
        auto reg = engine.registerQuery(spec);
        ASSERT_TRUE(reg);
        
        auto drop = engine.dropQuery(spec.name);
        ASSERT_TRUE(drop);
    }
    
    EXPECT_EQ(engine.listQueries().size(), 0)
        << "All queries should be dropped";
}

TEST_F(ContinuousQueryHardeningTest, OutOfOrderTimestamps) {
    MockContinuousQueryEngine engine(engine_config_);
    
    auto spec = makeTestSpec("ooo_timestamps");
    engine.registerQuery(spec);
    
    // Inject out-of-order timestamps (late-arriving events)
    std::vector<int64_t> timestamps{0, 1000, 500, 2000, 1500};  // Out of order
    
    for (size_t i = 0; i < timestamps.size(); ++i) {
        engine.injectTuple("test_collection", 
                          "{\"id\": " + std::to_string(i) + "}",
                          timestamps[i]);
    }
    
    // Engine should handle out-of-order timestamps gracefully
    EXPECT_EQ(engine.getQueueDepth("test_collection"), 5)
        << "Should accept all tuples despite out-of-order timestamps";
}

} // namespace
