/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_eviction_strategies.cpp                       ║
  Version:         0.0.14                                             ║
  Last Modified:   2026-02-21 16:53:09                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     285                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include <gtest/gtest.h>
#include <core/concerns/eviction_strategies.h>
#include <string>
#include <thread>
#include <chrono>

using namespace themis::core::concerns;

// ============================================================================
// LRU Strategy Tests
// ============================================================================

class LRUEvictionStrategyTest : public ::testing::Test {
protected:
    void SetUp() override {
        strategy = std::make_unique<LRUEvictionStrategy>();
    }

    std::unique_ptr<LRUEvictionStrategy> strategy;
};

TEST_F(LRUEvictionStrategyTest, BasicInsertionAndVictim) {
    strategy->onInsert("key1", 1000);
    strategy->onInsert("key2", 2000);
    strategy->onInsert("key3", 3000);

    EXPECT_EQ(strategy->size(), 3);
    
    // Least recently used should be key1
    auto victim = strategy->selectVictim();
    ASSERT_TRUE(victim.has_value());
    EXPECT_EQ(*victim, "key1");
}

TEST_F(LRUEvictionStrategyTest, AccessUpdatesRecency) {
    strategy->onInsert("key1", 1000);
    strategy->onInsert("key2", 2000);
    strategy->onInsert("key3", 3000);

    // Access key1, making it most recent
    strategy->onAccess("key1");

    // Now key2 should be least recent
    auto victim = strategy->selectVictim();
    ASSERT_TRUE(victim.has_value());
    EXPECT_EQ(*victim, "key2");
}

TEST_F(LRUEvictionStrategyTest, RemoveEntry) {
    strategy->onInsert("key1", 1000);
    strategy->onInsert("key2", 2000);
    
    EXPECT_EQ(strategy->size(), 2);
    
    strategy->onRemove("key1");
    
    EXPECT_EQ(strategy->size(), 1);
    
    auto victim = strategy->selectVictim();
    ASSERT_TRUE(victim.has_value());
    EXPECT_EQ(*victim, "key2");
}

TEST_F(LRUEvictionStrategyTest, Clear) {
    strategy->onInsert("key1", 1000);
    strategy->onInsert("key2", 2000);
    
    EXPECT_EQ(strategy->size(), 2);
    
    strategy->clear();
    
    EXPECT_EQ(strategy->size(), 0);
    EXPECT_FALSE(strategy->selectVictim().has_value());
}

TEST_F(LRUEvictionStrategyTest, Name) {
    EXPECT_EQ(strategy->getName(), "LRU");
}

// ============================================================================
// LFU Strategy Tests
// ============================================================================

class LFUEvictionStrategyTest : public ::testing::Test {
protected:
    void SetUp() override {
        strategy = std::make_unique<LFUEvictionStrategy>();
    }

    std::unique_ptr<LFUEvictionStrategy> strategy;
};

TEST_F(LFUEvictionStrategyTest, BasicInsertionAndVictim) {
    strategy->onInsert("key1", 1000);
    strategy->onInsert("key2", 2000);
    strategy->onInsert("key3", 3000);

    EXPECT_EQ(strategy->size(), 3);
    
    // All have frequency 1, should evict oldest (key1)
    auto victim = strategy->selectVictim();
    ASSERT_TRUE(victim.has_value());
    EXPECT_EQ(*victim, "key1");
}

TEST_F(LFUEvictionStrategyTest, FrequencyDeterminesVictim) {
    strategy->onInsert("key1", 1000);
    strategy->onInsert("key2", 2000);
    strategy->onInsert("key3", 3000);

    // Access key1 and key3 multiple times
    strategy->onAccess("key1");
    strategy->onAccess("key1");
    strategy->onAccess("key3");

    // key2 has lowest frequency (1), should be evicted
    auto victim = strategy->selectVictim();
    ASSERT_TRUE(victim.has_value());
    EXPECT_EQ(*victim, "key2");
}

TEST_F(LFUEvictionStrategyTest, TieBreakByAge) {
    strategy->onInsert("key1", 1000);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    strategy->onInsert("key2", 2000);

    // Both have frequency 1, key1 is older
    auto victim = strategy->selectVictim();
    ASSERT_TRUE(victim.has_value());
    EXPECT_EQ(*victim, "key1");
}

TEST_F(LFUEvictionStrategyTest, RemoveEntry) {
    strategy->onInsert("key1", 1000);
    strategy->onInsert("key2", 2000);
    
    EXPECT_EQ(strategy->size(), 2);
    
    strategy->onRemove("key1");
    
    EXPECT_EQ(strategy->size(), 1);
}

TEST_F(LFUEvictionStrategyTest, Name) {
    EXPECT_EQ(strategy->getName(), "LFU");
}

// ============================================================================
// TTL Strategy Tests
// ============================================================================

class TTLEvictionStrategyTest : public ::testing::Test {
protected:
    void SetUp() override {
        strategy = std::make_unique<TTLEvictionStrategy>(100);  // 100ms TTL
    }

    std::unique_ptr<TTLEvictionStrategy> strategy;
};

TEST_F(TTLEvictionStrategyTest, BasicInsertion) {
    strategy->onInsert("key1", 1000);
    strategy->onInsert("key2", 2000);

    EXPECT_EQ(strategy->size(), 2);
}

TEST_F(TTLEvictionStrategyTest, EvictsOldestWhenNoExpired) {
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()
    ).count();
    
    strategy->onInsert("key1", now - 50);  // Not expired
    strategy->onInsert("key2", now - 30);  // Not expired
    strategy->onInsert("key3", now - 10);  // Not expired

    // All recent, should evict oldest (key1)
    auto victim = strategy->selectVictim();
    ASSERT_TRUE(victim.has_value());
    EXPECT_EQ(*victim, "key1");
}

TEST_F(TTLEvictionStrategyTest, Name) {
    EXPECT_EQ(strategy->getName(), "TTL");
}

TEST_F(TTLEvictionStrategyTest, SetDefaultTTL) {
    strategy->setDefaultTTL(500);
    // No direct way to test this without implementation details
    // This just ensures the method doesn't crash
}

// ============================================================================
// TwoTier Strategy Tests
// ============================================================================

class TwoTierEvictionStrategyTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto l1 = std::make_unique<LRUEvictionStrategy>();
        auto l2 = std::make_unique<LFUEvictionStrategy>();
        
        strategy = std::make_unique<TwoTierEvictionStrategy>(
            std::move(l1),
            std::move(l2),
            2  // L1 capacity = 2
        );
    }

    std::unique_ptr<TwoTierEvictionStrategy> strategy;
};

TEST_F(TwoTierEvictionStrategyTest, BasicInsertion) {
    strategy->onInsert("key1", 1000);
    strategy->onInsert("key2", 2000);
    strategy->onInsert("key3", 3000);

    EXPECT_EQ(strategy->size(), 3);
}

TEST_F(TwoTierEvictionStrategyTest, EvictsFromL2First) {
    strategy->onInsert("key1", 1000);  // L1
    strategy->onInsert("key2", 2000);  // L1
    strategy->onInsert("key3", 3000);  // L2 (L1 full)
    strategy->onInsert("key4", 4000);  // L2

    // Should evict from L2 (slower tier) first
    auto victim = strategy->selectVictim();
    ASSERT_TRUE(victim.has_value());
    // Victim should be from L2
    EXPECT_TRUE(*victim == "key3" || *victim == "key4");
}

TEST_F(TwoTierEvictionStrategyTest, Name) {
    EXPECT_EQ(strategy->getName(), "TwoTier");
}

TEST_F(TwoTierEvictionStrategyTest, ClearBothTiers) {
    strategy->onInsert("key1", 1000);
    strategy->onInsert("key2", 2000);
    strategy->onInsert("key3", 3000);
    
    EXPECT_EQ(strategy->size(), 3);
    
    strategy->clear();
    
    EXPECT_EQ(strategy->size(), 0);
}

TEST_F(TwoTierEvictionStrategyTest, RemoveFromAnyTier) {
    strategy->onInsert("key1", 1000);  // L1
    strategy->onInsert("key2", 2000);  // L1
    strategy->onInsert("key3", 3000);  // L2
    
    strategy->onRemove("key1");
    strategy->onRemove("key3");
    
    EXPECT_EQ(strategy->size(), 1);
}
