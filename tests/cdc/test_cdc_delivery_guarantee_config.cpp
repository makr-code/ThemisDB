/**
 * Test: CDC Delivery Guarantee Configuration
 * Tests for IDeliveryGuaranteeConfig / InMemoryDeliveryGuaranteeConfig:
 *   - Default mode is AtLeastOnce
 *   - setMode / mode round-trip
 *   - Default ack timeout is 30s
 *   - setAckTimeout / ackTimeout round-trip
 *   - Default deduplication window is 5min
 *   - setDeduplicationWindow / deduplicationWindow round-trip
 *   - Mode switch from ExactlyOnce back to AtLeastOnce
 *   - Thread-safety: concurrent mode / timeout updates
 *
 * Tests for IIdempotentCDCListener / InMemoryIdempotentListener:
 *   - isDuplicate returns false for unprocessed (collection, sequence)
 *   - markProcessed + isDuplicate returns true
 *   - markProcessed on same (collection, sequence) twice is idempotent
 *   - processedCount increments per unique entry
 *   - FIFO eviction when max_window_size is reached
 *   - Multiple collections are independent
 *   - Thread-safety: concurrent markProcessed / isDuplicate
 *
 * Copyright (c) 2025 ThemisDB Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include "cdc/idelivery_guarantee_config.h"

#include <atomic>
#include <chrono>
#include <memory>
#include <string>
#include <thread>
#include <vector>

using namespace themis::cdc;
using namespace std::chrono_literals;

// ─────────────────────────────────────────────────────────────────────────────
// InMemoryDeliveryGuaranteeConfig — defaults
// ─────────────────────────────────────────────────────────────────────────────

TEST(DeliveryGuaranteeConfigTest, DefaultModeIsAtLeastOnce) {
    InMemoryDeliveryGuaranteeConfig cfg;
    EXPECT_EQ(cfg.mode(), DeliveryMode::AtLeastOnce);
}

TEST(DeliveryGuaranteeConfigTest, DefaultAckTimeout) {
    InMemoryDeliveryGuaranteeConfig cfg;
    EXPECT_EQ(cfg.ackTimeout(), std::chrono::seconds(30));
}

TEST(DeliveryGuaranteeConfigTest, DefaultDeduplicationWindow) {
    InMemoryDeliveryGuaranteeConfig cfg;
    EXPECT_EQ(cfg.deduplicationWindow(), std::chrono::minutes(5));
}

// ─────────────────────────────────────────────────────────────────────────────
// setMode
// ─────────────────────────────────────────────────────────────────────────────

TEST(DeliveryGuaranteeConfigTest, SetModeExactlyOnce) {
    InMemoryDeliveryGuaranteeConfig cfg;
    cfg.setMode(DeliveryMode::ExactlyOnce);
    EXPECT_EQ(cfg.mode(), DeliveryMode::ExactlyOnce);
}

TEST(DeliveryGuaranteeConfigTest, SwitchModeBackToAtLeastOnce) {
    InMemoryDeliveryGuaranteeConfig cfg;
    cfg.setMode(DeliveryMode::ExactlyOnce);
    cfg.setMode(DeliveryMode::AtLeastOnce);
    EXPECT_EQ(cfg.mode(), DeliveryMode::AtLeastOnce);
}

// ─────────────────────────────────────────────────────────────────────────────
// setAckTimeout
// ─────────────────────────────────────────────────────────────────────────────

TEST(DeliveryGuaranteeConfigTest, SetAckTimeout) {
    InMemoryDeliveryGuaranteeConfig cfg;
    cfg.setAckTimeout(10s);
    EXPECT_EQ(cfg.ackTimeout(), 10s);
}

TEST(DeliveryGuaranteeConfigTest, SetAckTimeoutZero) {
    InMemoryDeliveryGuaranteeConfig cfg;
    cfg.setAckTimeout(std::chrono::milliseconds(0));
    EXPECT_EQ(cfg.ackTimeout(), std::chrono::milliseconds(0));
}

// ─────────────────────────────────────────────────────────────────────────────
// setDeduplicationWindow
// ─────────────────────────────────────────────────────────────────────────────

TEST(DeliveryGuaranteeConfigTest, SetDeduplicationWindow) {
    InMemoryDeliveryGuaranteeConfig cfg;
    cfg.setDeduplicationWindow(std::chrono::minutes(10));
    EXPECT_EQ(cfg.deduplicationWindow(), std::chrono::minutes(10));
}

// ─────────────────────────────────────────────────────────────────────────────
// Constructor overrides
// ─────────────────────────────────────────────────────────────────────────────

TEST(DeliveryGuaranteeConfigTest, ConstructorOverridesDefaults) {
    InMemoryDeliveryGuaranteeConfig cfg(
        DeliveryMode::ExactlyOnce,
        std::chrono::seconds(5),
        std::chrono::minutes(1));
    EXPECT_EQ(cfg.mode(),                 DeliveryMode::ExactlyOnce);
    EXPECT_EQ(cfg.ackTimeout(),           std::chrono::seconds(5));
    EXPECT_EQ(cfg.deduplicationWindow(),  std::chrono::minutes(1));
}

// ─────────────────────────────────────────────────────────────────────────────
// Thread-safety
// ─────────────────────────────────────────────────────────────────────────────

TEST(DeliveryGuaranteeConfigTest, ConcurrentModeUpdates) {
    InMemoryDeliveryGuaranteeConfig cfg;
    constexpr int kIter    = 500;
    constexpr int kThreads = 4;

    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&, t] {
            for (int i = 0; i < kIter; ++i) {
                cfg.setMode((t + i) % 2 == 0
                    ? DeliveryMode::AtLeastOnce
                    : DeliveryMode::ExactlyOnce);
                (void)cfg.mode();
                (void)cfg.ackTimeout();
            }
        });
    }
    for (auto& th : threads) {
      th.join();
    }
    // Must be one of the two valid modes
    const auto m = cfg.mode();
    EXPECT_TRUE(m == DeliveryMode::AtLeastOnce || m == DeliveryMode::ExactlyOnce);
}

// ─────────────────────────────────────────────────────────────────────────────
// Polymorphic usage
// ─────────────────────────────────────────────────────────────────────────────

TEST(DeliveryGuaranteeConfigTest, PolymorphicUsage) {
    std::unique_ptr<IDeliveryGuaranteeConfig> cfg =
        std::make_unique<InMemoryDeliveryGuaranteeConfig>();

    EXPECT_EQ(cfg->mode(), DeliveryMode::AtLeastOnce);
    cfg->setMode(DeliveryMode::ExactlyOnce);
    EXPECT_EQ(cfg->mode(), DeliveryMode::ExactlyOnce);
    cfg->setAckTimeout(std::chrono::seconds(60));
    EXPECT_EQ(cfg->ackTimeout(), std::chrono::seconds(60));
}

// ═════════════════════════════════════════════════════════════════════════════
// InMemoryIdempotentListener
// ═════════════════════════════════════════════════════════════════════════════

TEST(InMemoryIdempotentListenerTest, NewSequenceIsNotDuplicate) {
    InMemoryIdempotentListener listener;
    EXPECT_FALSE(listener.isDuplicate("orders", 1));
}

TEST(InMemoryIdempotentListenerTest, MarkProcessedMakesDuplicate) {
    InMemoryIdempotentListener listener;
    listener.markProcessed("orders", 1);
    EXPECT_TRUE(listener.isDuplicate("orders", 1));
}

TEST(InMemoryIdempotentListenerTest, MarkProcessedIdempotent) {
    InMemoryIdempotentListener listener;
    listener.markProcessed("orders", 1);
    listener.markProcessed("orders", 1); // no-op
    EXPECT_EQ(listener.processedCount(), 1u);
}

TEST(InMemoryIdempotentListenerTest, ProcessedCountIncrements) {
    InMemoryIdempotentListener listener;
    for (uint64_t i = 1; i <= 5; ++i) {
        listener.markProcessed("orders", i);
    }
    EXPECT_EQ(listener.processedCount(), 5u);
}

TEST(InMemoryIdempotentListenerTest, MultipleCollectionsAreIndependent) {
    InMemoryIdempotentListener listener;
    listener.markProcessed("orders", 1);
    EXPECT_TRUE(listener.isDuplicate("orders",    1));
    EXPECT_FALSE(listener.isDuplicate("inventory", 1)); // different collection
}

TEST(InMemoryIdempotentListenerTest, FifoEvictionOnWindowFull) {
    // Window size = 3; adding a 4th entry evicts the first
    InMemoryIdempotentListener listener(3);
    listener.markProcessed("orders", 1);
    listener.markProcessed("orders", 2);
    listener.markProcessed("orders", 3);
    EXPECT_EQ(listener.processedCount(), 3u);

    listener.markProcessed("orders", 4); // evicts seq=1
    EXPECT_EQ(listener.processedCount(), 3u);

    EXPECT_FALSE(listener.isDuplicate("orders", 1)); // evicted
    EXPECT_TRUE(listener.isDuplicate("orders",  2));
    EXPECT_TRUE(listener.isDuplicate("orders",  3));
    EXPECT_TRUE(listener.isDuplicate("orders",  4));
}

TEST(InMemoryIdempotentListenerTest, ConcurrentMarkAndCheck) {
    InMemoryIdempotentListener listener;
    constexpr uint64_t kSeqs    = 100;
    constexpr int      kThreads = 4;

    // Multiple threads mark the same sequences
    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&] {
            for (uint64_t s = 1; s <= kSeqs; ++s) {
                listener.markProcessed("col", s);
                (void)listener.isDuplicate("col", s);
            }
        });
    }
    for (auto& th : threads) {
      th.join();
    }

    // Every sequence must be marked as duplicate now
    for (uint64_t s = 1; s <= kSeqs; ++s) {
        EXPECT_TRUE(listener.isDuplicate("col", s));
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Polymorphic: IIdempotentCDCListener
// ─────────────────────────────────────────────────────────────────────────────

TEST(InMemoryIdempotentListenerTest, PolymorphicUsage) {
    std::unique_ptr<IIdempotentCDCListener> listener =
        std::make_unique<InMemoryIdempotentListener>();

    EXPECT_FALSE(listener->isDuplicate("col", 42));
    listener->markProcessed("col", 42);
    EXPECT_TRUE(listener->isDuplicate("col", 42));
}
