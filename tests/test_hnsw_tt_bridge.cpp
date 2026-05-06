/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_hnsw_tt_bridge.cpp                            ║
  Version:         1.0.0                                              ║
  Last Modified:   2026-05-06                                         ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file test_hnsw_tt_bridge.cpp
 * @brief Unit tests for HnswTTBridge (Phase 1 + persistence).
 *
 * Acceptance criteria:
 *
 * HnswTTBridge basic operations — HTB-04..HTB-07
 *   HTB-04  add() + search() returns results
 *   HTB-05  save() on a populated bridge succeeds and creates a file
 *   HTB-06  load() after save() restores all vectors (size + searchability)
 *   HTB-07  load() with a non-existent path returns false
 */

#include <gtest/gtest.h>

#include "tensor/hnsw_tt_bridge.h"

#include <cstdio>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

using namespace themis::tensor;
using namespace themis::storage;

namespace {

std::vector<float> makeTestVec(size_t dim, float seed) {
    std::vector<float> v(dim);
    for (size_t i = 0; i < dim; ++i)
        v[i] = seed + static_cast<float>(i) * 0.05f;
    return v;
}

std::string htbTmpPath(const char* tag) {
    return std::string("/tmp/themis_htb_test_") + tag + ".bin";
}

} // namespace

// ============================================================================
// HTB-04 — add + search round-trip
// ============================================================================

TEST(HnswTTBridge, HTB04_AddAndSearch) {
    HnswTTBridge bridge({}, 0);

    auto v1 = makeTestVec(8, 0.0f);
    auto v2 = makeTestVec(8, 1.0f);
    auto v3 = makeTestVec(8, 2.0f);

    ASSERT_TRUE(bridge.addFlat(1, v1.data(), v1.size()));
    ASSERT_TRUE(bridge.addFlat(2, v2.data(), v2.size()));
    ASSERT_TRUE(bridge.addFlat(3, v3.data(), v3.size()));
    EXPECT_EQ(bridge.size(), 3u);

    auto query = makeTestVec(8, 0.05f);
    auto results = bridge.searchFlat(query.data(), query.size(), 2);
    EXPECT_FALSE(results.empty());
    EXPECT_LE(results.size(), 2u);
    // Nearest neighbor should be id 1 (closest seed)
    EXPECT_EQ(results.front().id, 1);
}

// ============================================================================
// HTB-05 — save() on populated bridge creates a valid file
// ============================================================================

TEST(HnswTTBridge, HTB05_SavePopulatedBridgeSucceeds) {
    const std::string path = htbTmpPath("save");
    std::remove(path.c_str());

    HnswTTBridge bridge({}, 0);
    auto v1 = makeTestVec(8, 1.0f);
    auto v2 = makeTestVec(8, 2.0f);
    ASSERT_TRUE(bridge.addFlat(1, v1.data(), v1.size()));
    ASSERT_TRUE(bridge.addFlat(2, v2.data(), v2.size()));

    EXPECT_TRUE(bridge.save(path));

    std::ifstream chk(path, std::ios::binary);
    EXPECT_TRUE(chk.good());
}

// ============================================================================
// HTB-06 — load() after save() restores all vectors and search works
// ============================================================================

TEST(HnswTTBridge, HTB06_LoadAfterSaveRestoresVectorsAndSearch) {
    const std::string path = htbTmpPath("roundtrip");
    std::remove(path.c_str());

    auto v1 = makeTestVec(8, 0.0f);
    auto v2 = makeTestVec(8, 5.0f);
    auto v3 = makeTestVec(8, 10.0f);

    {
        HnswTTBridge bridge({}, 0);
        ASSERT_TRUE(bridge.addFlat(10, v1.data(), v1.size()));
        ASSERT_TRUE(bridge.addFlat(20, v2.data(), v2.size()));
        ASSERT_TRUE(bridge.addFlat(30, v3.data(), v3.size()));
        ASSERT_EQ(bridge.size(), 3u);
        ASSERT_TRUE(bridge.save(path));
    }

    HnswTTBridge loaded({}, 0);
    EXPECT_TRUE(loaded.load(path));
    EXPECT_EQ(loaded.size(), 3u);

    // Search should return results after load
    auto query = makeTestVec(8, 0.1f);
    auto results = loaded.searchFlat(query.data(), query.size(), 2);
    EXPECT_FALSE(results.empty());
    EXPECT_LE(results.size(), 2u);
}

// ============================================================================
// HTB-07 — load() with non-existent path returns false
// ============================================================================

TEST(HnswTTBridge, HTB07_LoadNonExistentPathReturnsFalse) {
    HnswTTBridge bridge({}, 0);
    EXPECT_FALSE(bridge.load("/tmp/no_such_htb_file_themis_test.bin"));
}
