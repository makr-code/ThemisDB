#include <gtest/gtest.h>

#include "replication/logical_replication.h"

#include <filesystem>
#include <random>
#include <sstream>
#include <chrono>
#include <stdexcept>

using namespace themisdb::replication;

namespace {
ReplicationConfig makeConfig(const std::string& wal_dir) {
    ReplicationConfig cfg;
    cfg.enabled = true;
    cfg.mode = ReplicationMode::ASYNC;
    cfg.heartbeat_interval_ms = 100;
    cfg.election_timeout_min_ms = 150;
    cfg.election_timeout_max_ms = 300;
    cfg.batch_size = 32;
    cfg.batch_timeout_ms = 50;
    cfg.wal_directory = wal_dir;
    cfg.wal_sync_on_commit = false;
    return cfg;
}

struct TempDir {
    TempDir() {
        auto base = std::filesystem::temp_directory_path();
        std::random_device rd = {};
        std::mt19937_64 gen(rd());
        std::uniform_int_distribution<uint64_t> dist;
        int attempts = 0;
        do {
            std::ostringstream oss = {};
            oss << "themis_logical_repl-" << std::hex
                << std::chrono::steady_clock::now().time_since_epoch().count() << "-" << dist(gen);
            path = (base / oss.str()).string();
            if (++attempts > 10) {
                throw std::runtime_error("Failed to create unique temporary directory for test");
            }
        } while (std::filesystem::exists(path));
        std::filesystem::remove_all(path);
        std::filesystem::create_directories(path);
    }
    ~TempDir() { std::filesystem::remove_all(path); }
    std::string path = {};
};
}  // namespace

TEST(LogicalReplicationManagerTest, AppliesIncludeExcludeAndRowFilter) {
    TempDir td;
    auto wal = std::make_shared<WALManager>(makeConfig(td.path));
    LogicalReplicationManager::Config cfg;
    cfg.wal_directory = td.path;
    LogicalReplicationManager mgr(wal, cfg);

    LogicalReplicationManager::ReplicationFilter filter;
    filter.include_collections = {"orders"};
    filter.row_filter_expression = "tenant == 'acme'";

    mgr.createSlot("slot_filters", "json", filter);

    WALEntry entry{};
    entry.sequence_number = 1;
    entry.operation = "INSERT";
    entry.collection = "orders";
    entry.document_id = "o-1";
    entry.data = R"({"tenant":"acme","value":1})";

    mgr.onWALEntryApplied(entry);

    auto changes = mgr.readChanges("slot_filters", 10);
    ASSERT_EQ(changes.size(), 1u);
    EXPECT_EQ(changes[0].collection, "orders");
    EXPECT_EQ(changes[0].new_data["tenant"], "acme");

    WALEntry filtered{};
    filtered.sequence_number = 2;
    filtered.operation = "INSERT";
    filtered.collection = "orders";
    filtered.document_id = "o-2";
    filtered.data = R"({"tenant":"other","value":2})";
    mgr.onWALEntryApplied(filtered);

    auto none = mgr.readChanges("slot_filters", 10);
    EXPECT_TRUE(none.empty());
}

TEST(LogicalReplicationManagerTest, ReplicatesDDLWhenEnabled) {
    TempDir td;
    auto wal = std::make_shared<WALManager>(makeConfig(td.path));
    LogicalReplicationManager::Config cfg;
    cfg.wal_directory = td.path;
    LogicalReplicationManager mgr(wal, cfg);

    mgr.createSlot("slot_ddl", "json", {});

    mgr.recordDDLChange("CREATE COLLECTION foo", "2026-03-13", 5);
    auto changes = mgr.readChanges("slot_ddl", 5);
    ASSERT_EQ(changes.size(), 1u);
    EXPECT_EQ(changes[0].type, LogicalChange::Type::DDL);
    EXPECT_EQ(changes[0].ddl_statement, "CREATE COLLECTION foo");
    EXPECT_EQ(changes[0].schema_version, "2026-03-13");
}

TEST(LogicalReplicationManagerTest, AppliesTransformAndCrossVersionMetadata) {
    TempDir td;
    auto wal = std::make_shared<WALManager>(makeConfig(td.path));

    LogicalReplicationManager::Config cfg;
    cfg.wal_directory = td.path;
    cfg.source_version = "v1.5";
    cfg.target_version = "v1.6";
    cfg.transform = [](LogicalChange& change) {
        if (change.new_data.is_object()) {
            change.new_data["transformed"] = true;
        }
    };

    LogicalReplicationManager mgr(wal, cfg);
    mgr.createSlot("slot_transform", "json", {});

    WALEntry entry{};
    entry.sequence_number = 10;
    entry.operation = "UPDATE";
    entry.collection = "customers";
    entry.document_id = "c-1";
    entry.data = R"({"name":"alice"})";
    mgr.onWALEntryApplied(entry);

    auto changes = mgr.readChanges("slot_transform", 10);
    ASSERT_EQ(changes.size(), 1u);
    EXPECT_EQ(changes[0].schema_version, "v1.6");
    EXPECT_EQ(changes[0].source_version, "v1.5");
    EXPECT_TRUE(changes[0].new_data["transformed"].get<bool>());
}

TEST(LogicalReplicationManagerTest, InitialSyncSkipsConflictingChanges) {
    TempDir td;
    auto wal = std::make_shared<WALManager>(makeConfig(td.path));
    LogicalReplicationManager::Config cfg;
    cfg.wal_directory = td.path;
    LogicalReplicationManager mgr(wal, cfg);

    LogicalChange snapshot;
    snapshot.collection = "users";
    snapshot.new_data = {{"_id", "u1"}, {"value", 1}};
    snapshot.lsn = 0;

    mgr.createSlot("slot_init", "json", {}, true, {snapshot});

    WALEntry duplicate{};
    duplicate.sequence_number = 0;
    duplicate.operation = "INSERT";
    duplicate.collection = "users";
    duplicate.document_id = "u1";
    duplicate.data = R"({"_id":"u1","value":99})";
    mgr.onWALEntryApplied(duplicate);

    WALEntry new_entry{};
    new_entry.sequence_number = 1;
    new_entry.operation = "INSERT";
    new_entry.collection = "users";
    new_entry.document_id = "u2";
    new_entry.data = R"({"_id":"u2","value":2})";
    mgr.onWALEntryApplied(new_entry);

    auto changes = mgr.readChanges("slot_init", 10);
    ASSERT_EQ(changes.size(), 2u);
    EXPECT_EQ(changes[0].type, LogicalChange::Type::SNAPSHOT);
    EXPECT_EQ(changes[0].new_data["_id"], "u1");
    EXPECT_EQ(changes[1].collection, "users");
    EXPECT_EQ(changes[1].new_data["_id"], "u2");
}

// ---------------------------------------------------------------------------
// Parallel decoding tests
// ---------------------------------------------------------------------------

TEST(LogicalReplicationManagerTest, ParallelDecodingDispatchesToAllSlots) {
    TempDir td;
    auto wal = std::make_shared<WALManager>(makeConfig(td.path));

    LogicalReplicationManager::Config cfg;
    cfg.wal_directory = td.path;
    cfg.parallel_decoding = true;
    LogicalReplicationManager mgr(wal, cfg);

    // Create three independent slots — parallel path requires slots_copy.size() > 1
    mgr.createSlot("slot_a", "json", {});
    mgr.createSlot("slot_b", "json", {});
    mgr.createSlot("slot_c", "json", {});

    WALEntry entry{};
    entry.sequence_number = 1;
    entry.operation = "INSERT";
    entry.collection = "products";
    entry.document_id = "p-1";
    entry.data = R"({"sku":"ABC"})";
    mgr.onWALEntryApplied(entry);

    // Every slot must have received the change
    auto a = mgr.readChanges("slot_a", 10);
    auto b = mgr.readChanges("slot_b", 10);
    auto c = mgr.readChanges("slot_c", 10);
    ASSERT_EQ(a.size(), 1u);
    ASSERT_EQ(b.size(), 1u);
    ASSERT_EQ(c.size(), 1u);
    EXPECT_EQ(a[0].collection, "products");
    EXPECT_EQ(b[0].collection, "products");
    EXPECT_EQ(c[0].collection, "products");
}

TEST(LogicalReplicationManagerTest, ParallelDecodingAccumulatesStats) {
    TempDir td;
    auto wal = std::make_shared<WALManager>(makeConfig(td.path));

    LogicalReplicationManager::Config cfg;
    cfg.wal_directory = td.path;
    cfg.parallel_decoding = true;
    LogicalReplicationManager mgr(wal, cfg);

    mgr.createSlot("slot_x", "json", {});
    mgr.createSlot("slot_y", "json", {});

    // Apply two WAL entries so that stats are non-trivial
    for (int i = 1; i <= 2; ++i) {
        WALEntry e{};
        e.sequence_number = static_cast<uint64_t>(i);
        e.operation = "UPDATE";
        e.collection = "orders";
        e.document_id = "o-" + std::to_string(i);
        e.data = R"({"status":"shipped"})";
        mgr.onWALEntryApplied(e);
    }

    // 2 entries × 2 slots = 4 changes_enqueued
    auto stats = mgr.getStats();
    EXPECT_EQ(stats.changes_enqueued, 4u);
    EXPECT_EQ(stats.filtered_out, 0u);
}

TEST(LogicalReplicationManagerTest, ParallelDecodingFallsBackToSequentialWithSingleSlot) {
    TempDir td;
    auto wal = std::make_shared<WALManager>(makeConfig(td.path));

    LogicalReplicationManager::Config cfg;
    cfg.wal_directory = td.path;
    cfg.parallel_decoding = true;  // parallel enabled but only one slot → sequential path
    LogicalReplicationManager mgr(wal, cfg);

    mgr.createSlot("only_slot", "json", {});

    WALEntry e{};
    e.sequence_number = 42;
    e.operation = "DELETE";
    e.collection = "sessions";
    e.document_id = "s-99";
    e.data = "{}";
    mgr.onWALEntryApplied(e);

    auto changes = mgr.readChanges("only_slot", 10);
    ASSERT_EQ(changes.size(), 1u);
    EXPECT_EQ(changes[0].type, LogicalChange::Type::DELETE);
}

TEST(LogicalReplicationManagerTest, DisabledParallelDecodingUsesSequentialPath) {
    TempDir td;
    auto wal = std::make_shared<WALManager>(makeConfig(td.path));

    LogicalReplicationManager::Config cfg;
    cfg.wal_directory = td.path;
    cfg.parallel_decoding = false;  // explicitly disabled
    LogicalReplicationManager mgr(wal, cfg);

    mgr.createSlot("seq_slot_1", "json", {});
    mgr.createSlot("seq_slot_2", "json", {});

    WALEntry e{};
    e.sequence_number = 10;
    e.operation = "INSERT";
    e.collection = "logs";
    e.document_id = "l-1";
    e.data = R"({"level":"info"})";
    mgr.onWALEntryApplied(e);

    // Both slots must still receive the change via the sequential path
    auto c1 = mgr.readChanges("seq_slot_1", 10);
    auto c2 = mgr.readChanges("seq_slot_2", 10);
    ASSERT_EQ(c1.size(), 1u);
    ASSERT_EQ(c2.size(), 1u);
    EXPECT_EQ(c1[0].collection, "logs");
    EXPECT_EQ(c2[0].collection, "logs");
}
