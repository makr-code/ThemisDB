<<<<<<< HEAD
/**
 * @file test_tensor_delta_log.cpp
 * @brief CTest / GTest coverage for TensorDeltaLog — sub-issue #5472.
 *
 * ## Test IDs
 *   TDL-01  New delta log is empty and returns sequence 0
 *   TDL-02  appendDelta assigns monotonically increasing sequence numbers
 *   TDL-03  appendDelta rejects empty affected_entity_id (returns 0)
 *   TDL-04  size() and empty() reflect appended entries
 *   TDL-05  extractWindow returns entries in the requested range
 *   TDL-06  extractWindow returns nullopt for empty range
 *   TDL-07  extractWindow returns nullopt when start > end
 *   TDL-08  DeltaWindow::countInserts/Updates/Deletes count correctly
 *   TDL-09  DeltaWindow::estimateChangeFraction computes correct fraction
 *   TDL-10  DeltaLogEntry serialize → deserialize round-trip
 *   TDL-11  DeltaLogEntry::deserialize returns nullopt for malformed input
 *   TDL-12  garbage_collect removes entries below cutoff, returns removed count
 *   TDL-13  garbage_collect with cutoff 0 removes nothing
 *   TDL-14  getStats reflects accumulated mutation counters
 *   TDL-15  clear() resets all state
 *   TDL-16  DeltaWindow::isValid returns false for empty window
 *   TDL-17  DeltaWindow::toJSON / fromJSON round-trip preserves entries
 *   TDL-18  Sequence numbers remain consistent after garbage_collect
 *
 * @see src/distributed_tensor/include/tensor_delta_log.h
 * @see src/distributed_tensor/src/tensor_delta_log.cc
 * @see GitHub Issue #5472
 */

#include <gtest/gtest.h>

#include "src/distributed_tensor/include/tensor_delta_log.h"

#include <string>
#include <vector>

using namespace themis::distributed_tensor;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
namespace {

/// Build an entry and append it to the log; returns the assigned sequence.
uint64_t appendOne(TensorDeltaLog& log,
                   DeltaMutationType type,
                   const std::string& entity = "entity-1",
                   const std::string& txn    = "txn-1",
                   uint32_t payload_bytes     = 100) {
    return log.appendDelta(type, entity, txn, /*shard_hint=*/"shard-0", payload_bytes);
}

}  // namespace

// ---------------------------------------------------------------------------
// TDL-01: New delta log is empty with sequence 0
// ---------------------------------------------------------------------------
TEST(TensorDeltaLogTest, TDL01_NewLogIsEmptyAndSequenceIsZero) {
    TensorDeltaLog log("art-test");
    EXPECT_TRUE(log.empty());
    EXPECT_EQ(log.size(), 0u);
    EXPECT_EQ(log.getCurrentSequence(), 0u);
    EXPECT_EQ(log.getArtifactId(), "art-test");
}

// ---------------------------------------------------------------------------
// TDL-02: appendDelta assigns monotonically increasing sequence numbers
// ---------------------------------------------------------------------------
TEST(TensorDeltaLogTest, TDL02_AppendAssignsMonotonicSequences) {
    TensorDeltaLog log("art-seq");
    const uint64_t s1 = appendOne(log, DeltaMutationType::INSERT);
    const uint64_t s2 = appendOne(log, DeltaMutationType::UPDATE);
    const uint64_t s3 = appendOne(log, DeltaMutationType::DELETE);

    EXPECT_GT(s1, 0u);
    EXPECT_GT(s2, s1);
    EXPECT_GT(s3, s2);
    EXPECT_EQ(log.getCurrentSequence(), s3);
}

// ---------------------------------------------------------------------------
// TDL-03: appendDelta rejects empty entity_id (returns 0)
// ---------------------------------------------------------------------------
TEST(TensorDeltaLogTest, TDL03_AppendRejectsEmptyEntityId) {
    TensorDeltaLog log("art-val");
    const uint64_t seq = log.appendDelta(
        DeltaMutationType::INSERT, /*affected_entity_id=*/"", "txn-1", "", 100);
    EXPECT_EQ(seq, 0u);
    EXPECT_EQ(log.size(), 0u);
}

// ---------------------------------------------------------------------------
// TDL-04: size() and empty() stay consistent
// ---------------------------------------------------------------------------
TEST(TensorDeltaLogTest, TDL04_SizeAndEmptyAreConsistent) {
    TensorDeltaLog log("art-size");
    EXPECT_TRUE(log.empty());

    appendOne(log, DeltaMutationType::INSERT);
    EXPECT_FALSE(log.empty());
    EXPECT_EQ(log.size(), 1u);

    appendOne(log, DeltaMutationType::UPDATE);
    EXPECT_EQ(log.size(), 2u);
}

// ---------------------------------------------------------------------------
// TDL-05: extractWindow returns entries in range (inclusive)
// ---------------------------------------------------------------------------
TEST(TensorDeltaLogTest, TDL05_ExtractWindowReturnsEntriesInRange) {
    TensorDeltaLog log("art-win");
    const uint64_t s1 = appendOne(log, DeltaMutationType::INSERT,  "e1");
    const uint64_t s2 = appendOne(log, DeltaMutationType::UPDATE,  "e2");
    const uint64_t s3 = appendOne(log, DeltaMutationType::DELETE,  "e3");
    appendOne(log, DeltaMutationType::INSERT, "e4");  // outside range

    auto win = log.extractWindow(s1, s3);
    ASSERT_TRUE(win.has_value());
    EXPECT_EQ(win->entries.size(), 3u);
    EXPECT_EQ(win->sequence_start, s1);
    EXPECT_EQ(win->sequence_end, s3);
    EXPECT_EQ(win->artifact_id, "art-win");
}

// ---------------------------------------------------------------------------
// TDL-06: extractWindow returns nullopt when no entries exist
// ---------------------------------------------------------------------------
TEST(TensorDeltaLogTest, TDL06_ExtractWindowEmptyLogReturnsNullopt) {
    TensorDeltaLog log("art-empty");
    auto win = log.extractWindow(1, 5);
    EXPECT_FALSE(win.has_value());
}

// ---------------------------------------------------------------------------
// TDL-07: extractWindow returns nullopt when start > end
// ---------------------------------------------------------------------------
TEST(TensorDeltaLogTest, TDL07_ExtractWindowInvalidRangeReturnsNullopt) {
    TensorDeltaLog log("art-range");
    appendOne(log, DeltaMutationType::INSERT, "e1");
    appendOne(log, DeltaMutationType::INSERT, "e2");

    auto win = log.extractWindow(5, 1);  // start > end
    EXPECT_FALSE(win.has_value());
}

// ---------------------------------------------------------------------------
// TDL-08: DeltaWindow mutation counters
// ---------------------------------------------------------------------------
TEST(TensorDeltaLogTest, TDL08_DeltaWindowMutationCounters) {
    TensorDeltaLog log("art-cnt");
    appendOne(log, DeltaMutationType::INSERT,       "e1", "t1", 50);
    appendOne(log, DeltaMutationType::INSERT,       "e2", "t1", 50);
    appendOne(log, DeltaMutationType::UPDATE,       "e3", "t1", 50);
    appendOne(log, DeltaMutationType::DELETE,       "e4", "t1", 50);
    appendOne(log, DeltaMutationType::SHARD_CHANGE, "e5", "t1", 50);

    const uint64_t end_seq = log.getCurrentSequence();
    auto win = log.extractWindow(1, end_seq);
    ASSERT_TRUE(win.has_value());

    EXPECT_EQ(win->countInserts(),      2u);
    EXPECT_EQ(win->countUpdates(),      1u);
    EXPECT_EQ(win->countDeletes(),      1u);
    EXPECT_EQ(win->countShardChanges(), 1u);
}

// ---------------------------------------------------------------------------
// TDL-09: DeltaWindow::estimateChangeFraction
// ---------------------------------------------------------------------------
TEST(TensorDeltaLogTest, TDL09_EstimateChangeFractionIsCorrect) {
    TensorDeltaLog log("art-frac");
    appendOne(log, DeltaMutationType::INSERT, "e1", "t1", 1000);  // 1 000 B payload
    appendOne(log, DeltaMutationType::UPDATE, "e2", "t1",  500);  //   500 B payload

    const uint64_t end_seq = log.getCurrentSequence();
    auto win = log.extractWindow(1, end_seq);
    ASSERT_TRUE(win.has_value());

    constexpr uint64_t artifact_size = 10000;  // 10 KB artifact
    const double frac = win->estimateChangeFraction(artifact_size);
    EXPECT_DOUBLE_EQ(frac, 1500.0 / artifact_size);  // 0.15
}

// ---------------------------------------------------------------------------
// TDL-10: DeltaLogEntry serialize → deserialize round-trip
// ---------------------------------------------------------------------------
TEST(TensorDeltaLogTest, TDL10_SerializeDeserializeRoundTrip) {
    DeltaLogEntry entry;
    entry.sequence_number       = 42;
    entry.mutation_type         = DeltaMutationType::UPDATE;
    entry.affected_entity_id    = "entity-xyz";
    entry.recorded_at_ms        = 1718000000000LL;  // some timestamp
    entry.source_transaction_id = "txn-abcdef";
    entry.shard_hint            = "shard-3";
    entry.payload_size_bytes    = 512;
    entry.payload_checksum      = "crc32:aabbccdd";

    const std::string serialized = entry.serialize();
    ASSERT_FALSE(serialized.empty());

    auto parsed = DeltaLogEntry::deserialize(serialized);
    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->sequence_number,       entry.sequence_number);
    EXPECT_EQ(parsed->mutation_type,         entry.mutation_type);
    EXPECT_EQ(parsed->affected_entity_id,    entry.affected_entity_id);
    EXPECT_EQ(parsed->source_transaction_id, entry.source_transaction_id);
    EXPECT_EQ(parsed->shard_hint,            entry.shard_hint);
    EXPECT_EQ(parsed->payload_size_bytes,    entry.payload_size_bytes);
}

// ---------------------------------------------------------------------------
// TDL-11: DeltaLogEntry::deserialize returns nullopt for malformed input
// ---------------------------------------------------------------------------
TEST(TensorDeltaLogTest, TDL11_DeserializeReturnNulloptForGarbage) {
    EXPECT_FALSE(DeltaLogEntry::deserialize("").has_value());
    EXPECT_FALSE(DeltaLogEntry::deserialize("not|a|valid|entry").has_value());
    EXPECT_FALSE(DeltaLogEntry::deserialize("NaN|0|entity||txn||0|").has_value());
}

// ---------------------------------------------------------------------------
// TDL-12: garbage_collect removes entries below cutoff
// ---------------------------------------------------------------------------
TEST(TensorDeltaLogTest, TDL12_GarbageCollectRemovesOldEntries) {
    TensorDeltaLog log("art-gc");
    const uint64_t s1 = appendOne(log, DeltaMutationType::INSERT, "e1");
    appendOne(log, DeltaMutationType::INSERT, "e2");
    const uint64_t s3 = appendOne(log, DeltaMutationType::INSERT, "e3");

    // Remove all entries with sequence < s3
    const size_t removed = log.garbage_collect(s3);
    EXPECT_GE(removed, 1u);  // at least s1 and s2 were removed
    EXPECT_LT(log.size(), 3u);

    // s3 itself (and newer) should still be accessible
    auto win = log.extractWindow(s3, s3);
    EXPECT_TRUE(win.has_value());

    (void)s1;
}

// ---------------------------------------------------------------------------
// TDL-13: garbage_collect with cutoff 0 removes nothing
// ---------------------------------------------------------------------------
TEST(TensorDeltaLogTest, TDL13_GarbageCollectWithCutoffZeroRemovesNothing) {
    TensorDeltaLog log("art-gc0");
    appendOne(log, DeltaMutationType::INSERT, "e1");
    appendOne(log, DeltaMutationType::INSERT, "e2");
    ASSERT_EQ(log.size(), 2u);

    const size_t removed = log.garbage_collect(0);
    EXPECT_EQ(removed, 0u);
    EXPECT_EQ(log.size(), 2u);
}

// ---------------------------------------------------------------------------
// TDL-14: getStats reflects accumulated mutation counters
// ---------------------------------------------------------------------------
TEST(TensorDeltaLogTest, TDL14_GetStatsReflectsAccumulatedMutations) {
    TensorDeltaLog log("art-stats");
    appendOne(log, DeltaMutationType::INSERT, "e1", "t1", 200);
    appendOne(log, DeltaMutationType::UPDATE, "e2", "t1", 100);
    appendOne(log, DeltaMutationType::DELETE, "e3", "t1",  50);

    const auto stats = log.getStats();
    EXPECT_EQ(stats.total_deltas, 3u);
    EXPECT_EQ(stats.total_insert_mutations, 1u);
    EXPECT_EQ(stats.total_update_mutations, 1u);
    EXPECT_EQ(stats.total_delete_mutations, 1u);
    EXPECT_EQ(stats.total_payload_bytes, 350u);
}

// ---------------------------------------------------------------------------
// TDL-15: clear() resets all state
// ---------------------------------------------------------------------------
TEST(TensorDeltaLogTest, TDL15_ClearResetsAllState) {
    TensorDeltaLog log("art-clr");
    appendOne(log, DeltaMutationType::INSERT, "e1");
    appendOne(log, DeltaMutationType::INSERT, "e2");
    ASSERT_EQ(log.size(), 2u);

    log.clear();
    EXPECT_TRUE(log.empty());
    EXPECT_EQ(log.size(), 0u);
}

// ---------------------------------------------------------------------------
// TDL-16: DeltaWindow::isValid returns false for empty window
// ---------------------------------------------------------------------------
TEST(TensorDeltaLogTest, TDL16_EmptyDeltaWindowIsInvalid) {
    DeltaWindow win;
    EXPECT_FALSE(win.isValid());
}

// ---------------------------------------------------------------------------
// TDL-17: DeltaWindow JSON round-trip
// ---------------------------------------------------------------------------
TEST(TensorDeltaLogTest, TDL17_DeltaWindowJsonRoundTrip) {
    TensorDeltaLog log("art-json");
    appendOne(log, DeltaMutationType::INSERT, "entity-a", "txn-1", 100);
    appendOne(log, DeltaMutationType::UPDATE, "entity-b", "txn-2", 200);

    const uint64_t end_seq = log.getCurrentSequence();
    auto win = log.extractWindow(1, end_seq);
    ASSERT_TRUE(win.has_value());

    const std::string json_str = win->toJSON();
    ASSERT_FALSE(json_str.empty());

    auto restored = DeltaWindow::fromJSON(json_str);
    ASSERT_TRUE(restored.has_value());
    EXPECT_EQ(restored->artifact_id,     win->artifact_id);
    EXPECT_EQ(restored->sequence_start,  win->sequence_start);
    EXPECT_EQ(restored->sequence_end,    win->sequence_end);
    EXPECT_EQ(restored->entries.size(),  win->entries.size());
}

// ---------------------------------------------------------------------------
// TDL-18: Sequence numbers remain consistent after garbage_collect
// ---------------------------------------------------------------------------
TEST(TensorDeltaLogTest, TDL18_SequenceConsistencyAfterGarbageCollect) {
    TensorDeltaLog log("art-seq2");
    appendOne(log, DeltaMutationType::INSERT, "e1");
    appendOne(log, DeltaMutationType::INSERT, "e2");
    const uint64_t before_gc = log.getCurrentSequence();

    log.garbage_collect(before_gc);  // remove all but last

    // Appending a new entry after GC must continue from the current sequence.
    const uint64_t s_new = appendOne(log, DeltaMutationType::INSERT, "e3");
    EXPECT_GT(s_new, before_gc);
    EXPECT_EQ(log.getCurrentSequence(), s_new);
}
=======
#include <gtest/gtest.h>

#include "tensor_delta_log.h"

using namespace themis::distributed_tensor;

namespace {

TEST(TensorDeltaLogTest, AppendAssignsMonotonicSequenceNumbers) {
    TensorDeltaLog log("artifact-users");

    const auto seq1 = log.appendDelta(DeltaMutationType::INSERT, "node-1", "tx-1", "shard-a", 128);
    const auto seq2 = log.appendDelta(DeltaMutationType::UPDATE, "node-2", "tx-2", "shard-a", 256);

    EXPECT_EQ(seq1, 1u);
    EXPECT_EQ(seq2, 2u);
    EXPECT_EQ(log.getCurrentSequence(), 2u);
    EXPECT_EQ(log.size(), 2u);
}

TEST(TensorDeltaLogTest, ExtractWindowReturnsContiguousOrderedRange) {
    TensorDeltaLog log("artifact-users");
    ASSERT_EQ(log.appendDelta(DeltaMutationType::INSERT, "node-1", "tx-1", "shard-a", 64), 1u);
    ASSERT_EQ(log.appendDelta(DeltaMutationType::UPDATE, "node-2", "tx-2", "shard-a", 96), 2u);
    ASSERT_EQ(log.appendDelta(DeltaMutationType::UPDATE, "node-3", "tx-3", "shard-a", 128), 3u);

    const auto window = log.extractWindow(1, 3);
    ASSERT_TRUE(window.has_value());
    EXPECT_EQ(window->entries.size(), 3u);
    EXPECT_TRUE(window->isValid());
    EXPECT_EQ(window->countInserts(), 1u);
    EXPECT_EQ(window->countUpdates(), 2u);
    EXPECT_EQ(window->total_payload_size_bytes, 288u);
}

TEST(TensorDeltaLogTest, RetentionEvictsOldestEntriesAndInvalidatesOldWindows) {
    TensorDeltaLog log("artifact-users");
    log.setRetentionPolicy(2, 60'000);

    ASSERT_EQ(log.appendDelta(DeltaMutationType::INSERT, "node-1", "tx-1", "shard-a", 32), 1u);
    ASSERT_EQ(log.appendDelta(DeltaMutationType::INSERT, "node-2", "tx-2", "shard-a", 32), 2u);
    ASSERT_EQ(log.appendDelta(DeltaMutationType::INSERT, "node-3", "tx-3", "shard-a", 32), 3u);

    EXPECT_EQ(log.size(), 2u);
    EXPECT_FALSE(log.extractWindow(1, 2).has_value());

    const auto tail = log.extractWindow(2, 3);
    ASSERT_TRUE(tail.has_value());
    EXPECT_EQ(tail->entries.size(), 2u);
}

TEST(TensorDeltaLogTest, WindowSerializationRoundTrips) {
    TensorDeltaLog log("artifact-users");
    ASSERT_EQ(log.appendDelta(DeltaMutationType::UPDATE, "node-7", "tx-7", "shard-b", 512), 1u);
    ASSERT_EQ(log.appendDelta(DeltaMutationType::UPDATE, "node-8", "tx-8", "shard-b", 256), 2u);

    const auto window = log.extractWindow(1, 2);
    ASSERT_TRUE(window.has_value());

    const auto encoded = window->serialize();
    const auto decoded = DeltaWindow::deserialize(encoded);
    ASSERT_TRUE(decoded.has_value());
    EXPECT_EQ(decoded->artifact_id, "artifact-users");
    EXPECT_EQ(decoded->entries.size(), 2u);
    EXPECT_EQ(decoded->entries[1].affected_entity_id, "node-8");
}

}  // namespace
>>>>>>> origin/develop
