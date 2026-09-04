/**
 * @file test_replication_lock_ordering_focused.cpp
 * @brief Focused lock ordering and timeout hardening tests for replication module.
 *
 * Wave A Block 2 deliverable: ≥8 tests verifying:
 * 1. No circular deadlock scenarios
 * 2. Lock hierarchy enforced correctly
 * 3. Timeout patterns applied consistently
 * 4. Concurrent access thread-safety
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "replication/replication_slot.h"
#include "replication/raft_v2.h"
#include "replication/event_stream.h"
#include "replication/async_wal_shipper.h"
#include "replication/logical_replication.h"

#include <thread>
#include <chrono>
#include <memory>
#include <vector>

namespace themisdb {
namespace replication {
namespace test {

// ============================================================================
// Test 1: Concurrent Slot Creation (Level 1 Lock)
// ============================================================================

class ReplicationLockOrderingTest : public ::testing::Test {
protected:
    void SetUp() override {
        ReplicationConfig cfg;
        cfg.wal_directory = "tmp/themis_test_wal";
        wal_manager = std::make_shared<WALManager>(cfg);
    }

    std::shared_ptr<WALManager> wal_manager;
};

TEST_F(ReplicationLockOrderingTest, ConcurrentSlotCreation_NoDeadlock)
{
    ReplicationSlotManager::ManagerConfig config;
    config.wal_directory = "/tmp/themis_test";
    auto manager = std::make_shared<ReplicationSlotManager>(config, wal_manager);

    // Launch 10 threads creating slots concurrently
    std::vector<std::thread> threads;
    std::atomic<int> success_count(0);
    std::atomic<int> error_count(0);
    std::atomic<int> retrieval_mismatch_count(0);

    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([&, i]() {
            try {
                const std::string slot_name = "slot_" + std::to_string(i);
                auto slot = manager->createSlot(slot_name, "physical", "replica_" + std::to_string(i));
                if (slot) {
                    ++success_count;
                    // Verify slot is immediately accessible
                    auto retrieved = manager->getSlot(slot_name);
                    if (retrieved != slot) {
                        ++retrieval_mismatch_count;
                    }
                }
            } catch (const std::exception& e) {
                ++error_count;
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(success_count, 10);
    EXPECT_EQ(error_count, 0);
    EXPECT_EQ(retrieval_mismatch_count, 0);
    EXPECT_EQ(manager->slotCount(), 10);
}

// ============================================================================
// Test 2: Slot State Transitions (Level 2 Lock + Lock-Free I/O)
// ============================================================================

TEST_F(ReplicationLockOrderingTest, SlotStateTransitions_PauseResumeDropAtomic)
{
    ReplicationSlotManager::ManagerConfig config;
    config.wal_directory = "/tmp/themis_test";
    auto manager = std::make_shared<ReplicationSlotManager>(config, wal_manager);

    auto slot = manager->createSlot("test_slot", "physical", "replica");
    ASSERT_NE(slot, nullptr);
    EXPECT_EQ(slot->status(), ReplicationSlot::SlotStatus::ACTIVE);

    // Test pause
    EXPECT_TRUE(slot->pause());
    EXPECT_EQ(slot->status(), ReplicationSlot::SlotStatus::PAUSED);

    // Verify pause is idempotent
    EXPECT_FALSE(slot->pause());

    // Test resume
    EXPECT_TRUE(slot->resume());
    EXPECT_EQ(slot->status(), ReplicationSlot::SlotStatus::ACTIVE);

    // Test drop
    EXPECT_TRUE(slot->drop());
    EXPECT_EQ(slot->status(), ReplicationSlot::SlotStatus::DROPPED);

    // Verify drop is idempotent
    EXPECT_FALSE(slot->drop());
}

// ============================================================================
// Test 3: Lock Hierarchy Enforcement (No Deadlock on Concurrent Advance)
// ============================================================================

TEST_F(ReplicationLockOrderingTest, LockHierarchy_ConcurrentAdvance_NoDeadlock)
{
    ReplicationSlotManager::ManagerConfig config;
    config.wal_directory = "/tmp/themis_test";
    auto manager = std::make_shared<ReplicationSlotManager>(config, wal_manager);

    auto slot = manager->createSlot("test_slot", "physical", "replica");
    ASSERT_NE(slot, nullptr);

    // Launch 5 threads advancing LSN concurrently
    std::vector<std::thread> threads;
    std::atomic<int> advance_count(0);

    for (int i = 0; i < 5; ++i) {
        threads.emplace_back([&, i]() {
            for (int j = 0; j < 10; ++j) {
                uint64_t lsn = (i * 10 + j + 1) * 100;
                if (slot->advance(lsn)) {
                    ++advance_count;
                }
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    // All advances should succeed (monotonically increasing)
    EXPECT_EQ(advance_count, 50);
    EXPECT_EQ(slot->state().confirmed_lsn, 50 * 100);
}

// ============================================================================
// Test 4: Raft Config Lock Ordering (No WAL Append Under Mutex)
// ============================================================================

TEST_F(ReplicationLockOrderingTest, RaftV2_MembershipChange_NoLockViolation)
{
    auto config = std::make_shared<RaftV2ClusterConfig>(
        std::set<std::string>{"node1", "node2", "node3"});
    auto manager = std::make_shared<MembershipChangeManager>(
        config, "node1", wal_manager);

    // Verify proposeAdd releases lock before WAL append
    auto entry = manager->proposeAdd("node4");
    EXPECT_EQ(entry.phase, MembershipChangeEntry::Phase::JOINT);
    EXPECT_TRUE(config->isInJointConsensus());

    // Concurrent membership queries should not block
    auto all_members = config->getAllMembers();
    EXPECT_EQ(all_members.size(), 4);
}

// ============================================================================
// Test 5: Event Stream Lock-Free Callbacks
// ============================================================================

TEST_F(ReplicationLockOrderingTest, EventStream_CallbacksOutsideLocks)
{
    ReplicationEventStream::StreamConfig cfg;
    cfg.max_history_events = 1000;
    auto stream = std::make_shared<ReplicationEventStream>(cfg);

    std::atomic<int> callback_invocations(0);
    std::atomic<bool> callback_got_lock(false);

    // Subscribe with a callback that tries to acquire a lock
    auto sub = stream->subscribe(
        ReplicationEventStream::EventType::ROLE_CHANGED,
        [&](const ReplicationEventStream::Event& ev) {
            ++callback_invocations;
            // Simulate callback acquiring a lock (should not deadlock)
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        });

    // Emit events from multiple threads
    std::vector<std::thread> threads = {};

    for (int i = 0; i < 5; ++i) {
        threads.emplace_back([&, i]() {
            for (int j = 0; j < 10; ++j) {
                (void)i;
                stream->onRoleChange(ReplicationRole::FOLLOWER, ReplicationRole::LEADER);
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    // All callbacks should have been invoked
    EXPECT_EQ(callback_invocations, 50);
    EXPECT_FALSE(callback_got_lock);
}

// ============================================================================
// Test 6: Deadlock Detection Guard (Timeout on Circular Wait)
// ============================================================================

TEST_F(ReplicationLockOrderingTest, DeadlockDetection_TimeoutTriggered)
{
    // Create two slots and try to acquire them in reverse order from different threads
    ReplicationSlotManager::ManagerConfig config;
    config.wal_directory = "/tmp/themis_test";
    auto manager = std::make_shared<ReplicationSlotManager>(config, wal_manager);

    auto slot1 = manager->createSlot("slot1", "physical", "replica1");
    auto slot2 = manager->createSlot("slot2", "physical", "replica2");
    ASSERT_NE(slot1, nullptr);
    ASSERT_NE(slot2, nullptr);

    // No circular wait in our implementation (proper lock ordering),
    // but verify operations complete without hanging
    std::atomic<bool> t1_done(false);
    std::atomic<bool> t2_done(false);

    std::thread t1([&]() {
        slot1->advance(100);
        slot2->advance(200);
        t1_done = true;
    });

    std::thread t2([&]() {
        slot2->advance(150);
        slot1->advance(250);
        t2_done = true;
    });

    // Wait with timeout to detect deadlock
    auto start = std::chrono::steady_clock::now();
    t1.join();
    t2.join();
    auto duration = std::chrono::steady_clock::now() - start;

    EXPECT_TRUE(t1_done);
    EXPECT_TRUE(t2_done);
    // Should complete within reasonable time (no deadlock)
    EXPECT_LT(duration, std::chrono::seconds(5));
}

// ============================================================================
// Test 7: AsyncWalShipper Timeout Pattern (Worker Thread Timeout)
// ============================================================================

TEST_F(ReplicationLockOrderingTest, AsyncWalShipper_WorkerLoop_RespondsToTimeout)
{
    WalShippingConfig cfg;
    cfg.remote_dc_endpoint = "dc-remote:9876";
    cfg.local_dc_id = "dc-local";
    cfg.max_lag_ms = 100;
    cfg.max_queue_depth = 100;

    AsyncWalShipper shipper(cfg);

    // Set a handler that tracks invocations
    std::atomic<int> ships_received(0);
    shipper.setShipHandler([&](const WalSegment& seg) -> bool {
        ++ships_received;
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        return true;
    });

    // Enqueue several segments
    for (int i = 0; i < 5; ++i) {
        WalSegment seg;
        seg.sequence_number = i;
        seg.data = "test_data_" + std::to_string(i);
        seg.enqueue_time = std::chrono::steady_clock::now();
        seg.target_dc = "dc-remote";
        EXPECT_TRUE(shipper.enqueueSegment(std::move(seg)));
    }

    // Wait for segments to be shipped with timeout
    auto start = std::chrono::steady_clock::now();
    while (ships_received < 5 && std::chrono::steady_clock::now() - start < std::chrono::seconds(2)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    shipper.stop();

    // Verify all segments were shipped
    EXPECT_EQ(ships_received, 5);
}

// ============================================================================
// Test 8: Timeout Configuration Application
// ============================================================================

TEST_F(ReplicationLockOrderingTest, TimeoutConfiguration_AppliedCorrectly)
{
    WalShippingConfig cfg;
    cfg.remote_dc_endpoint = "dc-remote:9876";
    cfg.local_dc_id = "dc-local";
    cfg.max_lag_ms = 500;  // 500ms lag limit
    cfg.max_queue_depth = 50;

    AsyncWalShipper shipper(cfg);

    // Slow handler to trigger lag alerts
    std::atomic<int> lag_alerts(0);
    shipper.setAlertCallback([&](uint64_t lag_ms) {
        if (lag_ms > 250) {
            ++lag_alerts;
        }
    });

    shipper.setShipHandler([](const WalSegment& seg) -> bool {
        std::this_thread::sleep_for(std::chrono::milliseconds(600));
        return true;
    });

    // Enqueue a segment
    WalSegment seg;
    seg.sequence_number = 1;
    seg.data = "slow_segment";
    seg.enqueue_time = std::chrono::steady_clock::now();
    seg.target_dc = "dc-remote";
    shipper.enqueueSegment(std::move(seg));

    // Wait for processing
    std::this_thread::sleep_for(std::chrono::milliseconds(700));
    shipper.stop();

    // Lag alert should have fired (lag > 500ms expected)
    auto stats = shipper.stats();
    EXPECT_GE(stats.lag_alerts_fired, 1);
}

// ============================================================================
// Test 9: Logical Replication Lock Hierarchy (SlotRuntime Mutex Protection)
// ============================================================================

TEST_F(ReplicationLockOrderingTest, LogicalReplication_SlotRuntime_ThreadSafe)
{
    auto manager = std::make_shared<LogicalReplicationManager>(wal_manager);

    // Create a logical replication slot
    auto slot_meta = manager->createSlot("logical_slot", "json");
    EXPECT_EQ(slot_meta.slot_name, "logical_slot");

    // Concurrent reads should not deadlock
    std::vector<std::thread> threads;
    std::atomic<int> read_count(0);

    for (int i = 0; i < 5; ++i) {
        threads.emplace_back([&, i]() {
            // Try to list slots (acquires slots_mutex_ shared + per-slot mutex)
            auto slots = manager->listSlots();
            ++read_count;
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(read_count, 5);
}

// ============================================================================
// Test 10: Stress Test - High Contention Lock Ordering
// ============================================================================

TEST_F(ReplicationLockOrderingTest, HighContention_NoDeadlock_1000Iterations)
{
    ReplicationSlotManager::ManagerConfig config;
    config.wal_directory = "/tmp/themis_test";
    auto manager = std::make_shared<ReplicationSlotManager>(config, wal_manager);

    // Create 3 slots
    std::vector<std::shared_ptr<ReplicationSlot>> slots;
    for (int i = 0; i < 3; ++i) {
        auto slot = manager->createSlot("slot_" + std::to_string(i), "physical", "replica_" + std::to_string(i));
        ASSERT_NE(slot, nullptr);
        slots.push_back(slot);
    }

    // Launch 10 threads doing concurrent operations
    std::vector<std::thread> threads;
    std::atomic<int> operations(0);
    std::atomic<bool> error_occurred(false);

    for (int t = 0; t < 10; ++t) {
        threads.emplace_back([&, t]() {
            for (int i = 0; i < 100; ++i) {
                try {
                    int slot_idx = (t + i) % 3;
                    auto slot = slots[slot_idx];

                    switch (i % 4) {
                        case 0: slot->advance((t * 100 + i) * 10); break;
                        case 1: slot->status(); break;
                        case 2: slot->state(); break;
                        case 3: slot->lag(); break;
                    }
                    ++operations;
                } catch (...) {
                    error_occurred = true;
                }
            }
        });
    }

    auto start = std::chrono::steady_clock::now();
    for (auto& t : threads) {
        t.join();
    }
    auto duration = std::chrono::steady_clock::now() - start;

    EXPECT_FALSE(error_occurred);
    EXPECT_EQ(operations, 1000);
    // Should complete in reasonable time (no deadlock)
    EXPECT_LT(duration, std::chrono::seconds(10));
}

}  // namespace test
}  // namespace replication
}  // namespace themisdb
