/**
 * @file test_federated_privacy_training.cpp
 * @brief Unit tests for Wave C C2 — Federated Learning for Privacy-Preserving
 *        Training (FEDERATED-01 … FEDERATED-10).
 *
 * Acceptance criteria from issue #5040 C2:
 *  - Training convergence ≥ 95 % of centralised baseline
 *  - Gradient communication overhead ≤ 2.0 s per round
 *  - Configurable epsilon-differential privacy budget
 *
 * Components under test:
 *  - FederatedImportCoordinator::FederatedAggregator  (FedAvg, median)
 *  - FederatedImportCoordinator::DifferentialPrivacyManager
 *  - AllReduceAggregator / GradientTensor (llm::distributed_training)
 *
 * All tests are self-contained (no network, no GPU, no real LoRA weights).
 *
 * Reference: Kairouz et al. (2021) JMLR 2021, arXiv:2104.14881
 * Wave C issue: #5040
 */

#include <gtest/gtest.h>

#include "importers/federated_learning.h"
#include "llm/distributed_training_coordinator.h"

#include <chrono>
#include <cmath>
#include <numeric>
#include <string>
#include <vector>

using namespace themis::importers;
using namespace themis::llm;
using json = nlohmann::json;

// ============================================================================
// Helpers
// ============================================================================

static FederatedImportCoordinator::FederatedAggregator::ParticipantUpdate
makeUpdate(const std::string& id, double loss_val, double grad_norm) {
    FederatedImportCoordinator::FederatedAggregator::ParticipantUpdate u;
    u.participant_id       = id;
    u.schema_contribution  = json::object();
    u.statistics           = json{{"loss", loss_val}, {"grad_norm", grad_norm}};
    u.encrypted_gradient   = json::object();
    return u;
}

// ============================================================================
// FEDERATED-01: FedAvg aggregates numeric statistics correctly
// ============================================================================
TEST(FederatedPrivacyTraining, FEDERATED01_FedAvgAggregatesStatistics) {
    FederatedImportCoordinator::FederatedAggregator aggregator;

    std::vector<FederatedImportCoordinator::FederatedAggregator::ParticipantUpdate> updates = {
        makeUpdate("node-1", 1.0, 0.5),
        makeUpdate("node-2", 3.0, 0.7),
        makeUpdate("node-3", 2.0, 0.6),
    };

    json result = aggregator.aggregateUpdates(updates, "FedAvg");

    ASSERT_TRUE(result.contains("loss"))
        << "FEDERATED-01: aggregated result must contain 'loss'";
    const double avg_loss = result["loss"].get<double>();
    EXPECT_NEAR(avg_loss, 2.0, 1e-6)
        << "FEDERATED-01: FedAvg of {1.0, 3.0, 2.0} should be 2.0";

    ASSERT_TRUE(result.contains("grad_norm"))
        << "FEDERATED-01: aggregated result must contain 'grad_norm'";
    EXPECT_NEAR(result["grad_norm"].get<double>(), 0.6, 1e-6)
        << "FEDERATED-01: FedAvg of {0.5, 0.7, 0.6} should be 0.6";
}

// ============================================================================
// FEDERATED-02: Byzantine-robust median aggregation
// ============================================================================
TEST(FederatedPrivacyTraining, FEDERATED02_MedianAggregationIsRobust) {
    FederatedImportCoordinator::FederatedAggregator aggregator;

    // One Byzantine node contributes a wildly different value.
    std::vector<FederatedImportCoordinator::FederatedAggregator::ParticipantUpdate> updates = {
        makeUpdate("node-1", 1.0, 0.5),
        makeUpdate("node-2", 1.1, 0.52),
        makeUpdate("byzantine", 9999.0, 9999.0),  // outlier
    };

    json result = aggregator.aggregateUpdates(updates, "median");

    ASSERT_TRUE(result.contains("loss"))
        << "FEDERATED-02: aggregated result must contain 'loss'";
    const double med_loss = result["loss"].get<double>();

    // Median of {1.0, 1.1, 9999} = 1.1; should be far below the Byzantine value.
    EXPECT_LT(med_loss, 100.0)
        << "FEDERATED-02: median aggregation should suppress Byzantine outlier "
           "(expected ~1.1, got " << med_loss << ")";
}

// ============================================================================
// FEDERATED-03: Differential privacy manager adds noise to statistics
// ============================================================================
TEST(FederatedPrivacyTraining, FEDERATED03_DPManagerAddsGaussianNoise) {
    FederatedImportCoordinator::DifferentialPrivacyManager dp;

    const json clean = {{"loss", 1.0}, {"grad_norm", 0.5}};
    const json noisy = dp.addDifferentialPrivacy(clean, /*epsilon=*/0.1, /*delta=*/1e-5);

    // Noisy values must exist and differ from input (with overwhelming probability).
    ASSERT_TRUE(noisy.contains("loss"))
        << "FEDERATED-03: noisy output must contain 'loss'";
    ASSERT_TRUE(noisy.contains("grad_norm"))
        << "FEDERATED-03: noisy output must contain 'grad_norm'";

    // The values should not be equal to the clean inputs (noise is added).
    // There is a negligibly small probability this could fail if noise = 0.
    const double noisy_loss = noisy["loss"].get<double>();
    EXPECT_NE(noisy_loss, 1.0)
        << "FEDERATED-03: DP noise should alter the statistic (epsilon=0.1)";
}

// ============================================================================
// FEDERATED-04: Privacy budget is configurable (epsilon parameter)
// ============================================================================
TEST(FederatedPrivacyTraining, FEDERATED04_EpsilonPrivacyBudgetConfigurable) {
    FederatedImportCoordinator::DifferentialPrivacyManager dp_tight;
    FederatedImportCoordinator::DifferentialPrivacyManager dp_loose;

    const json stats = {{"grad", 1.0}};

    // Tight epsilon → larger noise; loose epsilon → smaller noise.
    // Collect multiple samples and compare variance.
    constexpr int N = 20;
    double sum_tight = 0.0, sum_loose = 0.0;

    for (int i = 0; i < N; ++i) {
        auto n_tight = dp_tight.addDifferentialPrivacy(stats, /*epsilon=*/0.01, 1e-5);
        auto n_loose = dp_loose.addDifferentialPrivacy(stats, /*epsilon=*/10.0, 1e-5);
        sum_tight += std::abs(n_tight["grad"].get<double>() - 1.0);
        sum_loose += std::abs(n_loose["grad"].get<double>() - 1.0);
    }

    EXPECT_GT(sum_tight / N, sum_loose / N)
        << "FEDERATED-04: tight epsilon (0.01) should produce more noise than loose (10.0)";
}

// ============================================================================
// FEDERATED-05: Privacy budget tracking across rounds
// ============================================================================
TEST(FederatedPrivacyTraining, FEDERATED05_PrivacyBudgetTrackedAcrossRounds) {
    FederatedImportCoordinator::DifferentialPrivacyManager dp;

    EXPECT_NEAR(dp.totalEpsilonSpent(), 0.0, 1e-9)
        << "FEDERATED-05: initial epsilon spent should be 0";

    dp.spendBudget(0.1);
    dp.spendBudget(0.1);

    EXPECT_NEAR(dp.totalEpsilonSpent(), 0.2, 1e-9)
        << "FEDERATED-05: after two rounds of 0.1, total epsilon should be 0.2";
}

// ============================================================================
// FEDERATED-06: verifyPrivacyBudget() accepts/rejects correctly
// ============================================================================
TEST(FederatedPrivacyTraining, FEDERATED06_PrivacyBudgetVerification) {
    FederatedImportCoordinator::DifferentialPrivacyManager dp;

    EXPECT_TRUE(dp.verifyPrivacyBudget(0.5, 1e-5))
        << "FEDERATED-06: epsilon=0.5 should be within budget";

    // A very large epsilon should fail the budget check.
    EXPECT_FALSE(dp.verifyPrivacyBudget(1e9, 1e-5))
        << "FEDERATED-06: epsilon=1e9 should exceed the budget";
}

// ============================================================================
// FEDERATED-07: AllReduceAggregator averages gradient tensors correctly
// ============================================================================
TEST(FederatedPrivacyTraining, FEDERATED07_AllReduceAveragesGradients) {
    AllReduceAggregator aggregator;

    // Two shards, each contributing one gradient tensor for the same layer.
    GradientTensor t1;
    t1.layer_name = "layer_0";
    t1.data       = {1.0f, 2.0f, 3.0f};
    t1.shape      = {3};

    GradientTensor t2;
    t2.layer_name = "layer_0";
    t2.data       = {3.0f, 4.0f, 5.0f};
    t2.shape      = {3};

    std::vector<std::vector<GradientTensor>> shard_grads = {{t1}, {t2}};
    auto aggregated = aggregator.aggregate(shard_grads);

    ASSERT_EQ(aggregated.size(), 1u)
        << "FEDERATED-07: one layer should yield one aggregated tensor";
    ASSERT_EQ(aggregated[0].data.size(), 3u)
        << "FEDERATED-07: aggregated tensor should have 3 elements";

    EXPECT_NEAR(aggregated[0].data[0], 2.0f, 1e-5f)
        << "FEDERATED-07: average of {1.0, 3.0} = 2.0";
    EXPECT_NEAR(aggregated[0].data[1], 3.0f, 1e-5f)
        << "FEDERATED-07: average of {2.0, 4.0} = 3.0";
    EXPECT_NEAR(aggregated[0].data[2], 4.0f, 1e-5f)
        << "FEDERATED-07: average of {3.0, 5.0} = 4.0";
}

// ============================================================================
// FEDERATED-08: AllReduceAggregator strategy name is correct
// ============================================================================
TEST(FederatedPrivacyTraining, FEDERATED08_AllReduceStrategyName) {
    AllReduceAggregator aggregator;
    EXPECT_EQ(aggregator.getStrategy(), "ALL_REDUCE")
        << "FEDERATED-08: AllReduceAggregator strategy must be 'ALL_REDUCE'";
}

// ============================================================================
// FEDERATED-09: FedAvg aggregation with 10 participants converges to mean
// ============================================================================
TEST(FederatedPrivacyTraining, FEDERATED09_FedAvgWith10NodesConvergesToMean) {
    FederatedImportCoordinator::FederatedAggregator aggregator;

    // Simulate 10 nodes with loss values centred around 1.5.
    std::vector<FederatedImportCoordinator::FederatedAggregator::ParticipantUpdate> updates;
    double expected_mean = 0.0;
    for (int i = 0; i < 10; ++i) {
        const double loss = 1.0 + 0.1 * i;  // 1.0 … 1.9
        updates.push_back(makeUpdate("node-" + std::to_string(i), loss, 0.5));
        expected_mean += loss;
    }
    expected_mean /= 10.0;  // = 1.45

    json result = aggregator.aggregateUpdates(updates, "FedAvg");

    ASSERT_TRUE(result.contains("loss"));
    EXPECT_NEAR(result["loss"].get<double>(), expected_mean, 1e-5)
        << "FEDERATED-09: FedAvg of 10 nodes should equal arithmetic mean "
           "(expected " << expected_mean << ")";
}

// ============================================================================
// FEDERATED-10: Gradient communication round completes within 2.0 s budget
// ============================================================================
TEST(FederatedPrivacyTraining, FEDERATED10_CommunicationRoundWithinBudget) {
    FederatedImportCoordinator::FederatedAggregator aggregator;
    FederatedImportCoordinator::DifferentialPrivacyManager dp;

    // Build a realistic round: 10 participants, 50 gradient statistics each.
    std::vector<FederatedImportCoordinator::FederatedAggregator::ParticipantUpdate> updates;
    for (int i = 0; i < 10; ++i) {
        json stats;
        for (int g = 0; g < 50; ++g) {
            stats["grad_" + std::to_string(g)] = 0.01 * g;
        }
        FederatedImportCoordinator::FederatedAggregator::ParticipantUpdate u;
        u.participant_id      = "node-" + std::to_string(i);
        u.statistics          = stats;
        u.schema_contribution = json::object();
        u.encrypted_gradient  = json::object();
        updates.push_back(std::move(u));
    }

    const auto t0 = std::chrono::steady_clock::now();

    // Aggregate + apply DP (one round).
    json aggregated = aggregator.aggregateUpdates(updates, "FedAvg");
    json noisy      = dp.addDifferentialPrivacy(aggregated, 0.1, 1e-5);
    dp.spendBudget(0.1);

    const auto elapsed =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - t0);

    (void)noisy;

    EXPECT_LE(elapsed.count(), 2000)
        << "FEDERATED-10: one federated round (aggregate + DP) must complete "
           "within 2000 ms; took " << elapsed.count() << " ms";
}
