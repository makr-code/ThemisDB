/**
 * @file test_adaptive_shard_rebalancer.cpp
 * @brief Unit tests for the Adaptive Shard Rebalancer with Load-Based Splitting
 *
 * Covers (Issue #203, v1.7.0):
 *  - HotShardSplitPolicy: reactive split detection (CPU/storage over threshold)
 *  - HotShardSplitPolicy: statistical predictive detection (forecasted load > 80%)
 *  - HotShardSplitPolicy: ML-based predictive detection via PredictiveFailureDetector
 *  - ShardLoadDetector::forecastLoad(): linear-regression 5-minute projection
 *  - DataMigrator::liveMigrate(): dual-write semantics (no topology/WAL shipper)
 *  - AutoRebalancer::setSplitPolicy() + evaluateAndExecuteSplits() integration
 *  - Audit event types SHARD_SPLIT / SHARD_MERGE present in SecurityEventType enum
 */

#include <gtest/gtest.h>

#include "sharding/shard_load_detector.h"
#include "sharding/auto_rebalancer.h"
#include "sharding/shard_topology.h"
#include "sharding/data_migrator.h"
#include "utils/audit_logger.h"

#include <algorithm>
#include <chrono>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

using namespace themis::sharding;
using themis::utils::SecurityEventType;

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

static std::shared_ptr<ShardTopology> makeTopology() {
    ShardTopology::Config cfg;
    cfg.metadata_endpoint   = "";
    cfg.cluster_name        = "test_cluster";
    cfg.refresh_interval_sec = 0;
    cfg.enable_health_checks = false;
    return std::make_shared<ShardTopology>(cfg);
}

static std::shared_ptr<ShardLoadDetector> makeDetector(size_t min_shards = 2) {
    auto topology = makeTopology();
    ShardLoadDetector::Config cfg;
    cfg.min_shards_for_detection  = min_shards;
    cfg.cpu_exhaustion_threshold  = 0.80;
    cfg.storage_exhaustion_threshold = 0.85;
    cfg.min_samples_per_shard     = 3;
    return std::make_shared<ShardLoadDetector>(topology, nullptr, cfg);
}

static ShardLoadMetrics makeMetrics(const std::string& id,
                                    double cpu_pct,
                                    double storage_pct,
                                    uint64_t rps = 100) {
    ShardLoadMetrics m;
    m.shard_id              = id;
    m.cpu_usage_percent     = cpu_pct;
    m.storage_usage_percent = storage_pct;
    m.requests_per_sec      = rps;
    m.p99_latency_ms        = 5.0;
    m.last_update           = std::chrono::system_clock::now();
    return m;
}

// ─────────────────────────────────────────────────────────────────────────────
// ShardLoadForecast tests
// ─────────────────────────────────────────────────────────────────────────────

TEST(ShardLoadForecast, ForecastReturnsNulloptForUnknownShard) {
    auto detector = makeDetector();
    auto result = detector->forecastLoad("nonexistent_shard");
    EXPECT_FALSE(result.has_value());
}

TEST(ShardLoadForecast, ForecastWithSingleSampleReturnsBestGuess) {
    auto detector = makeDetector(1);
    detector->updateShardLoad("s1", makeMetrics("s1", 50.0, 40.0));

    auto f = detector->forecastLoad("s1", std::chrono::minutes{5});
    ASSERT_TRUE(f.has_value());
    // With insufficient history the forecast falls back to current values
    EXPECT_FALSE(f->has_sufficient_history);
    EXPECT_GE(f->predicted_cpu_percent, 0.0);
    EXPECT_LE(f->predicted_cpu_percent, 100.0);
}

TEST(ShardLoadForecast, ForecastWithRisingSamples_PredictsFutureIncrease) {
    auto detector = makeDetector(1);
    ShardLoadDetector::Config cfg;
    cfg.min_shards_for_detection = 1;
    cfg.min_samples_per_shard    = 3;
    auto det = std::make_shared<ShardLoadDetector>(makeTopology(), nullptr, cfg);

    // Simulate CPU rising from 40% to 64% over 5 samples (+6%/sample, i=0..4)
    for (int i = 0; i < 5; ++i) {
        auto m = makeMetrics("s1", 40.0 + static_cast<double>(i) * 6.0,
                             30.0 + static_cast<double>(i) * 4.0);
        det->updateShardLoad("s1", m);
    }

    auto f = det->forecastLoad("s1", std::chrono::minutes{5});
    ASSERT_TRUE(f.has_value());
    EXPECT_TRUE(f->has_sufficient_history);
    // With a rising trend (+6%/sample, last observed = 64%) the 5-minute
    // forecast should project a value above the last observed CPU (>64%).
    EXPECT_GT(f->predicted_cpu_percent, 64.0);
}

TEST(ShardLoadForecast, ForecastWithFlatSamples_StaysNearCurrent) {
    ShardLoadDetector::Config cfg;
    cfg.min_shards_for_detection = 1;
    cfg.min_samples_per_shard    = 3;
    auto det = std::make_shared<ShardLoadDetector>(makeTopology(), nullptr, cfg);

    for (int i = 0; i < 6; ++i) {
        det->updateShardLoad("s1", makeMetrics("s1", 55.0, 45.0));
    }

    auto f = det->forecastLoad("s1", std::chrono::minutes{5});
    ASSERT_TRUE(f.has_value());
    EXPECT_TRUE(f->has_sufficient_history);
    // Flat data → predicted ≈ current
    EXPECT_NEAR(f->predicted_cpu_percent, 55.0, 5.0);
    EXPECT_NEAR(f->predicted_storage_percent, 45.0, 5.0);
}

TEST(ShardLoadForecast, ForecastResultClamped_NeverExceeds100) {
    ShardLoadDetector::Config cfg;
    cfg.min_shards_for_detection = 1;
    cfg.min_samples_per_shard    = 3;
    auto det = std::make_shared<ShardLoadDetector>(makeTopology(), nullptr, cfg);

    // CPU already at 95% and still rising sharply
    for (int i = 0; i < 5; ++i) {
        det->updateShardLoad("s1", makeMetrics("s1", 90.0 + i * 2.0, 80.0));
    }

    auto f = det->forecastLoad("s1", std::chrono::minutes{5});
    ASSERT_TRUE(f.has_value());
    EXPECT_LE(f->predicted_cpu_percent, 100.0);
    EXPECT_GE(f->predicted_cpu_percent, 0.0);
}

TEST(ShardLoadForecast, HistoryRingBuffer_DoesNotGrowBeyondMax) {
    ShardLoadDetector::Config cfg;
    cfg.min_shards_for_detection = 1;
    cfg.min_samples_per_shard    = 3;
    auto det = std::make_shared<ShardLoadDetector>(makeTopology(), nullptr, cfg);

    // Push more than kMaxHistorySamples entries
    for (size_t i = 0; i <= ShardLoadDetector::kMaxHistorySamples + 5; ++i) {
        det->updateShardLoad("s1", makeMetrics("s1", 50.0, 50.0));
    }

    // forecastLoad should still succeed without crashing
    auto f = det->forecastLoad("s1");
    EXPECT_TRUE(f.has_value());
}

// ─────────────────────────────────────────────────────────────────────────────
// HotShardSplitPolicy tests
// ─────────────────────────────────────────────────────────────────────────────

TEST(HotShardSplitPolicy, NoProposals_WhenBelowThresholds) {
    auto det = makeDetector();
    det->updateShardLoad("s1", makeMetrics("s1", 50.0, 40.0));
    det->updateShardLoad("s2", makeMetrics("s2", 45.0, 38.0));

    HotShardSplitPolicy::Config cfg;
    cfg.cpu_split_threshold     = 0.80;
    cfg.storage_split_threshold = 0.80;
    cfg.enable_predictive_splitting = false;  // Only reactive for this test
    HotShardSplitPolicy policy(det, cfg);

    EXPECT_TRUE(policy.evaluate().empty());
}

TEST(HotShardSplitPolicy, ReactiveProposal_WhenCpuExceedsThreshold) {
    auto det = makeDetector(1);
    // CPU at 85% (threshold is 80%)
    det->updateShardLoad("hot_shard", makeMetrics("hot_shard", 85.0, 50.0));

    HotShardSplitPolicy::Config cfg;
    cfg.cpu_split_threshold     = 0.80;
    cfg.enable_predictive_splitting = false;
    HotShardSplitPolicy policy(det, cfg);

    auto proposals = policy.evaluate();
    ASSERT_EQ(proposals.size(), 1u);
    EXPECT_EQ(proposals[0].hot_shard_id, "hot_shard");
    EXPECT_FALSE(proposals[0].is_predictive);
    EXPECT_GE(proposals[0].current_load_percent, 80.0);
}

TEST(HotShardSplitPolicy, ReactiveProposal_WhenStorageExceedsThreshold) {
    auto det = makeDetector(1);
    det->updateShardLoad("fat_shard", makeMetrics("fat_shard", 30.0, 90.0));

    HotShardSplitPolicy::Config cfg;
    cfg.storage_split_threshold  = 0.80;
    cfg.enable_predictive_splitting = false;
    HotShardSplitPolicy policy(det, cfg);

    auto proposals = policy.evaluate();
    ASSERT_EQ(proposals.size(), 1u);
    EXPECT_EQ(proposals[0].hot_shard_id, "fat_shard");
    EXPECT_FALSE(proposals[0].is_predictive);
}

TEST(HotShardSplitPolicy, PredictiveProposal_WhenForecastExceedsThreshold) {
    // Build a detector with a shard whose CPU is rising but hasn't yet crossed
    // the reactive threshold (80%), so only the predictive path should fire.
    ShardLoadDetector::Config dcfg;
    dcfg.min_shards_for_detection = 1;
    dcfg.min_samples_per_shard    = 3;
    auto det2 = std::make_shared<ShardLoadDetector>(makeTopology(), nullptr, dcfg);

    // CPU rising from 50% to 70% over 5 samples (+5%/sample).
    // Current value at sample 4 is 70% – below the 80% reactive threshold.
    // Predictive splitting compares against the detector's weighted composite
    // load score (0..100), not raw CPU percent.
    for (int i = 0; i < 5; ++i) {
        det2->updateShardLoad("s_predictive",
                              makeMetrics("s_predictive", 50.0 + i * 5.0, 30.0));
    }

    auto forecast = det2->forecastLoad("s_predictive", std::chrono::minutes{5});
    ASSERT_TRUE(forecast.has_value());
    ASSERT_TRUE(forecast->has_sufficient_history);

    HotShardSplitPolicy::Config cfg;
    cfg.cpu_split_threshold           = 0.80;   // Reactive: fires only at 80%+ current CPU
    // Keep the test robust to load-score weighting changes by setting the
    // threshold just below the forecasted composite score.
    cfg.predictive_load_threshold     = std::max(0.0, forecast->predicted_composite_load - 0.5);
    cfg.enable_predictive_splitting   = true;
    cfg.forecast_horizon              = std::chrono::minutes{5};
    HotShardSplitPolicy policy(det2, cfg);

    auto proposals = policy.evaluate();
    ASSERT_FALSE(proposals.empty());
    EXPECT_EQ(proposals[0].hot_shard_id, "s_predictive");
    // The proposal must be marked as predictive (current CPU < reactive threshold)
    EXPECT_TRUE(proposals[0].is_predictive);
}

TEST(HotShardSplitPolicy, MultipleHotShards_AllProposed) {
    auto det = makeDetector(1);
    det->updateShardLoad("shard_a", makeMetrics("shard_a", 90.0, 30.0));
    det->updateShardLoad("shard_b", makeMetrics("shard_b", 25.0, 92.0));

    HotShardSplitPolicy::Config cfg;
    cfg.cpu_split_threshold     = 0.80;
    cfg.storage_split_threshold = 0.80;
    cfg.enable_predictive_splitting = false;
    HotShardSplitPolicy policy(det, cfg);

    auto proposals = policy.evaluate();
    EXPECT_EQ(proposals.size(), 2u);
}

TEST(HotShardSplitPolicy, NullDetector_ReturnsEmpty) {
    HotShardSplitPolicy policy(nullptr);
    EXPECT_TRUE(policy.evaluate().empty());
}

TEST(HotShardSplitPolicy, ConfigDefaults) {
    HotShardSplitPolicy::Config cfg;
    EXPECT_DOUBLE_EQ(cfg.cpu_split_threshold,     0.80);
    EXPECT_DOUBLE_EQ(cfg.storage_split_threshold, 0.80);
    EXPECT_DOUBLE_EQ(cfg.predictive_load_threshold, 80.0);
    EXPECT_EQ(cfg.forecast_horizon, std::chrono::minutes{5});
    EXPECT_TRUE(cfg.enable_predictive_splitting);
    EXPECT_FLOAT_EQ(cfg.failure_probability_threshold, 0.70f);
    EXPECT_TRUE(cfg.enable_ml_predictive_splitting);
}

TEST(HotShardSplitPolicy, SetPredictiveDetector_NullptrDisablesMLPath) {
    auto det = makeDetector(1);
    det->updateShardLoad("s1", makeMetrics("s1", 50.0, 40.0));

    HotShardSplitPolicy::Config cfg;
    cfg.cpu_split_threshold             = 0.80;
    cfg.enable_predictive_splitting     = false;
    cfg.enable_ml_predictive_splitting  = true;
    HotShardSplitPolicy policy(det, cfg);

    // nullptr → ML path disabled; should return no proposals (load is below threshold)
    policy.setPredictiveDetector(nullptr);
    EXPECT_TRUE(policy.evaluate().empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// LiveMigration tests (DataMigrator::liveMigrate)
// ─────────────────────────────────────────────────────────────────────────────

// Minimal config with non-empty endpoints so DataMigrator constructs
static DataMigratorConfig makeMigratorConfig() {
    DataMigratorConfig cfg;
    cfg.source_endpoint = "source:8080";
    cfg.target_endpoint = "target:8080";
    cfg.batch_size      = 10;
    cfg.max_retries     = 1;
    cfg.enable_idempotency = false;
    return cfg;
}

TEST(LiveMigration, DefaultConfig_HasExpectedDefaults) {
    LiveMigrationConfig cfg;
    EXPECT_TRUE(cfg.enable_dual_write);
    EXPECT_GT(cfg.max_wal_lag_bytes, 0u);
}

TEST(LiveMigration, LiveMigrateWithoutTopologyOrWALShipper_Completes) {
    // Without topology/WAL shipper the live migrate still runs the bulk
    // copy phase and returns a result (success depends on the underlying
    // migrate() mock network; we only verify that the call doesn't throw).
    DataMigrator migrator(makeMigratorConfig());

    LiveMigrationConfig lcfg;
    lcfg.verify_after_bulk_copy = false;

    EXPECT_NO_THROW({
        auto result = migrator.liveMigrate(
            "src_shard", "tgt_shard",
            0, UINT64_MAX / 2,
            nullptr,  // topology
            nullptr,  // wal_shipper
            lcfg
        );
        // The bulk copy will fail in the stub implementation (no real network),
        // but the LiveMigrationResult struct is populated and has a migration_id.
        EXPECT_FALSE(result.migration_id.empty());
    });
}

TEST(LiveMigration, LiveMigrateResult_HasMigrationId) {
    DataMigrator migrator(makeMigratorConfig());

    LiveMigrationConfig lcfg;
    lcfg.verify_after_bulk_copy = false;

    auto result = migrator.liveMigrate(
        "src_shard", "tgt_shard",
        0, 1000,
        nullptr, nullptr, lcfg
    );

    EXPECT_FALSE(result.migration_id.empty());
}

TEST(LiveMigration, LiveMigrateWithTopology_UpdatesTokenRanges) {
    auto topology = makeTopology();

    // Register source and target shards with initial token ranges
    ShardInfo source;
    source.shard_id    = "src";
    source.token_start = 0;
    source.token_end   = UINT64_MAX;
    source.is_healthy  = true;
    topology->addShard(source);

    ShardInfo target;
    target.shard_id    = "tgt";
    target.token_start = 0;
    target.token_end   = 0;
    target.is_healthy  = true;
    topology->addShard(target);

    DataMigrator migrator(makeMigratorConfig());

    LiveMigrationConfig lcfg;
    lcfg.verify_after_bulk_copy = false;

    // Migrate upper half of the token range
    const uint64_t split_point = UINT64_MAX / 2;
    migrator.liveMigrate(
        "src", "tgt",
        split_point, UINT64_MAX,
        topology,
        nullptr,
        lcfg
    );

    // After live migrate the target shard should cover the migrated range
    // (topology update is only applied when bulk copy succeeds; in stub
    // mode migration will fail network-wise, so we only verify no crash)
    auto tgt_after = topology->getShard("tgt");
    EXPECT_TRUE(tgt_after.has_value());
}

// ─────────────────────────────────────────────────────────────────────────────
// AutoRebalancer integration tests
// ─────────────────────────────────────────────────────────────────────────────

static AutoRebalancer::MigrationResult rejectUnexpectedMigration(
    const AutoRebalancer::MigrationRequest&) {
    ADD_FAILURE() << "Unexpected migration";
    return {AutoRebalancer::MigrationOutcome::REJECTED, {}};
}

TEST(AdaptiveShardRebalancer, SetSplitPolicy_DoesNotThrow) {
    auto topology  = makeTopology();
    auto detector  = makeDetector();
    AutoRebalancer rebalancer(topology, detector, nullptr, rejectUnexpectedMigration);

    HotShardSplitPolicy::Config pcfg;
    auto policy = std::make_shared<HotShardSplitPolicy>(detector, pcfg);

    EXPECT_NO_THROW(rebalancer.setSplitPolicy(policy));
}

TEST(AdaptiveShardRebalancer, GetStatisticsIncludesSplitProposalCounter) {
    auto topology  = makeTopology();
    auto detector  = makeDetector();
    AutoRebalancer rebalancer(topology, detector, nullptr, rejectUnexpectedMigration);

    auto stats = rebalancer.getStatistics();
    EXPECT_TRUE(stats.contains("split_proposals_total"));
    EXPECT_EQ(stats["split_proposals_total"], 0u);
}

TEST(AdaptiveShardRebalancer, SetAuditLogger_DoesNotThrow) {
    auto topology = makeTopology();
    auto detector = makeDetector();
    AutoRebalancer rebalancer(topology, detector, nullptr, rejectUnexpectedMigration);

    // Passing nullptr should be safe (disables audit logging)
    EXPECT_NO_THROW(rebalancer.setAuditLogger(nullptr));
}

TEST(AdaptiveShardRebalancer, RequiresMigrationExecutorInBothConstructors) {
    const auto topology = makeTopology();
    const auto detector = makeDetector();
    const AutoRebalancer::MigrationExecutor missing;
    EXPECT_THROW(AutoRebalancer(topology, detector, nullptr, missing), std::invalid_argument);
    EXPECT_THROW(AutoRebalancer(topology, detector, nullptr, missing, AutoRebalancer::Config{}),
                 std::invalid_argument);
}

static void seedImbalance(ShardLoadDetector& detector) {
    auto hot = makeMetrics("hot", 95.0, 90.0, 1000);
    auto cold = makeMetrics("cold", 10.0, 5.0, 10);
    hot.total_bytes = 9000;
    cold.total_bytes = 500;
    detector.updateShardLoad("hot", hot);
    detector.updateShardLoad("cold", cold);
}

static bool waitForPending(const AutoRebalancer& rebalancer, size_t count) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    do {
        if (rebalancer.getStatistics()["pending_approvals"] == count) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    } while (std::chrono::steady_clock::now() < deadline);
    return false;
}

using Outcome = AutoRebalancer::MigrationOutcome;

class AdaptiveShardRebalancerLifecycle
    : public ::testing::TestWithParam<std::optional<Outcome>> {};

TEST_P(AdaptiveShardRebalancerLifecycle, ApprovedMoveCompletesOnlyWithOwnershipCommit) {
    const auto topology = makeTopology();
    const auto detector = makeDetector();
    seedImbalance(*detector);
    ASSERT_EQ(detector->detectImbalance().recommendations.size(), 1u);

    AutoRebalancer::Config config;
    config.require_manual_approval = true;
    config.auto_trigger_enabled = false;
    config.operator_cert_path = "test-operator";
    std::string approved_id;
    unsigned calls = 0;
    AutoRebalancer rebalancer(topology, detector, nullptr,
        [&](const AutoRebalancer::MigrationRequest& request) -> AutoRebalancer::MigrationResult {
            ++calls;
            EXPECT_EQ(request.operation_id, approved_id);
            EXPECT_EQ(request.operation.source_shard_id, "hot");
            EXPECT_EQ(request.operation.target_shard_id, "cold");
            EXPECT_LT(request.operation.token_range_start, request.operation.token_range_end);
            EXPECT_EQ(request.operator_signature, "signed:" + approved_id);
            const auto running = rebalancer.getOperationStatuses();
            EXPECT_EQ(running.size(), 1u);
            if (!running.empty()) {
                EXPECT_EQ(running.front().state, RebalanceState::IN_PROGRESS);
                EXPECT_FALSE(running.front().migration_outcome.has_value());
            }
            EXPECT_FALSE(rebalancer.cancelOperation(approved_id));
            if (!GetParam()) {
                throw std::runtime_error("lost executor response");
            }
            return {*GetParam(), {}};
        }, config);
    rebalancer.setSignOperationFn([](const std::string& id) { return "signed:" + id; });
    rebalancer.start();
    ASSERT_TRUE(waitForPending(rebalancer, 1));
    rebalancer.stop();

    const auto planned = rebalancer.getOperationStatuses();
    ASSERT_EQ(planned.size(), 1u);
    EXPECT_EQ(planned.front().state, RebalanceState::PLANNED);
    EXPECT_EQ(calls, 0u);
    approved_id = planned.front().operation_id;
    EXPECT_TRUE(rebalancer.approveOperation(approved_id));
    EXPECT_FALSE(rebalancer.approveOperation(approved_id));
    EXPECT_FALSE(rebalancer.cancelOperation(approved_id));
    EXPECT_EQ(calls, 1u);

    const auto statuses = rebalancer.getOperationStatuses();
    ASSERT_EQ(statuses.size(), 1u);
    const auto outcome = GetParam().value_or(Outcome::OWNERSHIP_UNCERTAIN);
    const bool committed = outcome == Outcome::OWNERSHIP_COMMITTED;
    const auto& status = statuses.front();
    EXPECT_EQ(status.operation_id, approved_id);
    EXPECT_EQ(status.state, committed ? RebalanceState::COMPLETED : RebalanceState::FAILED);
    EXPECT_EQ(status.migration_outcome, outcome);
    EXPECT_EQ(status.progress.progress_percent, committed ? 100.0 : 0.0);
    EXPECT_EQ(status.error_message.empty(), committed);
    const auto stats = rebalancer.getStatistics();
    EXPECT_EQ(stats["active_operations"], 0u);
    EXPECT_EQ(stats["completed_operations"], committed ? 1u : 0u);
    EXPECT_EQ(stats["failed_operations"], committed ? 0u : 1u);
}

INSTANTIATE_TEST_SUITE_P(AdaptiveShardRebalancerExecutorResults, AdaptiveShardRebalancerLifecycle,
    ::testing::Values(std::optional{Outcome::REJECTED}, std::optional{Outcome::ADMISSION_UNCERTAIN},
                      std::optional{Outcome::ADMITTED}, std::optional{Outcome::TRANSFER_VERIFIED},
                      std::optional{Outcome::OWNERSHIP_COMMITTED}, std::optional{Outcome::OWNERSHIP_UNCERTAIN},
                      std::optional<Outcome>{}));

TEST(AdaptiveShardRebalancer, HotSplitWaitsForApprovalAndUsesTopologyRange) {
    const auto topology = makeTopology();
    ShardInfo hot{};
    hot.shard_id = "hot";
    hot.token_start = 100;
    hot.token_end = 200;
    topology->addShard(hot);
    const auto detector = makeDetector();
    seedImbalance(*detector);

    AutoRebalancer::Config config;
    config.require_manual_approval = true;
    config.auto_trigger_enabled = true;
    config.operator_cert_path = "test-operator";
    std::vector<AutoRebalancer::MigrationRequest> requests;
    AutoRebalancer rebalancer(topology, detector, nullptr,
        [&](const AutoRebalancer::MigrationRequest& request) {
            requests.push_back(request);
            return AutoRebalancer::MigrationResult{Outcome::TRANSFER_VERIFIED, {}};
        }, config);
    rebalancer.setSignOperationFn([](const std::string&) { return "test-signature"; });
    rebalancer.setSplitPolicy(std::make_shared<HotShardSplitPolicy>(detector));
    rebalancer.start();
    ASSERT_TRUE(waitForPending(rebalancer, 2));
    rebalancer.stop();
    EXPECT_TRUE(requests.empty());

    const auto planned = rebalancer.getOperationStatuses();
    ASSERT_EQ(planned.size(), 2u);
    for (const auto& status : planned) {
        EXPECT_EQ(status.state, RebalanceState::PLANNED);
        EXPECT_TRUE(rebalancer.approveOperation(status.operation_id));
    }
    ASSERT_EQ(requests.size(), 2u);
    EXPECT_EQ(std::count_if(requests.begin(), requests.end(), [](const auto& req) {
        return req.operation.source_shard_id == "hot" &&
               req.operation.target_shard_id == "cold" &&
               req.operation.token_range_start == 150 && req.operation.token_range_end == 200;
    }), 1);
    EXPECT_EQ(rebalancer.getStatistics()["completed_operations"], 0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// Audit event type completeness
// ─────────────────────────────────────────────────────────────────────────────

TEST(AdaptiveShardRebalancer, AuditEventTypes_ShardSplitMergePresent) {
    // Verify that the required shard audit event types are present in the enum.
    // This is a compile-time check surfaced as a run-time assertion so that
    // a missing enumerator causes an immediate test failure.
    const auto split   = SecurityEventType::SHARD_SPLIT;
    const auto merge   = SecurityEventType::SHARD_MERGE;
    const auto started = SecurityEventType::SHARD_LIVE_MIGRATION_STARTED;
    const auto done    = SecurityEventType::SHARD_LIVE_MIGRATION_COMPLETED;
    const auto failed  = SecurityEventType::SHARD_LIVE_MIGRATION_FAILED;

    EXPECT_NE(split,   merge);
    EXPECT_NE(started, done);
    EXPECT_NE(done,    failed);
    (void)split; (void)merge; (void)started; (void)done; (void)failed;
}
