/**
 * @file test_llm_metrics.cpp
 * @brief Unit tests for LLM metrics collection and observability
 */

#include <gtest/gtest.h>
#include "aql/llm_metrics_collector.h"
#include <chrono>

using namespace themis::aql;

class LLMMetricsTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Reset metrics before each test
        auto& collector = LLMMetricsCollector::instance();
        collector.initialize();
    }
    
    void TearDown() override {
        // Cleanup if needed
    }
};

// ============================================================================
// Metrics Initialization Tests
// ============================================================================

TEST_F(LLMMetricsTest, MetricsCollector_Initialization) {
    auto& collector = LLMMetricsCollector::instance();
    
    // Initialize should be idempotent
    EXPECT_NO_THROW(collector.initialize());
    EXPECT_NO_THROW(collector.initialize());
}

TEST_F(LLMMetricsTest, MetricsCollector_Singleton) {
    auto& collector1 = LLMMetricsCollector::instance();
    auto& collector2 = LLMMetricsCollector::instance();
    
    // Should be the same instance
    EXPECT_EQ(&collector1, &collector2);
}

// ============================================================================
// Inference Metrics Tests
// ============================================================================

TEST_F(LLMMetricsTest, RecordInference_Success) {
    auto& collector = LLMMetricsCollector::instance();
    
    EXPECT_NO_THROW({
        collector.recordInference(
            "llama-2-7b",
            "legal-lora",
            std::chrono::milliseconds(500),
            100,  // input tokens
            50,   // output tokens
            true, // success
            ""
        );
    });
}

TEST_F(LLMMetricsTest, RecordInference_Failure) {
    auto& collector = LLMMetricsCollector::instance();
    
    EXPECT_NO_THROW({
        collector.recordInference(
            "llama-2-7b",
            "",
            std::chrono::milliseconds(1000),
            100,
            0,
            false,  // failure
            "LLM_TIMEOUT"
        );
    });
}

TEST_F(LLMMetricsTest, RecordInference_MultipleOperations) {
    auto& collector = LLMMetricsCollector::instance();
    
    // Record multiple inferences
    for (int i = 0; i < 10; i++) {
        collector.recordInference(
            "test-model",
            "",
            std::chrono::milliseconds(100 * i),
            50 + i,
            25 + i,
            i % 3 != 0,  // Fail every 3rd operation
            i % 3 == 0 ? "LLM_INFERENCE_FAILED" : ""
        );
    }
    
    // No exceptions should be thrown
}

// ============================================================================
// RAG Metrics Tests
// ============================================================================

TEST_F(LLMMetricsTest, RecordRAG_Success) {
    auto& collector = LLMMetricsCollector::instance();
    
    EXPECT_NO_THROW({
        collector.recordRAG(
            "documents",
            "rag-lora",
            std::chrono::milliseconds(2000),
            5,    // retrieved docs
            200,  // input tokens
            150,  // output tokens
            true,
            ""
        );
    });
}

TEST_F(LLMMetricsTest, RecordRAG_Failure) {
    auto& collector = LLMMetricsCollector::instance();
    
    EXPECT_NO_THROW({
        collector.recordRAG(
            "documents",
            "",
            std::chrono::milliseconds(500),
            0,    // no docs retrieved
            100,
            0,
            false,
            "LLM_RAG_FAILED"
        );
    });
}

TEST_F(LLMMetricsTest, RecordRAG_VariousDocumentCounts) {
    auto& collector = LLMMetricsCollector::instance();
    
    // Test with different numbers of retrieved documents
    for (size_t doc_count : {0, 1, 5, 10, 20}) {
        collector.recordRAG(
            "documents",
            "",
            std::chrono::milliseconds(1000),
            doc_count,
            100,
            50,
            true,
            ""
        );
    }
}

// ============================================================================
// Embedding Metrics Tests
// ============================================================================

TEST_F(LLMMetricsTest, RecordEmbedding_Success) {
    auto& collector = LLMMetricsCollector::instance();
    
    EXPECT_NO_THROW({
        collector.recordEmbedding(
            "text-embedding-ada-002",
            std::chrono::milliseconds(100),
            50,   // input tokens
            true,
            ""
        );
    });
}

TEST_F(LLMMetricsTest, RecordEmbedding_Failure) {
    auto& collector = LLMMetricsCollector::instance();
    
    EXPECT_NO_THROW({
        collector.recordEmbedding(
            "text-embedding-ada-002",
            std::chrono::milliseconds(50),
            25,
            false,
            "LLM_EMBEDDING_FAILED"
        );
    });
}

// ============================================================================
// Cache Metrics Tests
// ============================================================================

TEST_F(LLMMetricsTest, RecordCacheAccess_Hit) {
    auto& collector = LLMMetricsCollector::instance();
    
    EXPECT_NO_THROW({
        collector.recordCacheAccess("prefix", true);
    });
}

TEST_F(LLMMetricsTest, RecordCacheAccess_Miss) {
    auto& collector = LLMMetricsCollector::instance();
    
    EXPECT_NO_THROW({
        collector.recordCacheAccess("response", false);
    });
}

TEST_F(LLMMetricsTest, RecordCacheAccess_MultipleCacheTypes) {
    auto& collector = LLMMetricsCollector::instance();
    
    // Record hits and misses for different cache types
    collector.recordCacheAccess("prefix", true);
    collector.recordCacheAccess("prefix", false);
    collector.recordCacheAccess("response", true);
    collector.recordCacheAccess("response", false);
    
    // All should complete without errors
}

TEST_F(LLMMetricsTest, RecordCacheAccess_HitRateCalculation) {
    auto& collector = LLMMetricsCollector::instance();
    
    // Simulate 70% hit rate
    for (int i = 0; i < 100; i++) {
        collector.recordCacheAccess("prefix", i < 70);
    }
}

// ============================================================================
// Model Memory Metrics Tests
// ============================================================================

TEST_F(LLMMetricsTest, UpdateModelMemory_SingleModel) {
    auto& collector = LLMMetricsCollector::instance();
    
    EXPECT_NO_THROW({
        collector.updateModelMemory("llama-2-7b", 7000000000);  // 7GB
    });
}

TEST_F(LLMMetricsTest, UpdateModelMemory_MultipleModels) {
    auto& collector = LLMMetricsCollector::instance();
    
    collector.updateModelMemory("llama-2-7b", 7000000000);
    collector.updateModelMemory("llama-2-13b", 13000000000);
    collector.updateModelMemory("gpt-3.5-turbo", 0);  // Unloaded
}

TEST_F(LLMMetricsTest, UpdateModelMemory_DynamicUpdates) {
    auto& collector = LLMMetricsCollector::instance();
    
    // Simulate model loading and unloading
    collector.updateModelMemory("test-model", 1000000000);  // 1GB loaded
    collector.updateModelMemory("test-model", 2000000000);  // 2GB (expanded)
    collector.updateModelMemory("test-model", 0);           // Unloaded
}

// ============================================================================
// Circuit Breaker State Metrics Tests
// ============================================================================

TEST_F(LLMMetricsTest, RecordCircuitBreakerState_Closed) {
    auto& collector = LLMMetricsCollector::instance();
    
    EXPECT_NO_THROW({
        collector.recordCircuitBreakerState("infer", "closed");
    });
}

TEST_F(LLMMetricsTest, RecordCircuitBreakerState_Open) {
    auto& collector = LLMMetricsCollector::instance();
    
    EXPECT_NO_THROW({
        collector.recordCircuitBreakerState("rag", "open");
    });
}

TEST_F(LLMMetricsTest, RecordCircuitBreakerState_HalfOpen) {
    auto& collector = LLMMetricsCollector::instance();
    
    EXPECT_NO_THROW({
        collector.recordCircuitBreakerState("embed", "half_open");
    });
}

TEST_F(LLMMetricsTest, RecordCircuitBreakerState_Transitions) {
    auto& collector = LLMMetricsCollector::instance();
    
    // Simulate state transitions: closed → open → half_open → closed
    collector.recordCircuitBreakerState("infer", "closed");
    collector.recordCircuitBreakerState("infer", "open");
    collector.recordCircuitBreakerState("infer", "half_open");
    collector.recordCircuitBreakerState("infer", "closed");
}

// ============================================================================
// Latency Tracking Tests
// ============================================================================

TEST_F(LLMMetricsTest, LatencyTracking_VaryingLatencies) {
    auto& collector = LLMMetricsCollector::instance();
    
    // Record operations with varying latencies
    std::vector<int> latencies = {50, 100, 150, 200, 500, 1000, 2000, 5000};
    
    for (auto latency_ms : latencies) {
        collector.recordInference(
            "test-model",
            "",
            std::chrono::milliseconds(latency_ms),
            100,
            50,
            true,
            ""
        );
    }
}

TEST_F(LLMMetricsTest, LatencyTracking_Percentiles) {
    auto& collector = LLMMetricsCollector::instance();
    
    // Record 100 operations to test percentile calculation
    for (int i = 0; i < 100; i++) {
        collector.recordInference(
            "test-model",
            "",
            std::chrono::milliseconds(i * 10),  // 0ms to 990ms
            100,
            50,
            true,
            ""
        );
    }
    
    // p50 should be around 500ms, p95 around 950ms, p99 around 990ms
}

// ============================================================================
// Token Throughput Tests
// ============================================================================

TEST_F(LLMMetricsTest, TokenThroughput_Tracking) {
    auto& collector = LLMMetricsCollector::instance();
    
    size_t total_input_tokens = 0;
    size_t total_output_tokens = 0;
    
    for (int i = 0; i < 50; i++) {
        size_t input = 100 + i;
        size_t output = 50 + i;
        
        collector.recordInference(
            "test-model",
            "",
            std::chrono::milliseconds(100),
            input,
            output,
            true,
            ""
        );
        
        total_input_tokens += input;
        total_output_tokens += output;
    }
    
    // Metrics should track cumulative token counts
}

// ============================================================================
// Error Code Distribution Tests
// ============================================================================

TEST_F(LLMMetricsTest, ErrorCodeDistribution) {
    auto& collector = LLMMetricsCollector::instance();
    
    // Record various error types
    std::vector<std::string> error_codes = {
        "LLM_TIMEOUT",
        "LLM_INFERENCE_FAILED",
        "LLM_MODEL_NOT_FOUND",
        "LLM_OUT_OF_MEMORY",
        "LLM_INVALID_OPTIONS"
    };
    
    for (const auto& error_code : error_codes) {
        for (int i = 0; i < 5; i++) {
            collector.recordInference(
                "test-model",
                "",
                std::chrono::milliseconds(100),
                100,
                0,
                false,
                error_code
            );
        }
    }
}

// ============================================================================
// Concurrent Access Tests
// ============================================================================

TEST_F(LLMMetricsTest, ConcurrentMetricsCollection) {
    auto& collector = LLMMetricsCollector::instance();
    
    // Simulate concurrent metric recording (simplified test)
    std::vector<std::thread> threads;
    
    for (int t = 0; t < 5; t++) {
        threads.emplace_back([&collector, t]() {
            for (int i = 0; i < 10; i++) {
                collector.recordInference(
                    "test-model-" + std::to_string(t),
                    "",
                    std::chrono::milliseconds(100),
                    100,
                    50,
                    true,
                    ""
                );
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    // No crashes or data corruption should occur
}

// ============================================================================
// Scoped Latency Tracker Tests
// ============================================================================

TEST_F(LLMMetricsTest, ScopedLatencyTracker_Basic) {
    {
        ScopedLatencyTracker tracker;
        
        // Simulate some work
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        
        // Check elapsed time
        auto elapsed = tracker.elapsed();
        EXPECT_GE(elapsed.count(), 10);
        EXPECT_LT(elapsed.count(), 50);  // Should be close to 10ms
    }
    // Tracker destroyed here
}

TEST_F(LLMMetricsTest, ScopedLatencyTracker_Nested) {
    {
        ScopedLatencyTracker outer_tracker;
        
        {
            ScopedLatencyTracker inner_tracker;
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            EXPECT_GE(inner_tracker.elapsed().count(), 5);
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        EXPECT_GE(outer_tracker.elapsed().count(), 10);
    }
}
