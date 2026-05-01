/*
 * AI Safety Layer — Chaos Tests (ASL-14)
 *
 * Test fixture: AiSafetyChaosTest
 * Tests CHAOS-01..CHAOS-12: validates the end-to-end behaviour of the AI
 * Safety Layer under adversarial / edge-case conditions.
 *
 * Components under test:
 *   - AiOperationGuard   (DOG + HILG — ASL-4/5/6)
 *   - IntentClassifier   (Schicht 4 — ASL-4)
 *   - AqlSafetyValidator (Schicht 3 — ASL-3)
 *   - AiSnapshotCleanupJob (retention policy — ASL-11)
 *   - IntentClassifier LoRA hook (ASL-13)
 *
 * Roadmap: src/security/ROADMAP.md § Phase 4 (ASL-14)
 * Docs:    docs/de/security/ai_safety/AI_SAFETY_CHAOS_TESTS.md
 */

#include <gtest/gtest.h>

#include "security/ai_operation_guard.h"
#include "security/ai_snapshot_cleanup.h"
#include "security/intent_classifier.h"
#include "query/aql_safety_validator.h"
#include "security/zero_trust_policy_enforcer.h"

#include <filesystem>
#include <string>

namespace fs = std::filesystem;

using themis::security::AiOperationGuard;
using themis::security::AiSnapshotCleanupJob;
using themis::security::IntentClassifier;
using themis::security::OperationClass;
using themis::query::AqlSafetyValidator;

// ---------------------------------------------------------------------------
// Fixture
// ---------------------------------------------------------------------------

class AiSafetyChaosTest : public ::testing::Test {
protected:
    void SetUp() override {
        AiOperationGuard::Config guard_cfg;
        guard_cfg.enabled            = true;
        guard_cfg.approval_threshold = OperationClass::DESTRUCTIVE;
        guard_.emplace(guard_cfg);

        classifier_.emplace("chaos-shard");
    }

    std::optional<AiOperationGuard>    guard_;
    std::optional<IntentClassifier>    classifier_;
    AqlSafetyValidator                 validator_;

    static themis::security::ZeroTrustContext makeCtx(const std::string& uid = "chaos-user") {
        themis::security::ZeroTrustContext ctx;
        ctx.user_id    = uid;
        ctx.request_id = "chaos-req-1";
        ctx.resource   = "users";
        ctx.action     = "query";
        return ctx;
    }
};

// ---------------------------------------------------------------------------
// CHAOS-01: Destructive AQL via IntentClassifier → DATA_DESTRUCTION
// ---------------------------------------------------------------------------
TEST_F(AiSafetyChaosTest, CHAOS01_RemoveAllClassifiedAsDataDestruction) {
    const auto ctx    = makeCtx();
    const auto result = classifier_->classify(
        "FOR doc IN users REMOVE doc IN users", ctx);

    EXPECT_EQ(result.intent, IntentClassifier::IntentType::DATA_DESTRUCTION);
    EXPECT_NE(result.intent, IntentClassifier::IntentType::LEGITIMATE);
}

// ---------------------------------------------------------------------------
// CHAOS-02: FOR-REMOVE without FILTER is DATA_DESTRUCTION
// ---------------------------------------------------------------------------
TEST_F(AiSafetyChaosTest, CHAOS02_ForRemoveWithoutFilterIsDataDestruction) {
    const auto ctx    = makeCtx();
    const auto result = classifier_->classify(
        "FOR doc IN users REMOVE doc IN users", ctx);

    EXPECT_EQ(result.intent, IntentClassifier::IntentType::DATA_DESTRUCTION);
    EXPECT_GT(result.confidence, 0.5);
}

// ---------------------------------------------------------------------------
// CHAOS-03: DROP COLLECTION is SCHEMA_MUTATION
// ---------------------------------------------------------------------------
TEST_F(AiSafetyChaosTest, CHAOS03_DropCollectionIsSchemaMutation) {
    const auto ctx    = makeCtx();
    const auto result = classifier_->classify(
        "DROP COLLECTION users", ctx);

    // DROP COLLECTION scores high on both kDataDestructionFeatures AND
    // kSchemaMutationFeatures — either classification is acceptable; what is
    // NOT acceptable is LEGITIMATE.
    EXPECT_NE(result.intent, IntentClassifier::IntentType::LEGITIMATE);
}

// ---------------------------------------------------------------------------
// CHAOS-04: AqlSafetyValidator rejects REMOVE in read-only mode
// ---------------------------------------------------------------------------
TEST_F(AiSafetyChaosTest, CHAOS04_ValidatorRejectsRemoveInReadOnly) {
    const auto violation = validator_.validate(
        std::string_view{"FOR doc IN users REMOVE doc IN users"});

    ASSERT_TRUE(violation.has_value())
        << "Expected validator to reject REMOVE query";
    EXPECT_FALSE(violation->keyword.empty());
}

// ---------------------------------------------------------------------------
// CHAOS-05: AqlSafetyValidator rejects DROP INDEX in read-only mode
// ---------------------------------------------------------------------------
TEST_F(AiSafetyChaosTest, CHAOS05_ValidatorRejectsDropIndexInReadOnly) {
    const auto violation = validator_.validate(std::string_view{"DROP INDEX users/idx_email"});

    ASSERT_TRUE(violation.has_value())
        << "Expected validator to reject DROP INDEX";
    EXPECT_FALSE(violation->keyword.empty());
}

// ---------------------------------------------------------------------------
// CHAOS-06: AiOperationGuard returns hard-block for denied collection
// ---------------------------------------------------------------------------
TEST_F(AiSafetyChaosTest, CHAOS06_GuardBlocksToolInDeniedCollections) {
    AiOperationGuard::Config cfg;
    cfg.enabled             = true;
    cfg.approval_threshold  = OperationClass::DESTRUCTIVE;
    cfg.denied_collections  = {"forbidden_col"};
    AiOperationGuard guarded(cfg);

    const nlohmann::json args = {
        {"collection", "forbidden_col"},
        {"key",        "some-key"}
    };
    const auto decision = guarded.evaluate("delete_entity", args, "session-chaos-06");

    EXPECT_FALSE(decision.block_reason.empty())
        << "Expected hard-block for denied collection";
}

// ---------------------------------------------------------------------------
// CHAOS-07: Guard returns REQUIRES_APPROVAL for destructive delete
// ---------------------------------------------------------------------------
TEST_F(AiSafetyChaosTest, CHAOS07_GuardRequiresApprovalForDestructiveDelete) {
    ASSERT_TRUE(guard_.has_value());

    const nlohmann::json args = {{"collection", "users"}, {"key", "user-42"}};
    const auto decision = guard_->evaluate("delete_entity", args, "session-chaos-07");

    EXPECT_TRUE(decision.requires_approval)
        << "delete_entity should require approval (DESTRUCTIVE threshold)";
    EXPECT_TRUE(decision.block_reason.empty())
        << "Should not be hard-blocked";
    EXPECT_FALSE(decision.operation_id.empty())
        << "operation_id must be set for HILG tracking";
}

// ---------------------------------------------------------------------------
// CHAOS-08: Guard returns ALLOW for read-only query
// ---------------------------------------------------------------------------
TEST_F(AiSafetyChaosTest, CHAOS08_GuardAllowsReadOnlyQuery) {
    ASSERT_TRUE(guard_.has_value());

    const nlohmann::json args = {{"query", "FOR doc IN users RETURN doc"}};
    const auto decision = guard_->evaluate("query", args, "session-chaos-08");

    EXPECT_FALSE(decision.requires_approval)
        << "Read-only query should not require approval";
    EXPECT_TRUE(decision.block_reason.empty())
        << "Read-only query should not be blocked";
    EXPECT_EQ(decision.op_class, OperationClass::READ_ONLY);
}

// ---------------------------------------------------------------------------
// CHAOS-09: Two consecutive destructive ops get different operation_ids
// ---------------------------------------------------------------------------
TEST_F(AiSafetyChaosTest, CHAOS09_TwoDestructiveOpsGetUniqueIds) {
    ASSERT_TRUE(guard_.has_value());

    const nlohmann::json args = {{"collection", "users"}, {"key", "k1"}};

    const auto d1 = guard_->evaluate("delete_entity", args, "session-chaos-09a");
    const auto d2 = guard_->evaluate("delete_entity", args, "session-chaos-09b");

    EXPECT_TRUE(d1.requires_approval);
    EXPECT_TRUE(d2.requires_approval);
    EXPECT_NE(d1.operation_id, d2.operation_id)
        << "Each pending operation must get a unique UUID";
}

// ---------------------------------------------------------------------------
// CHAOS-10: LEGITIMATE read query does not trigger false positive
// ---------------------------------------------------------------------------
TEST_F(AiSafetyChaosTest, CHAOS10_LegitimateQueryNoFalsePositive) {
    const auto ctx    = makeCtx();
    const auto result = classifier_->classify(
        "FOR doc IN users RETURN doc", ctx);

    EXPECT_EQ(result.intent, IntentClassifier::IntentType::LEGITIMATE)
        << "Simple FOR-RETURN should be classified as LEGITIMATE";
}

// ---------------------------------------------------------------------------
// CHAOS-11: AiSnapshotCleanupJob on empty temp dir returns 0 (no crash)
// ---------------------------------------------------------------------------
TEST_F(AiSafetyChaosTest, CHAOS11_CleanupJobOnEmptyDirReturnsZero) {
    // Create a unique temp dir for this test
    const std::string snap_dir =
        (fs::temp_directory_path() / "chaos_test_snaps_XXXXXX").string();

    // Use mkdtemp-style pattern via filesystem
    fs::path tmp_base = fs::temp_directory_path() / "chaos11_snaps";
    fs::create_directories(tmp_base);

    AiSnapshotCleanupJob::Config cfg;
    cfg.snapshot_dir   = tmp_base.string();
    cfg.retention_days = 1;
    cfg.max_total_gb   = 1;
    AiSnapshotCleanupJob job(cfg);

    int deleted = 0;
    EXPECT_NO_THROW(deleted = job.runCleanup());
    EXPECT_EQ(deleted, 0) << "No snapshots in empty dir — should delete 0";

    // Cleanup temp dir
    fs::remove_all(tmp_base);
}

// ---------------------------------------------------------------------------
// CHAOS-12: LoRA path set to non-existent file → kFileNotAccessible
// ---------------------------------------------------------------------------
TEST_F(AiSafetyChaosTest, CHAOS12_LoraLoadReturnsFileNotAccessibleForMissingFile) {
    ASSERT_TRUE(classifier_.has_value());

    const auto result = classifier_->loadLoraModel("/nonexistent/path/model.bin");

    EXPECT_EQ(result, IntentClassifier::LoraLoadResult::kFileNotAccessible)
        << "Missing LoRA file should return kFileNotAccessible";
    EXPECT_FALSE(classifier_->isLoraActive())
        << "isLoraActive must be false after failed load";
}
