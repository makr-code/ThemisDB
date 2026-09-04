// Unit tests for the Learned Index Structures (ML-based B-tree replacement)
// Index module — Phase 3, Issue #1990
//
// Covers:
//   - LearnedIndex training on sorted numeric arrays
//   - Point lookup correctness (int64, uint64, double, float)
//   - Range query correctness
//   - Serialisation / deserialisation round-trip
//   - Security: malformed / overflowed buffer rejection
//   - Edge cases: tiny arrays, duplicate keys, uniform distributions
//   - Stale / untrained guard paths

#include "index/learned_index.h"

#include <gtest/gtest.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <numeric>
#include <random>
#include <set>
#include <vector>

using namespace themis::index;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

template <typename T>
static std::vector<T> makeSorted(size_t n, T start = T{0}, T step = T{1}) {
    std::vector<T> v(n);
    T val = start;
    for (auto& x : v) { x = val; val += step; }
    return v;
}

static std::vector<int64_t> randomSortedI64(size_t n, unsigned seed = 42) {
    std::mt19937_64 rng(seed);
    std::uniform_int_distribution<int64_t> dist(0, static_cast<int64_t>(n) * 10);
    std::set<int64_t> s = {};

    while (s.size() < n) {
      s.insert(dist(rng));
    }
    return {s.begin(), s.end()};
}

// ---------------------------------------------------------------------------
// Basic construction
// ---------------------------------------------------------------------------

TEST(LearnedIndexI64, DefaultConstruction) {
    LearnedIndexI64 idx;
    EXPECT_FALSE(idx.isTrained());
    EXPECT_FALSE(idx.isStale());
    EXPECT_EQ(idx.numKeys(), 0u);
}

TEST(LearnedIndexI64, ConstructWithConfig) {
    LearnedIndexConfig cfg(128, 50, 0.05);
    LearnedIndexI64 idx(cfg);
    EXPECT_FALSE(idx.isTrained());
}

// ---------------------------------------------------------------------------
// Training
// ---------------------------------------------------------------------------

TEST(LearnedIndexI64, TrainSuccess) {
    auto keys = makeSorted<int64_t>(1000, 0LL, 2LL);
    LearnedIndexI64 idx;
    auto r = idx.train(keys);
    EXPECT_TRUE(r.ok) << r.message;
    EXPECT_TRUE(idx.isTrained());
    EXPECT_EQ(r.num_keys, 1000u);
    EXPECT_GT(r.num_experts, 0u);
    EXPECT_GE(r.max_error, 0);
}

TEST(LearnedIndexI64, TrainTooFewKeys) {
    std::vector<int64_t> keys = {42};
    LearnedIndexConfig cfg;
    cfg.min_train_size = 2;
    LearnedIndexI64 idx(cfg);
    auto r = idx.train(keys);
    EXPECT_FALSE(r.ok);
    EXPECT_FALSE(idx.isTrained());
}

TEST(LearnedIndexI64, TrainExactMinimum) {
    std::vector<int64_t> keys = {10, 20};
    LearnedIndexI64 idx;
    auto r = idx.train(keys);
    EXPECT_TRUE(r.ok) << r.message;
    EXPECT_TRUE(idx.isTrained());
}

TEST(LearnedIndexI64, TrainDuplicateKeys) {
    std::vector<int64_t> keys = {1, 1, 2, 2, 3, 3, 4, 4, 5, 5};
    LearnedIndexI64 idx;
    auto r = idx.train(keys);
    EXPECT_TRUE(r.ok) << r.message;
    EXPECT_TRUE(idx.isTrained());
}

// ---------------------------------------------------------------------------
// Point lookup — all keys must be found
// ---------------------------------------------------------------------------

TEST(LearnedIndexI64, LookupAllKeys_Sequential) {
    const size_t N = 500;
    auto keys = makeSorted<int64_t>(N, 0LL, 3LL);
    LearnedIndexI64 idx;
    ASSERT_TRUE(idx.train(keys).ok);

    size_t found = 0;
    for (size_t i = 0; i < N; ++i) {
        auto pos = idx.lookupKey(keys[i], keys);
        if (pos.has_value()) {
            EXPECT_EQ(keys[*pos], keys[i]);
            ++found;
        }
    }
    EXPECT_EQ(found, N) << "Not all keys were found";
}

TEST(LearnedIndexI64, LookupAllKeys_Random) {
    const size_t N = 400;
    auto keys = randomSortedI64(N);
    LearnedIndexI64 idx;
    ASSERT_TRUE(idx.train(keys).ok);

    size_t found = 0;
    for (const auto& k : keys) {
        auto pos = idx.lookupKey(k, keys);
        if (pos.has_value()) {
            EXPECT_EQ(keys[*pos], k);
            ++found;
        }
    }
    EXPECT_EQ(found, N) << "Not all keys were found (random dist)";
}

TEST(LearnedIndexI64, LookupAbsentKey) {
    auto keys = makeSorted<int64_t>(100, 0LL, 2LL); // 0,2,4,...198
    LearnedIndexI64 idx;
    ASSERT_TRUE(idx.train(keys).ok);

    // Odd numbers are not in the set
    for (int64_t k : {1LL, 3LL, 7LL, 99LL, 197LL}) {
        EXPECT_FALSE(idx.lookupKey(k, keys).has_value())
            << "Key " << k << " should be absent";
    }
}

// ---------------------------------------------------------------------------
// Point lookup — other key types
// ---------------------------------------------------------------------------

TEST(LearnedIndexU64, LookupAllKeys) {
    const size_t N = 300;
    auto keys = makeSorted<uint64_t>(N, 0ULL, 5ULL);
    LearnedIndex<uint64_t> idx;
    ASSERT_TRUE(idx.train(keys).ok);

    for (size_t i = 0; i < N; ++i) {
        auto pos = idx.lookupKey(keys[i], keys);
        ASSERT_TRUE(pos.has_value()) << "Missing key at index " << i;
        EXPECT_EQ(keys[*pos], keys[i]);
    }
}

TEST(LearnedIndexF64, LookupAllKeys) {
    const size_t N = 200;
    std::vector<double> keys(N);
    for (size_t i = 0; i < N; ++i) {
      keys[i] = static_cast<double>(i) * 0.5;
    }

    LearnedIndex<double> idx;
    ASSERT_TRUE(idx.train(keys).ok);

    for (size_t i = 0; i < N; ++i) {
        auto pos = idx.lookupKey(keys[i], keys);
        ASSERT_TRUE(pos.has_value()) << "Missing double key " << keys[i];
        EXPECT_EQ(keys[*pos], keys[i]);
    }
}

TEST(LearnedIndexF32, LookupAllKeys) {
    const size_t N = 200;
    std::vector<float> keys(N);
    for (size_t i = 0; i < N; ++i) {
      keys[i] = static_cast<float>(i) * 0.25f;
    }

    LearnedIndex<float> idx;
    ASSERT_TRUE(idx.train(keys).ok);

    for (size_t i = 0; i < N; ++i) {
        auto pos = idx.lookupKey(keys[i], keys);
        ASSERT_TRUE(pos.has_value()) << "Missing float key " << keys[i];
        EXPECT_EQ(keys[*pos], keys[i]);
    }
}

// ---------------------------------------------------------------------------
// Range queries
// ---------------------------------------------------------------------------

TEST(LearnedIndexI64, RangeQuery_FullRange) {
    auto keys = makeSorted<int64_t>(100, 0LL, 1LL); // 0..99
    LearnedIndexI64 idx;
    ASSERT_TRUE(idx.train(keys).ok);

    auto [lo, hi] = idx.rangePositions(0LL, 99LL, keys);
    EXPECT_EQ(lo, 0u);
    EXPECT_EQ(hi, 100u);
}

TEST(LearnedIndexI64, RangeQuery_SubRange) {
    auto keys = makeSorted<int64_t>(100, 0LL, 1LL); // 0..99
    LearnedIndexI64 idx;
    ASSERT_TRUE(idx.train(keys).ok);

    auto [lo, hi] = idx.rangePositions(10LL, 19LL, keys);
    // positions 10..19, upper_bound of 19 is 20
    EXPECT_EQ(lo, 10u);
    EXPECT_EQ(hi, 20u);
    for (size_t i = lo; i < hi; ++i) {
        EXPECT_GE(keys[i], 10LL);
        EXPECT_LE(keys[i], 19LL);
    }
}

TEST(LearnedIndexI64, RangeQuery_EmptyRange) {
    auto keys = makeSorted<int64_t>(100, 0LL, 2LL); // 0,2,4,...198
    LearnedIndexI64 idx;
    ASSERT_TRUE(idx.train(keys).ok);

    // Range [1,1] has no even numbers
    auto [lo, hi] = idx.rangePositions(1LL, 1LL, keys);
    EXPECT_EQ(lo, hi); // empty
}

TEST(LearnedIndexI64, RangeQuery_Untrained_FallsBackToBinarySearch) {
    auto keys = makeSorted<int64_t>(50, 0LL, 1LL);
    LearnedIndexI64 idx; // not trained

    auto [lo, hi] = idx.rangePositions(10LL, 19LL, keys);
    EXPECT_EQ(lo, 10u);
    EXPECT_EQ(hi, 20u);
}

// ---------------------------------------------------------------------------
// raw lookup() sanity: returned position must be within bounds
// ---------------------------------------------------------------------------

TEST(LearnedIndexI64, RawLookupBounds) {
    auto keys = randomSortedI64(1000);
    LearnedIndexI64 idx;
    ASSERT_TRUE(idx.train(keys).ok);

    for (const auto& k : keys) {
        int64_t pos = idx.lookup(k);
        EXPECT_GE(pos, 0);
        EXPECT_LT(static_cast<size_t>(pos), keys.size());
    }
}

// ---------------------------------------------------------------------------
// Statistics
// ---------------------------------------------------------------------------

TEST(LearnedIndexI64, Stats) {
    auto keys = makeSorted<int64_t>(500, 1LL, 1LL);
    LearnedIndexConfig cfg(32);
    LearnedIndexI64 idx(cfg);
    auto r = idx.train(keys);
    ASSERT_TRUE(r.ok);

    auto s = idx.stats();
    EXPECT_EQ(s.num_keys, 500u);
    EXPECT_EQ(s.num_experts, idx.numExperts());
    EXPECT_GE(s.max_error, 0);
    EXPECT_GE(s.mean_error, 0.0);
    EXPECT_TRUE(s.trained);
    EXPECT_FALSE(s.stale);
}

TEST(LearnedIndexI64, MarkStale) {
    auto keys = makeSorted<int64_t>(100);
    LearnedIndexI64 idx;
    ASSERT_TRUE(idx.train(keys).ok);
    EXPECT_FALSE(idx.isStale());
    idx.markStale();
    EXPECT_TRUE(idx.isStale());
}

// ---------------------------------------------------------------------------
// Serialisation round-trip
// ---------------------------------------------------------------------------

TEST(LearnedIndexI64, SerialiseDeserialise) {
    auto keys = randomSortedI64(300);
    LearnedIndexI64 original;
    ASSERT_TRUE(original.train(keys).ok);

    auto buf = original.serialize();
    EXPECT_FALSE(buf.empty());

    LearnedIndexI64 restored;
    ASSERT_TRUE(restored.deserialize(buf));

    EXPECT_EQ(restored.isTrained(),    original.isTrained());
    EXPECT_EQ(restored.numKeys(),      original.numKeys());
    EXPECT_EQ(restored.maxError(),     original.maxError());
    EXPECT_EQ(restored.numExperts(),   original.numExperts());

    // Lookup must be identical after round-trip
    for (size_t i = 0; i < std::min<size_t>(50, keys.size()); ++i) {
        EXPECT_EQ(restored.lookup(keys[i]), original.lookup(keys[i]))
            << "Mismatch at key index " << i;
    }
}

TEST(LearnedIndexI64, DeserialiseInvalidMagic) {
    std::vector<uint8_t> bad(64, 0xFFu);
    LearnedIndexI64 idx;
    EXPECT_FALSE(idx.deserialize(bad));
}

TEST(LearnedIndexI64, DeserialiseEmptyBuffer) {
    LearnedIndexI64 idx;
    EXPECT_FALSE(idx.deserialize({}));
}

TEST(LearnedIndexI64, DeserialiseTruncatedBuffer) {
    // Truncating a valid serialised buffer must be rejected
    auto keys = makeSorted<int64_t>(100, 0LL, 1LL);
    LearnedIndexI64 original;
    ASSERT_TRUE(original.train(keys).ok);
    auto buf = original.serialize();

    // Try every truncation length
    for (size_t len = 0; len < buf.size(); ++len) {
        std::vector<uint8_t> truncated(buf.begin(), buf.begin() + len);
        LearnedIndexI64 idx;
        EXPECT_FALSE(idx.deserialize(truncated))
            << "Should reject truncated buffer of length " << len;
    }
}

TEST(LearnedIndexI64, DeserialiseOverflowNe) {
    // Craft a buffer with a valid magic but ne value that would cause
    // ne * 16 + 2 to integer-overflow — must be rejected without crashing.
    auto keys = makeSorted<int64_t>(5, 0LL, 1LL);
    LearnedIndexI64 original;
    ASSERT_TRUE(original.train(keys).ok);
    auto buf = original.serialize();

    // Patch ne (bytes 4..11) to a value causing overflow: (SIZE_MAX/16)+2
    const uint64_t evil_ne =
        (static_cast<uint64_t>(std::numeric_limits<size_t>::max()) / 16u) + 2u;
    std::memcpy(buf.data() + 4, &evil_ne, 8);
    // Ensure the buffer is large enough so that the overflowed size check would
    // pass if the guard were absent.
    // Serialisation header: 4 (magic) + 8*4 (fields) + 16 (root) = 52 bytes.
    // Overflowed ne*16+2 = 34 bytes → attacker needs ≥ 52+34 = 86 bytes.
    if (buf.size() < 86u) {
      buf.resize(86u, 0u);
    }

    LearnedIndexI64 victim;
    EXPECT_FALSE(victim.deserialize(buf))
        << "Must reject buffer with ne causing size_t overflow";
}

// ---------------------------------------------------------------------------
// Untrained guard
// ---------------------------------------------------------------------------

TEST(LearnedIndexI64, LookupUntrainedReturnsMinusOne) {
    LearnedIndexI64 idx;
    EXPECT_EQ(idx.lookup(42LL), -1);
}

TEST(LearnedIndexI64, LookupKeyUntrainedReturnsNullopt) {
    std::vector<int64_t> keys = {1, 2, 3};
    LearnedIndexI64 idx;
    EXPECT_FALSE(idx.lookupKey(2LL, keys).has_value());
}

// ---------------------------------------------------------------------------
// Large-scale correctness spot-check
// ---------------------------------------------------------------------------

TEST(LearnedIndexI64, LargeScale_AllFound) {
    const size_t N = 5000;
    auto keys = randomSortedI64(N, 99);
    LearnedIndexConfig cfg(256);
    LearnedIndexI64 idx(cfg);
    auto r = idx.train(keys);
    ASSERT_TRUE(r.ok) << r.message;

    size_t found = 0;
    for (size_t i = 0; i < N; ++i) {
        auto pos = idx.lookupKey(keys[i], keys);
        if (pos.has_value() && keys[*pos] == keys[i]) {
          ++found;
        }
    }
    // All keys must be found
    EXPECT_EQ(found, N);
}
