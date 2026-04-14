/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            example_replication.cpp                            ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-04-14 18:36:11                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   92.0/100                                       ║
    • Total Lines:     474                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • fc77bc508d  2026-04-12  [MODULE] replication — perf tests for design constraints,... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file example_replication.cpp
 * @brief ThemisDB Replication Module — Comprehensive Usage Example
 *
 * Demonstrates the main building blocks of the ThemisDB Replication module:
 *
 *   1. WALManager — append, read, and inspect Write-Ahead Log entries
 *   2. ReplicationManager — leader-follower setup, replica management,
 *      WAL-entry replication, and cluster health inspection
 *   3. VectorClock — causal ordering for multi-master writes
 *   4. HybridLogicalClock (HLC) — per-node logical timestamps
 *   5. BidirectionalReplicationManager — active-active replication with
 *      configurable per-collection conflict resolution
 *   6. MultiTierReplicationManager — hierarchical tiering with automatic
 *      promotion/demotion based on access patterns
 *   7. ReplicationAnalytics — lag history, bottleneck detection, and
 *      Prometheus metrics export
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include "replication/replication_manager.h"
#include "replication/multi_master_replication.h"
#include "replication/multi_tier_replication.h"

#include <chrono>
#include <filesystem>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

using namespace themisdb::replication;

// ============================================================================
// Helpers
// ============================================================================

static ReplicationConfig makeLeaderConfig(const std::string& wal_dir) {
    ReplicationConfig cfg;
    cfg.enabled                      = true;
    cfg.mode                         = ReplicationMode::SEMI_SYNC;
    cfg.heartbeat_interval_ms        = 500;
    cfg.election_timeout_min_ms      = 1500;
    cfg.election_timeout_max_ms      = 3000;
    cfg.batch_size                   = 200;
    cfg.batch_timeout_ms             = 50;
    cfg.wal_directory                = wal_dir;
    cfg.min_sync_replicas            = 1;
    cfg.wal_sync_on_commit           = false;
    cfg.enable_leader_lease          = true;
    cfg.leader_lease_duration_ms     = 1200;
    cfg.failure_detection_timeout_ms = 3000;
    cfg.degraded_lag_threshold_ms    = 5000;
    cfg.enable_wal_compression       = true;
    cfg.wal_compression_algorithm    = "zstd";
    cfg.wal_compression_level        = 3;
    return cfg;
}

static WALEntry makeEntry(uint64_t seq, const std::string& op,
                          const std::string& collection,
                          const std::string& doc_id,
                          const std::string& payload) {
    WALEntry e;
    e.sequence_number = seq;
    e.term            = 1;
    e.timestamp       = std::chrono::system_clock::now();
    e.operation       = op;
    e.collection      = collection;
    e.document_id     = doc_id;
    e.data            = payload;
    return e;
}

// ============================================================================
// 1. WALManager — Write-Ahead Log basics
// ============================================================================

static void demo_wal_manager(const std::string& wal_dir) {
    std::cout << "\n=== 1. WALManager ===\n";

    ReplicationConfig cfg = makeLeaderConfig(wal_dir);
    WALManager wal(cfg);

    // Append a few WAL entries
    const std::vector<std::pair<std::string, std::string>> ops = {
        {"INSERT", R"({"name":"Alice","role":"admin"})"},
        {"INSERT", R"({"name":"Bob","role":"viewer"})"},
        {"UPDATE", R"({"name":"Alice","role":"superadmin"})"},
        {"DELETE", R"({})"},
    };

    std::cout << "  Appending " << ops.size() << " WAL entries...\n";
    uint64_t last_seq = 0;
    for (size_t i = 0; i < ops.size(); ++i) {
        auto entry = makeEntry(
            static_cast<uint64_t>(i + 1),
            ops[i].first,
            "users",
            "user_" + std::to_string(i + 1),
            ops[i].second
        );
        last_seq = wal.append(entry);
    }
    std::cout << "  Last assigned sequence: " << last_seq << "\n";
    std::cout << "  Current sequence: " << wal.getCurrentSequence() << "\n";

    // Read entries back
    auto entries = wal.readFrom(1, 10);
    std::cout << "  Entries read back: " << entries.size() << "\n";
    for (const auto& e : entries) {
        std::cout << "    [" << e.sequence_number << "] "
                  << e.operation << " users/" << e.document_id << "\n";
    }

    // Serialize / deserialize round-trip
    if (!entries.empty()) {
        auto bytes = entries[0].serialize();
        auto restored = WALEntry::deserialize(bytes);
        if (restored) {
            std::cout << "  Serialize/deserialize round-trip OK: seq="
                      << restored->sequence_number << "\n";
        }
    }
}

// ============================================================================
// 2. ReplicationManager — leader setup + replica management
// ============================================================================

static void demo_replication_manager(const std::string& wal_dir) {
    std::cout << "\n=== 2. ReplicationManager ===\n";

    ReplicationConfig cfg = makeLeaderConfig(wal_dir);
    ReplicationManager mgr(cfg);

    if (!mgr.initialize()) {
        std::cerr << "  ERROR: Failed to initialize ReplicationManager\n";
        return;
    }
    std::cout << "  ReplicationManager initialized\n";
    std::cout << "  Role: " << (mgr.getRole() == ReplicationRole::LEADER
                                    ? "LEADER" : "FOLLOWER") << "\n";

    // Add two follower replicas
    ReplicaInfo follower1;
    follower1.node_id                = "follower-1";
    follower1.endpoint               = "10.0.0.2:7000";
    follower1.role                   = ReplicationRole::FOLLOWER;
    follower1.is_voting_member       = true;
    follower1.datacenter             = "dc-west";
    follower1.priority               = 5;
    follower1.last_applied_sequence  = 0;

    ReplicaInfo follower2;
    follower2.node_id                = "follower-2";
    follower2.endpoint               = "10.0.0.3:7000";
    follower2.role                   = ReplicationRole::FOLLOWER;
    follower2.is_voting_member       = true;
    follower2.datacenter             = "dc-east";
    follower2.priority               = 3;
    follower2.last_applied_sequence  = 0;

    mgr.addReplica(follower1);
    mgr.addReplica(follower2);

    // Add a witness node (vote-only, no data) for quorum in edge deployments
    mgr.addWitnessNode("witness-1", "10.0.0.4:7001");

    auto replicas = mgr.getReplicas();
    std::cout << "  Registered replicas: " << replicas.size() << "\n";
    for (const auto& r : replicas) {
        std::cout << "    " << r.node_id << " @ " << r.endpoint
                  << " [" << r.datacenter << "]\n";
    }

    // Replicate some writes
    std::cout << "  Replicating 3 writes...\n";
    for (int i = 1; i <= 3; ++i) {
        WALEntry e = makeEntry(
            static_cast<uint64_t>(i),
            "INSERT", "orders",
            "order_" + std::to_string(i),
            R"({"amount":)" + std::to_string(i * 100) + R"(,"status":"pending"})"
        );
        bool ok = mgr.replicate(e);
        std::cout << "    Write " << i << ": " << (ok ? "OK" : "FAILED") << "\n";
    }

    // Prometheus metrics
    std::string prom = mgr.exportPrometheusMetrics();
    if (!prom.empty()) {
        std::cout << "  Prometheus metrics available (" << prom.size() << " bytes)\n";
    }

    mgr.shutdown();
    std::cout << "  ReplicationManager shut down\n";
}

// ============================================================================
// 3. VectorClock + HybridLogicalClock — multi-master conflict detection
// ============================================================================

static void demo_vector_clock_and_hlc() {
    std::cout << "\n=== 3. VectorClock + HybridLogicalClock ===\n";

    // ── VectorClock ──────────────────────────────────────────────────────────
    VectorClock vc_node_a("node-a");
    VectorClock vc_node_b("node-b");

    vc_node_a.increment("node-a");   // a performs a write
    vc_node_b.increment("node-b");   // b performs a write concurrently

    int cmp = vc_node_a.compare(vc_node_b);
    std::cout << "  VectorClock comparison (a vs b): "
              << (cmp == 0 ? "CONCURRENT (expected)" :
                  cmp < 0  ? "a happens-before b"    : "b happens-before a")
              << "\n";

    // After b receives a's clock → causal ordering established
    vc_node_b.merge(vc_node_a);
    vc_node_b.increment("node-b");

    int cmp2 = vc_node_a.compare(vc_node_b);
    std::cout << "  After merge+increment: a "
              << (cmp2 < 0 ? "happens-before b (expected)" :
                  cmp2 > 0 ? "happens-after b"              : "concurrent with b")
              << "\n";

    // ── HybridLogicalClock ───────────────────────────────────────────────────
    HybridLogicalClock hlc_primary("primary");
    HybridLogicalClock hlc_secondary("secondary");

    auto t1 = hlc_primary.now();
    auto t2 = hlc_primary.now();
    std::cout << "  HLC on primary: t1=" << t1.toString()
              << "  t2=" << t2.toString() << "\n";
    std::cout << "  Monotonic? " << (!(t2 < t1) ? "YES" : "NO") << "\n";

    // Secondary receives primary's timestamp and advances its own clock
    auto t3 = hlc_secondary.receive(t2);
    std::cout << "  Secondary after receive: t3=" << t3.toString() << "\n";
    std::cout << "  t3 >= t2? " << (!(t3 < t2) ? "YES (causal ordering preserved)" : "NO") << "\n";
}

// ============================================================================
// 4. BidirectionalReplicationManager — active-active setup
// ============================================================================

static void demo_bidirectional_replication() {
    std::cout << "\n=== 4. BidirectionalReplicationManager (active-active) ===\n";

    BidirectionalReplicationManager::BidiConfig cfg;
    cfg.local_node_id          = "us-west-1";
    cfg.remote_node_id         = "us-east-1";
    cfg.remote_endpoint        = "us-east-1.example.com:7000";
    cfg.default_strategy       = ConflictResolution::LAST_WRITE_WINS;
    cfg.track_origin           = true;
    cfg.replicate_foreign_changes = false;  // prevent replication loops
    cfg.bidirectional_sync     = true;
    cfg.replicate_ddl          = true;
    cfg.sync_interval_ms       = 1000;

    // Per-collection conflict resolution override
    cfg.collection_strategies["financial_ledger"] = ConflictResolution::VECTOR_CLOCK;
    cfg.collection_strategies["user_preferences"] = ConflictResolution::FIRST_WRITE_WINS;

    BidirectionalReplicationManager bidi(cfg);

    if (!bidi.start()) {
        std::cerr << "  ERROR: Failed to start BidirectionalReplicationManager\n";
        return;
    }
    std::cout << "  BidirectionalReplicationManager started\n";
    std::cout << "  Local:  " << cfg.local_node_id << "\n";
    std::cout << "  Remote: " << cfg.remote_node_id << " @ " << cfg.remote_endpoint << "\n";

    // Submit some local writes
    for (int i = 1; i <= 4; ++i) {
        uint64_t seq = bidi.submitWrite(
            "txn_" + std::to_string(i),
            "financial_ledger",
            "INSERT",
            R"({"amount":)" + std::to_string(i * 500) + R"(,"currency":"USD"})"
        );
        std::cout << "    Local write txn_" << i << " → local_seq=" << seq << "\n";
    }

    // Simulate a remote write arriving (build a BidiWriteEntry from the peer)
    BidirectionalReplicationManager::BidiWriteEntry remote_entry;
    remote_entry.document_id  = "txn_remote_1";
    remote_entry.collection   = "financial_ledger";
    remote_entry.operation    = "INSERT";
    remote_entry.data         = R"({"amount":9999,"currency":"EUR"})";
    remote_entry.origin_node  = cfg.remote_node_id;
    remote_entry.origin_seq   = 1;
    remote_entry.timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    bool accepted = bidi.applyRemoteWrite(remote_entry);
    std::cout << "  Remote write accepted: " << (accepted ? "YES" : "NO") << "\n";

    // Check synchronisation status
    auto status = bidi.getSyncStatus();
    std::cout << "  SyncStatus: local_seq=" << status.local_sequence
              << " remote_seq=" << status.remote_sequence
              << " lag=" << status.lag_ms << "ms"
              << " synchronized=" << (status.is_synchronized ? "YES" : "NO") << "\n";

    bidi.stop();
    std::cout << "  BidirectionalReplicationManager stopped\n";
}

// ============================================================================
// 5. MultiTierReplicationManager — tiered replication
// ============================================================================

static void demo_multi_tier_replication() {
    std::cout << "\n=== 5. MultiTierReplicationManager ===\n";

    MultiTierConfig cfg;
    cfg.auto_tiering_enabled   = true;
    cfg.hot_access_threshold   = 10;  // ≥ 10 accesses → Tier 1
    cfg.cold_access_threshold  = 2;   // < 2 accesses  → Tier 3

    MultiTierReplicationManager mgr(cfg);

    // Explicit tier assignments
    mgr.assignTier("financial_transactions", ReplicationTier::TIER_1_CRITICAL);
    mgr.assignTier("user_profiles",          ReplicationTier::TIER_2_STANDARD);
    mgr.assignTier("audit_logs",             ReplicationTier::TIER_3_ARCHIVAL);

    // Inspect assigned configs
    const std::vector<std::pair<std::string, ReplicationTier>> assignments = {
        {"financial_transactions", ReplicationTier::TIER_1_CRITICAL},
        {"user_profiles",          ReplicationTier::TIER_2_STANDARD},
        {"audit_logs",             ReplicationTier::TIER_3_ARCHIVAL},
    };

    for (const auto& [col, expected_tier] : assignments) {
        auto tier_cfg = mgr.getTierConfig(col);
        std::cout << "  Collection '" << col << "':"
                  << " tier=" << static_cast<int>(tier_cfg.tier)
                  << " replicas=" << tier_cfg.replica_count
                  << " max_latency=" << tier_cfg.max_latency_ms << "ms\n";
        (void)expected_tier;
    }

    // Demonstrate auto-tiering: simulate hot access to "product_catalog"
    mgr.assignTier("product_catalog", ReplicationTier::TIER_2_STANDARD);
    for (int i = 0; i < 15; ++i) {
        mgr.recordAccess("product_catalog");
    }
    mgr.evaluateTierPromotion("product_catalog");

    auto promoted_cfg = mgr.getTierConfig("product_catalog");
    std::cout << "  'product_catalog' after 15 accesses: tier="
              << static_cast<int>(promoted_cfg.tier)
              << " (expected: " << static_cast<int>(ReplicationTier::TIER_1_CRITICAL) << ")\n";

    // Stats summary
    auto stats = mgr.getStats();
    std::cout << "  Total promotions: " << stats.total_promotions << "\n";
    std::cout << "  Total demotions:  " << stats.total_demotions  << "\n";
}

// ============================================================================
// 6. ReplicationAnalytics — observability
// ============================================================================

static void demo_replication_analytics() {
    std::cout << "\n=== 6. ReplicationAnalytics ===\n";

    ReplicationAnalytics analytics;

    // Record realistic lag samples for two replicas
    const std::string r1 = "follower-1";
    const std::string r2 = "follower-2";

    // follower-1: stable, low lag
    for (int i = 0; i < 20; ++i) {
        analytics.recordLag(r1, 5 + (i % 3));
    }

    // follower-2: spike at sample 15 → triggers LAG_SPIKE insight
    for (int i = 0; i < 20; ++i) {
        int64_t lag = (i == 15) ? 120 : 8 + (i % 5);
        analytics.recordLag(r2, lag);
    }

    // Lag history for follower-1
    auto hist = analytics.getLagHistory(r1, std::chrono::hours(24));
    std::cout << "  follower-1 lag history:"
              << " avg=" << hist.avg_lag_ms << "ms"
              << " p95=" << hist.p95_lag_ms << "ms"
              << " p99=" << hist.p99_lag_ms << "ms"
              << " max=" << hist.max_lag_ms << "ms\n";

    // Insights
    auto insights = analytics.getInsights();
    std::cout << "  Insights detected: " << insights.size() << "\n";
    for (const auto& ins : insights) {
        std::cout << "    [" << ins.type << "] " << ins.description << "\n";
        if (!ins.recommendation.empty()) {
            std::cout << "      → " << ins.recommendation << "\n";
        }
    }

    // Bottleneck detection
    auto bottlenecks = analytics.detectBottlenecks();
    std::cout << "  Bottlenecks detected: " << bottlenecks.size() << "\n";
    for (const auto& b : bottlenecks) {
        std::cout << "    " << b.replica_id << " — " << b.bottleneck_type
                  << " (severity=" << b.severity << ")\n";
    }

    // Prometheus export
    std::string prom = analytics.exportPrometheusMetrics();
    std::cout << "  Prometheus metrics exported: " << prom.size() << " bytes\n";
}

// ============================================================================
// main
// ============================================================================

int main() {
    std::cout << "========================================\n";
    std::cout << "  ThemisDB Replication Module Example\n";
    std::cout << "========================================\n";

    // Temporary WAL directories (cleaned up on exit)
    const std::string wal_root = "/tmp/themisdb_replication_example";
    std::filesystem::remove_all(wal_root);
    std::filesystem::create_directories(wal_root + "/wal");
    std::filesystem::create_directories(wal_root + "/mgr");

    demo_wal_manager(wal_root + "/wal");
    demo_replication_manager(wal_root + "/mgr");
    demo_vector_clock_and_hlc();
    demo_bidirectional_replication();
    demo_multi_tier_replication();
    demo_replication_analytics();

    std::filesystem::remove_all(wal_root);

    std::cout << "\n========================================\n";
    std::cout << "  All demos completed successfully.\n";
    std::cout << "========================================\n";

    return 0;
}
