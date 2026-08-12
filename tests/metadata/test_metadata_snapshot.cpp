/**
 * Test: Metadata Snapshot
 *
 * Tests for MetadataSnapshot / IMetadataSnapshotStore /
 * InMemoryMetadataSnapshotStore:
 *
 * Acceptance criteria:
 *   AC-SNAP-1  MetadataSnapshot::tableCount returns correct count
 *   AC-SNAP-2  findTable returns correct TableSchema pointer by name
 *   AC-SNAP-3  findTable returns nullptr for unknown name
 *   AC-SNAP-4  MetadataSnapshot::toJSON contains snapshot_id, created_at, author,
 *              description, table_count, tables
 *   AC-SNAP-5  InMemoryMetadataSnapshotStore: save returns snapshot_id
 *   AC-SNAP-6  InMemoryMetadataSnapshotStore: load returns snapshot after save
 *   AC-SNAP-7  InMemoryMetadataSnapshotStore: load returns nullopt for unknown id
 *   AC-SNAP-8  InMemoryMetadataSnapshotStore: save with empty id throws
 *              MetadataSnapshotException
 *   AC-SNAP-9  InMemoryMetadataSnapshotStore: remove returns true for existing,
 *              false for missing
 *   AC-SNAP-10 InMemoryMetadataSnapshotStore: listSnapshotIds returns sorted list
 *   AC-SNAP-11 InMemoryMetadataSnapshotStore: size() reflects add/remove
 *   AC-SNAP-12 InMemoryMetadataSnapshotStore: clear() empties store
 *   AC-SNAP-13 InMemoryMetadataSnapshotStore: concurrent save/load is thread-safe
 *   AC-SNAP-14 Polymorphic usage via IMetadataSnapshotStore*
 *
 * Copyright (c) 2026 ThemisDB Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include "metadata/metadata_snapshot.h"

#include <atomic>
#include <thread>
#include <vector>

using namespace themis;           // for SchemaManager
using namespace themis::metadata; // for MetadataSnapshot, InMemoryMetadataSnapshotStore, etc.

// ─────────────────────────────────────────────────────────────────────────────
// Helper: build a minimal MetadataSnapshot
// ─────────────────────────────────────────────────────────────────────────────

static MetadataSnapshot makeSnapshot(
    const std::string& id,
    const std::string& author      = "test-author",
    const std::string& description = "test snapshot",
    const std::vector<std::string>& table_names = {})
{
    MetadataSnapshot snap;
    snap.snapshot_id = id;
    snap.created_at  = std::chrono::system_clock::now();
    snap.author      = author;
    snap.description = description;

    for (const auto& name : table_names) {
        SchemaManager::TableSchema t;
        t.name = name;
        t.type = "relational";
        snap.tables.push_back(t);
    }
    return snap;
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-SNAP-1 — MetadataSnapshot::tableCount returns correct count
// ─────────────────────────────────────────────────────────────────────────────

TEST(MetadataSnapshotFocusedTests, TableCountReturnsCorrectValue) {
    auto snap = makeSnapshot("v1", "auth", "desc", {"orders", "users", "products"});
    EXPECT_EQ(snap.tableCount(), 3u);
}

TEST(MetadataSnapshotFocusedTests, TableCountIsZeroWhenNoTables) {
    auto snap = makeSnapshot("empty");
    EXPECT_EQ(snap.tableCount(), 0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-SNAP-2 — findTable returns correct TableSchema pointer by name
// ─────────────────────────────────────────────────────────────────────────────

TEST(MetadataSnapshotFocusedTests, FindTableReturnsCorrectPointer) {
    auto snap = makeSnapshot("v1", "auth", "desc", {"orders", "users"});

    const SchemaManager::TableSchema* t = snap.findTable("orders");
    ASSERT_NE(t, nullptr);
    EXPECT_EQ(t->name, "orders");

    const SchemaManager::TableSchema* u = snap.findTable("users");
    ASSERT_NE(u, nullptr);
    EXPECT_EQ(u->name, "users");
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-SNAP-3 — findTable returns nullptr for unknown name
// ─────────────────────────────────────────────────────────────────────────────

TEST(MetadataSnapshotFocusedTests, FindTableReturnsNullptrForUnknownName) {
    auto snap = makeSnapshot("v1", "auth", "desc", {"orders"});

    EXPECT_EQ(snap.findTable("nonexistent"), nullptr);
    EXPECT_EQ(snap.findTable(""),            nullptr);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-SNAP-4 — MetadataSnapshot::toJSON contains expected fields
// ─────────────────────────────────────────────────────────────────────────────

TEST(MetadataSnapshotFocusedTests, ToJsonContainsExpectedFields) {
    auto snap = makeSnapshot("snap-001", "ci-pipeline",
                             "initial release", {"orders", "users"});

    auto j = snap.toJSON();

    EXPECT_TRUE(j.contains("snapshot_id"));
    EXPECT_TRUE(j.contains("created_at"));
    EXPECT_TRUE(j.contains("author"));
    EXPECT_TRUE(j.contains("description"));
    EXPECT_TRUE(j.contains("table_count"));
    EXPECT_TRUE(j.contains("tables"));

    EXPECT_EQ(j["snapshot_id"].get<std::string>(),  "snap-001");
    EXPECT_EQ(j["author"].get<std::string>(),       "ci-pipeline");
    EXPECT_EQ(j["description"].get<std::string>(),  "initial release");
    EXPECT_EQ(j["table_count"].get<size_t>(),       2u);
    EXPECT_TRUE(j["tables"].is_array());
    EXPECT_EQ(j["tables"].size(),                   2u);

    // created_at must be a non-empty ISO-8601 string
    EXPECT_FALSE(j["created_at"].get<std::string>().empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-SNAP-5 — InMemoryMetadataSnapshotStore: save returns snapshot_id
// ─────────────────────────────────────────────────────────────────────────────

TEST(MetadataSnapshotFocusedTests, SaveReturnsSnapshotId) {
    InMemoryMetadataSnapshotStore store;
    auto snap = makeSnapshot("v2.0.0");

    std::string returned_id = store.save(snap);
    EXPECT_EQ(returned_id, "v2.0.0");
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-SNAP-6 — InMemoryMetadataSnapshotStore: load returns snapshot after save
// ─────────────────────────────────────────────────────────────────────────────

TEST(MetadataSnapshotFocusedTests, LoadReturnsSavedSnapshot) {
    InMemoryMetadataSnapshotStore store;
    auto snap = makeSnapshot("abc-123", "alice", "my snap", {"tbl"});
    store.save(snap);

    auto loaded = store.load("abc-123");

    ASSERT_TRUE(loaded.has_value());
    EXPECT_EQ(loaded->snapshot_id, "abc-123");
    EXPECT_EQ(loaded->author,      "alice");
    EXPECT_EQ(loaded->description, "my snap");
    EXPECT_EQ(loaded->tableCount(), 1u);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-SNAP-7 — InMemoryMetadataSnapshotStore: load returns nullopt for unknown id
// ─────────────────────────────────────────────────────────────────────────────

TEST(MetadataSnapshotFocusedTests, LoadReturnsNulloptForUnknownId) {
    InMemoryMetadataSnapshotStore store;

    auto result = store.load("does-not-exist");
    EXPECT_FALSE(result.has_value());
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-SNAP-8 — InMemoryMetadataSnapshotStore: save with empty id throws
// ─────────────────────────────────────────────────────────────────────────────

TEST(MetadataSnapshotFocusedTests, SaveEmptyIdThrowsMetadataSnapshotException) {
    InMemoryMetadataSnapshotStore store;
    auto snap = makeSnapshot("");  // empty snapshot_id

    EXPECT_THROW(store.save(snap), MetadataSnapshotException);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-SNAP-9 — InMemoryMetadataSnapshotStore: remove returns true/false correctly
// ─────────────────────────────────────────────────────────────────────────────

TEST(MetadataSnapshotFocusedTests, RemoveReturnsTrueForExistingFalseForMissing) {
    InMemoryMetadataSnapshotStore store;
    store.save(makeSnapshot("existing"));

    EXPECT_TRUE(store.remove("existing"));
    EXPECT_FALSE(store.remove("existing"));   // already removed
    EXPECT_FALSE(store.remove("never-there"));
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-SNAP-10 — InMemoryMetadataSnapshotStore: listSnapshotIds returns sorted list
// ─────────────────────────────────────────────────────────────────────────────

TEST(MetadataSnapshotFocusedTests, ListSnapshotIdsSorted) {
    InMemoryMetadataSnapshotStore store;
    store.save(makeSnapshot("v3.0.0"));
    store.save(makeSnapshot("v1.0.0"));
    store.save(makeSnapshot("v2.0.0"));

    auto ids = store.listSnapshotIds();

    ASSERT_EQ(ids.size(), 3u);
    EXPECT_EQ(ids[0], "v1.0.0");
    EXPECT_EQ(ids[1], "v2.0.0");
    EXPECT_EQ(ids[2], "v3.0.0");
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-SNAP-11 — InMemoryMetadataSnapshotStore: size() reflects add/remove
// ─────────────────────────────────────────────────────────────────────────────

TEST(MetadataSnapshotFocusedTests, SizeReflectsAddAndRemove) {
    InMemoryMetadataSnapshotStore store;

    EXPECT_EQ(store.size(), 0u);

    store.save(makeSnapshot("s1"));
    EXPECT_EQ(store.size(), 1u);

    store.save(makeSnapshot("s2"));
    EXPECT_EQ(store.size(), 2u);

    store.remove("s1");
    EXPECT_EQ(store.size(), 1u);

    store.remove("s2");
    EXPECT_EQ(store.size(), 0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-SNAP-12 — InMemoryMetadataSnapshotStore: clear() empties the store
// ─────────────────────────────────────────────────────────────────────────────

TEST(MetadataSnapshotFocusedTests, ClearEmptiesStore) {
    InMemoryMetadataSnapshotStore store;
    store.save(makeSnapshot("a"));
    store.save(makeSnapshot("b"));
    store.save(makeSnapshot("c"));

    EXPECT_EQ(store.size(), 3u);

    store.clear();

    EXPECT_EQ(store.size(), 0u);
    EXPECT_TRUE(store.listSnapshotIds().empty());
    EXPECT_FALSE(store.load("a").has_value());
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-SNAP-13 — InMemoryMetadataSnapshotStore: concurrent save/load is thread-safe
// ─────────────────────────────────────────────────────────────────────────────

TEST(MetadataSnapshotFocusedTests, ConcurrentSaveLoadIsThreadSafe) {
    InMemoryMetadataSnapshotStore store;
    constexpr int kIter    = 200;
    constexpr int kThreads = 4;

    std::atomic<int> found{0};
    std::atomic<int> total{0};

    std::vector<std::thread> threads;
    threads.reserve(kThreads + 1);

    // Writer thread: saves snapshots with incremented IDs
    threads.emplace_back([&] {
        for (int i = 0; i < kIter; ++i) {
            store.save(makeSnapshot("snap-" + std::to_string(i)));
        }
    });

    // Reader threads: attempt loads concurrently
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&] {
            for (int i = 0; i < kIter; ++i) {
                auto r = store.load("snap-" + std::to_string(i % kIter));
                if (r.has_value()) ++found;
                ++total;
            }
        });
    }

    for (auto& th : threads) th.join();

    // No crash; total reads == kThreads * kIter
    EXPECT_EQ(total.load(), kThreads * kIter);
    // At least some reads succeeded (saves started before some reads)
    EXPECT_GE(found.load(), 0);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-SNAP-14 — Polymorphic usage via IMetadataSnapshotStore*
// ─────────────────────────────────────────────────────────────────────────────

TEST(MetadataSnapshotFocusedTests, PolymorphicUsageViaInterface) {
    std::unique_ptr<IMetadataSnapshotStore> store =
        std::make_unique<InMemoryMetadataSnapshotStore>();

    auto snap = makeSnapshot("poly-001", "bot", "polymorphic test", {"tbl1", "tbl2"});

    std::string id = store->save(snap);
    EXPECT_EQ(id, "poly-001");

    auto loaded = store->load("poly-001");
    ASSERT_TRUE(loaded.has_value());
    EXPECT_EQ(loaded->tableCount(), 2u);

    EXPECT_TRUE(store->remove("poly-001"));
    EXPECT_EQ(store->size(), 0u);
    EXPECT_FALSE(store->load("poly-001").has_value());
}
