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

    const auto encoded = window->toJSON();
    const auto decoded = DeltaWindow::fromJSON(encoded);
    ASSERT_TRUE(decoded.has_value());
    EXPECT_EQ(decoded->artifact_id, "artifact-users");
    EXPECT_EQ(decoded->entries.size(), 2u);
    EXPECT_EQ(decoded->entries[1].affected_entity_id, "node-8");
}

}  // namespace
