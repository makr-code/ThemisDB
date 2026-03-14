/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_logical_replication.cpp                       ║
  Version:         0.0.1                                              ║
  Last Modified:   2026-03-13                                         ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   98.0/100                                       ║
    • Total Lines:     ~190                                           ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include <gtest/gtest.h>

#include "replication/logical_replication.h"

#include <filesystem>
#include <random>
#include <sstream>

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
        std::random_device rd;
        std::mt19937_64 gen(rd());
        std::uniform_int_distribution<uint64_t> dist;
        std::ostringstream oss;
        oss << "themis_logical_repl-" << std::hex << dist(gen);
        path = (base / oss.str()).string();
        std::filesystem::remove_all(path);
        std::filesystem::create_directories(path);
    }
    ~TempDir() { std::filesystem::remove_all(path); }
    std::string path;
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
