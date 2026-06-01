/**
 * @file artifact_lifecycle_test.cc
 * @brief Contract tests for IArtifactLifecycle (sub-issue #5442).
 *
 * Validates factory construction, upsert/lookup, freshness evaluation,
 * invalidation, rebuild triggering, policy registration, and callback setup.
 * Production staleness tracking is tracked in sub-issue #5442.
 */

#include "evaluation/include/artifact_lifecycle.h"

#include <gtest/gtest.h>
#include <chrono>
#include <memory>
#include <string>

using namespace themis::evaluation;

namespace {

ArtifactRecord makeRecord(const std::string& id,
                           ArtifactFreshness freshness = ArtifactFreshness::Fresh) {
    ArtifactRecord r;
    r.id            = id;
    r.source_id     = "src-" + id;
    r.artifact_type = "hnsw_index";
    r.freshness     = freshness;
    r.created_at    = std::chrono::system_clock::now();
    r.expires_at    = r.created_at + std::chrono::hours(1);
    r.storage_path  = "/tmp/test-artifact/" + id;
    return r;
}

} // namespace

class ArtifactLifecycleTest : public ::testing::Test {
protected:
    void SetUp() override {
        lifecycle_ = makeArtifactLifecycle();
        ASSERT_NE(lifecycle_, nullptr);
    }

    std::unique_ptr<IArtifactLifecycle> lifecycle_;
};

TEST_F(ArtifactLifecycleTest, FactoryReturnsNonNull) {
    EXPECT_NE(lifecycle_, nullptr);
}

TEST_F(ArtifactLifecycleTest, LookupUnknownIdReturnsNullopt) {
    auto result = lifecycle_->lookup("no-such-artifact");
    EXPECT_FALSE(result.has_value());
}

TEST_F(ArtifactLifecycleTest, UpsertThenLookup) {
    lifecycle_->upsert(makeRecord("art-001"));
    auto result = lifecycle_->lookup("art-001");
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result->id, "art-001");
}

TEST_F(ArtifactLifecycleTest, EvaluateFreshRecordReturnsFresh) {
    lifecycle_->upsert(makeRecord("art-002", ArtifactFreshness::Fresh));
    ArtifactFreshness f = lifecycle_->evaluate("art-002");
    EXPECT_EQ(f, ArtifactFreshness::Fresh);
}

TEST_F(ArtifactLifecycleTest, EvaluateMissingArtifactReturnsMissing) {
    ArtifactFreshness f = lifecycle_->evaluate("art-missing");
    EXPECT_EQ(f, ArtifactFreshness::Missing);
}

TEST_F(ArtifactLifecycleTest, InvalidateChangesToStale) {
    lifecycle_->upsert(makeRecord("art-003", ArtifactFreshness::Fresh));
    lifecycle_->invalidate("art-003", StalenessTrigger::SourceUpdated);
    auto result = lifecycle_->lookup("art-003");
    ASSERT_TRUE(result.has_value());
    EXPECT_NE(result->freshness, ArtifactFreshness::Fresh);
}

TEST_F(ArtifactLifecycleTest, InvalidateNonexistentDoesNotThrow) {
    EXPECT_NO_THROW(
        lifecycle_->invalidate("no-such-artifact", StalenessTrigger::ManualInvalidation));
}

TEST_F(ArtifactLifecycleTest, TriggerRebuildDoesNotThrow) {
    lifecycle_->upsert(makeRecord("art-004", ArtifactFreshness::Stale));
    EXPECT_NO_THROW(lifecycle_->triggerRebuild("art-004"));
}

TEST_F(ArtifactLifecycleTest, RegisterPolicyDoesNotThrow) {
    ArtifactPolicy policy;
    policy.artifact_type     = "tensor_summary";
    policy.ttl               = std::chrono::seconds{1800};
    policy.auto_rebuild      = true;
    policy.max_rebuild_retries = 2;
    EXPECT_NO_THROW(lifecycle_->registerPolicy(policy));
}

TEST_F(ArtifactLifecycleTest, RebuildCallbackRegistrationDoesNotThrow) {
    EXPECT_NO_THROW(lifecycle_->onRebuild([](const ArtifactRecord&) {}));
}

TEST_F(ArtifactLifecycleTest, UpsertOverwritesPreviousRecord) {
    lifecycle_->upsert(makeRecord("art-005", ArtifactFreshness::Fresh));
    auto updated = makeRecord("art-005", ArtifactFreshness::Stale);
    lifecycle_->upsert(updated);
    auto result = lifecycle_->lookup("art-005");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->freshness, ArtifactFreshness::Stale);
}
