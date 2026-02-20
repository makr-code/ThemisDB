/**
 * Unit tests for Temporal Conflict Resolver
 * 
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include "temporal/temporal_conflict_resolver.h"

using namespace themisdb::temporal;
using namespace themisdb::replication;

class TemporalConflictResolverTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create test snapshots with different HLC timestamps
    }
    
    TemporalSnapshot createSnapshot(
        const std::string& id,
        uint64_t physical,
        uint32_t logical,
        const std::string& node_id,
        const std::string& value
    ) {
        TemporalSnapshot snapshot;
        snapshot.snapshot_id = id;
        snapshot.hlc.physical = physical;
        snapshot.hlc.logical = logical;
        snapshot.hlc.node_id = node_id;
        snapshot.source_node_id = node_id;
        snapshot.data = {{"field", value}};
        snapshot.checksum = "test_checksum";
        return snapshot;
    }
};

// ============================================================================
// Last-Write-Wins Tests
// ============================================================================

TEST_F(TemporalConflictResolverTest, LastWriteWins_RemoteNewer) {
    TemporalConflictResolver resolver(ConflictPolicy::LAST_WRITE_WINS);
    
    auto local = createSnapshot("local_1", 1000, 5, "node_a", "local_value");
    auto remote = createSnapshot("remote_1", 1001, 3, "node_b", "remote_value");
    
    auto winner = resolver.resolve(local, remote);
    
    EXPECT_EQ(winner.hlc.physical, 1001);
    EXPECT_EQ(winner.data["field"], "remote_value");
    
    auto stats = resolver.getStatistics();
    EXPECT_EQ(stats["total_conflicts"], 1);
    EXPECT_EQ(stats["lww_resolutions"], 1);
}

TEST_F(TemporalConflictResolverTest, LastWriteWins_LocalNewer) {
    TemporalConflictResolver resolver(ConflictPolicy::LAST_WRITE_WINS);
    
    auto local = createSnapshot("local_1", 1001, 5, "node_a", "local_value");
    auto remote = createSnapshot("remote_1", 1000, 3, "node_b", "remote_value");
    
    auto winner = resolver.resolve(local, remote);
    
    EXPECT_EQ(winner.hlc.physical, 1001);
    EXPECT_EQ(winner.data["field"], "local_value");
}

TEST_F(TemporalConflictResolverTest, LastWriteWins_EqualHLC_TiebreakerNodeID) {
    TemporalConflictResolver resolver(ConflictPolicy::LAST_WRITE_WINS);
    
    auto local = createSnapshot("local_1", 1000, 5, "node_a", "local_value");
    auto remote = createSnapshot("remote_1", 1000, 5, "node_b", "remote_value");
    
    auto winner = resolver.resolve(local, remote);
    
    // node_b > node_a lexicographically
    EXPECT_EQ(winner.hlc.node_id, "node_b");
    EXPECT_EQ(winner.data["field"], "remote_value");
}

TEST_F(TemporalConflictResolverTest, LastWriteWins_EqualPhysical_HigherLogical) {
    TemporalConflictResolver resolver(ConflictPolicy::LAST_WRITE_WINS);
    
    auto local = createSnapshot("local_1", 1000, 5, "node_a", "local_value");
    auto remote = createSnapshot("remote_1", 1000, 10, "node_b", "remote_value");
    
    auto winner = resolver.resolve(local, remote);
    
    // Higher logical counter wins (1000,10 > 1000,5)
    EXPECT_EQ(winner.hlc.physical, 1000);
    EXPECT_EQ(winner.hlc.logical, 10);
    EXPECT_EQ(winner.data["field"], "remote_value");
}

// ============================================================================
// First-Write-Wins Tests
// ============================================================================

TEST_F(TemporalConflictResolverTest, FirstWriteWins_LocalOlder) {
    TemporalConflictResolver resolver(ConflictPolicy::FIRST_WRITE_WINS);
    
    auto local = createSnapshot("local_1", 1000, 5, "node_a", "local_value");
    auto remote = createSnapshot("remote_1", 1001, 3, "node_b", "remote_value");
    
    auto winner = resolver.resolve(local, remote);
    
    EXPECT_EQ(winner.hlc.physical, 1000);
    EXPECT_EQ(winner.data["field"], "local_value");
    
    auto stats = resolver.getStatistics();
    EXPECT_EQ(stats["total_conflicts"], 1);
    EXPECT_EQ(stats["fww_resolutions"], 1);
}

TEST_F(TemporalConflictResolverTest, FirstWriteWins_RemoteOlder) {
    TemporalConflictResolver resolver(ConflictPolicy::FIRST_WRITE_WINS);
    
    auto local = createSnapshot("local_1", 1001, 5, "node_a", "local_value");
    auto remote = createSnapshot("remote_1", 1000, 3, "node_b", "remote_value");
    
    auto winner = resolver.resolve(local, remote);
    
    EXPECT_EQ(winner.hlc.physical, 1000);
    EXPECT_EQ(winner.data["field"], "remote_value");
}

TEST_F(TemporalConflictResolverTest, FirstWriteWins_EqualHLC_TiebreakerNodeID) {
    TemporalConflictResolver resolver(ConflictPolicy::FIRST_WRITE_WINS);
    
    auto local = createSnapshot("local_1", 1000, 5, "node_a", "local_value");
    auto remote = createSnapshot("remote_1", 1000, 5, "node_b", "remote_value");
    
    auto winner = resolver.resolve(local, remote);
    
    // node_a < node_b lexicographically, so local wins
    EXPECT_EQ(winner.hlc.node_id, "node_a");
    EXPECT_EQ(winner.data["field"], "local_value");
}

// ============================================================================
// Node Priority Tests
// ============================================================================

TEST_F(TemporalConflictResolverTest, NodePriority_LocalHigherPriority) {
    TemporalConflictResolver resolver(ConflictPolicy::NODE_PRIORITY);
    
    auto local = createSnapshot("local_1", 1000, 5, "node_a", "local_value");
    auto remote = createSnapshot("remote_1", 1001, 3, "node_b", "remote_value");
    
    auto winner = resolver.resolve(local, remote);
    
    // node_a < node_b lexicographically, so node_a has higher priority
    EXPECT_EQ(winner.source_node_id, "node_a");
    EXPECT_EQ(winner.data["field"], "local_value");
}

TEST_F(TemporalConflictResolverTest, NodePriority_RemoteHigherPriority) {
    TemporalConflictResolver resolver(ConflictPolicy::NODE_PRIORITY);
    
    auto local = createSnapshot("local_1", 1000, 5, "node_b", "local_value");
    auto remote = createSnapshot("remote_1", 1001, 3, "node_a", "remote_value");
    
    auto winner = resolver.resolve(local, remote);
    
    // node_a < node_b lexicographically, so node_a has higher priority
    EXPECT_EQ(winner.source_node_id, "node_a");
    EXPECT_EQ(winner.data["field"], "remote_value");
}

// ============================================================================
// Manual Resolution Tests
// ============================================================================

TEST_F(TemporalConflictResolverTest, ManualResolution_QueueConflict) {
    TemporalConflictResolver resolver(ConflictPolicy::MANUAL);
    
    auto local = createSnapshot("local_1", 1000, 5, "node_a", "local_value");
    auto remote = createSnapshot("remote_1", 1000, 5, "node_b", "remote_value");
    
    auto result = resolver.resolve(local, remote);
    
    // Should keep local until manual resolution
    EXPECT_EQ(result.hlc.node_id, "node_a");
    
    auto unresolved = resolver.getUnresolvedConflicts();
    EXPECT_EQ(unresolved.size(), 1);
    EXPECT_FALSE(unresolved[0].resolved);
}

TEST_F(TemporalConflictResolverTest, ManualResolution_ResolveManually) {
    TemporalConflictResolver resolver(ConflictPolicy::MANUAL);
    
    auto local = createSnapshot("local_1", 1000, 5, "node_a", "local_value");
    auto remote = createSnapshot("remote_1", 1000, 5, "node_b", "remote_value");
    
    resolver.resolve(local, remote);
    
    auto unresolved = resolver.getUnresolvedConflicts();
    ASSERT_EQ(unresolved.size(), 1);
    
    std::string conflict_id = unresolved[0].conflict_id;
    resolver.resolveManually(conflict_id, "remote");
    
    auto unresolved_after = resolver.getUnresolvedConflicts();
    EXPECT_EQ(unresolved_after.size(), 0);
    
    auto stats = resolver.getStatistics();
    EXPECT_EQ(stats["manual_resolutions"], 1);
}

// ============================================================================
// CRDT Merge Tests
// ============================================================================

TEST_F(TemporalConflictResolverTest, CRDTMerge_FallbackToLWW) {
    TemporalConflictResolver resolver(ConflictPolicy::CRDT_MERGE);
    
    auto local = createSnapshot("local_1", 1000, 5, "node_a", "local_value");
    auto remote = createSnapshot("remote_1", 1001, 3, "node_b", "remote_value");
    
    auto winner = resolver.resolve(local, remote);
    
    // Currently falls back to Last-Write-Wins
    EXPECT_EQ(winner.hlc.physical, 1001);
    EXPECT_EQ(winner.data["field"], "remote_value");
    
    auto stats = resolver.getStatistics();
    EXPECT_EQ(stats["crdt_merges"], 1);
}

TEST_F(TemporalConflictResolverTest, CRDTMerge_MergesFieldsFromBothSnapshots) {
    TemporalConflictResolver resolver(ConflictPolicy::CRDT_MERGE);

    // Local snapshot has field "a" only; remote has field "b" only.
    // CRDT merge should produce both fields, preferring the newer value
    // on any shared key.
    TemporalSnapshot local;
    local.snapshot_id = "local_multi";
    local.hlc         = {1000, 5, "node_a"};
    local.source_node_id = "node_a";
    local.data        = {{"a", 1}, {"shared", "old"}};
    local.checksum    = "c1";

    TemporalSnapshot remote;
    remote.snapshot_id = "remote_multi";
    remote.hlc         = {1001, 3, "node_b"};
    remote.source_node_id = "node_b";
    remote.data        = {{"b", 2}, {"shared", "new"}};
    remote.checksum    = "c2";

    auto winner = resolver.resolve(local, remote);

    // Both fields should be present
    ASSERT_TRUE(winner.data.contains("a"));
    ASSERT_TRUE(winner.data.contains("b"));
    // Shared field: newer (remote) wins
    EXPECT_EQ(winner.data["shared"], "new");
    EXPECT_EQ(winner.data["a"], 1);
    EXPECT_EQ(winner.data["b"], 2);
}

// ============================================================================
// Policy Override Tests
// ============================================================================

TEST_F(TemporalConflictResolverTest, PolicyOverride_DefaultLWW_OverrideFWW) {
    TemporalConflictResolver resolver(ConflictPolicy::LAST_WRITE_WINS);
    
    auto local = createSnapshot("local_1", 1000, 5, "node_a", "local_value");
    auto remote = createSnapshot("remote_1", 1001, 3, "node_b", "remote_value");
    
    // Override with First-Write-Wins
    auto winner = resolver.resolve(local, remote, ConflictPolicy::FIRST_WRITE_WINS);
    
    EXPECT_EQ(winner.hlc.physical, 1000);
    EXPECT_EQ(winner.data["field"], "local_value");
    
    auto stats = resolver.getStatistics();
    EXPECT_EQ(stats["fww_resolutions"], 1);
    EXPECT_EQ(stats["lww_resolutions"], 0);
}

// ============================================================================
// Statistics Tests
// ============================================================================

TEST_F(TemporalConflictResolverTest, Statistics_MultipleConflicts) {
    TemporalConflictResolver resolver(ConflictPolicy::LAST_WRITE_WINS);
    
    auto local1 = createSnapshot("local_1", 1000, 5, "node_a", "local_value");
    auto remote1 = createSnapshot("remote_1", 1001, 3, "node_b", "remote_value");
    
    auto local2 = createSnapshot("local_2", 2000, 5, "node_a", "local_value2");
    auto remote2 = createSnapshot("remote_2", 2001, 3, "node_b", "remote_value2");
    
    resolver.resolve(local1, remote1);
    resolver.resolve(local2, remote2);
    
    auto stats = resolver.getStatistics();
    EXPECT_EQ(stats["total_conflicts"], 2);
    EXPECT_EQ(stats["lww_resolutions"], 2);
}

// ============================================================================
// TemporalSnapshot JSON Serialization Tests
// ============================================================================

TEST_F(TemporalConflictResolverTest, TemporalSnapshot_ToJson) {
    auto snapshot = createSnapshot("snap_1", 1000, 5, "node_a", "test_value");
    
    auto json = snapshot.toJson();
    
    EXPECT_EQ(json["snapshot_id"], "snap_1");
    EXPECT_EQ(json["hlc"]["physical"], 1000);
    EXPECT_EQ(json["hlc"]["logical"], 5);
    EXPECT_EQ(json["hlc"]["node_id"], "node_a");
    EXPECT_EQ(json["source_node_id"], "node_a");
    EXPECT_EQ(json["data"]["field"], "test_value");
}

TEST_F(TemporalConflictResolverTest, TemporalSnapshot_FromJson) {
    nlohmann::json json = {
        {"snapshot_id", "snap_1"},
        {"hlc", {
            {"physical", 1000},
            {"logical", 5},
            {"node_id", "node_a"}
        }},
        {"source_node_id", "node_a"},
        {"data", {{"field", "test_value"}}},
        {"checksum", "test_checksum"}
    };
    
    auto snapshot_opt = TemporalSnapshot::fromJson(json);
    
    ASSERT_TRUE(snapshot_opt.has_value());
    auto snapshot = snapshot_opt.value();
    
    EXPECT_EQ(snapshot.snapshot_id, "snap_1");
    EXPECT_EQ(snapshot.hlc.physical, 1000);
    EXPECT_EQ(snapshot.hlc.logical, 5);
    EXPECT_EQ(snapshot.hlc.node_id, "node_a");
    EXPECT_EQ(snapshot.source_node_id, "node_a");
    EXPECT_EQ(snapshot.data["field"], "test_value");
}
