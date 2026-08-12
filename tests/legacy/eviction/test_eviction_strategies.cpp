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

TEST_F(TTLEvictionStrategyTest, SelectVictimPrefersExpiredEntry) {
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()
    ).count();

    // key1 was inserted well past the 100ms TTL (expired)
    strategy->onInsert("key1", now - 200);
    // key2 was inserted recently (not expired)
    strategy->onInsert("key2", now - 10);

    // selectVictim must return the expired entry, not the recent one
    auto victim = strategy->selectVictim();
    ASSERT_TRUE(victim.has_value());
    EXPECT_EQ(*victim, "key1");
}

TEST_F(TTLEvictionStrategyTest, SetDefaultTTL) {
    // After lowering TTL to 50ms, an entry inserted 100ms ago becomes expired.
    strategy->setDefaultTTL(50);

    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()
    ).count();

    strategy->onInsert("key1", now - 100);  // expired under 50ms TTL
    strategy->onInsert("key2", now - 10);   // not expired

    auto victim = strategy->selectVictim();
    ASSERT_TRUE(victim.has_value());
    EXPECT_EQ(*victim, "key1");
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

// ============================================================================
// ARC Strategy Tests
// ============================================================================

class ARCEvictionStrategyTest : public ::testing::Test {
protected:
    void SetUp() override {
        strategy = std::make_unique<ARCEvictionStrategy>(4);
    }

    std::unique_ptr<ARCEvictionStrategy> strategy;
};

TEST_F(ARCEvictionStrategyTest, BasicInsertionAndVictim) {
    strategy->onInsert("key1", 1000);
    strategy->onInsert("key2", 2000);
    strategy->onInsert("key3", 3000);

    EXPECT_EQ(strategy->size(), 3);

    // All in T1; victim should be LRU of T1 (key1)
    auto victim = strategy->selectVictim();
    ASSERT_TRUE(victim.has_value());
    EXPECT_EQ(*victim, "key1");
}

TEST_F(ARCEvictionStrategyTest, AccessPromotesT1ToT2) {
    strategy->onInsert("key1", 1000);
    strategy->onInsert("key2", 2000);
    strategy->onInsert("key3", 3000);

    // Access key1 → promoted from T1 to T2
    strategy->onAccess("key1");

    EXPECT_EQ(strategy->size(), 3);

    // key2 is now the LRU of T1 (key1 was moved to T2)
    auto victim = strategy->selectVictim();
    ASSERT_TRUE(victim.has_value());
    EXPECT_EQ(*victim, "key2");
}

TEST_F(ARCEvictionStrategyTest, RemoveMovesToGhost) {
    strategy->onInsert("key1", 1000);
    strategy->onInsert("key2", 2000);

    EXPECT_EQ(strategy->size(), 2);

    strategy->onRemove("key1");

    EXPECT_EQ(strategy->size(), 1);

    // After removal from T1, key1 is a ghost in B1
    // Re-inserting key1 should trigger p adaptation (B1 ghost hit)
    strategy->onInsert("key1", 3000);
    EXPECT_EQ(strategy->size(), 2);
}

TEST_F(ARCEvictionStrategyTest, Clear) {
    strategy->onInsert("key1", 1000);
    strategy->onInsert("key2", 2000);

    EXPECT_EQ(strategy->size(), 2);

    strategy->clear();

    EXPECT_EQ(strategy->size(), 0);
    EXPECT_FALSE(strategy->selectVictim().has_value());
}

TEST_F(ARCEvictionStrategyTest, Name) {
    EXPECT_EQ(strategy->getName(), "ARC");
}

TEST_F(ARCEvictionStrategyTest, CapacityEviction) {
    // Insert 4 entries (capacity is 4)
    strategy->onInsert("k1", 1000);
    strategy->onInsert("k2", 2000);
    strategy->onInsert("k3", 3000);
    strategy->onInsert("k4", 4000);
    EXPECT_EQ(strategy->size(), 4);

    // Inserting a 5th should not crash; victim selection works
    auto victim = strategy->selectVictim();
    ASSERT_TRUE(victim.has_value());
    strategy->onRemove(*victim);
    strategy->onInsert("k5", 5000);
    EXPECT_EQ(strategy->size(), 4);
}

TEST_F(ARCEvictionStrategyTest, EmptyReturnsNullopt) {
    EXPECT_FALSE(strategy->selectVictim().has_value());
}
