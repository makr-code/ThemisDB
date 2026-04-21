/**
 * @file test_federated_distillation_coordinator.cpp
 * @brief Focused tests for FederatedDistillationCoordinator (FDF-01..10)
 *
 * FDF-01: Invalid config throws on construction.
 * FDF-02: submitSoftLabels advances round counter.
 * FDF-03: broadcastToStudents without prior submit throws.
 * FDF-04: broadcastToStudents dispatches to all registered students.
 * FDF-05: DP noise is applied (require_dp=true) — probabilities differ from input.
 * FDF-06: DP-noised probabilities remain valid (≥0, ≤1, sum≈1).
 * FDF-07: Policy gate rejection throws and does not dispatch to students.
 * FDF-08: Audit callback is invoked with correct fields.
 * FDF-09: Rollback trigger fires when student utility drops below threshold.
 * FDF-10: Privacy budget exhaustion blocks submit when max_rounds exceeded.
 */

#include <gtest/gtest.h>
#include "distributed_knowledge/federated_distillation_coordinator.h"
#include <atomic>
#include <numeric>

using namespace themis::distributed_knowledge;

// ── helpers ──────────────────────────────────────────────────────────────────

static SoftLabel makeSoftLabel(const std::string& qid = "q1",
                               std::vector<double> probs = {0.6, 0.3, 0.1})
{
    SoftLabel sl;
    sl.query_id     = qid;
    sl.probabilities = std::move(probs);
    sl.temperature  = 4.0;
    sl.teacher_id   = "teacher-large";
    return sl;
}

static DistillationConfig defaultCfg()
{
    DistillationConfig c;
    c.dp_epsilon   = 0.5;
    c.dp_delta     = 1e-5;
    c.dp_sensitivity = 0.1;
    c.temperature  = 4.0;
    c.max_rounds   = 0;
    c.require_dp   = true;
    return c;
}

// ── FDF-01: Invalid config throws ────────────────────────────────────────────

TEST(FDF_Tests, FDF_01_InvalidConfigThrows)
{
    DistillationConfig bad;
    bad.dp_epsilon = -1.0;    // invalid
    EXPECT_THROW(FederatedDistillationCoordinator{bad}, std::invalid_argument);
}

// ── FDF-02: submitSoftLabels advances round counter ───────────────────────────

TEST(FDF_Tests, FDF_02_SubmitAdvancesRound)
{
    FederatedDistillationCoordinator coord{defaultCfg()};
    EXPECT_EQ(coord.currentRound(), 0u);
    EXPECT_EQ(coord.submittedCount(), 0u);

    coord.submitSoftLabels("teacher-A", {makeSoftLabel()});

    EXPECT_EQ(coord.currentRound(), 1u);
    EXPECT_EQ(coord.submittedCount(), 1u);
}

// ── FDF-03: broadcastToStudents without submit throws ────────────────────────

TEST(FDF_Tests, FDF_03_BroadcastWithoutSubmitThrows)
{
    FederatedDistillationCoordinator coord{defaultCfg()};
    EXPECT_THROW(coord.broadcastToStudents(), std::runtime_error);
}

// ── FDF-04: broadcastToStudents dispatches to all registered students ─────────

TEST(FDF_Tests, FDF_04_BroadcastDispatchesToAllStudents)
{
    FederatedDistillationCoordinator coord{defaultCfg()};

    std::atomic<int> calls_A{0}, calls_B{0};
    std::string last_teacher;

    coord.registerStudent("student-A",
        [&calls_A, &last_teacher](const DistillationRound& r) {
            ++calls_A;
            last_teacher = r.teacher_id;
        });
    coord.registerStudent("student-B",
        [&calls_B](const DistillationRound&) { ++calls_B; });

    coord.submitSoftLabels("teacher-X", {makeSoftLabel("q1"), makeSoftLabel("q2")});
    const auto round = coord.broadcastToStudents();

    EXPECT_EQ(calls_A.load(), 1);
    EXPECT_EQ(calls_B.load(), 1);
    EXPECT_EQ(last_teacher, "teacher-X");
    EXPECT_EQ(round.label_count, 2u);
    EXPECT_EQ(round.teacher_id, "teacher-X");
    EXPECT_EQ(round.round, 1u);

    // lastRound() is populated
    ASSERT_TRUE(coord.lastRound().has_value());
    EXPECT_EQ(coord.lastRound()->round, 1u);
}

// ── FDF-05: DP noise changes probabilities ────────────────────────────────────

TEST(FDF_Tests, FDF_05_DPNoiseAltersDistribution)
{
    auto cfg      = defaultCfg();
    cfg.dp_epsilon   = 1.0;    // higher epsilon → less noise, but still nonzero
    cfg.dp_sensitivity = 1.0;  // large sensitivity → clearly visible noise
    cfg.require_dp = true;

    std::vector<double> original_probs = {0.7, 0.2, 0.1};
    bool any_diff = false;

    // Run 5 broadcasts to reduce chance of coincidental exact match
    for (int trial = 0; trial < 5; ++trial) {
        FederatedDistillationCoordinator coord{cfg};

        std::vector<double> received;
        coord.registerStudent("s1",
            [&received](const DistillationRound& r) {
                if (!r.labels.empty()) {
                    received = r.labels[0].probabilities;
                }
            });

        coord.submitSoftLabels("teacher", {makeSoftLabel("q1", original_probs)});
        coord.broadcastToStudents();

        if (received != original_probs) { any_diff = true; break; }
    }
    EXPECT_TRUE(any_diff) << "DP noise must alter at least one probability";
}

// ── FDF-06: DP-noised probabilities are valid ─────────────────────────────────

TEST(FDF_Tests, FDF_06_DPNoisedProbabilitiesAreValid)
{
    auto cfg = defaultCfg();
    cfg.dp_sensitivity = 1.0;
    FederatedDistillationCoordinator coord{cfg};

    std::vector<double> received;
    coord.registerStudent("s1",
        [&received](const DistillationRound& r) {
            if (!r.labels.empty()) received = r.labels[0].probabilities;
        });

    // 10-class distribution
    std::vector<double> probs(10, 0.1);
    coord.submitSoftLabels("teacher", {makeSoftLabel("q1", probs)});
    coord.broadcastToStudents();

    ASSERT_EQ(received.size(), 10u);
    double sum = 0.0;
    for (double p : received) {
        EXPECT_GE(p, 0.0) << "Probability must be >= 0";
        EXPECT_LE(p, 1.0) << "Probability must be <= 1";
        sum += p;
    }
    EXPECT_NEAR(sum, 1.0, 1e-9) << "Probabilities must sum to 1.0";
}

// ── FDF-07: Policy gate rejection blocks broadcast ────────────────────────────

TEST(FDF_Tests, FDF_07_PolicyGateBlocksBroadcast)
{
    FederatedDistillationCoordinator coord{defaultCfg()};

    std::atomic<int> student_calls{0};
    coord.registerStudent("s1",
        [&student_calls](const DistillationRound&) { ++student_calls; });

    // Block all broadcasts
    coord.setPolicyGate(
        [](uint64_t, const std::string&) -> bool { return false; });

    coord.submitSoftLabels("teacher", {makeSoftLabel()});
    EXPECT_THROW(coord.broadcastToStudents(), std::runtime_error);

    // Student must NOT have been called
    EXPECT_EQ(student_calls.load(), 0);
    // lastRound must still be empty
    EXPECT_FALSE(coord.lastRound().has_value());
}

// ── FDF-08: Audit callback receives correct fields ────────────────────────────

TEST(FDF_Tests, FDF_08_AuditCallbackCorrectFields)
{
    FederatedDistillationCoordinator coord{defaultCfg()};

    nlohmann::json audit_record;
    coord.setAuditCallback([&audit_record](const nlohmann::json& r) {
        audit_record = r;
    });

    coord.submitSoftLabels("teacher-audit",
                           {makeSoftLabel("q1"), makeSoftLabel("q2")});
    coord.broadcastToStudents();

    EXPECT_EQ(audit_record.value("event", ""), "distillation_broadcast");
    EXPECT_EQ(audit_record.value("teacher_id", ""), "teacher-audit");
    EXPECT_EQ(audit_record.value("label_count", size_t{0}), 2u);
    EXPECT_EQ(audit_record.value("round", uint64_t{0}), uint64_t{1});
    EXPECT_TRUE(audit_record.value("dp_applied", false));
    EXPECT_GT(audit_record.value("epsilon_spent", 0.0), 0.0);
}

// ── FDF-09: Rollback trigger fires on low student utility ─────────────────────

TEST(FDF_Tests, FDF_09_RollbackTriggerOnLowUtility)
{
    auto cfg = defaultCfg();
    cfg.min_utility_threshold = 0.90;
    FederatedDistillationCoordinator coord{cfg};

    std::atomic<bool>   triggered{false};
    uint64_t            rollback_round = 0;
    double              rollback_utility = 1.0;

    coord.setRollbackTrigger(
        [&triggered, &rollback_round, &rollback_utility](uint64_t rnd, double u) {
            triggered.store(true);
            rollback_round   = rnd;
            rollback_utility = u;
        });

    coord.submitSoftLabels("teacher", {makeSoftLabel()});
    coord.broadcastToStudents();

    // Report utility below threshold
    coord.reportStudentUtility("student-1", 0.85);

    EXPECT_TRUE(triggered.load());
    EXPECT_EQ(rollback_round, 1u);
    EXPECT_NEAR(rollback_utility, 0.85, 1e-9);
}

TEST(FDF_Tests, FDF_09b_RollbackTriggerNotFiredAboveThreshold)
{
    auto cfg = defaultCfg();
    cfg.min_utility_threshold = 0.90;
    FederatedDistillationCoordinator coord{cfg};

    bool triggered = false;
    coord.setRollbackTrigger(
        [&triggered](uint64_t, double) { triggered = true; });

    coord.submitSoftLabels("teacher", {makeSoftLabel()});
    coord.broadcastToStudents();

    // Utility above threshold — no rollback
    coord.reportStudentUtility("student-1", 0.95);
    EXPECT_FALSE(triggered);
}

// ── FDF-10: Privacy budget exhaustion blocks further submissions ───────────────

TEST(FDF_Tests, FDF_10_PrivacyBudgetExhausted)
{
    DistillationConfig cfg = defaultCfg();
    cfg.max_rounds = 2;   // only 2 rounds allowed
    FederatedDistillationCoordinator coord{cfg};

    // Round 1
    coord.submitSoftLabels("teacher", {makeSoftLabel()});
    coord.broadcastToStudents();

    // Round 2
    coord.submitSoftLabels("teacher", {makeSoftLabel()});
    coord.broadcastToStudents();

    // Round 3 must be rejected (budget exhausted)
    EXPECT_THROW(coord.submitSoftLabels("teacher", {makeSoftLabel()}),
                 std::runtime_error);
}

// ── Bonus: getStats fields ────────────────────────────────────────────────────

TEST(FDF_Tests, FDF_STATS_FieldsPopulated)
{
    FederatedDistillationCoordinator coord{defaultCfg()};

    coord.submitSoftLabels("teacher", {makeSoftLabel()});
    coord.broadcastToStudents();

    const auto stats = coord.getStats();
    EXPECT_EQ(stats.value("current_round", 0u),   1u);
    EXPECT_EQ(stats.value("broadcast_count", 0u), 1u);
    EXPECT_FALSE(stats.value("has_pending", true));
    EXPECT_GT(stats.value("total_epsilon", 0.0),  0.0);
    EXPECT_TRUE(stats.contains("config"));
}
