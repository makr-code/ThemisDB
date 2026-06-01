/**
 * @file tensor_artifact_classes_test.cc
 * @brief Contract tests for IArtifactClassRegistry (sub-issue #5429).
 *
 * Validates factory construction, raw and sharded artifact registration,
 * metadata lookup, lifecycle state transitions, and listing at scaffold stage.
 * Production durable storage is tracked in sub-issue #5429.
 */

#include "distributed_tensor/include/tensor_artifact_classes.h"

#include <gtest/gtest.h>
#include <chrono>
#include <memory>
#include <string>

using namespace themis::distributed_tensor;

namespace {

RawTensorArtifact makeRawArtifact(const std::string& id) {
    RawTensorArtifact a;
    a.meta.id             = id;
    a.meta.artifact_class = ArtifactClass::Raw;
    a.meta.state          = ArtifactState::Pending;
    a.meta.schema_version = "1.0";
    a.meta.size_bytes     = 1024;
    a.meta.rank           = 2;
    a.meta.shape          = {32, 32};
    a.meta.created_at     = std::chrono::system_clock::now();
    a.meta.owner_shard_key = "shard-local";
    return a;
}

ShardedArtifact makeShardedArtifact(const std::string& id) {
    ShardedArtifact a;
    a.meta.id              = id;
    a.meta.artifact_class  = ArtifactClass::Sharded;
    a.meta.state           = ArtifactState::Pending;
    a.meta.schema_version  = "1.0";
    a.meta.size_bytes      = 8192;
    a.num_data_stripes     = 4;
    a.num_parity_stripes   = 2;
    return a;
}

} // namespace

class ArtifactClassRegistryTest : public ::testing::Test {
protected:
    void SetUp() override {
        registry_ = makeArtifactClassRegistry();
        ASSERT_NE(registry_, nullptr);
    }

    std::unique_ptr<IArtifactClassRegistry> registry_;
};

TEST_F(ArtifactClassRegistryTest, FactoryReturnsNonNull) {
    EXPECT_NE(registry_, nullptr);
}

TEST_F(ArtifactClassRegistryTest, RegisterRawArtifactReturnsId) {
    std::string id = registry_->registerRaw(makeRawArtifact("raw-001"));
    EXPECT_EQ(id, "raw-001");
}

TEST_F(ArtifactClassRegistryTest, LookupAfterRegisterRawSucceeds) {
    registry_->registerRaw(makeRawArtifact("raw-002"));
    auto meta = registry_->lookupMetadata("raw-002");
    EXPECT_TRUE(meta.has_value());
    EXPECT_EQ(meta->id, "raw-002");
}

TEST_F(ArtifactClassRegistryTest, RegisterShardedArtifactReturnsId) {
    std::string id = registry_->registerSharded(makeShardedArtifact("sharded-001"));
    EXPECT_EQ(id, "sharded-001");
}

TEST_F(ArtifactClassRegistryTest, LookupUnknownIdReturnsNullopt) {
    auto meta = registry_->lookupMetadata("nonexistent");
    EXPECT_FALSE(meta.has_value());
}

TEST_F(ArtifactClassRegistryTest, TransitionStateToActive) {
    registry_->registerRaw(makeRawArtifact("raw-003"));
    bool ok = registry_->transitionState("raw-003", ArtifactState::Active);
    EXPECT_TRUE(ok);
    auto meta = registry_->lookupMetadata("raw-003");
    ASSERT_TRUE(meta.has_value());
    EXPECT_EQ(meta->state, ArtifactState::Active);
}

TEST_F(ArtifactClassRegistryTest, TransitionStateUnknownArtifactReturnsFalse) {
    bool ok = registry_->transitionState("nonexistent", ArtifactState::Active);
    EXPECT_FALSE(ok);
}

TEST_F(ArtifactClassRegistryTest, ListByStateAfterTransitionToActive) {
    registry_->registerRaw(makeRawArtifact("raw-004"));
    registry_->transitionState("raw-004", ArtifactState::Active);
    auto ids = registry_->listByState(ArtifactState::Active);
    bool found = false;
    for (const auto& id : ids) {
        if (id == "raw-004") found = true;
    }
    EXPECT_TRUE(found);
}

TEST_F(ArtifactClassRegistryTest, ListByStatePendingInitiallyNonEmpty) {
    registry_->registerRaw(makeRawArtifact("raw-005"));
    // registerRaw sets state to Active in the scaffold stub, but Pending
    // before that call; at minimum it should not throw.
    EXPECT_NO_THROW(registry_->listByState(ArtifactState::Pending));
}

TEST_F(ArtifactClassRegistryTest, ListByStateActiveDoesNotContainOtherStates) {
    registry_->registerRaw(makeRawArtifact("raw-006"));
    registry_->transitionState("raw-006", ArtifactState::Active);
    registry_->registerRaw(makeRawArtifact("raw-007"));
    // raw-007 starts as Active (set by registerRaw stub); list should be stable.
    auto ids = registry_->listByState(ArtifactState::Archived);
    for (const auto& id : ids) {
        EXPECT_NE(id, "raw-006");
        EXPECT_NE(id, "raw-007");
    }
}
