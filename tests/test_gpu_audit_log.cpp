#include <gtest/gtest.h>
#include "themis/gpu/audit_log.h"

using namespace themis::gpu;

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

TEST(GPUAuditLogTest, InitialState_Empty) {
    GPUAuditLog log(32);
    EXPECT_EQ(log.size(), 0u);
    EXPECT_EQ(log.capacity(), 32u);
    EXPECT_EQ(log.totalRecorded(), 0u);
    EXPECT_TRUE(log.snapshot().empty());
}

// ---------------------------------------------------------------------------
// record / snapshot
// ---------------------------------------------------------------------------

TEST(GPUAuditLogTest, Record_Single_AppearsInSnapshot) {
    GPUAuditLog log(16);
    log.recordAllocSuccess(4096, "vector_index", "tenant_a");

    const auto events = log.snapshot();
    ASSERT_EQ(events.size(), 1u);
    EXPECT_EQ(events[0].type, GPUAuditLog::EventType::ALLOC_SUCCESS);
    EXPECT_EQ(events[0].size_bytes, 4096u);
    EXPECT_EQ(events[0].tag, "vector_index");
    EXPECT_EQ(events[0].tenant_id, "tenant_a");
}

TEST(GPUAuditLogTest, Record_Multiple_OrderIsOldestFirst) {
    GPUAuditLog log(16);
    log.recordAllocSuccess(100, "a");
    log.recordDealloc(100, "a");
    log.recordFallbackToCPU("OOM");

    const auto events = log.snapshot();
    ASSERT_EQ(events.size(), 3u);
    EXPECT_EQ(events[0].type, GPUAuditLog::EventType::ALLOC_SUCCESS);
    EXPECT_EQ(events[1].type, GPUAuditLog::EventType::DEALLOC);
    EXPECT_EQ(events[2].type, GPUAuditLog::EventType::FALLBACK_TO_CPU);
}

// ---------------------------------------------------------------------------
// Ring-buffer wrap-around
// ---------------------------------------------------------------------------

TEST(GPUAuditLogTest, RingBuffer_WrapAround_OldestEvicted) {
    GPUAuditLog log(4);
    // Fill to capacity.
    log.recordAllocSuccess(1, "old_1");
    log.recordAllocSuccess(2, "old_2");
    log.recordAllocSuccess(3, "old_3");
    log.recordAllocSuccess(4, "old_4");
    EXPECT_EQ(log.size(), 4u);

    // Write one more — evicts the oldest.
    log.recordAllocSuccess(5, "new");
    EXPECT_EQ(log.size(), 4u);  // still at capacity
    EXPECT_EQ(log.totalRecorded(), 5u);

    const auto events = log.snapshot();
    // The oldest "old_1" must be gone; "new" must be present.
    EXPECT_EQ(events[0].tag, "old_2");
    EXPECT_EQ(events[3].tag, "new");
}

TEST(GPUAuditLogTest, RingBuffer_TotalRecorded_ExceedsCapacity) {
    GPUAuditLog log(4);
    for (int i = 0; i < 10; ++i) {
        log.recordAllocSuccess(static_cast<uint64_t>(i), "t");
    }
    EXPECT_EQ(log.totalRecorded(), 10u);
    EXPECT_EQ(log.size(), 4u);  // only 4 kept
}

// ---------------------------------------------------------------------------
// Convenience helpers
// ---------------------------------------------------------------------------

TEST(GPUAuditLogTest, RecordAllocFailGlobalLimit_HasMessage) {
    GPUAuditLog log(8);
    log.recordAllocFailGlobalLimit(1024, "index", "t1");
    const auto ev = log.snapshot();
    ASSERT_EQ(ev.size(), 1u);
    EXPECT_EQ(ev[0].type, GPUAuditLog::EventType::ALLOC_FAIL_GLOBAL_LIMIT);
    EXPECT_FALSE(ev[0].message.empty());
    EXPECT_EQ(ev[0].tenant_id, "t1");
}

TEST(GPUAuditLogTest, RecordAllocFailTenantQuota_HasMessage) {
    GPUAuditLog log(8);
    log.recordAllocFailTenantQuota(2048, "cache", "t2");
    const auto ev = log.snapshot();
    ASSERT_EQ(ev.size(), 1u);
    EXPECT_EQ(ev[0].type, GPUAuditLog::EventType::ALLOC_FAIL_TENANT_QUOTA);
    EXPECT_FALSE(ev[0].message.empty());
}

TEST(GPUAuditLogTest, RecordDeviceUnavailable_StoredCorrectly) {
    GPUAuditLog log(8);
    log.recordDeviceUnavailable("no GPU found");
    const auto ev = log.snapshot();
    ASSERT_EQ(ev.size(), 1u);
    EXPECT_EQ(ev[0].type, GPUAuditLog::EventType::DEVICE_UNAVAILABLE);
    EXPECT_EQ(ev[0].message, "no GPU found");
}

TEST(GPUAuditLogTest, RecordCircuitOpened_StoredCorrectly) {
    GPUAuditLog log(8);
    log.recordCircuitOpened("3 consecutive device errors");
    const auto ev = log.snapshot();
    ASSERT_EQ(ev.size(), 1u);
    EXPECT_EQ(ev[0].type, GPUAuditLog::EventType::CIRCUIT_OPENED);
}

TEST(GPUAuditLogTest, RecordCircuitReset_StoredCorrectly) {
    GPUAuditLog log(8);
    log.recordCircuitReset();
    const auto ev = log.snapshot();
    ASSERT_EQ(ev.size(), 1u);
    EXPECT_EQ(ev[0].type, GPUAuditLog::EventType::CIRCUIT_RESET);
}

// ---------------------------------------------------------------------------
// Timestamps
// ---------------------------------------------------------------------------

TEST(GPUAuditLogTest, Events_HaveTimestamp) {
    GPUAuditLog log(8);
    const auto before = std::chrono::system_clock::now();
    log.recordAllocSuccess(512, "ts_test");
    const auto after = std::chrono::system_clock::now();

    const auto ev = log.snapshot();
    ASSERT_EQ(ev.size(), 1u);
    EXPECT_GE(ev[0].timestamp, before);
    EXPECT_LE(ev[0].timestamp, after);
}

// ---------------------------------------------------------------------------
// clear
// ---------------------------------------------------------------------------

TEST(GPUAuditLogTest, Clear_RemovesEvents_PreservesTotal) {
    GPUAuditLog log(8);
    log.recordAllocSuccess(1, "a");
    log.recordAllocSuccess(2, "b");
    EXPECT_EQ(log.size(), 2u);
    EXPECT_EQ(log.totalRecorded(), 2u);

    log.clear();
    EXPECT_EQ(log.size(), 0u);
    EXPECT_TRUE(log.snapshot().empty());
    EXPECT_EQ(log.totalRecorded(), 2u);  // lifetime count preserved
}

// ---------------------------------------------------------------------------
// Thread safety — concurrent writes
// ---------------------------------------------------------------------------

TEST(GPUAuditLogTest, ConcurrentWrites_NoDataRace) {
    GPUAuditLog log(256);
    constexpr int THREADS = 8, OPS_PER_THREAD = 50;

    std::vector<std::thread> threads;
    threads.reserve(THREADS);
    for (int t = 0; t < THREADS; ++t) {
        threads.emplace_back([&log, t] {
            for (int i = 0; i < OPS_PER_THREAD; ++i) {
                log.recordAllocSuccess(static_cast<uint64_t>(i),
                                       "t" + std::to_string(t));
            }
        });
    }
    for (auto& th : threads) th.join();

    EXPECT_EQ(log.totalRecorded(),
              static_cast<uint64_t>(THREADS * OPS_PER_THREAD));
    EXPECT_LE(log.size(), log.capacity());
}

// ---------------------------------------------------------------------------
// Capacity = 1 edge case
// ---------------------------------------------------------------------------

TEST(GPUAuditLogTest, CapacityOne_AlwaysHoldsLatestEvent) {
    GPUAuditLog log(1);
    log.recordAllocSuccess(1, "first");
    log.recordAllocSuccess(2, "second");
    log.recordAllocSuccess(3, "third");

    EXPECT_EQ(log.size(), 1u);
    const auto ev = log.snapshot();
    EXPECT_EQ(ev[0].tag, "third");
    EXPECT_EQ(log.totalRecorded(), 3u);
}
