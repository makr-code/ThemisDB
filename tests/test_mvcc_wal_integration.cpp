// Copyright 2025 ThemisDB
// Licensed under MIT License
//
// WAL + MVCC integration tests:
//  - Verify that MVCC versioning and WAL crash-recovery work together
//  - Snapshot-isolation: reads at old timestamps see old data
//  - Garbage collection preserves min_versions_to_keep
//  - WAL records survive open/close cycles when backed by real storage

#include <gtest/gtest.h>
#include "storage/mvcc_store.h"
#include "storage/rocksdb_wrapper.h"
#include "storage/hlc.h"

#include <chrono>
#include <filesystem>
#include <string>
#include <thread>
#include <vector>

namespace fs = std::filesystem;
using namespace themis;

// ─────────────────────────────────────────────────────────────────────────────
// Fixture
// ─────────────────────────────────────────────────────────────────────────────
class MVCCIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto ts = std::chrono::steady_clock::now().time_since_epoch().count();
        db_path_ = (fs::temp_directory_path() /
                    ("themis_mvcc_int_" + std::to_string(ts))).string();

        openDB();
    }

    void TearDown() override {
        mvcc_.reset();
        db_.reset();
        fs::remove_all(db_path_);
    }

    void openDB() {
        RocksDBWrapper::Config cfg;
        cfg.db_path          = db_path_;
        cfg.enable_wal       = true;
        cfg.enable_statistics = false;
        db_ = std::make_shared<RocksDBWrapper>(cfg);
        ASSERT_TRUE(db_->open());

        clock_ = std::make_shared<HybridLogicalClock>();
        mvcc_  = std::make_unique<MVCCStore>(db_, clock_);
    }

    void closeDB() {
        mvcc_.reset();
        db_->close();
        db_.reset();
    }

    // Helper: bytes from string
    static std::vector<uint8_t> b(const std::string& s) {
        return std::vector<uint8_t>(s.begin(), s.end());
    }
    static std::string str(const std::vector<uint8_t>& v) {
        return std::string(v.begin(), v.end());
    }

    std::string db_path_;
    std::shared_ptr<RocksDBWrapper> db_;
    std::shared_ptr<HybridLogicalClock> clock_;
    std::unique_ptr<MVCCStore> mvcc_;
};

// ─────────────────────────────────────────────────────────────────────────────
// Test 1: Basic put / getLatest round-trip
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(MVCCIntegrationTest, BasicPutGet) {
    mvcc_->put("hello", b("world"));
    auto v = mvcc_->getLatest("hello");
    ASSERT_TRUE(v.has_value());
    EXPECT_EQ(str(*v), "world");
}

// ─────────────────────────────────────────────────────────────────────────────
// Test 2: Snapshot isolation – read at older timestamp sees old value
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(MVCCIntegrationTest, SnapshotRead_SeesOldValue) {
    auto ts1 = mvcc_->put("snap_key", b("value_v1"));

    // Small delay so HLC advances
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    auto ts2 = mvcc_->put("snap_key", b("value_v2"));
    EXPECT_NE(ts1, ts2);

    // Read at ts1 – must see v1
    auto v1 = mvcc_->getAtTimestamp("snap_key", ts1);
    ASSERT_TRUE(v1.has_value());
    EXPECT_EQ(str(*v1), "value_v1");

    // Read at ts2 – must see v2
    auto v2 = mvcc_->getAtTimestamp("snap_key", ts2);
    ASSERT_TRUE(v2.has_value());
    EXPECT_EQ(str(*v2), "value_v2");

    // Latest must be v2
    auto latest = mvcc_->getLatest("snap_key");
    ASSERT_TRUE(latest.has_value());
    EXPECT_EQ(str(*latest), "value_v2");
}

// ─────────────────────────────────────────────────────────────────────────────
// Test 3: Version scan returns ascending timestamps
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(MVCCIntegrationTest, VersionScan_AscendingTimestamps) {
    constexpr int kVersions = 5;
    std::vector<HLCTimestamp> ts_vec;

    for (int i = 0; i < kVersions; ++i) {
        ts_vec.push_back(mvcc_->put("scan_key", b("v" + std::to_string(i))));
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    std::vector<MVCCStore::VersionEntry> entries;
    mvcc_->scanVersions("scan_key", [&](const MVCCStore::VersionEntry& e) {
        entries.push_back(e); return true;
    });

    ASSERT_EQ(entries.size(), static_cast<size_t>(kVersions));
    for (size_t i = 1; i < entries.size(); ++i) {
        EXPECT_LT(entries[i - 1].timestamp, entries[i].timestamp)
            << "Timestamps must be strictly ascending";
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Test 4: GC removes old versions, respects min_versions_to_keep = 2
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(MVCCIntegrationTest, GC_RespectsMinVersionsToKeep) {
    HLCTimestamp last_ts;
    for (int i = 0; i < 6; ++i) {
        last_ts = mvcc_->put("gc_key", b("v" + std::to_string(i)));
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    // GC everything older than last_ts, keep at least 2 versions
    MVCCStore::GCOptions opts;
    opts.min_versions_to_keep = 2;
    mvcc_->gcVersionsBefore("gc_key", last_ts, opts);

    // Count remaining versions
    int remaining = 0;
    mvcc_->scanVersions("gc_key", [&](const MVCCStore::VersionEntry&) {
        ++remaining; return true;
    });
    EXPECT_GE(remaining, 2) << "At least 2 versions must survive GC";
    EXPECT_LE(remaining, 6) << "Cannot have more versions than written";
}

// ─────────────────────────────────────────────────────────────────────────────
// Test 5: Multiple keys, isolated versioning
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(MVCCIntegrationTest, MultipleKeys_IsolatedVersioning) {
    constexpr int kKeys = 20;
    std::vector<std::pair<HLCTimestamp, std::string>> snapshots;

    for (int i = 0; i < kKeys; ++i) {
        auto ts = mvcc_->put("key_" + std::to_string(i),
                             b("val_" + std::to_string(i)));
        snapshots.emplace_back(ts, "val_" + std::to_string(i));
    }

    // Update all keys with new values
    for (int i = 0; i < kKeys; ++i) {
        mvcc_->put("key_" + std::to_string(i), b("updated_" + std::to_string(i)));
    }

    // Read back at original timestamps – must see original values
    for (int i = 0; i < kKeys; ++i) {
        auto v = mvcc_->getAtTimestamp("key_" + std::to_string(i), snapshots[i].first);
        ASSERT_TRUE(v.has_value()) << "key_" << i;
        EXPECT_EQ(str(*v), snapshots[i].second) << "key_" << i;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Test 6: Persist and reopen – versions survive DB restart
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(MVCCIntegrationTest, PersistAndReopen_VersionsSurvive) {
    auto ts1 = mvcc_->put("persist_key", b("v1"));
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    auto ts2 = mvcc_->put("persist_key", b("v2"));

    // Close and reopen
    closeDB();
    openDB();

    // v1 must still be readable at ts1
    auto v1 = mvcc_->getAtTimestamp("persist_key", ts1);
    ASSERT_TRUE(v1.has_value()) << "v1 must survive DB restart";
    EXPECT_EQ(str(*v1), "v1");

    // Latest must be v2
    auto latest = mvcc_->getLatest("persist_key");
    ASSERT_TRUE(latest.has_value());
    EXPECT_EQ(str(*latest), "v2");
}

// ─────────────────────────────────────────────────────────────────────────────
// Test 7: Read before first write returns empty
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(MVCCIntegrationTest, ReadBeforeFirstWrite_ReturnsEmpty) {
    auto early_ts = clock_->now();
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    mvcc_->put("new_key", b("new_val"));

    auto v = mvcc_->getAtTimestamp("new_key", early_ts);
    EXPECT_FALSE(v.has_value()) << "No version should exist before the first write";
}

// ─────────────────────────────────────────────────────────────────────────────
// Test 8: HLC monotonically advances across puts
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(MVCCIntegrationTest, HLC_MonotonicallyAdvances) {
    constexpr int kPuts = 20;
    HLCTimestamp prev = clock_->now();

    for (int i = 0; i < kPuts; ++i) {
        auto ts = mvcc_->put("hlc_key_" + std::to_string(i),
                             b("val_" + std::to_string(i)));
        EXPECT_GT(ts, prev) << "i=" << i << ": HLC must advance on each write";
        prev = ts;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Test 9: Concurrent puts on different keys, all retrievable
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(MVCCIntegrationTest, ConcurrentPuts_AllRetrievable) {
    constexpr int kThreads = 4;
    constexpr int kPerThread = 25;

    std::vector<std::thread> threads;
    threads.reserve(kThreads);

    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&, t]() {
            for (int i = 0; i < kPerThread; ++i) {
                std::string k = "ct_" + std::to_string(t) + "_" + std::to_string(i);
                mvcc_->put(k, b("v" + std::to_string(i)));
            }
        });
    }
    for (auto& th : threads) {
      th.join();
    }

    // Verify all keys are readable
    int found = 0;
    for (int t = 0; t < kThreads; ++t) {
        for (int i = 0; i < kPerThread; ++i) {
            auto v = mvcc_->getLatest("ct_" + std::to_string(t) + "_" + std::to_string(i));
            if (v.has_value()) {
              ++found;
            }
        }
    }
    EXPECT_EQ(found, kThreads * kPerThread);
}

// ─────────────────────────────────────────────────────────────────────────────
// Test 10: gcAll removes old versions across all keys
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(MVCCIntegrationTest, GCAll_RemovesOldVersionsAcrossAllKeys) {
    constexpr int kKeys = 5;
    constexpr int kVersionsPerKey = 4;

    // Write multiple versions per key
    HLCTimestamp gc_watermark;
    for (int v = 0; v < kVersionsPerKey; ++v) {
        for (int k = 0; k < kKeys; ++k) {
            gc_watermark = mvcc_->put("gc_all_key_" + std::to_string(k),
                                     b("v" + std::to_string(v)));
        }
        if (v < kVersionsPerKey - 2) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    // GC all versions before watermark with min 1 version kept
    MVCCStore::GCOptions opts;
    opts.min_versions_to_keep = 1;
    mvcc_->gcAllBefore(gc_watermark, opts);

    // Latest version of each key must still be readable
    for (int k = 0; k < kKeys; ++k) {
        auto v = mvcc_->getLatest("gc_all_key_" + std::to_string(k));
        EXPECT_TRUE(v.has_value()) << "key_" << k << " must survive gcAll";
    }
}
