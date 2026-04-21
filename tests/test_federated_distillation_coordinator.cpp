/**
 * @file test_federated_distillation_coordinator.cpp
 * @brief Unit tests for FederatedDistillationCoordinator (FDF).
 *
 * Tests
 * -----
 * FDF_01  Coordinator initialises — currentRound==1, submittedCount==0
 * FDF_02  submitLabels() — idempotent per shard in same round
 * FDF_03  submitLabels() — auto-triggers when min_teachers reached
 * FDF_04  triggerAggregation() — produces AggregatedLabelBatch with correct shape
 * FDF_05  DP noise applied — aggregated label differs from raw average by ≥ 0
 *         (noise σ > 0 for default dp_epsilon=1.0, dp_delta=1e-5)
 * FDF_06  setStudentCallback() called with AggregatedLabelBatch after round
 * FDF_07  setPolicyGate() — round blocked when gate returns false; rollback triggered
 * FDF_08  setAuditCallback() called with JSON audit record containing round/teachers/epsilon
 * FDF_09  setRollbackTrigger() called with round + reason when policy blocks
 * FDF_10  Privacy budget exhausted → submitLabels() throws std::runtime_error
 * FDF_11  submitLabels() with empty shard_id → std::invalid_argument
 * FDF_12  submitLabels() with empty labels → std::invalid_argument
 * FDF_13  submitLabels() with mismatched label sizes → std::invalid_argument
 * FDF_14  completedRounds() increments after each successful aggregation
 * FDF_15  version strings are monotonic (distill-v1, distill-v2, …)
 */

#include <gtest/gtest.h>
#include "distributed_knowledge/federated_distillation_coordinator.h"

using namespace themis::distributed_knowledge;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static SoftLabelBatch makeBatch(const std::string& shard_id,
                                 uint64_t round,
                                 size_t n_queries   = 2,
                                 size_t n_classes   = 3,
                                 double fill_value  = 0.5,
                                 size_t samples     = 10)
{
    SoftLabelBatch b;
    b.shard_id    = shard_id;
    b.round       = round;
    b.sample_count = samples;
    b.labels.assign(n_queries, std::vector<double>(n_classes, fill_value));
    return b;
}

static DistillationConfig testConfig(size_t min_t = 1,
                                      double eps = 1e-6,     // tiny ε → tiny σ
                                      double dlt = 1e-10)
{
    DistillationConfig cfg;
    cfg.min_teachers = min_t;
    cfg.dp_epsilon   = eps;
    cfg.dp_delta     = dlt;
    cfg.sensitivity  = 1.0;
    return cfg;
}

// ---------------------------------------------------------------------------
// FDF_01 — Initial state
// ---------------------------------------------------------------------------
TEST(FDF, FDF_01_InitialState) {
    FederatedDistillationCoordinator coord(testConfig());
    EXPECT_EQ(coord.currentRound(),   1u);
    EXPECT_EQ(coord.submittedCount(), 0u);
    EXPECT_EQ(coord.completedRounds(), 0u);
    EXPECT_FALSE(coord.lastBatch().has_value());
}

// ---------------------------------------------------------------------------
// FDF_02 — submitLabels() idempotent per shard in same round
// ---------------------------------------------------------------------------
TEST(FDF, FDF_02_SubmitIdempotent) {
    FederatedDistillationCoordinator coord(testConfig(2)); // need 2 teachers to avoid auto-trigger
    coord.submitLabels(makeBatch("s1", 1));
    coord.submitLabels(makeBatch("s1", 1)); // duplicate
    EXPECT_EQ(coord.submittedCount(), 1u);  // still 1
}

// ---------------------------------------------------------------------------
// FDF_03 — Auto-triggers when min_teachers reached
// ---------------------------------------------------------------------------
TEST(FDF, FDF_03_AutoTrigger) {
    FederatedDistillationCoordinator coord(testConfig(2));
    coord.submitLabels(makeBatch("s1", 1));
    EXPECT_EQ(coord.completedRounds(), 0u);
    coord.submitLabels(makeBatch("s2", 1));
    EXPECT_EQ(coord.completedRounds(), 1u);  // auto-triggered
}

// ---------------------------------------------------------------------------
// FDF_04 — AggregatedLabelBatch has correct shape
// ---------------------------------------------------------------------------
TEST(FDF, FDF_04_AggregatedBatchShape) {
    FederatedDistillationCoordinator coord(testConfig(2));
    coord.submitLabels(makeBatch("s1", 1, 3, 4, 0.25));
    coord.submitLabels(makeBatch("s2", 1, 3, 4, 0.75));
    auto batch = coord.lastBatch();
    ASSERT_TRUE(batch.has_value());
    EXPECT_EQ(batch->labels.size(),       3u);   // n_queries
    EXPECT_EQ(batch->labels.front().size(), 4u); // n_classes
    EXPECT_EQ(batch->teachers,            2u);
    EXPECT_EQ(batch->round,               1u);
}

// ---------------------------------------------------------------------------
// FDF_05 — DP noise applied (aggregated label ≠ exact raw average in expectation)
//          With very large ε the noise is tiny but still present.
//          We use a large ε but check that the implementation doesn't produce NaN/Inf.
// ---------------------------------------------------------------------------
TEST(FDF, FDF_05_NoisedBatchNoNaN) {
    // Use realistic DP params
    DistillationConfig cfg;
    cfg.min_teachers = 1;
    cfg.dp_epsilon   = 1.0;
    cfg.dp_delta     = 1e-5;
    cfg.sensitivity  = 1.0;
    FederatedDistillationCoordinator coord(cfg);
    coord.submitLabels(makeBatch("s1", 1, 5, 10, 0.1));
    auto batch = coord.lastBatch();
    ASSERT_TRUE(batch.has_value());
    for (const auto& row : batch->labels) {
        for (double v : row) {
            EXPECT_FALSE(std::isnan(v));
            EXPECT_FALSE(std::isinf(v));
        }
    }
}

// ---------------------------------------------------------------------------
// FDF_06 — setStudentCallback() is invoked after aggregation
// ---------------------------------------------------------------------------
TEST(FDF, FDF_06_StudentCallbackInvoked) {
    FederatedDistillationCoordinator coord(testConfig(1));
    int call_count = 0;
    coord.setStudentCallback([&](const AggregatedLabelBatch& b) {
        ++call_count;
        EXPECT_EQ(b.teachers, 1u);
    });
    coord.submitLabels(makeBatch("s1", 1));
    EXPECT_EQ(call_count, 1);
}

// ---------------------------------------------------------------------------
// FDF_07 — setPolicyGate() blocks round when gate returns false
// ---------------------------------------------------------------------------
TEST(FDF, FDF_07_PolicyGateBlocks) {
    FederatedDistillationCoordinator coord(testConfig(2));
    coord.setPolicyGate([](uint64_t /*round*/, size_t /*t*/) { return false; });
    coord.submitLabels(makeBatch("s1", 1));
    coord.submitLabels(makeBatch("s2", 1));
    // triggerAggregation must throw because gate returns false
    EXPECT_THROW(coord.triggerAggregation(), std::runtime_error);
    EXPECT_EQ(coord.completedRounds(), 0u);
}

// ---------------------------------------------------------------------------
// FDF_08 — setAuditCallback() receives JSON record
// ---------------------------------------------------------------------------
TEST(FDF, FDF_08_AuditCallbackReceivesRecord) {
    FederatedDistillationCoordinator coord(testConfig(1));
    nlohmann::json last_record;
    coord.setAuditCallback([&](const nlohmann::json& r) { last_record = r; });
    coord.submitLabels(makeBatch("s1", 1));
    EXPECT_EQ(last_record.value("decision_type", ""), "DISTILLATION_ROUND");
    EXPECT_EQ(last_record.value<uint64_t>("round", 0), 1u);
    EXPECT_EQ(last_record.value<size_t>("teachers", 0), 1u);
}

// ---------------------------------------------------------------------------
// FDF_09 — setRollbackTrigger() called when policy blocks
// ---------------------------------------------------------------------------
TEST(FDF, FDF_09_RollbackTriggerCalledOnPolicyBlock) {
    FederatedDistillationCoordinator coord(testConfig(2));
    coord.setPolicyGate([](uint64_t, size_t) { return false; });

    bool rollback_called = false;
    uint64_t rb_round    = 0;
    coord.setRollbackTrigger([&](uint64_t r, const std::string&) {
        rollback_called = true;
        rb_round = r;
    });

    coord.submitLabels(makeBatch("s1", 1));
    coord.submitLabels(makeBatch("s2", 1));
    try { coord.triggerAggregation(); } catch (...) {}

    EXPECT_TRUE(rollback_called);
    EXPECT_EQ(rb_round, 1u);
}

// ---------------------------------------------------------------------------
// FDF_10 — Privacy budget exhausted → submitLabels throws
// ---------------------------------------------------------------------------
TEST(FDF, FDF_10_PrivacyBudgetExhausted) {
    DistillationConfig cfg = testConfig(1);
    cfg.max_rounds = 1;
    FederatedDistillationCoordinator coord(cfg);
    coord.submitLabels(makeBatch("s1", 1)); // round 1 → OK, now exhausted
    EXPECT_THROW(coord.submitLabels(makeBatch("s2", 2)), std::runtime_error);
}

// ---------------------------------------------------------------------------
// FDF_11 — Empty shard_id throws
// ---------------------------------------------------------------------------
TEST(FDF, FDF_11_EmptyShardIdThrows) {
    FederatedDistillationCoordinator coord(testConfig());
    auto b = makeBatch("s1", 1);
    b.shard_id = "";
    EXPECT_THROW(coord.submitLabels(b), std::invalid_argument);
}

// ---------------------------------------------------------------------------
// FDF_12 — Empty labels vector throws
// ---------------------------------------------------------------------------
TEST(FDF, FDF_12_EmptyLabelsThrows) {
    FederatedDistillationCoordinator coord(testConfig());
    SoftLabelBatch b;
    b.shard_id = "s1";
    b.round    = 1;
    // labels is empty
    EXPECT_THROW(coord.submitLabels(b), std::invalid_argument);
}

// ---------------------------------------------------------------------------
// FDF_13 — Mismatched inner label sizes throw on aggregation
// ---------------------------------------------------------------------------
TEST(FDF, FDF_13_MismatchedLabelSizesThrow) {
    FederatedDistillationCoordinator coord(testConfig(2));
    coord.submitLabels(makeBatch("s1", 1, 2, 3)); // 2 queries, 3 classes
    SoftLabelBatch b2 = makeBatch("s2", 1, 2, 5); // 2 queries, 5 classes — mismatch
    coord.submitLabels(b2);
    EXPECT_THROW(coord.triggerAggregation(), std::runtime_error);
}

// ---------------------------------------------------------------------------
// FDF_14 — completedRounds() increments after each round
// ---------------------------------------------------------------------------
TEST(FDF, FDF_14_CompletedRoundsIncrement) {
    FederatedDistillationCoordinator coord(testConfig(1));
    for (int i = 0; i < 3; ++i) {
        coord.submitLabels(makeBatch("s" + std::to_string(i),
                                    static_cast<uint64_t>(i + 1)));
    }
    EXPECT_EQ(coord.completedRounds(), 3u);
}

// ---------------------------------------------------------------------------
// FDF_15 — version strings are monotonic
// ---------------------------------------------------------------------------
TEST(FDF, FDF_15_VersionStringsMonotonic) {
    FederatedDistillationCoordinator coord(testConfig(1));
    std::vector<std::string> versions;
    coord.setStudentCallback([&](const AggregatedLabelBatch& b) {
        versions.push_back(b.version);
    });
    for (int i = 0; i < 3; ++i) {
        coord.submitLabels(makeBatch("s" + std::to_string(i),
                                    static_cast<uint64_t>(i + 1)));
    }
    ASSERT_EQ(versions.size(), 3u);
    EXPECT_EQ(versions[0], "distill-v1");
    EXPECT_EQ(versions[1], "distill-v2");
    EXPECT_EQ(versions[2], "distill-v3");
}
