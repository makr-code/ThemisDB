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

// ============================================================================
// Conflict Audit Trail Tests
// ============================================================================

TEST_F(TemporalConflictResolverTest, ConflictHistory_InitiallyEmpty) {
    TemporalConflictResolver resolver(ConflictPolicy::LAST_WRITE_WINS);
    EXPECT_TRUE(resolver.getConflictHistory().empty());
}

TEST_F(TemporalConflictResolverTest, ConflictHistory_RecordsResolvedConflicts) {
    TemporalConflictResolver resolver(ConflictPolicy::LAST_WRITE_WINS);

    auto local  = createSnapshot("l1", 1000, 5, "node_a", "v1");
    auto remote = createSnapshot("r1", 1001, 5, "node_b", "v2");
    resolver.resolve(local, remote);

    auto history = resolver.getConflictHistory();
    ASSERT_EQ(history.size(), 1u);
    EXPECT_EQ(history[0].winner, "remote");
    EXPECT_TRUE(history[0].resolved);
}

TEST_F(TemporalConflictResolverTest, ConflictHistory_IncludesUnresolved) {
    TemporalConflictResolver resolver(ConflictPolicy::MANUAL);

    auto local  = createSnapshot("l1", 1000, 5, "node_a", "v1");
    auto remote = createSnapshot("r1", 1001, 5, "node_b", "v2");
    resolver.resolve(local, remote); // queued, not resolved

    auto history = resolver.getConflictHistory();
    ASSERT_EQ(history.size(), 1u);
    EXPECT_FALSE(history[0].resolved);
}

TEST_F(TemporalConflictResolverTest, ConflictHistory_AfterManualResolution) {
    TemporalConflictResolver resolver(ConflictPolicy::MANUAL);

    auto local  = createSnapshot("l1", 1000, 5, "node_a", "v1");
    auto remote = createSnapshot("r1", 1001, 5, "node_b", "v2");
    resolver.resolve(local, remote);

    auto unresolved = resolver.getUnresolvedConflicts();
    ASSERT_EQ(unresolved.size(), 1u);
    resolver.resolveManually(unresolved[0].conflict_id, "local");

    auto history = resolver.getConflictHistory();
    ASSERT_EQ(history.size(), 1u);
    EXPECT_TRUE(history[0].resolved);
    EXPECT_EQ(history[0].winner, "local");
}

TEST_F(TemporalConflictResolverTest, ConflictHistory_MultipleConflictsAccumulate) {
    TemporalConflictResolver resolver(ConflictPolicy::LAST_WRITE_WINS);

    for (int i = 0; i < 5; ++i) {
        auto local  = createSnapshot("l", 1000 + i, 0, "node_a", "v");
        auto remote = createSnapshot("r", 1001 + i, 0, "node_b", "v");
        resolver.resolve(local, remote);
    }

    EXPECT_EQ(resolver.getConflictHistory().size(), 5u);
}

TEST_F(TemporalConflictResolverTest, ExportAuditLog_IsJsonArray) {
    TemporalConflictResolver resolver(ConflictPolicy::LAST_WRITE_WINS);

    auto local  = createSnapshot("l1", 1000, 5, "node_a", "v1");
    auto remote = createSnapshot("r1", 1001, 5, "node_b", "v2");
    resolver.resolve(local, remote);

    auto log = resolver.exportAuditLog();
    EXPECT_TRUE(log.is_array());
    ASSERT_EQ(log.size(), 1u);

    EXPECT_TRUE(log[0].contains("conflict_id"));
    EXPECT_TRUE(log[0].contains("winner"));
    EXPECT_TRUE(log[0].contains("policy"));
    EXPECT_TRUE(log[0].contains("resolved"));
    EXPECT_TRUE(log[0].contains("detected_at_ms"));

    EXPECT_EQ(log[0]["winner"], "remote");
    EXPECT_EQ(log[0]["policy"], "LWW");
    EXPECT_EQ(log[0]["resolved"], true);
}

TEST_F(TemporalConflictResolverTest, ExportAuditLog_EmptyWhenNoConflicts) {
    TemporalConflictResolver resolver(ConflictPolicy::CRDT_MERGE);
    auto log = resolver.exportAuditLog();
    EXPECT_TRUE(log.is_array());
    EXPECT_TRUE(log.empty());
}

TEST_F(TemporalConflictResolverTest, ExportAuditLog_PoliciesEncoded) {
    // Verify policy names are encoded correctly in the audit log
    struct TestCase { ConflictPolicy policy; std::string expected_name; };
    std::vector<TestCase> cases = {
        {ConflictPolicy::LAST_WRITE_WINS,  "LWW"},
        {ConflictPolicy::FIRST_WRITE_WINS, "FWW"},
        {ConflictPolicy::CRDT_MERGE,       "CRDT_MERGE"},
    };

    for (const auto& tc : cases) {
        TemporalConflictResolver resolver(tc.policy);
        auto local  = createSnapshot("l", 1000, 5, "node_a", "v1");
        auto remote = createSnapshot("r", 1001, 5, "node_b", "v2");
        resolver.resolve(local, remote);

        auto log = resolver.exportAuditLog();
        ASSERT_EQ(log.size(), 1u) << "policy=" << tc.expected_name;
        EXPECT_EQ(log[0]["policy"], tc.expected_name) << "policy=" << tc.expected_name;
    }
}

// ============================================================================
// TemporalConflictDetector Tests
// ============================================================================

class TemporalConflictDetectorTest : public ::testing::Test {
protected:
    TemporalConflictDetector detector;

    TemporalSnapshot makeSnap(
        const std::string& id,
        uint64_t physical,
        uint32_t logical,
        const std::string& node_id,
        nlohmann::json data
    ) {
        TemporalSnapshot s;
        s.snapshot_id    = id;
        s.hlc.physical   = physical;
        s.hlc.logical    = logical;
        s.hlc.node_id    = node_id;
        s.source_node_id = node_id;
        s.data           = std::move(data);
        s.checksum       = "chk";
        return s;
    }
};

// --- CONCURRENT_UPDATE ---

TEST_F(TemporalConflictDetectorTest, DetectConcurrentUpdate_ConcurrentHLC) {
    // Same physical + logical, different nodes, different data → concurrent update
    auto local  = makeSnap("l1", 1000, 5, "node_a", {{"x", 1}});
    auto remote = makeSnap("r1", 1000, 5, "node_b", {{"x", 2}});

    auto conflicts = detector.detectConflicts("tbl", local, remote);

    auto it = std::find_if(conflicts.begin(), conflicts.end(), [](const Conflict& c) {
        return c.type == themisdb::temporal::ConflictType::CONCURRENT_UPDATE;
    });
    ASSERT_NE(it, conflicts.end());
    EXPECT_EQ(it->affected_columns, std::vector<std::string>{"x"});
}

TEST_F(TemporalConflictDetectorTest, DetectConcurrentUpdate_OrderedHLC_NoConflict) {
    // local HLC < remote HLC: causally ordered — no concurrent conflict
    auto local  = makeSnap("l1", 1000, 5, "node_a", {{"x", 1}});
    auto remote = makeSnap("r1", 1001, 5, "node_b", {{"x", 2}});

    auto conflicts = detector.detectConflicts("tbl", local, remote);

    for (const auto& c : conflicts) {
        EXPECT_NE(c.type, themisdb::temporal::ConflictType::CONCURRENT_UPDATE);
    }
}

TEST_F(TemporalConflictDetectorTest, DetectConcurrentUpdate_SameNode_SameData_NoConflict) {
    // Same HLC, same node, same data — identical write, no conflict
    auto local  = makeSnap("l1", 1000, 5, "node_a", {{"x", 1}});
    auto remote = makeSnap("r1", 1000, 5, "node_a", {{"x", 1}});

    auto conflicts = detector.detectConflicts("tbl", local, remote);

    for (const auto& c : conflicts) {
        EXPECT_NE(c.type, themisdb::temporal::ConflictType::CONCURRENT_UPDATE);
    }
}

// --- OVERLAPPING_PERIODS ---

TEST_F(TemporalConflictDetectorTest, DetectOverlappingPeriods_Overlap) {
    // [100, 200) vs [150, 250): overlapping
    auto local  = makeSnap("l1", 1000, 1, "node_a",
                           {{"valid_start", 100}, {"valid_end", 200}, {"v", "a"}});
    auto remote = makeSnap("r1", 1001, 1, "node_b",
                           {{"valid_start", 150}, {"valid_end", 250}, {"v", "b"}});

    auto conflicts = detector.detectConflicts("tbl", local, remote);

    auto it = std::find_if(conflicts.begin(), conflicts.end(), [](const Conflict& c) {
        return c.type == themisdb::temporal::ConflictType::OVERLAPPING_PERIODS;
    });
    ASSERT_NE(it, conflicts.end());
    EXPECT_TRUE(std::find(it->affected_columns.begin(),
                          it->affected_columns.end(), "valid_start") !=
                it->affected_columns.end());
}

TEST_F(TemporalConflictDetectorTest, DetectOverlappingPeriods_NoOverlap) {
    // [100, 200) vs [200, 300): adjacent, not overlapping
    auto local  = makeSnap("l1", 1000, 1, "node_a",
                           {{"valid_start", 100}, {"valid_end", 200}, {"v", "a"}});
    auto remote = makeSnap("r1", 1001, 1, "node_b",
                           {{"valid_start", 200}, {"valid_end", 300}, {"v", "b"}});

    auto conflicts = detector.detectConflicts("tbl", local, remote);

    for (const auto& c : conflicts) {
        EXPECT_NE(c.type, themisdb::temporal::ConflictType::OVERLAPPING_PERIODS);
    }
}

TEST_F(TemporalConflictDetectorTest, DetectOverlappingPeriods_MissingFields_NoConflict) {
    // Snapshots without valid_start/valid_end: no period conflict
    auto local  = makeSnap("l1", 1000, 1, "node_a", {{"v", "a"}});
    auto remote = makeSnap("r1", 1001, 1, "node_b", {{"v", "b"}});

    auto conflicts = detector.detectConflicts("tbl", local, remote);

    for (const auto& c : conflicts) {
        EXPECT_NE(c.type, themisdb::temporal::ConflictType::OVERLAPPING_PERIODS);
    }
}

// --- REFERENTIAL_INTEGRITY ---

TEST_F(TemporalConflictDetectorTest, DetectReferentialIntegrity_DifferentRef) {
    auto local  = makeSnap("l1", 1000, 1, "node_a", {{"ref_entity_id", "entity_1"}});
    auto remote = makeSnap("r1", 1000, 1, "node_b", {{"ref_entity_id", "entity_2"}});

    auto conflicts = detector.detectConflicts("tbl", local, remote);

    auto it = std::find_if(conflicts.begin(), conflicts.end(), [](const Conflict& c) {
        return c.type == themisdb::temporal::ConflictType::REFERENTIAL_INTEGRITY;
    });
    ASSERT_NE(it, conflicts.end());
    EXPECT_EQ(it->affected_columns, std::vector<std::string>{"ref_entity_id"});
}

TEST_F(TemporalConflictDetectorTest, DetectReferentialIntegrity_SameRef_NoConflict) {
    auto local  = makeSnap("l1", 1000, 1, "node_a", {{"ref_entity_id", "entity_1"}});
    auto remote = makeSnap("r1", 1000, 1, "node_b", {{"ref_entity_id", "entity_1"}});

    auto conflicts = detector.detectConflicts("tbl", local, remote);

    for (const auto& c : conflicts) {
        EXPECT_NE(c.type, themisdb::temporal::ConflictType::REFERENTIAL_INTEGRITY);
    }
}

// --- UNIQUENESS_VIOLATION ---

TEST_F(TemporalConflictDetectorTest, DetectUniquenessViolation_DifferentNodes_DifferentData) {
    auto local  = makeSnap("l1", 1000, 5, "node_a", {{"name", "Alice"}});
    auto remote = makeSnap("r1", 1000, 5, "node_b", {{"name", "Bob"}});

    auto conflicts = detector.detectConflicts("tbl", local, remote);

    auto it = std::find_if(conflicts.begin(), conflicts.end(), [](const Conflict& c) {
        return c.type == themisdb::temporal::ConflictType::UNIQUENESS_VIOLATION;
    });
    ASSERT_NE(it, conflicts.end());
    EXPECT_FALSE(it->affected_columns.empty());
}

TEST_F(TemporalConflictDetectorTest, DetectUniquenessViolation_SameNode_NoConflict) {
    // Same source_node_id: not a distributed uniqueness conflict
    auto local  = makeSnap("l1", 1000, 5, "node_a", {{"name", "Alice"}});
    auto remote = makeSnap("r1", 1001, 5, "node_a", {{"name", "Bob"}});

    auto conflicts = detector.detectConflicts("tbl", local, remote);

    for (const auto& c : conflicts) {
        EXPECT_NE(c.type, themisdb::temporal::ConflictType::UNIQUENESS_VIOLATION);
    }
}

TEST_F(TemporalConflictDetectorTest, DetectUniquenessViolation_SameData_NoConflict) {
    auto local  = makeSnap("l1", 1000, 5, "node_a", {{"name", "Alice"}});
    auto remote = makeSnap("r1", 1000, 5, "node_b", {{"name", "Alice"}});

    auto conflicts = detector.detectConflicts("tbl", local, remote);

    for (const auto& c : conflicts) {
        EXPECT_NE(c.type, themisdb::temporal::ConflictType::UNIQUENESS_VIOLATION);
    }
}

// --- autoResolveConflict ---

TEST_F(TemporalConflictDetectorTest, AutoResolve_LWW_ReturnsNewer) {
    auto local  = makeSnap("l1", 1000, 5, "node_a", {{"x", 1}});
    auto remote = makeSnap("r1", 1001, 5, "node_b", {{"x", 2}});

    Conflict c;
    c.type           = themisdb::temporal::ConflictType::CONCURRENT_UPDATE;
    c.entity_id      = "e1";
    c.local_version  = local;
    c.remote_version = remote;

    auto winner = detector.autoResolveConflict(c, ConflictPolicy::LAST_WRITE_WINS);

    ASSERT_TRUE(winner.has_value());
    EXPECT_EQ(winner->hlc.physical, 1001);  // remote is newer
}

TEST_F(TemporalConflictDetectorTest, AutoResolve_ManualPolicy_ReturnsNullopt) {
    auto local  = makeSnap("l1", 1000, 5, "node_a", {{"x", 1}});
    auto remote = makeSnap("r1", 1000, 5, "node_b", {{"x", 2}});

    Conflict c;
    c.type           = themisdb::temporal::ConflictType::CONCURRENT_UPDATE;
    c.entity_id      = "e1";
    c.local_version  = local;
    c.remote_version = remote;

    auto result = detector.autoResolveConflict(c, ConflictPolicy::MANUAL);

    EXPECT_FALSE(result.has_value());
}

// --- queueForManualResolution ---

TEST_F(TemporalConflictDetectorTest, Queue_AddAndRetrieve) {
    auto local  = makeSnap("l1", 1000, 5, "node_a", {{"x", 1}});
    auto remote = makeSnap("r1", 1000, 5, "node_b", {{"x", 2}});

    Conflict c;
    c.type           = themisdb::temporal::ConflictType::CONCURRENT_UPDATE;
    c.entity_id      = "entity_1";
    c.local_version  = local;
    c.remote_version = remote;

    bool queued = detector.queueForManualResolution("orders", c);
    EXPECT_TRUE(queued);

    auto queued_conflicts = detector.getQueuedConflicts();
    ASSERT_EQ(queued_conflicts.size(), 1u);
    EXPECT_EQ(queued_conflicts[0].type, themisdb::temporal::ConflictType::CONCURRENT_UPDATE);
    EXPECT_EQ(queued_conflicts[0].entity_id, "entity_1");
    EXPECT_EQ(queued_conflicts[0].table_name, "orders");
}

TEST_F(TemporalConflictDetectorTest, Queue_DuplicateNotQueued) {
    auto local  = makeSnap("l1", 1000, 5, "node_a", {{"x", 1}});
    auto remote = makeSnap("r1", 1000, 5, "node_b", {{"x", 2}});

    Conflict c;
    c.type           = themisdb::temporal::ConflictType::CONCURRENT_UPDATE;
    c.entity_id      = "entity_1";
    c.local_version  = local;
    c.remote_version = remote;

    EXPECT_TRUE(detector.queueForManualResolution("orders", c));
    EXPECT_FALSE(detector.queueForManualResolution("orders", c));  // duplicate

    EXPECT_EQ(detector.getQueuedConflicts().size(), 1u);
}

TEST_F(TemporalConflictDetectorTest, Queue_SameConflictDifferentTables_BothQueued) {
    // Identical conflict data but different table_name must be stored separately.
    auto local  = makeSnap("l1", 1000, 5, "node_a", {{"x", 1}});
    auto remote = makeSnap("r1", 1000, 5, "node_b", {{"x", 2}});

    Conflict c;
    c.type           = themisdb::temporal::ConflictType::CONCURRENT_UPDATE;
    c.entity_id      = "entity_1";
    c.local_version  = local;
    c.remote_version = remote;

    EXPECT_TRUE(detector.queueForManualResolution("table_a", c));
    EXPECT_TRUE(detector.queueForManualResolution("table_b", c));  // different table

    EXPECT_EQ(detector.getQueuedConflicts().size(), 2u);
}

TEST_F(TemporalConflictDetectorTest, Queue_ClearEmptiesQueue) {
    auto local  = makeSnap("l1", 1000, 5, "node_a", {{"x", 1}});
    auto remote = makeSnap("r1", 1000, 5, "node_b", {{"x", 2}});

    Conflict c;
    c.type           = themisdb::temporal::ConflictType::UNIQUENESS_VIOLATION;
    c.entity_id      = "entity_1";
    c.local_version  = local;
    c.remote_version = remote;

    detector.queueForManualResolution("tbl", c);
    ASSERT_EQ(detector.getQueuedConflicts().size(), 1u);

    detector.clearQueue();
    EXPECT_TRUE(detector.getQueuedConflicts().empty());
}

// --- detectConflicts: table_name and entity_id fields ---

TEST_F(TemporalConflictDetectorTest, DetectConflicts_TableNameAndEntityIdSet) {
    auto local  = makeSnap("snap1", 1000, 5, "node_a", {{"v", 1}});
    auto remote = makeSnap("snap2", 1000, 5, "node_b", {{"v", 2}});

    auto conflicts = detector.detectConflicts("orders", local, remote);
    ASSERT_FALSE(conflicts.empty());
    for (const auto& c : conflicts) {
        EXPECT_EQ(c.table_name, "orders") << "table_name should be set to 'orders'";
        EXPECT_EQ(c.entity_id, "snap1")   << "entity_id should be local snapshot_id";
    }
}

// --- detectConflicts: no conflict for identical snapshots ---

TEST_F(TemporalConflictDetectorTest, DetectConflicts_IdenticalSnapshots_Empty) {
    auto local = makeSnap("s1", 1000, 5, "node_a", {{"k", "v"}});

    // Identical snapshot from same node — no conflict of any kind
    auto conflicts = detector.detectConflicts("tbl", local, local);
    EXPECT_TRUE(conflicts.empty());
}

// --- Uniqueness violation: asymmetric keys ---

TEST_F(TemporalConflictDetectorTest, DetectUniquenessViolation_AsymmetricKeys_Detected) {
    // local has key "a"; remote has key "b". Shared keys all match (none).
    // Both sides diverge because each has a key the other lacks.
    auto local  = makeSnap("l1", 1000, 5, "node_a", {{"a", 1}});
    auto remote = makeSnap("r1", 1000, 5, "node_b", {{"b", 2}});

    auto conflicts = detector.detectConflicts("tbl", local, remote);

    auto it = std::find_if(conflicts.begin(), conflicts.end(), [](const Conflict& c) {
        return c.type == themisdb::temporal::ConflictType::UNIQUENESS_VIOLATION;
    });
    ASSERT_NE(it, conflicts.end()) << "Asymmetric keys should trigger UNIQUENESS_VIOLATION";
    // Both "a" (only in local) and "b" (only in remote) should be in affected_columns
    EXPECT_TRUE(std::find(it->affected_columns.begin(), it->affected_columns.end(), "a") !=
                it->affected_columns.end());
    EXPECT_TRUE(std::find(it->affected_columns.begin(), it->affected_columns.end(), "b") !=
                it->affected_columns.end());
}

// --- OVERLAPPING_PERIODS: non-integer valid_time fields are ignored ---

TEST_F(TemporalConflictDetectorTest, DetectOverlappingPeriods_NonIntegerFields_NoConflict) {
    // valid_start/valid_end present but as strings — should be treated as absent
    auto local  = makeSnap("l1", 1000, 1, "node_a",
                           {{"valid_start", "not-a-number"}, {"valid_end", "not-a-number"}, {"v", "a"}});
    auto remote = makeSnap("r1", 1001, 1, "node_b",
                           {{"valid_start", "not-a-number"}, {"valid_end", "not-a-number"}, {"v", "b"}});

    auto conflicts = detector.detectConflicts("tbl", local, remote);

    for (const auto& c : conflicts) {
        EXPECT_NE(c.type, themisdb::temporal::ConflictType::OVERLAPPING_PERIODS);
    }
}

// ============================================================================
// MergeResolver Tests  (MCR-01..07)
// ============================================================================
// Tests verify that MergeResolver implementations satisfy the CRDT contracts
// (commutativity, idempotency) and that TemporalConflictResolver correctly
// delegates to an injected resolver.
// ============================================================================

namespace {

// Helper: make a snapshot with arbitrary multi-field JSON data.
TemporalSnapshot makeMultiFieldSnap(
    const std::string& id,
    uint64_t physical, uint32_t logical, const std::string& node_id,
    nlohmann::json data
) {
    TemporalSnapshot s;
    s.snapshot_id    = id;
    s.hlc.physical   = physical;
    s.hlc.logical    = logical;
    s.hlc.node_id    = node_id;
    s.source_node_id = node_id;
    s.data           = std::move(data);
    s.checksum       = "chk";
    return s;
}

} // anonymous namespace

// MCR-01: LWWFieldMergeResolver is commutative.
//         merge(a,b).data == merge(b,a).data  (field content, not metadata)
TEST(MergeResolverTest, LWWField_Commutative) {
    LWWFieldMergeResolver resolver;

    auto a = makeMultiFieldSnap("a", 1000, 1, "n1", {{"x", 1}, {"y", "hello"}});
    auto b = makeMultiFieldSnap("b", 2000, 1, "n2", {{"x", 2}, {"z", true}});

    auto ab = resolver.merge(a, b);
    auto ba = resolver.merge(b, a);

    EXPECT_EQ(ab.data, ba.data)
        << "LWWFieldMergeResolver must be commutative";
}

// MCR-02: LWWFieldMergeResolver is idempotent.
//         merge(a,a).data == a.data
TEST(MergeResolverTest, LWWField_Idempotent) {
    LWWFieldMergeResolver resolver;

    auto a = makeMultiFieldSnap("a", 1000, 1, "n1", {{"k", 42}});

    auto aa = resolver.merge(a, a);

    EXPECT_EQ(aa.data, a.data)
        << "LWWFieldMergeResolver must be idempotent";
}

// MCR-03: UnionMergeResolver includes fields from both snapshots.
TEST(MergeResolverTest, Union_IncludesAllFields) {
    UnionMergeResolver resolver;

    auto a = makeMultiFieldSnap("a", 1000, 1, "n1", {{"from_a", 1}});
    auto b = makeMultiFieldSnap("b", 2000, 1, "n2", {{"from_b", 2}});

    auto merged = resolver.merge(a, b);

    ASSERT_TRUE(merged.data.contains("from_a"))
        << "Union merge must include fields from the older snapshot";
    ASSERT_TRUE(merged.data.contains("from_b"))
        << "Union merge must include fields from the newer snapshot";
    EXPECT_EQ(merged.data["from_a"], 1);
    EXPECT_EQ(merged.data["from_b"], 2);
}

// MCR-04: UnionMergeResolver is commutative.
TEST(MergeResolverTest, Union_Commutative) {
    UnionMergeResolver resolver;

    auto a = makeMultiFieldSnap("a", 1000, 1, "n1", {{"x", "alpha"}, {"shared", "old"}});
    auto b = makeMultiFieldSnap("b", 2000, 1, "n2", {{"y", "beta"},  {"shared", "new"}});

    auto ab = resolver.merge(a, b);
    auto ba = resolver.merge(b, a);

    EXPECT_EQ(ab.data, ba.data)
        << "UnionMergeResolver must be commutative";
}

// MCR-05: CustomMergeResolver uses the provided callable.
TEST(MergeResolverTest, Custom_CallableInvoked) {
    bool called = false;
    TemporalSnapshot expected_result =
        makeMultiFieldSnap("custom", 9999, 0, "custom", {{"result", "custom"}});

    CustomMergeResolver resolver(
        [&](const TemporalSnapshot&, const TemporalSnapshot&) -> TemporalSnapshot {
            called = true;
            return expected_result;
        }
    );

    auto a = makeMultiFieldSnap("a", 1000, 1, "n1", {});
    auto b = makeMultiFieldSnap("b", 2000, 1, "n2", {});

    auto result = resolver.merge(a, b);

    EXPECT_TRUE(called)
        << "CustomMergeResolver must invoke the provided callable";
    EXPECT_EQ(result.snapshot_id, "custom");
    EXPECT_EQ(result.data["result"], "custom");
}

// MCR-06: TemporalConflictResolver::setMergeResolver injects custom strategy
//         that is used when ConflictPolicy::CRDT_MERGE is active.
TEST(MergeResolverTest, InjectedResolverUsedByCRDTMerge) {
    TemporalConflictResolver cr(ConflictPolicy::CRDT_MERGE);

    bool called = false;
    auto custom = std::make_shared<CustomMergeResolver>(
        [&](const TemporalSnapshot& l, const TemporalSnapshot& r) -> TemporalSnapshot {
            called = true;
            // Return the newer snapshot unchanged.
            return (l.hlc < r.hlc) ? r : l;
        }
    );

    cr.setMergeResolver(custom);
    EXPECT_EQ(cr.getMergeResolver(), custom);

    auto a = makeMultiFieldSnap("a", 1000, 1, "n1", {{"v", "a"}});
    auto b = makeMultiFieldSnap("b", 2000, 1, "n2", {{"v", "b"}});

    auto result = cr.resolve(a, b);

    EXPECT_TRUE(called)
        << "TemporalConflictResolver must delegate to the injected MergeResolver";
    EXPECT_EQ(result.data["v"], "b");
}

// MCR-07: setMergeResolver(nullptr) reverts to built-in LWW-per-field behaviour.
TEST(MergeResolverTest, NullResolverFallsBackToBuiltInLWW) {
    TemporalConflictResolver cr(ConflictPolicy::CRDT_MERGE);

    // First inject a custom resolver that always returns a sentinel.
    auto sentinel = std::make_shared<CustomMergeResolver>(
        [](const TemporalSnapshot&, const TemporalSnapshot&) -> TemporalSnapshot {
            TemporalSnapshot s;
            s.snapshot_id = "SENTINEL";
            s.data = {{"sentinel", true}};
            return s;
        }
    );
    cr.setMergeResolver(sentinel);

    // Now reset to nullptr — built-in LWW should take over.
    cr.setMergeResolver(nullptr);
    EXPECT_EQ(cr.getMergeResolver(), nullptr);

    auto a = makeMultiFieldSnap("a", 1000, 1, "n1", {{"v", "old"}});
    auto b = makeMultiFieldSnap("b", 2000, 1, "n2", {{"v", "new"}});

    auto result = cr.resolve(a, b);

    EXPECT_NE(result.snapshot_id, "SENTINEL")
        << "After reset to nullptr, built-in LWW must be active";
    // Built-in LWW should produce the newer snapshot's value for field "v".
    EXPECT_EQ(result.data["v"], "new");
}
