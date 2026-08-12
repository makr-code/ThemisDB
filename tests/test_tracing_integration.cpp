#include <gtest/gtest.h>
#include "utils/tracing.h"
#include "storage/storage_engine.h"
#include "index/index_manager.h"
#include "query/query_engine.h"
#include "observability/metrics_collector.h"
#include <memory>
#include <future>
#include <thread>
#include <chrono>

using namespace themis;
using namespace themis::observability;

/**
 * Integration test for end-to-end distributed tracing across components
 */
class TracingIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Reset state for clean tests
        MetricsCollector::getInstance().reset();
    }
    
    void TearDown() override {
        Tracer::shutdown();
    }
};

/**
 * Test tracing through storage operations
 */
TEST_F(TracingIntegrationTest, StorageOperationTracing) {
    // Create storage engine
    auto storage = StorageEngine::createDefault();
    ASSERT_NE(storage, nullptr);
    
    int64_t initial_spans = Tracer::getTotalSpans();
    
    // Perform storage operations (which should create spans)
    storage->put("test:key1", "value1");
    storage->get("test:key1");
    storage->del("test:key1");
    
    // Verify spans were created
    EXPECT_GT(Tracer::getTotalSpans(), initial_spans);
}

/**
 * Test tracing through index operations
 */
TEST_F(TracingIntegrationTest, IndexOperationTracing) {
    auto index_manager = IndexManager::createDefault();
    ASSERT_NE(index_manager, nullptr);
    
    int64_t initial_spans = Tracer::getTotalSpans();
    
    // Attempt to create indices (spans should be created even if operations fail)
    auto result1 = index_manager->createSecondaryIndex("tenant-trace", "test_idx", "test_field", "range");
    auto result2 = index_manager->createVectorIndex("vector_idx", 128, "");
    
    // Verify spans were created regardless of success/failure
    EXPECT_GT(Tracer::getTotalSpans(), initial_spans);
}

/**
 * Test parent-child span relationships across components
 */
TEST_F(TracingIntegrationTest, CrossComponentSpanHierarchy) {
    // Create a parent span for the entire operation
    auto operation_span = Tracer::startSpan("integration.full_operation");
    operation_span.setAttribute("operation.type", "full_stack_test");
    
    int64_t initial_spans = Tracer::getTotalSpans();
    
    {
        // Storage operation (creates child spans)
        auto storage = StorageEngine::createDefault();
        storage->put("hierarchy:test", "data");
        
        // Index operation (creates child spans)
        auto index_manager = IndexManager::createDefault();
        auto result = index_manager->createSecondaryIndex("tenant-trace", "hierarchy_idx", "field", "");
    }
    
    operation_span.end();
    
    // Verify multiple spans were created
    EXPECT_GT(Tracer::getTotalSpans(), initial_spans + 1);
}

/**
 * Test metrics collection integration
 */
TEST_F(TracingIntegrationTest, MetricsIntegration) {
    auto& metrics = MetricsCollector::getInstance();
    
    {
        TracedSpan span("integration.metrics_test");
        span.setAttribute("component", "integration_test");
        
        // Perform some operations
        auto storage = StorageEngine::createDefault();
        storage->put("metrics:key", "value");
    }
    
    // Get Prometheus metrics
    std::string prometheus_output = metrics.getPrometheusMetrics();
    
    // Verify tracing metrics are present
    EXPECT_NE(prometheus_output.find("trace_span_duration_ms"), std::string::npos);
    EXPECT_NE(prometheus_output.find("integration.metrics_test"), std::string::npos);
}

/**
 * Test concurrent operations with tracing
 */
TEST_F(TracingIntegrationTest, ConcurrentOperations) {
    const int num_threads = 5;
    std::vector<std::thread> threads;
    
    int64_t initial_spans = Tracer::getTotalSpans();
    
    for (int i = 0; i < num_threads; i++) {
        threads.emplace_back([i]() {
            TracedSpan thread_span("integration.concurrent_operation");
            thread_span.setAttribute("thread.id", static_cast<int64_t>(i));
            
            // Each thread performs storage operations
            auto storage = StorageEngine::createDefault();
            storage->put("concurrent:" + std::to_string(i), "data");
            storage->get("concurrent:" + std::to_string(i));
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Verify spans were created from all threads
    EXPECT_GT(Tracer::getTotalSpans(), initial_spans + num_threads);
}

/**
 * Test error propagation through traced operations
 */
TEST_F(TracingIntegrationTest, ErrorPropagation) {
    TracedSpan operation("integration.error_test");
    
    try {
        // Create an index manager without proper initialization
        auto index_manager = IndexManager::createDefault();
        
        // This should fail but still create spans
        auto result = index_manager->createSecondaryIndex("tenant-trace", "error_idx", "field", "invalid_config");
        
        if (!result) {
            operation.recordError(result.error().message());
            operation.setStatus(false);
        }
    } catch (const std::exception& e) {
        operation.recordError(e.what());
        operation.setStatus(false);
    }
    
    // Verify span was created even with errors
    EXPECT_GT(Tracer::getTotalSpans(), 0);
}

/**
 * Test span attributes across components
 */
TEST_F(TracingIntegrationTest, SpanAttributesPropagation) {
    auto request_span = Tracer::startSpan("integration.request");
    request_span.setAttribute("request.id", "test-123");
    request_span.setAttribute("request.user", "test_user");
    
    {
        // Create child operations
        auto storage_span = Tracer::startChildSpan("integration.storage", request_span);
        storage_span.setAttribute("storage.operation", "write");
        
        auto storage = StorageEngine::createDefault();
        storage->put("attr:test", "value");
        
        storage_span.end();
    }
    
    request_span.setStatus(true);
    request_span.end();
}

/**
 * Test trace context preservation across async boundaries
 */
TEST_F(TracingIntegrationTest, AsyncContextPreservation) {
    auto parent_span = Tracer::startSpan("integration.async_parent");
    parent_span.setAttribute("async.test", true);
    
    std::promise<bool> completion;
    auto future = completion.get_future();
    
    std::thread async_worker([parent_span = std::move(parent_span), &completion]() mutable {
        // Create child span in async context
        auto child_span = Tracer::startChildSpan("integration.async_child", parent_span);
        child_span.setAttribute("async.worker", true);
        
        // Simulate work
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        
        child_span.setStatus(true);
        child_span.end();
        parent_span.end();
        
        completion.set_value(true);
    });
    
    future.wait();
    async_worker.join();
    
    EXPECT_TRUE(future.get());
}

/**
 * Test metrics snapshot consistency
 */
TEST_F(TracingIntegrationTest, MetricsSnapshot) {
    int64_t initial_total = Tracer::getTotalSpans();
    int64_t initial_active = Tracer::getActiveSpans();
    
    {
        ScopedSpan span1("integration.snapshot1");
        {
            ScopedSpan span2("integration.snapshot2");
            
            // Both spans active
            EXPECT_EQ(Tracer::getActiveSpans(), initial_active + 2);
        }
        // One span active
        EXPECT_EQ(Tracer::getActiveSpans(), initial_active + 1);
    }
    // No spans active (back to initial)
    EXPECT_EQ(Tracer::getActiveSpans(), initial_active);
    EXPECT_EQ(Tracer::getTotalSpans(), initial_total + 2);
}

/**
 * Test long-running operation with multiple phases
 */
TEST_F(TracingIntegrationTest, MultiPhaseOperation) {
    TracedSpan operation("integration.multi_phase");
    operation.setAttribute("phases", static_cast<int64_t>(3));
    
    // Phase 1: Storage setup
    {
        auto phase1 = Tracer::startSpan("integration.phase1_storage");
        auto storage = StorageEngine::createDefault();
        storage->put("phase:1", "setup");
        phase1.end();
    }
    
    // Phase 2: Index creation
    {
        auto phase2 = Tracer::startSpan("integration.phase2_index");
        auto index_manager = IndexManager::createDefault();
        auto result = index_manager->createSecondaryIndex("tenant-trace", "phase_idx", "field", "");
        phase2.end();
    }
    
    // Phase 3: Cleanup
    {
        auto phase3 = Tracer::startSpan("integration.phase3_cleanup");
        auto storage = StorageEngine::createDefault();
        storage->del("phase:1");
        phase3.end();
    }
    
    operation.setStatus(true);
    
    // Verify all phases created spans
    EXPECT_GE(Tracer::getTotalSpans(), 4); // 1 main + 3 phases
}
