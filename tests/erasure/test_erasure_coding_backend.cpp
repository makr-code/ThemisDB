/*
 * ThemisDB | File: test_erasure_coding_backend.cpp | Version: 0.0.13
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */
// RESTORED FROM HISTORY: 892fbc132819cf3446b54bb51b8b14ec2dd61db5


/**
 * ThemisDB Erasure Coding Backend Tests
 *
 * Focused test suite for ErasureCodingBackend (v1.7.0, Issue #207).
 *
 * Coverage:
 *  - RS(10,4), RS(6,3), RS(4,2) configurations
 *  - Encode / decode round-trips
 *  - Single and multi-shard failure recovery
 *  - Boundary conditions (empty input, single byte, large blobs)
 *  - Storage overhead and fault tolerance metrics
 *  - BlobRedundancyManager integration in PARITY mode
 */

#include <gtest/gtest.h>
#include "storage/erasure_coding_backend.h"
#include "storage/blob_redundancy_manager.h"
#include <algorithm>
#include <map>
#include <random>
#include <string>
#include <vector>

using namespace themisdb::storage;

// ============================================================
// Helpers
// ============================================================

static std::vector<uint8_t> makeData(size_t size, uint8_t seed = 0xAB) {
    std::vector<uint8_t> d(size);
    for (size_t i = 0; i < size; ++i) {
        d[i] = static_cast<uint8_t>((seed + i) & 0xFF);
    }
    return d;
}

static ErasureCodingConfig makeConfig(uint32_t data_shards, uint32_t parity_shards,
                                      ErasureCodingAlgorithm algo =
                                          ErasureCodingAlgorithm::REED_SOLOMON) {
    ErasureCodingConfig cfg;
    cfg.data_shards   = data_shards;
    cfg.parity_shards = parity_shards;
    cfg.algorithm     = algo;
    return cfg;
}

// ============================================================
// Test Suite
// ============================================================

class ErasureCodingFocusedTests : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

// ------------------------------------------------------------
// 1. Configuration helpers
// ------------------------------------------------------------

TEST_F(ErasureCodingFocusedTests, ConfigRS10_4_Overhead) {
    auto cfg = makeConfig(10, 4);
    ErasureCodingBackend b(cfg);
    EXPECT_EQ(b.dataShards(),   10u);
    EXPECT_EQ(b.parityShards(),  4u);
    EXPECT_EQ(b.totalShards(),  14u);
    EXPECT_EQ(b.faultTolerance(), 4u);
    EXPECT_NEAR(b.storageOverhead(), 1.4, 0.01);  // 40% overhead
}

TEST_F(ErasureCodingFocusedTests, ConfigRS6_3_Overhead) {
    auto cfg = makeConfig(6, 3);
    ErasureCodingBackend b(cfg);
    EXPECT_EQ(b.dataShards(),  6u);
    EXPECT_EQ(b.parityShards(), 3u);
    EXPECT_EQ(b.totalShards(),  9u);
    EXPECT_EQ(b.faultTolerance(), 3u);
    EXPECT_NEAR(b.storageOverhead(), 1.5, 0.01);  // 50% overhead
}

TEST_F(ErasureCodingFocusedTests, ConfigRS4_2_Overhead) {
    auto cfg = makeConfig(4, 2);
    ErasureCodingBackend b(cfg);
    EXPECT_EQ(b.dataShards(),  4u);
    EXPECT_EQ(b.parityShards(), 2u);
    EXPECT_EQ(b.totalShards(),  6u);
    EXPECT_EQ(b.faultTolerance(), 2u);
    EXPECT_NEAR(b.storageOverhead(), 1.5, 0.01);
}

TEST_F(ErasureCodingFocusedTests, CanRecoverWithinTolerance) {
    ErasureCodingBackend b(makeConfig(4, 2));
    EXPECT_TRUE(b.canRecover(0));
    EXPECT_TRUE(b.canRecover(1));
    EXPECT_TRUE(b.canRecover(2));
    EXPECT_FALSE(b.canRecover(3));
}

TEST_F(ErasureCodingFocusedTests, InvalidConfigThrows_DataShards) {
    EXPECT_THROW(ErasureCodingBackend(makeConfig(1, 2)), std::invalid_argument);
}

TEST_F(ErasureCodingFocusedTests, InvalidConfigThrows_ParityShards) {
    EXPECT_THROW(ErasureCodingBackend(makeConfig(4, 0)), std::invalid_argument);
}

// ------------------------------------------------------------
// 2. Encode shape and shard properties
// ------------------------------------------------------------

TEST_F(ErasureCodingFocusedTests, EncodeProducesCorrectShardCount_RS4_2) {
    ErasureCodingBackend b(makeConfig(4, 2));
    auto shards = b.encode("blob1", makeData(256));
    EXPECT_EQ(shards.size(), 6u);
}

TEST_F(ErasureCodingFocusedTests, EncodeProducesCorrectShardCount_RS10_4) {
    ErasureCodingBackend b(makeConfig(10, 4));
    auto shards = b.encode("blob1", makeData(1000));
    EXPECT_EQ(shards.size(), 14u);
}

TEST_F(ErasureCodingFocusedTests, EncodeParityShardsMarkedCorrectly) {
    ErasureCodingBackend b(makeConfig(4, 2));
    auto shards = b.encode("blob1", makeData(256));
    for (const auto& s : shards) {
        if (s.shard_index < 4) {
            EXPECT_FALSE(s.is_parity) << "shard " << s.shard_index;
        } else {
            EXPECT_TRUE(s.is_parity) << "shard " << s.shard_index;
        }
    }
}

TEST_F(ErasureCodingFocusedTests, EncodeStoresOriginalSize) {
    ErasureCodingBackend b(makeConfig(4, 2));
    const size_t sz = 1234;
    auto shards = b.encode("blob1", makeData(sz));
    for (const auto& s : shards) {
        EXPECT_EQ(s.original_size, sz);
    }
}

TEST_F(ErasureCodingFocusedTests, EncodeEmptyThrows) {
    ErasureCodingBackend b(makeConfig(4, 2));
    EXPECT_THROW(b.encode("blob1", {}), std::invalid_argument);
}

// ------------------------------------------------------------
// 3. Round-trip: encode then decode (no failures)
// ------------------------------------------------------------

TEST_F(ErasureCodingFocusedTests, RoundTrip_RS4_2_SmallBlob) {
    ErasureCodingBackend b(makeConfig(4, 2));
    auto orig  = makeData(100);
    auto shards = b.encode("blob1", orig);

    std::map<uint32_t, EncodedShard> smap;
    for (auto& s : shards) smap[s.shard_index] = s;

    auto rec = b.decode("blob1", smap, orig.size());
    EXPECT_EQ(rec, orig);
}

TEST_F(ErasureCodingFocusedTests, RoundTrip_RS6_3_MediumBlob) {
    ErasureCodingBackend b(makeConfig(6, 3));
    auto orig  = makeData(4096);
    auto shards = b.encode("blob2", orig);

    std::map<uint32_t, EncodedShard> smap;
    for (auto& s : shards) smap[s.shard_index] = s;

    auto rec = b.decode("blob2", smap, orig.size());
    EXPECT_EQ(rec, orig);
}

TEST_F(ErasureCodingFocusedTests, RoundTrip_RS10_4_LargeBlob) {
    ErasureCodingBackend b(makeConfig(10, 4));
    auto orig  = makeData(65536);
    auto shards = b.encode("blob3", orig);

    std::map<uint32_t, EncodedShard> smap;
    for (auto& s : shards) smap[s.shard_index] = s;

    auto rec = b.decode("blob3", smap, orig.size());
    EXPECT_EQ(rec, orig);
}

TEST_F(ErasureCodingFocusedTests, RoundTrip_SingleByte) {
    ErasureCodingBackend b(makeConfig(4, 2));
    auto orig = makeData(1, 0x42);
    auto shards = b.encode("tiny", orig);

    std::map<uint32_t, EncodedShard> smap;
    for (auto& s : shards) smap[s.shard_index] = s;

    auto rec = b.decode("tiny", smap, orig.size());
    EXPECT_EQ(rec, orig);
}

// ------------------------------------------------------------
// 4. Fault tolerance: single shard failure
// ------------------------------------------------------------

TEST_F(ErasureCodingFocusedTests, SingleDataShardLoss_RS4_2) {
    ErasureCodingBackend b(makeConfig(4, 2));
    auto orig  = makeData(512);
    auto shards = b.encode("blob", orig);

    // Remove shard 0 (data)
    std::map<uint32_t, EncodedShard> smap;
    for (auto& s : shards) {
        if (s.shard_index != 0) smap[s.shard_index] = s;
    }

    auto rec = b.decode("blob", smap, orig.size());
    EXPECT_EQ(rec, orig);
}

TEST_F(ErasureCodingFocusedTests, SingleParityShardLoss_RS4_2) {
    ErasureCodingBackend b(makeConfig(4, 2));
    auto orig  = makeData(512);
    auto shards = b.encode("blob", orig);

    // Remove shard 4 (first parity)
    std::map<uint32_t, EncodedShard> smap;
    for (auto& s : shards) {
        if (s.shard_index != 4) smap[s.shard_index] = s;
    }

    auto rec = b.decode("blob", smap, orig.size());
    EXPECT_EQ(rec, orig);
}

// ------------------------------------------------------------
// 5. Fault tolerance: multi-shard failure (up to parity_shards)
// ------------------------------------------------------------

TEST_F(ErasureCodingFocusedTests, TwoShardLoss_RS4_2_MaxTolerance) {
    ErasureCodingBackend b(makeConfig(4, 2));
    auto orig  = makeData(512);
    auto shards = b.encode("blob", orig);

    // Remove shards 1 and 3 (both data)
    std::map<uint32_t, EncodedShard> smap;
    for (auto& s : shards) {
        if (s.shard_index != 1 && s.shard_index != 3) smap[s.shard_index] = s;
    }

    auto rec = b.decode("blob", smap, orig.size());
    EXPECT_EQ(rec, orig);
}

TEST_F(ErasureCodingFocusedTests, FourShardLoss_RS10_4_MaxTolerance) {
    ErasureCodingBackend b(makeConfig(10, 4));
    auto orig  = makeData(2048);
    auto shards = b.encode("blob", orig);

    // Remove shards 0, 2, 4, 6 (four data shards)
    std::map<uint32_t, EncodedShard> smap;
    for (auto& s : shards) {
        uint32_t idx = s.shard_index;
        if (idx != 0 && idx != 2 && idx != 4 && idx != 6) {
            smap[idx] = s;
        }
    }

    auto rec = b.decode("blob", smap, orig.size());
    EXPECT_EQ(rec, orig);
}

TEST_F(ErasureCodingFocusedTests, ThreeShardLoss_RS6_3_MaxTolerance) {
    ErasureCodingBackend b(makeConfig(6, 3));
    auto orig  = makeData(768);
    auto shards = b.encode("blob", orig);

    // Lose shards 1, 3, 5 (mixed data/parity)
    std::map<uint32_t, EncodedShard> smap;
    for (auto& s : shards) {
        uint32_t idx = s.shard_index;
        if (idx != 1 && idx != 3 && idx != 5) smap[idx] = s;
    }

    auto rec = b.decode("blob", smap, orig.size());
    EXPECT_EQ(rec, orig);
}

// ------------------------------------------------------------
// 6. Unrecoverable scenarios (too many failures)
// ------------------------------------------------------------

TEST_F(ErasureCodingFocusedTests, TooManyLosses_RS4_2_ThrowsOrFails) {
    ErasureCodingBackend b(makeConfig(4, 2));
    auto orig  = makeData(512);
    auto shards = b.encode("blob", orig);

    // Remove 3 shards (exceeds parity_shards=2)
    std::map<uint32_t, EncodedShard> smap;
    for (auto& s : shards) {
        if (s.shard_index > 2) smap[s.shard_index] = s;
    }

    EXPECT_THROW(b.decode("blob", smap, orig.size()), std::runtime_error);
}

// ------------------------------------------------------------
// 7. High-level put / get
// ------------------------------------------------------------

TEST_F(ErasureCodingFocusedTests, PutGet_RoundTrip_RS4_2) {
    ErasureCodingBackend b(makeConfig(4, 2));
    auto orig = makeData(1024);

    b.put("key1", orig);
    auto result = b.get("key1");

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, orig);
}

TEST_F(ErasureCodingFocusedTests, PutGet_NotFound) {
    ErasureCodingBackend b(makeConfig(4, 2));
    EXPECT_FALSE(b.get("missing").has_value());
}

TEST_F(ErasureCodingFocusedTests, PutGet_Remove) {
    ErasureCodingBackend b(makeConfig(4, 2));
    b.put("key1", makeData(256));
    EXPECT_TRUE(b.get("key1").has_value());
    b.remove("key1");
    EXPECT_FALSE(b.get("key1").has_value());
}

TEST_F(ErasureCodingFocusedTests, PutGet_Overwrite) {
    ErasureCodingBackend b(makeConfig(4, 2));
    auto d1 = makeData(256, 0xAA);
    auto d2 = makeData(256, 0xBB);

    b.put("key1", d1);
    b.put("key1", d2);  // overwrite

    auto result = b.get("key1");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, d2);
    EXPECT_NE(*result, d1);
}

TEST_F(ErasureCodingFocusedTests, AvailableShardCount_Full) {
    ErasureCodingBackend b(makeConfig(4, 2));
    b.put("key1", makeData(256));
    EXPECT_EQ(b.availableShardCount("key1"), 6u);
}

TEST_F(ErasureCodingFocusedTests, AvailableShardCount_AfterDrop) {
    ErasureCodingBackend b(makeConfig(4, 2));
    b.put("key1", makeData(256));
    b.dropShard("key1", 0);
    b.dropShard("key1", 3);
    EXPECT_EQ(b.availableShardCount("key1"), 4u);
}

// ------------------------------------------------------------
// 8. Fault-tolerance via put/get/dropShard
// ------------------------------------------------------------

TEST_F(ErasureCodingFocusedTests, FaultTolerance_OneShard_PutGetDropGet) {
    ErasureCodingBackend b(makeConfig(4, 2));
    auto orig = makeData(512);
    b.put("blob", orig);

    b.dropShard("blob", 2);  // drop one data shard

    auto result = b.get("blob");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, orig);
}

TEST_F(ErasureCodingFocusedTests, FaultTolerance_TwoShards_PutGetDropGet) {
    ErasureCodingBackend b(makeConfig(4, 2));
    auto orig = makeData(512);
    b.put("blob", orig);

    b.dropShard("blob", 0);
    b.dropShard("blob", 4);  // one data + one parity

    auto result = b.get("blob");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, orig);
}

TEST_F(ErasureCodingFocusedTests, FaultTolerance_RS10_4_FourDropped) {
    ErasureCodingBackend b(makeConfig(10, 4));
    auto orig = makeData(4096);
    b.put("big", orig);

    // Drop 4 shards (max tolerance)
    b.dropShard("big", 0);
    b.dropShard("big", 5);
    b.dropShard("big", 10);
    b.dropShard("big", 12);

    auto result = b.get("big");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, orig);
}

TEST_F(ErasureCodingFocusedTests, FaultTolerance_RS6_3_ThreeDropped) {
    ErasureCodingBackend b(makeConfig(6, 3));
    auto orig = makeData(2048);
    b.put("blob", orig);

    b.dropShard("blob", 1);
    b.dropShard("blob", 4);
    b.dropShard("blob", 7);

    auto result = b.get("blob");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, orig);
}

TEST_F(ErasureCodingFocusedTests, FaultTolerance_TooManyDropped_ReturnsNullopt) {
    ErasureCodingBackend b(makeConfig(4, 2));
    b.put("blob", makeData(512));

    b.dropShard("blob", 0);
    b.dropShard("blob", 1);
    b.dropShard("blob", 2);  // now only 3 of 6 shards remain (need 4)

    EXPECT_FALSE(b.get("blob").has_value());
}

// ------------------------------------------------------------
// 9. Storage efficiency vs mirroring
// ------------------------------------------------------------

TEST_F(ErasureCodingFocusedTests, StorageOverhead_RS10_4_LessThan50Percent) {
    ErasureCodingBackend b(makeConfig(10, 4));
    // RS(10,4) → 40% overhead (1.4× total storage)
    EXPECT_LT(b.storageOverhead(), 1.5);
    EXPECT_GE(b.storageOverhead(), 1.0);
}

TEST_F(ErasureCodingFocusedTests, StorageOverhead_RS4_2_Is50Percent) {
    ErasureCodingBackend b(makeConfig(4, 2));
    EXPECT_NEAR(b.storageOverhead(), 1.5, 0.01);
}

// ------------------------------------------------------------
// 10. Cauchy Reed-Solomon variant
// ------------------------------------------------------------

TEST_F(ErasureCodingFocusedTests, CauchyRS_RoundTrip_RS4_2) {
    ErasureCodingBackend b(makeConfig(4, 2, ErasureCodingAlgorithm::CAUCHY));
    auto orig  = makeData(512);
    auto shards = b.encode("blob", orig);

    std::map<uint32_t, EncodedShard> smap;
    for (auto& s : shards) smap[s.shard_index] = s;

    auto rec = b.decode("blob", smap, orig.size());
    EXPECT_EQ(rec, orig);
}

// ------------------------------------------------------------
// 11. BlobRedundancyManager integration in PARITY mode
// ------------------------------------------------------------

class ErasureCodingBlobManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
#ifdef _WIN32
        GTEST_SKIP() << "Skipping unstable erasure blob manager tests on Windows";
#endif
    }

    // Simple in-memory shard store
    std::map<std::string, std::map<std::string, std::vector<uint8_t>>> shard_store_;

    BlobRedundancyManager::WriteHandler writeHandler() {
        return [this](const std::string& shard_id, const std::string& path,
                      const std::vector<uint8_t>& data) -> bool {
            shard_store_[shard_id][path] = data;
            return true;
        };
    }

    BlobRedundancyManager::ReadHandler readHandler() {
        return [this](const std::string& shard_id,
                      const std::string& path)
               -> std::optional<std::vector<uint8_t>> {
            auto it = shard_store_.find(shard_id);
            if (it == shard_store_.end()) return std::nullopt;
            auto jt = it->second.find(path);
            if (jt == it->second.end()) return std::nullopt;
            return jt->second;
        };
    }

    // Register a blob in PARITY mode and return its ID
    std::string registerParityBlob(BlobRedundancyManager& mgr,
                                   uint32_t data_shards, uint32_t parity_shards,
                                   size_t size = 1024) {
        BlobRedundancyConfig cfg;
        cfg.mode = RedundancyMode::PARITY;
        cfg.erasure_coding.data_shards   = data_shards;
        cfg.erasure_coding.parity_shards = parity_shards;

        mgr.setCollectionOverride("test-collection", cfg);
        return mgr.registerBlob(BlobType::BLOB_MEDIUM,
                                "/path/to/blob",
                                size,
                                "test-collection");
    }
};

TEST_F(ErasureCodingBlobManagerTest, WriteReadRoundTrip_RS4_2) {
    BlobRedundancyManager::Config cfg;
    BlobRedundancyManager mgr(cfg);

    auto blob_id = registerParityBlob(mgr, 4, 2);
    auto data    = makeData(1024);

    auto wres = mgr.writeBlob(blob_id, data, writeHandler());
    EXPECT_TRUE(wres.has_value()) << "writeBlob failed";

    // All 6 chunks should be stored
    size_t total_chunks = 0;
    for (const auto& [sid, paths] : shard_store_) total_chunks += paths.size();
    EXPECT_EQ(total_chunks, 6u);

    auto rres = mgr.readBlob(blob_id, readHandler());
    ASSERT_TRUE(rres.has_value()) << "readBlob failed";
    EXPECT_EQ(rres.value(), data);
}

TEST_F(ErasureCodingBlobManagerTest, WriteRead_RS10_4) {
    BlobRedundancyManager::Config cfg;
    BlobRedundancyManager mgr(cfg);

    auto blob_id = registerParityBlob(mgr, 10, 4, 4096);
    auto data    = makeData(4096);

    auto wres = mgr.writeBlob(blob_id, data, writeHandler());
    EXPECT_TRUE(wres.has_value());

    auto rres = mgr.readBlob(blob_id, readHandler());
    ASSERT_TRUE(rres.has_value());
    EXPECT_EQ(rres.value(), data);
}

TEST_F(ErasureCodingBlobManagerTest, WriteRead_RS6_3) {
    BlobRedundancyManager::Config cfg;
    BlobRedundancyManager mgr(cfg);

    auto blob_id = registerParityBlob(mgr, 6, 3);
    auto data    = makeData(768);

    auto wres = mgr.writeBlob(blob_id, data, writeHandler());
    EXPECT_TRUE(wres.has_value());

    auto rres = mgr.readBlob(blob_id, readHandler());
    ASSERT_TRUE(rres.has_value());
    EXPECT_EQ(rres.value(), data);
}

TEST_F(ErasureCodingBlobManagerTest, ReadAfterShardLoss_StillRecovers_RS4_2) {
    BlobRedundancyManager::Config cfg;
    BlobRedundancyManager mgr(cfg);

    auto blob_id = registerParityBlob(mgr, 4, 2);
    auto data    = makeData(1024);

    auto write_res = mgr.writeBlob(blob_id, data, writeHandler());
    EXPECT_TRUE(write_res.has_value());

    // Drop one chunk by clearing a shard from the store
    auto first_shard = shard_store_.begin()->first;
    shard_store_.erase(first_shard);

    auto rres = mgr.readBlob(blob_id, readHandler());
    ASSERT_TRUE(rres.has_value()) << "Should recover from 1 shard loss";
    EXPECT_EQ(rres.value(), data);
}
