/**
 * @file test_ingestion_queue_saturation_stress.cpp
 * @brief Stress tests for Phase 2.9 — Queue limits, saturation handling, and memory exhaustion.
 *
 * Phase 2.9 (Bounded Resources + Stress Tests) — ING-IMPL-001, ING-IMPL-002
 *
 * Validates:
 * - Queue saturation detection and blocking semantics
 * - Memory exhaustion detection and alerts
 * - Backpressure cascade across multiple connectors
 * - Distributed flow control under sustained load
 * - Stress scenarios with high throughput and memory pressure
 *
 * Test families:
 * - INGQ-01..04: Basic queue capacity and saturation
 * - INGQ-05..08: Blocking and timeout semantics
 * - INGQ-09..12: Memory limits and exhaustion detection
 * - INGQ-13..16: Distributed flow control and multi-node coordination
 *
 * @see include/ingestion/bounded_queue.h
 * @see include/ingestion/resource_monitor.h
 * @see src/ingestion/ROADMAP.md — Phase 2.9 item
 */

#include <gtest/gtest.h>

#include "ingestion/bounded_queue.h"
#include "ingestion/resource_monitor.h"

#include <chrono>
#include <memory>
#include <string>
#include <thread>
#include <vector>

using namespace themis::ingestion;
using namespace std::chrono_literals;

// ---------------------------------------------------------------------------
// TestItem and QueueItemSize<TestItem> specialization must live in the same
// namespace as the primary QueueItemSize template so explicit specialization
// is well-formed (C++17 [temp.expl.spec]/2).
// ---------------------------------------------------------------------------
namespace themis::ingestion {

struct TestItem {
    int id = 0;
    std::string data;
    std::vector<char> payload;

    explicit TestItem(int id = 0, const std::string& d = "",
                      std::size_t payload_size = 0)
        : id(id), data(d), payload(payload_size, 'x') {}

    TestItem(const TestItem& other) = default;
    TestItem& operator=(const TestItem& other) = default;
    TestItem(TestItem&& other) = default;
    TestItem& operator=(TestItem&& other) = default;
};

// Specialization of QueueItemSize for TestItem
template <>
struct QueueItemSize<TestItem> {
    static std::size_t getSize(const TestItem& item) {
        return sizeof(TestItem) + item.data.capacity() + item.payload.capacity();
    }
};

}  // namespace themis::ingestion

// ===========================================================================
// INGQ-01..04 — Basic queue capacity and saturation
// ===========================================================================

TEST(IngestionQueueSaturationINGQ01, BasicEnqueueDequeue) {
    QueueResourceLimitConfig config;
    config.max_queue_size = 100;
    BoundedQueue<TestItem> queue(config);

    // Enqueue items
    for (int i = 0; i < 10; ++i) {
        auto result = queue.enqueue(TestItem(i, "data"), false);
        EXPECT_TRUE(result.success) << "Enqueue should succeed when below capacity";
    }

    EXPECT_EQ(queue.size(), 10);
    EXPECT_EQ(queue.getStats().enqueue_successes, 10);
}

TEST(IngestionQueueSaturationINGQ02, QueueSaturationDetection) {
    QueueResourceLimitConfig config;
    config.max_queue_size = 5;
    config.blocking_on_saturation = false;  // Non-blocking for this test
    BoundedQueue<TestItem> queue(config);

    // Fill queue to capacity
    for (int i = 0; i < 5; ++i) {
        queue.enqueue(TestItem(i), false);
    }

    // Next enqueue should fail with saturation
    auto result = queue.enqueue(TestItem(5), false);
    EXPECT_FALSE(result.success);
    EXPECT_TRUE(result.is_saturation);
    EXPECT_FALSE(result.is_timeout);

    auto stats = queue.getStats();
    EXPECT_EQ(stats.current_state, QueueSaturationState::SATURATED);
}

TEST(IngestionQueueSaturationINGQ03, SaturationWarningThreshold) {
    QueueResourceLimitConfig config;
    config.max_queue_size = 100;
    config.saturation_warning_threshold_percent = 70;
    config.blocking_on_saturation = false;
    BoundedQueue<TestItem> queue(config);

    // Add items up to warning threshold (70 items)
    for (int i = 0; i < 70; ++i) {
        queue.enqueue(TestItem(i), false);
    }

    auto stats = queue.getStats();
    EXPECT_EQ(stats.current_state, QueueSaturationState::SATURATION_WARNING);
    EXPECT_EQ(stats.current_item_count, 70);
}

TEST(IngestionQueueSaturationINGQ04, StatePeakTracking) {
    QueueResourceLimitConfig config;
    config.max_queue_size = 100;
    BoundedQueue<TestItem> queue(config);

    // Add some items
    for (int i = 0; i < 50; ++i) {
        queue.enqueue(TestItem(i), false);
    }

    auto stats1 = queue.getStats();
    EXPECT_EQ(stats1.max_item_count_observed, 50);

    // Add more items to reach 80
    for (int i = 50; i < 80; ++i) {
        queue.enqueue(TestItem(i), false);
    }

    auto stats2 = queue.getStats();
    EXPECT_EQ(stats2.max_item_count_observed, 80) << "Peak should track highest count";
}

// ===========================================================================
// INGQ-05..08 — Blocking and timeout semantics
// ===========================================================================

TEST(IngestionQueueSaturationINGQ05, BlockingEnqueueWaitsForSpace) {
    QueueResourceLimitConfig config;
    config.max_queue_size = 2;
    config.blocking_on_saturation = true;
    config.blocking_enqueue_timeout_ms = 1000ms;
    BoundedQueue<TestItem> queue(config);

    // Fill queue
    queue.enqueue(TestItem(0), false);
    queue.enqueue(TestItem(1), false);

    // Start dequeue thread
    std::thread dequeue_thread([&queue]() {
        std::this_thread::sleep_for(100ms);
        queue.tryDequeue();
    });

    // This should block and then succeed when space becomes available
    auto start = std::chrono::steady_clock::now();
    auto result = queue.enqueue(TestItem(2), true);
    auto elapsed = std::chrono::steady_clock::now() - start;

    EXPECT_TRUE(result.success) << "Blocking enqueue should succeed after dequeue";
    EXPECT_GE(elapsed, 50ms);  // Should have waited at least a bit
    EXPECT_LT(elapsed, 500ms);  // But not too long

    dequeue_thread.join();
}

TEST(IngestionQueueSaturationINGQ06, EnqueueTimeoutOnProlongedSaturation) {
    QueueResourceLimitConfig config;
    config.max_queue_size = 1;
    config.blocking_on_saturation = true;
    config.blocking_enqueue_timeout_ms = 100ms;
    BoundedQueue<TestItem> queue(config);

    // Fill queue
    queue.enqueue(TestItem(0), false);

    // Try to enqueue with timeout (should timeout since nothing is dequeuing)
    auto start = std::chrono::steady_clock::now();
    auto result = queue.enqueue(TestItem(1), true);
    auto elapsed = std::chrono::steady_clock::now() - start;

    EXPECT_FALSE(result.success);
    EXPECT_TRUE(result.is_timeout);
    EXPECT_GE(elapsed, 80ms);  // Should have waited ~100ms before timing out
}

TEST(IngestionQueueSaturationINGQ07, DequeueWithTimeout) {
    BoundedQueue<TestItem> queue;
    queue.enqueue(TestItem(0, "data0"), false);

    auto item = queue.dequeueWithTimeout(100ms);
    ASSERT_TRUE(item.has_value());
    EXPECT_EQ(item->id, 0);
    EXPECT_EQ(item->data, "data0");

    // Next dequeue should timeout
    auto item2 = queue.dequeueWithTimeout(50ms);
    EXPECT_FALSE(item2.has_value());
}

TEST(IngestionQueueSaturationINGQ08, TryDequeueNonBlocking) {
    BoundedQueue<TestItem> queue;
    
    // Empty queue
    auto empty_result = queue.tryDequeue();
    EXPECT_FALSE(empty_result.has_value());

    // Enqueue and dequeue
    queue.enqueue(TestItem(42, "test"), false);
    auto result = queue.tryDequeue();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->id, 42);
}

// ===========================================================================
// INGQ-09..12 — Memory limits and exhaustion detection
// ===========================================================================

TEST(IngestionQueueSaturationINGQ09, MemoryLimitEnforcement) {
    QueueResourceLimitConfig config;
    config.max_queue_size = 10000;  // High item limit
    config.max_memory_bytes = 5000;  // But low memory limit
    config.blocking_on_saturation = false;
    BoundedQueue<TestItem> queue(config);

    // Add items until memory limit is reached
    TestItem large_item(0, "data", 3000);  // 3000 bytes of payload
    auto result1 = queue.enqueue(large_item, false);
    EXPECT_TRUE(result1.success);

    // Second item should fail due to memory limit
    TestItem large_item2(1, "data", 3000);
    auto result2 = queue.enqueue(large_item2, false);
    EXPECT_FALSE(result2.success);
    EXPECT_TRUE(result2.is_memory_limit);
}

TEST(IngestionQueueSaturationINGQ10, MemoryUsageTracking) {
    QueueResourceLimitConfig config;
    config.max_queue_size = 1000;
    config.max_memory_bytes = 1024 * 1024;  // 1 MB
    BoundedQueue<TestItem> queue(config);

    // Add item with 10KB payload
    TestItem item(0, "data", 10000);
    queue.enqueue(item, false);

    auto stats = queue.getStats();
    EXPECT_GT(stats.current_memory_bytes, 10000);
    EXPECT_GT(stats.max_memory_observed, 0);
}

TEST(IngestionQueueSaturationINGQ11, MemoryExhaustionState) {
    QueueResourceLimitConfig config;
    config.max_queue_size = 10000;
    config.max_memory_bytes = 1000;  // Very small limit
    config.blocking_on_saturation = false;
    BoundedQueue<TestItem> queue(config);

    // Add items until memory exhausted
    for (int i = 0; i < 100; ++i) {
        TestItem item(i, "data", 100);  // 100 bytes each
        auto result = queue.enqueue(item, false);
        if (!result.success) {
            break;
        }
    }

    auto stats = queue.getStats();
    EXPECT_TRUE(stats.current_state == QueueSaturationState::MEMORY_EXHAUSTION ||
                stats.current_state == QueueSaturationState::SATURATED);
}

TEST(IngestionQueueSaturationINGQ12, MemoryRecoveryAfterDequeue) {
    QueueResourceLimitConfig config;
    config.max_queue_size = 100;
    config.max_memory_bytes = 5000;
    BoundedQueue<TestItem> queue(config);

    // Fill with one large item
    TestItem item(0, "data", 3000);
    queue.enqueue(item, false);

    auto mem1 = queue.currentMemoryBytes();
    EXPECT_GT(mem1, 3000);

    // Dequeue
    queue.tryDequeue();

    auto mem2 = queue.currentMemoryBytes();
    EXPECT_LT(mem2, mem1);
}

// ===========================================================================
// INGQ-13..16 — Distributed flow control and stress scenarios
// ===========================================================================

TEST(IngestionQueueSaturationINGQ13, StressHighThroughput) {
    QueueResourceLimitConfig config;
    config.max_queue_size = 1000;
    config.blocking_on_saturation = false;
    BoundedQueue<TestItem> queue(config);

    const int num_items = 500;

    // Producer thread
    std::thread producer([&queue, num_items]() {
        for (int i = 0; i < num_items; ++i) {
            queue.enqueue(TestItem(i, "item"), false);
        }
    });

    // Consumer thread
    int consumed = 0;
    std::thread consumer([&queue, &consumed]() {
        while (consumed < 500) {
            if (auto item = queue.tryDequeue()) {
                consumed++;
            }
        }
    });

    producer.join();
    consumer.join();

    EXPECT_GE(consumed, 300);  // Should have consumed most items
}

TEST(IngestionQueueSaturationINGQ14, StressMemoryExhaustionRecovery) {
    QueueResourceLimitConfig config;
    config.max_queue_size = 10000;
    config.max_memory_bytes = 100000;  // 100 KB
    config.blocking_on_saturation = true;
    config.blocking_enqueue_timeout_ms = 500ms;
    BoundedQueue<TestItem> queue(config);

    // Fill queue to saturation
    int enqueued = 0;
    for (int i = 0; i < 1000; ++i) {
        TestItem item(i, "data", 500);  // 500 bytes each
        auto result = queue.enqueue(item, false);
        if (result.success) {
            enqueued++;
        } else {
            break;  // Hit limit
        }
    }

    // Start dequeue thread
    std::thread dequeuer([&queue]() {
        std::this_thread::sleep_for(50ms);
        while (!queue.empty()) {
            queue.tryDequeue();
            std::this_thread::sleep_for(1ms);
        }
    });

    // Try to enqueue more (should block then succeed as space frees up)
    auto result = queue.enqueue(TestItem(999, "recovery"), true);
    EXPECT_TRUE(result.success);

    dequeuer.join();
}

TEST(IngestionQueueSaturationINGQ15, ResourceMonitorBasic) {
    ResourceMonitor monitor;

    // Set thresholds
    monitor.setMemoryExhaustionThreshold(90.0);
    EXPECT_EQ(monitor.getMemoryExhaustionThreshold(), 90.0);

    // Track component memory
    monitor.registerComponentMemory("connector_1", 1024 * 1024);  // 1 MB
    monitor.registerComponentMemory("connector_2", 2048 * 1024);  // 2 MB

    auto mem1 = monitor.getComponentMemory("connector_1");
    ASSERT_TRUE(mem1.has_value());
    EXPECT_EQ(*mem1, 1024 * 1024);

    auto total = monitor.getTotalComponentMemory();
    EXPECT_EQ(total, 3072 * 1024);
}

TEST(IngestionQueueSaturationINGQ16, ResourceMonitorDistributedClusterSize) {
    ResourceMonitor monitor;
    
    monitor.setDistributedClusterSize(5);
    EXPECT_EQ(monitor.getDistributedClusterSize(), 5);

    // Calculate per-node quota
    auto per_node = monitor.calculatePerNodeQuota(10000);
    EXPECT_EQ(per_node, 2000);  // 10000 / 5
}

