/**
 * Tests for TemporalColdStore (InMemoryBackend + FileSystemBackend)
 *
 * Test IDs: TCS-IM-01..10 (InMemoryBackend)
 *           TCS-FS-01..08 (FileSystemBackend — real filesystem I/O in /tmp)
 *           TCS-CONC-01   (concurrent reads)
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include <atomic>
#include <filesystem>
#include <thread>
#include "temporal/temporal_cold_store.h"

using namespace themisdb::temporal;
namespace fs = std::filesystem;

// ── Helpers ──────────────────────────────────────────────────────────────────

static VersionedDocument makeClosedDoc(const std::string& key,
                                       Timestamp sys_start,
                                       Timestamp sys_end,
                                       const std::string& value = "v") {
    VersionedDocument doc;
    doc.key        = key;
    doc.data       = {{"val", value}};
    doc.sys_time   = {sys_start, sys_end};
    doc.valid_time = {sys_start, sys_end};
    return doc;
}

static VersionedDocument makeOpenDoc(const std::string& key,
                                      Timestamp sys_start) {
    VersionedDocument doc;
    doc.key        = key;
    doc.data       = {{"val", "current"}};
    doc.sys_time   = {sys_start, kMaxTimestamp};
    doc.valid_time = {sys_start, kMaxTimestamp};
    return doc;
}

// ── InMemoryBackend tests ─────────────────────────────────────────────────────

class ColdStoreInMemoryTest : public ::testing::Test {
protected:
    TemporalColdStore cs{};  // default = InMemoryBackend
};

// TCS-IM-01: Storing a closed version succeeds and increments versionCount.
TEST_F(ColdStoreInMemoryTest, Store_ClosedVersion_Accepted) {
    EXPECT_TRUE(cs.store("orders", makeClosedDoc("k1", 100, 200)));
    EXPECT_EQ(cs.totalVersionCount(), 1u);
    EXPECT_EQ(cs.versionCount("orders", "k1"), 1u);
}

// TCS-IM-02: Storing a current (open-ended) version is rejected.
TEST_F(ColdStoreInMemoryTest, Store_CurrentVersion_Rejected) {
    EXPECT_FALSE(cs.store("orders", makeOpenDoc("k1", 100)));
    EXPECT_EQ(cs.totalVersionCount(), 0u);
}

// TCS-IM-03: getAsOf finds the correct version.
TEST_F(ColdStoreInMemoryTest, GetAsOf_MatchingVersion_Found) {
    cs.store("t", makeClosedDoc("k", 100, 300, "v1"));
    cs.store("t", makeClosedDoc("k", 300, 500, "v2"));
    cs.store("t", makeClosedDoc("k", 500, 700, "v3"));

    auto r = cs.getAsOf("t", "k", 400);
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(r->data["val"].get<std::string>(), "v2");
}

// TCS-IM-04: getAsOf returns nullopt when no version covers the timestamp.
TEST_F(ColdStoreInMemoryTest, GetAsOf_NoMatch_NullOpt) {
    cs.store("t", makeClosedDoc("k", 100, 200));
    EXPECT_FALSE(cs.getAsOf("t", "k", 50).has_value());
    EXPECT_FALSE(cs.getAsOf("t", "k", 250).has_value());
}

// TCS-IM-05: getAsOf on an unknown key returns nullopt.
TEST_F(ColdStoreInMemoryTest, GetAsOf_UnknownKey_NullOpt) {
    EXPECT_FALSE(cs.getAsOf("t", "nonexistent", 100).has_value());
}

// TCS-IM-06: getAll returns all versions sorted by sys_start.
TEST_F(ColdStoreInMemoryTest, GetAll_ReturnsSortedVersions) {
    // Insert out of order to verify sort
    cs.store("t", makeClosedDoc("k", 300, 500));
    cs.store("t", makeClosedDoc("k", 100, 300));
    cs.store("t", makeClosedDoc("k", 500, 700));

    auto all = cs.getAll("t", "k");
    ASSERT_EQ(all.size(), 3u);
    EXPECT_EQ(all[0].sys_time.start, 100);
    EXPECT_EQ(all[1].sys_time.start, 300);
    EXPECT_EQ(all[2].sys_time.start, 500);
}

// TCS-IM-07: getRange returns only overlapping versions.
TEST_F(ColdStoreInMemoryTest, GetRange_FiltersCorrectly) {
    cs.store("t", makeClosedDoc("k", 100, 200));
    cs.store("t", makeClosedDoc("k", 200, 400));
    cs.store("t", makeClosedDoc("k", 400, 600));

    auto r = cs.getRange("t", "k", {150, 450});
    // [100,200) overlaps [150,450), [200,400) overlaps, [400,600) overlaps at 400
    EXPECT_GE(r.size(), 2u);
    for (const auto& doc : r) {
        EXPECT_TRUE(doc.sys_time.overlaps({150, 450}));
    }
}

// TCS-IM-08: remove() removes all versions for a key.
TEST_F(ColdStoreInMemoryTest, Remove_Key_RemovesAllVersions) {
    cs.store("t", makeClosedDoc("k1", 100, 200));
    cs.store("t", makeClosedDoc("k1", 200, 300));
    cs.store("t", makeClosedDoc("k2", 100, 200));

    EXPECT_EQ(cs.remove("t", "k1"), 2u);
    EXPECT_EQ(cs.versionCount("t", "k1"), 0u);
    EXPECT_EQ(cs.versionCount("t", "k2"), 1u);
    EXPECT_EQ(cs.totalVersionCount(), 1u);
}

// TCS-IM-09: removeTable() removes all versions for a table.
TEST_F(ColdStoreInMemoryTest, RemoveTable_RemovesAllTableVersions) {
    cs.store("t1", makeClosedDoc("k", 100, 200));
    cs.store("t1", makeClosedDoc("k", 200, 300));
    cs.store("t2", makeClosedDoc("k", 100, 200));

    EXPECT_EQ(cs.removeTable("t1"), 2u);
    EXPECT_EQ(cs.totalVersionCount(), 1u);
    EXPECT_EQ(cs.versionCount("t2", "k"), 1u);
}

// TCS-IM-10: clear() resets everything.
TEST_F(ColdStoreInMemoryTest, Clear_ResetsEverything) {
    cs.store("t", makeClosedDoc("k", 100, 200));
    cs.store("t", makeClosedDoc("k", 200, 300));
    cs.clear();
    EXPECT_EQ(cs.totalVersionCount(), 0u);
    EXPECT_FALSE(cs.getAsOf("t", "k", 150).has_value());
}

// ── FileSystemBackend tests ───────────────────────────────────────────────────

class ColdStoreFileSystemTest : public ::testing::Test {
protected:
    fs::path tmp_dir;
    std::unique_ptr<TemporalColdStore> cs;

    void SetUp() override {
        tmp_dir = fs::temp_directory_path() / "themisdb_cold_store_test";
        fs::remove_all(tmp_dir);
        cs = std::make_unique<TemporalColdStore>(
            std::make_unique<FileSystemBackend>(tmp_dir));
    }

    void TearDown() override {
        cs.reset();
        fs::remove_all(tmp_dir);
    }
};

// TCS-FS-01: Files are created on disk for stored versions.
TEST_F(ColdStoreFileSystemTest, Store_CreatesFilesOnDisk) {
    EXPECT_TRUE(cs->store("orders", makeClosedDoc("doc1", 100, 200)));
    EXPECT_TRUE(cs->store("orders", makeClosedDoc("doc1", 200, 300)));

    // Count .json files in tmp_dir
    size_t file_count = 0;
    for (auto& entry : fs::recursive_directory_iterator(tmp_dir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".json")
            ++file_count;
    }
    EXPECT_EQ(file_count, 2u);
    EXPECT_EQ(cs->totalVersionCount(), 2u);
}

// TCS-FS-02: getAsOf works correctly with FileSystemBackend.
TEST_F(ColdStoreFileSystemTest, GetAsOf_CorrectVersionFromDisk) {
    cs->store("orders", makeClosedDoc("doc1", 100, 300, "v1"));
    cs->store("orders", makeClosedDoc("doc1", 300, 500, "v2"));

    auto r = cs->getAsOf("orders", "doc1", 400);
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(r->data["val"].get<std::string>(), "v2");
}

// TCS-FS-03: RAM index is rebuilt correctly from disk after restart.
TEST_F(ColdStoreFileSystemTest, RebuildIndex_FromExistingFiles) {
    cs->store("t", makeClosedDoc("k", 100, 200));
    cs->store("t", makeClosedDoc("k", 200, 300));

    // Simulate restart: create fresh store pointing at same directory
    auto cs2 = std::make_unique<TemporalColdStore>(
        std::make_unique<FileSystemBackend>(tmp_dir));

    EXPECT_EQ(cs2->totalVersionCount(), 2u);
    EXPECT_EQ(cs2->versionCount("t", "k"), 2u);

    auto r = cs2->getAsOf("t", "k", 150);
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(r->sys_time.start, 100);
}

// TCS-FS-04: remove() deletes files and updates index.
TEST_F(ColdStoreFileSystemTest, Remove_DeletesFilesFromDisk) {
    cs->store("t", makeClosedDoc("k", 100, 200));
    cs->store("t", makeClosedDoc("k", 200, 300));
    EXPECT_EQ(cs->remove("t", "k"), 2u);
    EXPECT_EQ(cs->totalVersionCount(), 0u);

    // No .json files should remain for this key
    size_t count = 0;
    if (fs::exists(tmp_dir)) {
        for (auto& e : fs::recursive_directory_iterator(tmp_dir))
            if (e.is_regular_file() && e.path().extension() == ".json") {
              ++count;
            }
    }
    EXPECT_EQ(count, 0u);
}

// TCS-FS-05: Keys with special characters are safely encoded/decoded.
TEST_F(ColdStoreFileSystemTest, SpecialCharKeys_SafelyEncodedOnDisk) {
    const std::string special_key = "user/123?foo=bar&x=<>\"";
    EXPECT_TRUE(cs->store("my_table", makeClosedDoc(special_key, 100, 200)));
    EXPECT_EQ(cs->versionCount("my_table", special_key), 1u);

    auto r = cs->getAsOf("my_table", special_key, 150);
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(r->key, special_key);
}

// TCS-FS-06: clear() removes all files from disk.
TEST_F(ColdStoreFileSystemTest, Clear_RemovesAllFilesFromDisk) {
    cs->store("t", makeClosedDoc("k1", 100, 200));
    cs->store("t", makeClosedDoc("k2", 100, 200));
    cs->clear();
    EXPECT_EQ(cs->totalVersionCount(), 0u);

    size_t count = 0;
    if (fs::exists(tmp_dir)) {
        for (auto& e : fs::recursive_directory_iterator(tmp_dir))
            if (e.is_regular_file()) {
              ++count;
            }
    }
    EXPECT_EQ(count, 0u);
}

// TCS-FS-07: getAll returns all versions after rebuild from disk.
TEST_F(ColdStoreFileSystemTest, GetAll_AfterRebuild_AllVersionsReturned) {
    constexpr int N = 20;
    for (int i = 0; i < N; ++i)
        cs->store("t", makeClosedDoc("k", i * 10, (i + 1) * 10));

    // Rebuild
    auto cs2 = std::make_unique<TemporalColdStore>(
        std::make_unique<FileSystemBackend>(tmp_dir));
    auto all = cs2->getAll("t", "k");
    EXPECT_EQ(all.size(), static_cast<size_t>(N));
}

// TCS-FS-08: backend_reads counter is incremented for each value retrieval.
TEST_F(ColdStoreFileSystemTest, Stats_BackendReadCountIncremented) {
    cs->store("t", makeClosedDoc("k", 100, 200, "a"));
    cs->store("t", makeClosedDoc("k", 200, 300, "b"));
    cs->store("t", makeClosedDoc("k", 300, 400, "c"));

    auto all = cs->getAll("t", "k");
    EXPECT_EQ(all.size(), 3u);

    auto s = cs->stats();
    EXPECT_GE(s.backend_reads, 3u);
}

// ── Concurrent reads ──────────────────────────────────────────────────────────

// TCS-CONC-01: Multiple threads can call getAsOf concurrently.
TEST(ColdStoreConcurrencyTest, ConcurrentReads_NoDeadlock) {
    TemporalColdStore cs{};
    constexpr int N = 200;
    for (int i = 0; i < N; ++i)
        cs.store("t", makeClosedDoc("k", i * 10, (i + 1) * 10, std::to_string(i)));

    constexpr int NUM_THREADS = 8;
    std::atomic<int> hits{0};
    std::vector<std::thread> threads = {};

    for (int t = 0; t < NUM_THREADS; ++t) {
        threads.emplace_back([&, t]() {
            for (int q = 0; q < 50; ++q) {
                Timestamp pt = static_cast<Timestamp>(((t * 25 + q) % N) * 10 + 4);
                auto r = cs.getAsOf("t", "k", pt);
                if (r.has_value()) {
                  hits.fetch_add(1, std::memory_order_relaxed);
                }
            }
        });
    }
    for (auto& th : threads) {
      th.join();
    }
    EXPECT_EQ(hits.load(), NUM_THREADS * 50);
}
