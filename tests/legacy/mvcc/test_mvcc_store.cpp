/*
 * ThemisDB | File: test_mvcc_store.cpp | Version: 0.0.47
 * Maturity: 🟢 PRODUCTION-READY | Score: 98/100
 * Gap Summary: total=4; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=1, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

// Copyright 2025 ThemisDB
// Licensed under MIT License
//
// Tests for:
//  - HLCTimestamp encoding/decoding and ordering
//  - HybridLogicalClock monotonicity, receive-update, and peek
//  - MVCCStore: put, getLatest, getAtTimestamp, scanVersions, gcVersionsBefore

#include <gtest/gtest.h>
#include "storage/hlc.h"
#include "storage/mvcc_store.h"
#include "storage/rocksdb_wrapper.h"
#include <filesystem>
#include <thread>
#include <chrono>
#include <vector>
#include <string>

using namespace themis;

// ─────────────────────────────────────────────────────────────────────────────
// HLCTimestamp tests
// ─────────────────────────────────────────────────────────────────────────────

TEST(HLCTimestampTest, BuildFromComponents) {
    auto ts = HLCTimestamp::from(1000, 5);
    EXPECT_EQ(ts.physical(), 1000u);
    EXPECT_EQ(ts.logical(),  5u);
}

TEST(HLCTimestampTest, Ordering) {
    auto a = HLCTimestamp::from(100, 0);
    auto b = HLCTimestamp::from(100, 1);
    auto c = HLCTimestamp::from(101, 0);

    EXPECT_LT(a, b);
    EXPECT_LT(b, c);
    EXPECT_LT(a, c);
    EXPECT_GT(c, a);
    EXPECT_EQ(a, HLCTimestamp::from(100, 0));
}

TEST(HLCTimestampTest, EncodeDecode_RoundTrip) {
    auto original = HLCTimestamp::from(1234567890ull, 42u);
    std::string encoded = original.encodeToString();
    ASSERT_EQ(encoded.size(), 8u);

    auto decoded = HLCTimestamp::decodeFromString(encoded);
    EXPECT_EQ(decoded, original);
}

TEST(HLCTimestampTest, Encode_LexicographicOrder) {
    // Encoded strings must sort in the same order as the values.
    auto ts1 = HLCTimestamp::from(100, 0);
    auto ts2 = HLCTimestamp::from(100, 1);
    auto ts3 = HLCTimestamp::from(200, 0);

    EXPECT_LT(ts1.encodeToString(), ts2.encodeToString());
    EXPECT_LT(ts2.encodeToString(), ts3.encodeToString());
}

TEST(HLCTimestampTest, ToString) {
    auto ts = HLCTimestamp::from(500, 7);
    std::string s = ts.toString();
    // Should contain the physical part and logical part separated by '.'
    EXPECT_NE(s.find("500"), std::string::npos);
    EXPECT_NE(s.find("7"),   std::string::npos);
}

TEST(HLCTimestampTest, DecodeFromBytes_RoundTrip) {
    auto original = HLCTimestamp::from(9999999ull, 1023u);
    uint8_t buf[8]{};
    original.encodeToBytes(buf);
    auto decoded = HLCTimestamp::decodeFromBytes(buf);
    EXPECT_EQ(decoded, original);
}

// ─────────────────────────────────────────────────────────────────────────────
// HybridLogicalClock tests
// ─────────────────────────────────────────────────────────────────────────────

TEST(HLCTest, NowIsMonotonic) {
    HybridLogicalClock hlc;
    HLCTimestamp prev = hlc.now();
    for (int i = 0; i < 1000; ++i) {
        HLCTimestamp next = hlc.now();
        EXPECT_GT(next, prev) << "HLC must be strictly monotonic";
        prev = next;
    }
}

TEST(HLCTest, UpdateAdvancesClock) {
    HybridLogicalClock hlc;
    auto local = hlc.now();

    // Simulate a received message with a much-newer timestamp.
    HLCTimestamp remote = HLCTimestamp::from(local.physical() + 1000, 5u);
    auto after = hlc.update(remote);

    EXPECT_GT(after, local);
    EXPECT_GE(after.physical(), remote.physical());
}

TEST(HLCTest, UpdateWithOlderTimestampStillAdvances) {
    HybridLogicalClock hlc;
    auto local = hlc.now();

    // Received timestamp is *older* than local.
    HLCTimestamp old_remote = HLCTimestamp::from(1u, 0u);
    auto after = hlc.update(old_remote);

    // Must remain strictly greater than local.
    EXPECT_GT(after, local);
}

TEST(HLCTest, PeekDoesNotAdvance) {
    HybridLogicalClock hlc;
    auto a = hlc.now();
    auto b = hlc.peek();
    auto c = hlc.peek();
    EXPECT_EQ(b, c);
    EXPECT_GE(b, a);
}

// ─────────────────────────────────────────────────────────────────────────────
// MVCCStore test fixture
// ─────────────────────────────────────────────────────────────────────────────

class MVCCStoreTest : public ::testing::Test {
protected:
    void SetUp() override {
        db_path_ = "./data/themis_mvcc_store_test";
        if (std::filesystem::exists(db_path_)) {
            std::filesystem::remove_all(db_path_);
        }

        RocksDBWrapper::Config cfg;
        cfg.db_path = db_path_;
        cfg.enable_wal = true;
        rocksdb_ = std::make_shared<RocksDBWrapper>(cfg);
        ASSERT_TRUE(rocksdb_->open());

        clock_ = std::make_shared<HybridLogicalClock>();
        store_ = std::make_unique<MVCCStore>(rocksdb_, clock_);
    }

    void TearDown() override {
        store_.reset();
        rocksdb_.reset();
        if (std::filesystem::exists(db_path_)) {
            std::filesystem::remove_all(db_path_);
        }
    }

    static std::vector<uint8_t> bytes(std::string_view s) {
        return {s.begin(), s.end()};
    }

    /** Return the timestamp immediately before @p ts (saturates at 0). */
    static HLCTimestamp justBefore(HLCTimestamp ts) {
        return HLCTimestamp(ts.value > 0 ? ts.value - 1 : 0);
    }

    std::string db_path_;
    std::shared_ptr<RocksDBWrapper>     rocksdb_;
    std::shared_ptr<HybridLogicalClock> clock_;
    std::unique_ptr<MVCCStore>          store_;
};

// ─────────────────────────────────────────────────────────────────────────────
// Key encoding tests (white-box)
// ─────────────────────────────────────────────────────────────────────────────

TEST(MVCCKeyEncodingTest, EncodeVersionedKey) {
    auto ts = HLCTimestamp::from(42u, 0u);
    std::string vkey = MVCCStore::encodeVersionedKey("mykey", ts);

    // Must start with the base key followed by '\x00'.
    ASSERT_GE(vkey.size(), 6u + 8u);
    EXPECT_EQ(vkey.substr(0, 5), "mykey");
    EXPECT_EQ(vkey[5], '\x00');
    EXPECT_EQ(vkey.size(), 5u + 1u + 8u);
}

TEST(MVCCKeyEncodingTest, EncodeVersionPrefix) {
    std::string prefix = MVCCStore::encodeVersionPrefix("mykey");
    EXPECT_EQ(prefix, std::string("mykey") + '\x00');
}

TEST(MVCCKeyEncodingTest, DecodeTimestamp_RoundTrip) {
    auto ts = HLCTimestamp::from(9876u, 11u);
    std::string vkey = MVCCStore::encodeVersionedKey("k", ts);
    auto recovered = MVCCStore::decodeTimestamp(vkey);
    EXPECT_EQ(recovered, ts);
}

TEST(MVCCKeyEncodingTest, DecodeTimestamp_InvalidKey) {
    // A key that is too short should return zero timestamp.
    HLCTimestamp ts = MVCCStore::decodeTimestamp("short");
    EXPECT_EQ(ts, HLCTimestamp(0));
}

// ─────────────────────────────────────────────────────────────────────────────
// MVCCStore: basic put / getLatest
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(MVCCStoreTest, PutAndGetLatest) {
    auto v = bytes("hello");
    store_->put("key1", v);

    auto result = store_->getLatest("key1");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, v);
}

TEST_F(MVCCStoreTest, GetLatest_MissingKey) {
    auto result = store_->getLatest("nonexistent");
    EXPECT_FALSE(result.has_value());
}

TEST_F(MVCCStoreTest, MultipleVersions_LatestReturnsNewest) {
    auto v1 = bytes("v1");
    auto v2 = bytes("v2");
    auto v3 = bytes("v3");

    store_->put("k", v1);
    store_->put("k", v2);
    store_->put("k", v3);

    auto result = store_->getLatest("k");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, v3);
}

// ─────────────────────────────────────────────────────────────────────────────
// MVCCStore: getAtTimestamp (snapshot reads)
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(MVCCStoreTest, GetAtTimestamp_ExactMatch) {
    auto v1 = bytes("version1");
    HLCTimestamp ts1 = store_->put("key", v1);

    auto result = store_->getAtTimestamp("key", ts1);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, v1);
}

TEST_F(MVCCStoreTest, GetAtTimestamp_BeforeFirstVersion) {
    auto v = bytes("value");
    HLCTimestamp ts = store_->put("key", v);

    // Read at timestamp strictly before the first version.
    auto result = store_->getAtTimestamp("key", justBefore(ts));
    // No version exists at this point.
    EXPECT_FALSE(result.has_value());
}

TEST_F(MVCCStoreTest, GetAtTimestamp_ReturnsCorrectVersion) {
    auto v1 = bytes("old");
    auto v2 = bytes("new");

    HLCTimestamp ts1 = store_->put("key", v1);
    HLCTimestamp ts2 = store_->put("key", v2);

    // ts1 read should return v1.
    auto r1 = store_->getAtTimestamp("key", ts1);
    ASSERT_TRUE(r1.has_value());
    EXPECT_EQ(*r1, v1);

    // ts2 read should return v2.
    auto r2 = store_->getAtTimestamp("key", ts2);
    ASSERT_TRUE(r2.has_value());
    EXPECT_EQ(*r2, v2);
}

TEST_F(MVCCStoreTest, GetAtTimestamp_BetweenVersions) {
    auto v1 = bytes("first");
    auto v2 = bytes("second");

    HLCTimestamp ts1 = store_->put("key", v1);
    HLCTimestamp ts2 = store_->put("key", v2);

    // A timestamp strictly between ts1 and ts2 should return v1.
    ASSERT_GT(ts2, ts1);
    HLCTimestamp between{ts1.value + 1};
    if (between < ts2) {
        auto r = store_->getAtTimestamp("key", between);
        ASSERT_TRUE(r.has_value());
        EXPECT_EQ(*r, v1);
    }
}

TEST_F(MVCCStoreTest, PutWithTimestamp_ExplicitTs) {
    HLCTimestamp ts = HLCTimestamp::from(100000u, 0u);
    auto v = bytes("explicit");
    store_->putWithTimestamp("key", v, ts);

    auto r = store_->getAtTimestamp("key", ts);
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(*r, v);
}

// ─────────────────────────────────────────────────────────────────────────────
// MVCCStore: scanVersions
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(MVCCStoreTest, ScanVersions_AscendingOrder) {
    auto v1 = bytes("a");
    auto v2 = bytes("b");
    auto v3 = bytes("c");

    store_->put("row", v1);
    store_->put("row", v2);
    store_->put("row", v3);

    std::vector<std::vector<uint8_t>> values;
    store_->scanVersions("row", [&](const MVCCStore::VersionEntry& e) {
        values.push_back(e.value);
        return true; // continue
    });

    ASSERT_EQ(values.size(), 3u);
    EXPECT_EQ(values[0], v1);
    EXPECT_EQ(values[1], v2);
    EXPECT_EQ(values[2], v3);
}

TEST_F(MVCCStoreTest, ScanVersions_EarlyStop) {
    for (int i = 0; i < 5; ++i) {
        store_->put("r", bytes(std::string(1, static_cast<char>('a' + i))));
    }

    int count = 0;
    store_->scanVersions("r", [&](const MVCCStore::VersionEntry&) {
        ++count;
        return count < 2; // stop after 2
    });
    EXPECT_EQ(count, 2);
}

TEST_F(MVCCStoreTest, ScanVersions_Empty) {
    bool called = false;
    store_->scanVersions("no-such-key", [&](const MVCCStore::VersionEntry&) {
        called = true;
        return true;
    });
    EXPECT_FALSE(called);
}

// ─────────────────────────────────────────────────────────────────────────────
// MVCCStore: garbage collection
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(MVCCStoreTest, GC_RemovesOldVersions) {
    auto v1 = bytes("old1");
    auto v2 = bytes("old2");
    auto v3 = bytes("keep");

    HLCTimestamp ts1 = store_->put("key", v1);
    HLCTimestamp ts2 = store_->put("key", v2);
    HLCTimestamp ts3 = store_->put("key", v3);

    (void)ts1; (void)ts2; (void)ts3;

    // GC everything before ts3 (should delete v1 and v2).
    MVCCStore::GCOptions opts;
    opts.min_versions_to_keep = 1;
    uint64_t deleted = store_->gcVersionsBefore("key", ts3, opts);
    EXPECT_EQ(deleted, 2u);

    // The latest version should still be accessible.
    auto latest = store_->getLatest("key");
    ASSERT_TRUE(latest.has_value());
    EXPECT_EQ(*latest, v3);
}

TEST_F(MVCCStoreTest, GC_RespectsMinVersionsToKeep) {
    auto v1 = bytes("v1");
    auto v2 = bytes("v2");
    auto v3 = bytes("v3");

    store_->put("key", v1);
    store_->put("key", v2);
    HLCTimestamp ts3 = store_->put("key", v3);

    // min_versions_to_keep = 2: only 1 deletion should happen (v1), keeping v2+v3.
    MVCCStore::GCOptions opts;
    opts.min_versions_to_keep = 2;
    uint64_t deleted = store_->gcVersionsBefore("key", ts3, opts);
    EXPECT_EQ(deleted, 1u);

    // v2 and v3 should remain.
    std::vector<std::vector<uint8_t>> remaining;
    store_->scanVersions("key", [&](const MVCCStore::VersionEntry& e) {
        remaining.push_back(e.value);
        return true;
    });
    ASSERT_EQ(remaining.size(), 2u);
}

TEST_F(MVCCStoreTest, GC_NothingToDelete) {
    HLCTimestamp ts = store_->put("key", bytes("v1"));
    // GC before the only version should delete nothing.
    uint64_t deleted = store_->gcVersionsBefore("key", justBefore(ts));
    EXPECT_EQ(deleted, 0u);
}

TEST_F(MVCCStoreTest, GcAll_AcrossMultipleKeys) {
    HLCTimestamp ts1 = store_->put("a", bytes("old_a"));
    store_->put("a", bytes("new_a"));

    HLCTimestamp ts2 = store_->put("b", bytes("old_b"));
    store_->put("b", bytes("new_b"));

    // Choose a GC watermark that is after both initial writes.
    HLCTimestamp watermark{std::max(ts1.value, ts2.value) + 1};

    uint64_t deleted = store_->gcAllBefore(watermark);
    EXPECT_GE(deleted, 2u); // at least one old version per key

    // Latest versions must still be readable.
    auto ra = store_->getLatest("a");
    auto rb = store_->getLatest("b");
    ASSERT_TRUE(ra.has_value());
    ASSERT_TRUE(rb.has_value());
    EXPECT_EQ(*ra, bytes("new_a"));
    EXPECT_EQ(*rb, bytes("new_b"));
}

// ─────────────────────────────────────────────────────────────────────────────
// MVCCStore: clock access
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(MVCCStoreTest, CurrentTimestamp_AdvancesAfterPut) {
    auto before = store_->currentTimestamp();
    store_->put("x", bytes("y"));
    auto after = store_->currentTimestamp();
    EXPECT_GE(after, before);
}

TEST_F(MVCCStoreTest, UpdateClock_AdvancesToReceived) {
    HLCTimestamp far_future = HLCTimestamp::from(9999999999ull, 0u);
    store_->updateClock(far_future);
    auto now = store_->currentTimestamp();
    EXPECT_GE(now, far_future);
}

// ─────────────────────────────────────────────────────────────────────────────
// Isolation: reads on one key are not affected by writes to another
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(MVCCStoreTest, IsolatedKeys) {
    auto va = bytes("a_value");
    auto vb = bytes("b_value");

    HLCTimestamp ts_a = store_->put("keyA", va);
    HLCTimestamp ts_b = store_->put("keyB", vb);

    auto ra = store_->getAtTimestamp("keyA", ts_a);
    auto rb = store_->getAtTimestamp("keyB", ts_b);

    ASSERT_TRUE(ra.has_value());
    ASSERT_TRUE(rb.has_value());
    EXPECT_EQ(*ra, va);
    EXPECT_EQ(*rb, vb);

    // keyA should have no version at ts_b if ts_b > ts_a.
    if (ts_b > ts_a) {
        // keyA at ts_b should still return va (latest before ts_b).
        auto ra2 = store_->getAtTimestamp("keyA", ts_b);
        ASSERT_TRUE(ra2.has_value());
        EXPECT_EQ(*ra2, va);
    }
}
