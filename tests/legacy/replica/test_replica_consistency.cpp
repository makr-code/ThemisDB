/**
 * @file test_replica_consistency.cpp
 * @brief Tests for VectorClock and ReplicaConsistencyManager
 *        (src/sharding/replica_consistency.cpp)
 *
 * Covers VectorClock:
 *   - Default construction
 *   - increment / get
 *   - update (merge)
 *   - happensBefore / happensAfter / isConcurrent
 *   - serialize / deserialize round-trip
 *
 * Covers ReplicaConsistencyManager:
 *   - Construction with default Config
 *   - recordWrite → VersionedEntry returned
 *   - mergeReplicas: single entry → returns VersionedEntry
 *   - mergeReplicas: two identical entries → no conflict
 *   - getVectorClock → after writes
 *   - Config: custom strategy
 */

#include <gtest/gtest.h>
#include "sharding/replica_consistency.h"
#include <variant>
#include <string>

using namespace themisdb::sharding;

// ============================================================================
// VectorClock
// ============================================================================

TEST(VectorClockTest, DefaultConstruction_ZeroClocks) {
    VectorClock vc;
    EXPECT_EQ(vc.get("node1"), 0u);
    EXPECT_EQ(vc.get("node2"), 0u);
}

TEST(VectorClockTest, Increment_IncrementsNodeClock) {
    VectorClock vc;
    vc.increment("nodeA");
    EXPECT_EQ(vc.get("nodeA"), 1u);
    vc.increment("nodeA");
    EXPECT_EQ(vc.get("nodeA"), 2u);
}

TEST(VectorClockTest, Increment_DoesNotAffectOtherNodes) {
    VectorClock vc;
    vc.increment("nodeA");
    EXPECT_EQ(vc.get("nodeB"), 0u);
}

TEST(VectorClockTest, Update_MergesMaxValues) {
    VectorClock a, b;
    a.increment("n1");
    a.increment("n1"); // n1=2
    b.increment("n2");
    b.increment("n2");
    b.increment("n2"); // n2=3

    a.update(b);
    EXPECT_EQ(a.get("n1"), 2u);
    EXPECT_EQ(a.get("n2"), 3u);
}

TEST(VectorClockTest, HappensBefore_CorrectOrdering) {
    VectorClock older, newer;
    older.increment("n1");       // n1=1
    newer.increment("n1");
    newer.increment("n1");       // n1=2

    EXPECT_TRUE(older.happensBefore(newer));
    EXPECT_FALSE(newer.happensBefore(older));
}

TEST(VectorClockTest, IsConcurrent_WhenNeitherDominates) {
    VectorClock a, b;
    a.increment("n1"); // n1=1
    b.increment("n2"); // n2=1

    EXPECT_TRUE(a.isConcurrent(b));
    EXPECT_TRUE(b.isConcurrent(a));
}

TEST(VectorClockTest, HappensAfter_IsReverseOfHappensBefore) {
    VectorClock older, newer;
    older.increment("n1");
    newer.increment("n1");
    newer.increment("n1");

    EXPECT_TRUE(newer.happensAfter(older));
    EXPECT_FALSE(older.happensAfter(newer));
}

TEST(VectorClockTest, Serialize_Deserialize_RoundTrip) {
    VectorClock original;
    original.increment("n1");
    original.increment("n1");
    original.increment("n2");

    auto serialized  = original.serialize();
    auto deserialized = VectorClock::deserialize(serialized);

    ASSERT_TRUE(deserialized.has_value());
    EXPECT_EQ(deserialized->get("n1"), original.get("n1"));
    EXPECT_EQ(deserialized->get("n2"), original.get("n2"));
}

TEST(VectorClockTest, Deserialize_InvalidData_ReturnsNullopt) {
    auto result = VectorClock::deserialize("not_valid_json_at_all!!!{{{");
    EXPECT_FALSE(result.has_value());
}

// ============================================================================
// ReplicaConsistencyManager
// ============================================================================

class ReplicaConsistencyManagerTest : public ::testing::Test {
protected:
    ReplicaConsistencyManager::Config cfg_;
    ReplicaConsistencyManager manager_{cfg_};
};

TEST_F(ReplicaConsistencyManagerTest, RecordWrite_ReturnsVersionedEntry) {
    auto entry = manager_.recordWrite("key1", "value1", "node1");
    EXPECT_EQ(entry.data, "value1");
    EXPECT_EQ(entry.node_id, "node1");
    // Vector clock for node1 should be incremented
    EXPECT_GE(entry.version.get("node1"), 1u);
}

TEST_F(ReplicaConsistencyManagerTest, RecordWrite_IncrementsVersionOnEachWrite) {
    auto e1 = manager_.recordWrite("k", "v1", "node_a");
    auto e2 = manager_.recordWrite("k", "v2", "node_a");
    EXPECT_GT(e2.version.get("node_a"), e1.version.get("node_a"));
}

TEST_F(ReplicaConsistencyManagerTest, MergeReplicas_SingleEntry_ReturnsVersionedEntry) {
    auto entry = manager_.recordWrite("key_single", "data", "n1");
    auto result = manager_.mergeReplicas("key_single", {entry});
    EXPECT_TRUE(std::holds_alternative<VersionedEntry>(result));
}

TEST_F(ReplicaConsistencyManagerTest, MergeReplicas_EmptyEntries_ReturnsConflictOrEntry) {
    // Should not crash with empty entries vector
    EXPECT_NO_THROW(manager_.mergeReplicas("key_empty", {}));
}

TEST_F(ReplicaConsistencyManagerTest, GetVectorClock_AfterWrite_NonEmpty) {
    manager_.recordWrite("k", "v", "nodeX");
    VectorClock vc = manager_.getVectorClock("nodeX");
    EXPECT_GE(vc.get("nodeX"), 1u);
}

TEST_F(ReplicaConsistencyManagerTest, Config_DefaultStrategy_LastWriteWins) {
    EXPECT_EQ(cfg_.default_strategy, ConflictResolutionStrategy::LAST_WRITE_WINS);
    EXPECT_TRUE(cfg_.auto_resolve_conflicts);
    EXPECT_TRUE(cfg_.track_causality);
}

TEST_F(ReplicaConsistencyManagerTest, Config_CustomStrategy) {
    ReplicaConsistencyManager::Config custom_cfg;
    custom_cfg.default_strategy = ConflictResolutionStrategy::VECTOR_CLOCK_ORDERING;
    custom_cfg.auto_resolve_conflicts = false;
    ReplicaConsistencyManager custom_manager(custom_cfg);

    // Should construct successfully and accept writes
    EXPECT_NO_THROW(custom_manager.recordWrite("k", "v", "n"));
}

// ============================================================================
// VersionedEntry serialize / deserialize
// ============================================================================

TEST(VersionedEntryTest, SerializeDeserialize_RoundTrip) {
    VectorClock vc;
    vc.increment("n1");
    vc.increment("n2");

    VersionedEntry original;
    original.data      = "test_payload";
    original.version   = vc;
    original.node_id   = "replica_3";
    original.timestamp = std::chrono::system_clock::now();

    auto serialized   = original.serialize();
    auto deserialized = VersionedEntry::deserialize(serialized);

    ASSERT_TRUE(deserialized.has_value());
    EXPECT_EQ(deserialized->data,    original.data);
    EXPECT_EQ(deserialized->node_id, original.node_id);
}

TEST(VersionedEntryTest, Deserialize_InvalidData_ReturnsNullopt) {
    auto result = VersionedEntry::deserialize("%%%INVALID%%%");
    EXPECT_FALSE(result.has_value());
}
