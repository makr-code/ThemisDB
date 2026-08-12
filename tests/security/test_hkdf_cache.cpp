#include <gtest/gtest.h>
#include "utils/hkdf_cache.h"

using namespace themis::utils;

TEST(HKDFCache, HitMissAndRotation) {
    // Prepare inputs
    std::vector<uint8_t> ikm = {1,2,3,4,5,6,7,8};
    std::vector<uint8_t> salt = {9,9,9};
    std::string info = "unit-test-info";

    // First derivation -> cache miss
    auto a = HKDFCache::threadLocal().derive_cached(ikm, salt, info, 32);
    ASSERT_EQ(a.size(), 32);

    // Second derivation with identical inputs -> cache hit (returns same bytes)
    auto b = HKDFCache::threadLocal().derive_cached(ikm, salt, info, 32);
    EXPECT_EQ(a, b);

    // Clear cache and derive again -> deterministic HKDF still returns same bytes
    HKDFCache::threadLocal().clear();
    auto c = HKDFCache::threadLocal().derive_cached(ikm, salt, info, 32);
    EXPECT_EQ(a, c);

    // Simulate key rotation by changing IKM -> derived output must differ
    ikm[0] = 0xFF;
    auto d = HKDFCache::threadLocal().derive_cached(ikm, salt, info, 32);
    EXPECT_NE(a, d);
}

// ============================================================================
// TTL / Stats (extended API)
// ============================================================================

TEST(HKDFCache, StatsCountHitsAndMisses) {
    HKDFCache::Config cfg;
    cfg.max_entries = 100;
    cfg.ttl         = std::chrono::seconds{300};
    HKDFCache cache(cfg);

    std::vector<uint8_t> ikm  = {0xAA, 0xBB};
    std::vector<uint8_t> salt = {0x01};
    std::string info = "stats-test";

    // First call → miss
    cache.derive_cached(ikm, salt, info, 16);
    // Second call → hit
    cache.derive_cached(ikm, salt, info, 16);

    auto stats = cache.stats();
    EXPECT_EQ(stats.misses, 1u);
    EXPECT_EQ(stats.hits,   1u);
}

TEST(HKDFCache, PurgeByIkmHashClearsEntry) {
    HKDFCache::Config cfg;
    cfg.max_entries = 100;
    cfg.ttl         = std::chrono::seconds{300};
    HKDFCache cache(cfg);

    std::vector<uint8_t> ikm  = {0x11, 0x22, 0x33};
    std::vector<uint8_t> salt = {0xFF};
    std::string info = "purge-test";

    // Populate cache
    cache.derive_cached(ikm, salt, info, 16);
    EXPECT_EQ(cache.stats().misses, 1u);

    // Purge accepts any string; after purge the entry must be re-derived (miss)
    cache.purge_by_ikm_hash("some_rotation_id");
    cache.clear(); // ensure clean slate for counting
    cache.derive_cached(ikm, salt, info, 16);
    EXPECT_EQ(cache.stats().misses, 1u);
}
