/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_hkdf_cache.cpp                                ║
  Version:         0.0.15                                             ║
  Last Modified:   2026-02-21 17:07:48                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     55                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

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
