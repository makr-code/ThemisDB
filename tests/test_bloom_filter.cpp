/*
 * ThemisDB | File: test_bloom_filter.cpp | Version: 0.0.13
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#include <gtest/gtest.h>
#include "utils/bloom_filter.h"

using namespace themis::utils;

// ============================================================================
// Construction
// ============================================================================

TEST(BloomFilter, ConstructionDoesNotThrow) {
    EXPECT_NO_THROW(BloomFilter(1000, 0.01));
}

TEST(BloomFilter, BitsetSizePositive) {
    BloomFilter bf(1000, 0.01);
    EXPECT_GT(bf.bitset_size(), 0u);
}

TEST(BloomFilter, FalsePositiveRateStored) {
    BloomFilter bf(1000, 0.01);
    EXPECT_DOUBLE_EQ(bf.false_positive_rate(), 0.01);
}

// ============================================================================
// Membership
// ============================================================================

TEST(BloomFilter, InsertedKeyIsFound) {
    BloomFilter bf(100, 0.01);
    bf.insert("hello");
    EXPECT_TRUE(bf.contains("hello"));
}

TEST(BloomFilter, UninsertedKeyIsNotFound) {
    BloomFilter bf(100, 0.01);
    bf.insert("hello");
    // "world" was never inserted; must return false (no false negative).
    EXPECT_FALSE(bf.contains("world"));
}

TEST(BloomFilter, MultipleInserts) {
    BloomFilter bf(1000, 0.01);
    for (int i = 0; i < 500; ++i) {
        bf.insert(std::to_string(i));
    }
    for (int i = 0; i < 500; ++i) {
        EXPECT_TRUE(bf.contains(std::to_string(i)));
    }
}

TEST(BloomFilter, SizeTracksInsertions) {
    BloomFilter bf(100, 0.01);
    EXPECT_EQ(bf.size(), 0u);
    bf.insert("a");
    EXPECT_EQ(bf.size(), 1u);
    bf.insert("b");
    EXPECT_EQ(bf.size(), 2u);
}

// ============================================================================
// Clear
// ============================================================================

TEST(BloomFilter, ClearResetsFilter) {
    BloomFilter bf(100, 0.01);
    bf.insert("hello");
    bf.clear();
    EXPECT_EQ(bf.size(), 0u);
    EXPECT_FALSE(bf.contains("hello"));
}
