/**
 * @file test_ab_test_manager.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 88/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/// @file test_ab_test_manager.cpp
/// @brief Unit tests for ABTestManager (base module, A/B testing with module swapping)
///
/// Tests run without real plugin binaries — they verify the API surface,
/// traffic-routing determinism, metrics accumulation, statistical evaluation,
/// state-machine transitions, and concurrency safety.

#include <gtest/gtest.h>
#include "themis/base/ab_test_manager.h"
#include "themis/base/hot_reload_manager.h"
#include "themis/base/module_loader.h"
#include "themis/base/interfaces/storage_interface.h"
#include "observability/metrics_collector.h"

#include <nlohmann/json.hpp>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

using namespace themis::modules;
using namespace themis;

// =============================================================================
// Helpers
// =============================================================================

static ABModuleTestConfig makeConfig(const std::string& test_id,
                                     const std::string& module  = "test_module",
                                     double             split   = 0.5) {
    ABModuleTestConfig cfg;
    cfg.test_id        = test_id;
    cfg.module_name    = module;
    cfg.control_path   = "/nonexistent/control.so";
    cfg.treatment_path = "/nonexistent/treatment.so";
    cfg.traffic_split  = split;
    cfg.min_samples    = 30;
    return cfg;
}

// =============================================================================
// ABTestStatus – enum sanity
// =============================================================================

TEST(ABTestStatus, ValuesAreDistinct) {
    EXPECT_NE(static_cast<int>(ABTestStatus::ACTIVE),
              static_cast<int>(ABTestStatus::PROMOTED));
    EXPECT_NE(static_cast<int>(ABTestStatus::ACTIVE),
              static_cast<int>(ABTestStatus::ROLLED_BACK));
    EXPECT_NE(static_cast<int>(ABTestStatus::ACTIVE),
              static_cast<int>(ABTestStatus::CANCELLED));
    EXPECT_NE(static_cast<int>(ABTestStatus::PROMOTED),
              static_cast<int>(ABTestStatus::ROLLED_BACK));
}

// =============================================================================
// ABTestManager construction
// =============================================================================

TEST(ABTestManager, DefaultConstruction) {
    EXPECT_NO_THROW({ ABTestManager m; });
}

TEST(ABTestManager, ConstructWithReloadManager) {
    HotReloadManager hr;
    EXPECT_NO_THROW({ ABTestManager m(hr); });
}

// =============================================================================
// treatmentKey helper
// =============================================================================

TEST(ABTestManager, TreatmentKeyFormat) {
    auto key = ABTestManager::treatmentKey("themis_storage");
    EXPECT_EQ(key, "themis_storage__ab_treatment__");
}

TEST(ABTestManager, TreatmentKeyIsUniquePerModule) {
    EXPECT_NE(ABTestManager::treatmentKey("mod_a"),
              ABTestManager::treatmentKey("mod_b"));
}

// =============================================================================
// Test-suite fixture
// =============================================================================

class ABTestManagerTest : public ::testing::Test {
protected:
    ABTestManager  mgr;
    ModuleLoader   loader;

    /// Start a test that will fail to load the treatment binary (expected in tests).
    bool startTest(const std::string& test_id,
                   const std::string& module = "test_module",
                   double split = 0.5) {
        return mgr.startTest(makeConfig(test_id, module, split), loader);
    }
};

// =============================================================================
// startTest
// =============================================================================

TEST_F(ABTestManagerTest, StartTestSucceeds) {
    EXPECT_TRUE(startTest("t1"));
}

TEST_F(ABTestManagerTest, StartTestDuplicateFails) {
    EXPECT_TRUE(startTest("t1"));
    EXPECT_FALSE(startTest("t1"));  // duplicate
}

TEST_F(ABTestManagerTest, StartTestSetsStatusToActive) {
    startTest("t1");
    EXPECT_EQ(mgr.getTestStatus("t1"), ABTestStatus::ACTIVE);
}

TEST_F(ABTestManagerTest, StartTestAppearsInActiveList) {
    startTest("t1");
    startTest("t2");
    auto active = mgr.getActiveTests();
    EXPECT_EQ(active.size(), 2u);
    bool found_t1 = false, found_t2 = false;
    for (const auto& id : active) {
        if (id == "t1") {
          found_t1 = true;
        }
        if (id == "t2") {
          found_t2 = true;
        }
    }
    EXPECT_TRUE(found_t1);
    EXPECT_TRUE(found_t2);
}

// =============================================================================
// getTestStatus for unknown test
// =============================================================================

TEST_F(ABTestManagerTest, GetStatusUnknownTestReturnsCancelled) {
    // By convention, unknown test_id returns CANCELLED.
    EXPECT_EQ(mgr.getTestStatus("does_not_exist"), ABTestStatus::CANCELLED);
}

// =============================================================================
// Traffic routing
// =============================================================================

TEST_F(ABTestManagerTest, RoutingReturnsFalseForUnknownTest) {
    EXPECT_FALSE(mgr.shouldUseTreatment("unknown", "user_1"));
}

TEST_F(ABTestManagerTest, RoutingIsDeterministic) {
    startTest("t1");
    // Same key must always produce the same routing decision.
    bool first  = mgr.shouldUseTreatment("t1", "user_abc");
    bool second = mgr.shouldUseTreatment("t1", "user_abc");
    EXPECT_EQ(first, second);
}

TEST_F(ABTestManagerTest, RoutingRespectsSplitApproximately) {
    // Use split=0.5 so roughly half the requests go to each variant.
    startTest("t1", "mod", 0.5);

    int treatment_count = 0;
    const int total     = 1000;
    for (int i = 0; i < total; ++i) {
        if (mgr.shouldUseTreatment("t1", "user_" + std::to_string(i))) {
            ++treatment_count;
        }
    }
    double actual = static_cast<double>(treatment_count) / total;
    EXPECT_NEAR(actual, 0.5, 0.1);
}

TEST_F(ABTestManagerTest, RoutingWithZeroSplitAlwaysReturnsControl) {
    startTest("t1", "mod", 0.0);
    for (int i = 0; i < 100; ++i) {
        EXPECT_FALSE(mgr.shouldUseTreatment("t1", "user_" + std::to_string(i)));
    }
}

TEST_F(ABTestManagerTest, RoutingWithFullSplitAlwaysReturnsTreatment) {
    startTest("t1", "mod", 1.0);
    for (int i = 0; i < 100; ++i) {
        EXPECT_TRUE(mgr.shouldUseTreatment("t1", "user_" + std::to_string(i)));
    }
}

// =============================================================================
// Metrics recording
// =============================================================================

TEST_F(ABTestManagerTest, MetricsDefaultZero) {
    startTest("t1");
    auto ctrl = mgr.getControlMetrics("t1");
    auto trt  = mgr.getTreatmentMetrics("t1");
    EXPECT_EQ(ctrl.sample_count,  0u);
    EXPECT_EQ(trt.sample_count,   0u);
    EXPECT_EQ(ctrl.success_count, 0u);
    EXPECT_EQ(trt.success_count,  0u);
}

TEST_F(ABTestManagerTest, RecordOutcomeUpdatesControlMetrics) {
    startTest("t1");
    mgr.recordOutcome("t1", false, true,  10.0);
    mgr.recordOutcome("t1", false, false, 20.0);

    auto ctrl = mgr.getControlMetrics("t1");
    EXPECT_EQ(ctrl.sample_count,  2u);
    EXPECT_EQ(ctrl.success_count, 1u);
    EXPECT_NEAR(ctrl.success_rate, 0.5, 0.001);
    EXPECT_NEAR(ctrl.mean_latency_ms, 15.0, 0.001);
}

TEST_F(ABTestManagerTest, RecordOutcomeUpdatesTreatmentMetrics) {
    startTest("t1");
    for (int i = 0; i < 8; ++i)
        mgr.recordOutcome("t1", true, true, 5.0);
    for (int i = 0; i < 2; ++i)
        mgr.recordOutcome("t1", true, false, 5.0);

    auto trt = mgr.getTreatmentMetrics("t1");
    EXPECT_EQ(trt.sample_count,  10u);
    EXPECT_EQ(trt.success_count,  8u);
    EXPECT_NEAR(trt.success_rate, 0.8, 0.001);
}

TEST_F(ABTestManagerTest, RecordOutcomeIgnoredForUnknownTest) {
    EXPECT_NO_THROW(mgr.recordOutcome("no_such_test", false, true));
}

TEST_F(ABTestManagerTest, RecordOutcomeIgnoredAfterCancel) {
    startTest("t1");
    mgr.cancelTest("t1");
    mgr.recordOutcome("t1", false, true);
    // Metrics must remain zero since the test is no longer ACTIVE.
    auto ctrl = mgr.getControlMetrics("t1");
    EXPECT_EQ(ctrl.sample_count, 0u);
}

// =============================================================================
// Statistical evaluation
// =============================================================================

TEST_F(ABTestManagerTest, EvaluateReturnsEmptyForUnknownTest) {
    auto r = mgr.evaluateTest("unknown");
    EXPECT_EQ(r.sample_size_control,   0u);
    EXPECT_EQ(r.sample_size_treatment, 0u);
    EXPECT_FALSE(r.is_significant);
}

TEST_F(ABTestManagerTest, EvaluateNotSignificantWhenBelowMinSamples) {
    startTest("t1");
    // Add fewer than min_samples observations.
    for (int i = 0; i < 10; ++i) {
        mgr.recordOutcome("t1", false, true);
        mgr.recordOutcome("t1", true,  true);
    }
    auto r = mgr.evaluateTest("t1");
    EXPECT_FALSE(r.is_significant);
}

TEST_F(ABTestManagerTest, EvaluateNotSignificantForSmallDifference) {
    startTest("t1");
    // Control 70%, treatment 72% — small difference.
    for (int i = 0; i < 100; ++i) {
        mgr.recordOutcome("t1", false, (i < 70));
        mgr.recordOutcome("t1", true,  (i < 72));
    }
    auto r = mgr.evaluateTest("t1");
    EXPECT_FALSE(r.is_significant);
}

TEST_F(ABTestManagerTest, EvaluateSignificantForLargeDifference) {
    auto cfg = makeConfig("t1");
    cfg.thompson_stop_threshold = 0.0; // keep test active so all samples are recorded
    ASSERT_TRUE(mgr.startTest(cfg, loader));
    // Control 20%, treatment 80% — large difference.
    for (int i = 0; i < 100; ++i) {
        mgr.recordOutcome("t1", false, (i < 20));
        mgr.recordOutcome("t1", true,  (i < 80));
    }
    auto r = mgr.evaluateTest("t1");
    EXPECT_TRUE(r.is_significant);
    EXPECT_LT(r.p_value, 0.05);
    EXPECT_NEAR(r.improvement, 0.6, 0.01);
}

TEST_F(ABTestManagerTest, EvaluateSampleSizesMatchRecorded) {
    startTest("t1");
    for (int i = 0; i < 40; ++i) {
      mgr.recordOutcome("t1", false, true);
    }
    for (int i = 0; i < 60; ++i) {
      mgr.recordOutcome("t1", true,  true);
    }

    auto r = mgr.evaluateTest("t1");
    EXPECT_EQ(r.sample_size_control,   40u);
    EXPECT_EQ(r.sample_size_treatment, 60u);
}

// =============================================================================
// promoteTest
// =============================================================================

TEST_F(ABTestManagerTest, PromoteUnknownTestFails) {
    EXPECT_FALSE(mgr.promoteTest("unknown"));
}

TEST_F(ABTestManagerTest, PromoteInactiveTestFails) {
    startTest("t1");
    mgr.cancelTest("t1");
    EXPECT_FALSE(mgr.promoteTest("t1"));
}

TEST_F(ABTestManagerTest, PromoteActiveTestSetsStatusPromoted) {
    startTest("t1");
    // Promote succeeds (no hot-reload manager; treatment binary absent is OK).
    mgr.promoteTest("t1");
    EXPECT_EQ(mgr.getTestStatus("t1"), ABTestStatus::PROMOTED);
}

TEST_F(ABTestManagerTest, PromotedTestNoLongerInActiveList) {
    startTest("t1");
    startTest("t2");
    mgr.promoteTest("t1");

    auto active = mgr.getActiveTests();
    EXPECT_EQ(active.size(), 1u);
    EXPECT_EQ(active[0], "t2");
}

// =============================================================================
// rollbackTest
// =============================================================================

TEST_F(ABTestManagerTest, RollbackUnknownTestFails) {
    EXPECT_FALSE(mgr.rollbackTest("unknown"));
}

TEST_F(ABTestManagerTest, RollbackInactiveTestFails) {
    startTest("t1");
    mgr.cancelTest("t1");
    EXPECT_FALSE(mgr.rollbackTest("t1"));
}

TEST_F(ABTestManagerTest, RollbackActiveTestSetsStatusRolledBack) {
    startTest("t1");
    EXPECT_TRUE(mgr.rollbackTest("t1"));
    EXPECT_EQ(mgr.getTestStatus("t1"), ABTestStatus::ROLLED_BACK);
}

TEST_F(ABTestManagerTest, RolledBackTestNoLongerInActiveList) {
    startTest("t1");
    startTest("t2");
    mgr.rollbackTest("t2");

    auto active = mgr.getActiveTests();
    EXPECT_EQ(active.size(), 1u);
    EXPECT_EQ(active[0], "t1");
}

// =============================================================================
// cancelTest
// =============================================================================

TEST_F(ABTestManagerTest, CancelUnknownTestIsNoOp) {
    EXPECT_NO_THROW(mgr.cancelTest("unknown"));
}

TEST_F(ABTestManagerTest, CancelAlreadyCancelledIsNoOp) {
    startTest("t1");
    mgr.cancelTest("t1");
    EXPECT_NO_THROW(mgr.cancelTest("t1"));
    EXPECT_EQ(mgr.getTestStatus("t1"), ABTestStatus::CANCELLED);
}

TEST_F(ABTestManagerTest, CancelActiveTestSetsStatusCancelled) {
    startTest("t1");
    mgr.cancelTest("t1");
    EXPECT_EQ(mgr.getTestStatus("t1"), ABTestStatus::CANCELLED);
}

TEST_F(ABTestManagerTest, CancelledTestNotInActiveList) {
    startTest("t1");
    mgr.cancelTest("t1");
    EXPECT_TRUE(mgr.getActiveTests().empty());
}

// =============================================================================
// Multiple concurrent tests
// =============================================================================

TEST_F(ABTestManagerTest, MultipleTestsIndependent) {
    startTest("t1", "mod_a", 0.1);
    startTest("t2", "mod_b", 0.9);

    mgr.recordOutcome("t1", false, true);
    mgr.recordOutcome("t2", true,  false);

    // t1 control metrics must not be polluted by t2 outcomes.
    auto m1 = mgr.getControlMetrics("t1");
    auto m2 = mgr.getControlMetrics("t2");
    EXPECT_EQ(m1.sample_count, 1u);
    EXPECT_EQ(m2.sample_count, 0u);
}

TEST_F(ABTestManagerTest, CompletingOneTestDoesNotAffectOther) {
    startTest("t1");
    startTest("t2");
    mgr.rollbackTest("t1");

    EXPECT_EQ(mgr.getTestStatus("t1"), ABTestStatus::ROLLED_BACK);
    EXPECT_EQ(mgr.getTestStatus("t2"), ABTestStatus::ACTIVE);
}

// =============================================================================
// HotReloadManager integration — no real binary
// =============================================================================

TEST(ABTestManagerWithReloadMgr, PromoteCallsReloadManager) {
    HotReloadManager hr;
    ModuleLoader loader;
    ABTestManager mgr(hr);

    // Register the module so the reload manager knows about it.
    hr.registerModule("test_mod", loader);

    ABModuleTestConfig cfg = makeConfig("t1", "test_mod", 0.5);
    mgr.startTest(cfg, loader);

    // promoteTest will attempt reloadModule; with a nonexistent path it fails,
    // but the status should still be set to PROMOTED.
    mgr.promoteTest("t1");
    EXPECT_EQ(mgr.getTestStatus("t1"), ABTestStatus::PROMOTED);
}

// =============================================================================
// Thread-safety
// =============================================================================

TEST_F(ABTestManagerTest, ConcurrentStartAndCancel) {
    const int N = 20;
    std::vector<std::thread> threads;
    threads.reserve(N * 2);

    for (int i = 0; i < N; ++i) {
        threads.emplace_back([this, i]() {
            startTest("mod_" + std::to_string(i));
        });
    }
    for (auto& t : threads) {
      t.join();
    }
    threads.clear();

    for (int i = 0; i < N; ++i) {
        threads.emplace_back([this, i]() {
            mgr.cancelTest("mod_" + std::to_string(i));
        });
    }
    for (auto& t : threads) {
      t.join();
    }

    EXPECT_TRUE(mgr.getActiveTests().empty());
}

TEST_F(ABTestManagerTest, ConcurrentRecordOutcome) {
    startTest("t1");
    const int N = 50;
    std::vector<std::thread> threads;
    threads.reserve(N);
    for (int i = 0; i < N; ++i) {
        threads.emplace_back([this, i]() {
            mgr.recordOutcome("t1", (i % 2 == 0), true, 1.0);
        });
    }
    for (auto& t : threads) {
      t.join();
    }

    auto ctrl = mgr.getControlMetrics("t1");
    auto trt  = mgr.getTreatmentMetrics("t1");
    EXPECT_EQ(ctrl.sample_count + trt.sample_count, static_cast<size_t>(N));
}

TEST_F(ABTestManagerTest, ConcurrentRouting) {
    startTest("t1", "mod", 0.3);
    const int N = 100;
    std::vector<std::thread> threads;
    threads.reserve(N);
    // Concurrent calls must not crash or deadlock.
    for (int i = 0; i < N; ++i) {
        threads.emplace_back([this, i]() {
            mgr.shouldUseTreatment("t1", "user_" + std::to_string(i));
        });
    }
    for (auto& t : threads) {
      t.join();
    }
    EXPECT_TRUE(true); // No crash / deadlock.
}

// =============================================================================
// isTreatmentLoaded
// =============================================================================

TEST_F(ABTestManagerTest, TreatmentNotLoadedForNonExistentBinary) {
    startTest("t1");
    // The treatment binary path is nonexistent, so it should not be loaded.
    EXPECT_FALSE(mgr.isTreatmentLoaded("t1"));
}

TEST_F(ABTestManagerTest, TreatmentLoadedQueryUnknownTest) {
    EXPECT_FALSE(mgr.isTreatmentLoaded("unknown"));
}

// =============================================================================
// Latency metrics
// =============================================================================

TEST_F(ABTestManagerTest, EvaluateLatencyMetricsPopulated) {
    startTest("t1");
    // Use 40 samples — comfortably above min_samples (30) so the test is
    // evaluable, and simple enough to reason about the expected latency mean.
    constexpr int kSamples = 40;
    for (int i = 0; i < kSamples; ++i)
        mgr.recordOutcome("t1", false, true, 10.0);  // control: 10 ms
    for (int i = 0; i < kSamples; ++i)
        mgr.recordOutcome("t1", true, true, 20.0);   // treatment: 20 ms

    auto r = mgr.evaluateTest("t1");
    EXPECT_NEAR(r.control_mean_latency_ms,   10.0, 0.01);
    EXPECT_NEAR(r.treatment_mean_latency_ms, 20.0, 0.01);
}

TEST_F(ABTestManagerTest, ControlStdDevComputedForVariedLatency) {
    startTest("t1");
    mgr.recordOutcome("t1", false, true, 10.0);
    mgr.recordOutcome("t1", false, true, 20.0);  // mean = 15, std_dev > 0

    auto m = mgr.getControlMetrics("t1");
    EXPECT_NEAR(m.mean_latency_ms, 15.0, 0.01);
    EXPECT_GT(m.std_dev_latency, 0.0);
}

TEST_F(ABTestManagerTest, StdDevZeroForConstantLatency) {
    startTest("t1");
    for (int i = 0; i < 5; ++i)
        mgr.recordOutcome("t1", false, true, 10.0);

    auto m = mgr.getControlMetrics("t1");
    EXPECT_NEAR(m.std_dev_latency, 0.0, 1e-9);
}

// =============================================================================
// promoteTest TOCTOU guard: status not overwritten if already terminal
// =============================================================================

TEST_F(ABTestManagerTest, PromoteAfterCancelDoesNotOverrideCancelledStatus) {
    startTest("t1");
    // Cancel makes the test terminal.
    mgr.cancelTest("t1");
    EXPECT_EQ(mgr.getTestStatus("t1"), ABTestStatus::CANCELLED);
    // Attempt to promote a non-ACTIVE test: must fail AND leave status CANCELLED.
    bool ok = mgr.promoteTest("t1");
    EXPECT_FALSE(ok);
    EXPECT_EQ(mgr.getTestStatus("t1"), ABTestStatus::CANCELLED);
}

TEST_F(ABTestManagerTest, RollbackAfterCancelDoesNotOverrideCancelledStatus) {
    startTest("t1");
    mgr.cancelTest("t1");
    EXPECT_EQ(mgr.getTestStatus("t1"), ABTestStatus::CANCELLED);
    bool ok = mgr.rollbackTest("t1");
    EXPECT_FALSE(ok);
    EXPECT_EQ(mgr.getTestStatus("t1"), ABTestStatus::CANCELLED);
}

// =============================================================================
// exportMetricsSnapshot
// =============================================================================

TEST_F(ABTestManagerTest, ExportMetricsSnapshotEmptyWithNoTests) {
    auto rows = mgr.exportMetricsSnapshot();
    EXPECT_TRUE(rows.empty());
}

TEST_F(ABTestManagerTest, ExportMetricsSnapshotHasTwoRowsPerTest) {
    startTest("t1");
    auto rows = mgr.exportMetricsSnapshot();
    EXPECT_EQ(rows.size(), 2u);
}

TEST_F(ABTestManagerTest, ExportMetricsSnapshotContainsControlAndTreatment) {
    startTest("t1");
    auto rows = mgr.exportMetricsSnapshot();
    bool has_ctrl = false, has_trt = false;
    for (const auto& r : rows) {
        EXPECT_EQ(r.test_id, "t1");
        if (r.variant == "control") {
          has_ctrl = true;
        }
        if (r.variant == "treatment") {
          has_trt  = true;
        }
    }
    EXPECT_TRUE(has_ctrl);
    EXPECT_TRUE(has_trt);
}

TEST_F(ABTestManagerTest, ExportMetricsSnapshotReflectsAccumulatedMetrics) {
    startTest("t1");
    for (int i = 0; i < 8; ++i) {
      mgr.recordOutcome("t1", false, true,  10.0);
    }
    for (int i = 0; i < 2; ++i) {
      mgr.recordOutcome("t1", false, false, 10.0);
    }

    auto rows = mgr.exportMetricsSnapshot();
    for (const auto& r : rows) {
        if (r.variant == "control") {
            EXPECT_EQ(r.requests,    10u);
            EXPECT_EQ(r.conversions,  8u);
            EXPECT_NEAR(r.success_rate,    0.8, 0.001);
            EXPECT_NEAR(r.mean_latency_ms, 10.0, 0.001);
        }
    }
}

TEST_F(ABTestManagerTest, ExportMetricsSnapshotP99IsAtLeastMean) {
    startTest("t1");
    for (int i = 0; i < 10; ++i) {
      mgr.recordOutcome("t1", false, true, 5.0);
    }
    for (int i = 0; i < 10; ++i) {
      mgr.recordOutcome("t1", false, true, 15.0);
    }

    auto rows = mgr.exportMetricsSnapshot();
    for (const auto& r : rows) {
        if (r.variant == "control") {
            EXPECT_GE(r.latency_p99_ms, r.mean_latency_ms);
        }
    }
}

TEST_F(ABTestManagerTest, ExportMetricsSnapshotStatusMatchesTestStatus) {
    startTest("t1");
    mgr.rollbackTest("t1");

    auto rows = mgr.exportMetricsSnapshot();
    for (const auto& r : rows) {
        EXPECT_EQ(r.status, ABTestStatus::ROLLED_BACK);
    }
}

TEST_F(ABTestManagerTest, ExportMetricsSnapshotCoversFourRowsForTwoTests) {
    startTest("t1");
    startTest("t2");
    auto rows = mgr.exportMetricsSnapshot();
    EXPECT_EQ(rows.size(), 4u);
}

// =============================================================================
// Thompson Sampling auto-stop
// =============================================================================

TEST_F(ABTestManagerTest, ThompsonSamplingNotTriggeredWhenBelowMinSamples) {
    ABModuleTestConfig cfg = makeConfig("t_thompson", "mod", 0.5);
    cfg.min_samples             = 100;
    cfg.thompson_stop_threshold = 0.95;
    mgr.startTest(cfg, loader);

    for (int i = 0; i < 50; ++i) {
      mgr.recordOutcome("t_thompson", false, true, 1.0);
    }
    for (int i = 0; i < 50; ++i) {
      mgr.recordOutcome("t_thompson", true,  true, 1.0);
    }

    EXPECT_EQ(mgr.getTestStatus("t_thompson"), ABTestStatus::ACTIVE);
}

TEST_F(ABTestManagerTest, ThompsonSamplingPromotesTreatmentWhenItClearlyWins) {
    ABModuleTestConfig cfg = makeConfig("t_wins", "mod", 0.5);
    cfg.min_samples             = 50;
    cfg.thompson_stop_threshold = 0.95;
    mgr.startTest(cfg, loader);

    // Control: 20% success rate, Treatment: 90% success rate.
    for (int i = 0; i < 100; ++i) {
      mgr.recordOutcome("t_wins", false, (i < 20), 1.0);
    }
    for (int i = 0; i < 100; ++i) {
      mgr.recordOutcome("t_wins", true,  (i < 90), 1.0);
    }

    EXPECT_EQ(mgr.getTestStatus("t_wins"), ABTestStatus::PROMOTED);
}

TEST_F(ABTestManagerTest, ThompsonSamplingRollsBackWhenControlClearlyWins) {
    ABModuleTestConfig cfg = makeConfig("t_ctrl_wins", "mod", 0.5);
    cfg.min_samples             = 50;
    cfg.thompson_stop_threshold = 0.95;
    mgr.startTest(cfg, loader);

    // Control: 90% success rate, Treatment: 10% success rate.
    for (int i = 0; i < 100; ++i) {
      mgr.recordOutcome("t_ctrl_wins", false, (i < 90), 1.0);
    }
    for (int i = 0; i < 100; ++i) {
      mgr.recordOutcome("t_ctrl_wins", true,  (i < 10), 1.0);
    }

    EXPECT_EQ(mgr.getTestStatus("t_ctrl_wins"), ABTestStatus::ROLLED_BACK);
}

TEST_F(ABTestManagerTest, ThompsonSamplingDisabledWhenThresholdIsZero) {
    ABModuleTestConfig cfg = makeConfig("t_disabled", "mod", 0.5);
    cfg.min_samples             = 50;
    cfg.thompson_stop_threshold = 0.0;
    mgr.startTest(cfg, loader);

    for (int i = 0; i < 100; ++i) {
      mgr.recordOutcome("t_disabled", false, (i < 20), 1.0);
    }
    for (int i = 0; i < 100; ++i) {
      mgr.recordOutcome("t_disabled", true,  (i < 90), 1.0);
    }

    EXPECT_EQ(mgr.getTestStatus("t_disabled"), ABTestStatus::ACTIVE);
}

// =============================================================================
// Persistence wiring (setStorageEngine / start / re-activation)
// =============================================================================

/// Minimal in-memory IStorageEngine for testing persistence without RocksDB.
class InMemoryStorage : public themis::IStorageEngine {
public:
    Result<void> open(const std::string&) override { return OkVoid(); }
    void         close() override {}

    Result<void> put(const std::string& key, const std::string& value) override {
        store_[key] = value;
        return OkVoid();
    }

    Result<std::string> get(const std::string& key) override {
        auto it = store_.find(key);
        if (it == store_.end())
            return Err<std::string>(errors::ErrorCode::ERR_STORAGE_FILE_NOT_FOUND, key);
        return Ok(it->second);
    }

    Result<void> del(const std::string& key) override {
        store_.erase(key);
        return OkVoid();
    }

    Result<void> scanPrefix(
            std::string_view prefix,
            std::function<bool(std::string_view, std::string_view)> callback) override {
        for (const auto& [k, v] : store_) {
            if (k.rfind(std::string(prefix), 0) == 0) {
                if (!callback(k, v)) {
                  break;
                }
            }
        }
        return OkVoid();
    }

    std::unordered_map<std::string, std::string> store_;
};

class ABTestManagerPersistenceTest : public ::testing::Test {
protected:
    InMemoryStorage storage;
    ModuleLoader    loader;

    ABModuleTestConfig makePersistedConfig(const std::string& test_id) {
        ABModuleTestConfig cfg;
        cfg.test_id        = test_id;
        cfg.module_name    = "mod";
        cfg.control_path   = "/nonexistent/control.so";
        cfg.treatment_path = "/nonexistent/treatment.so";
        cfg.traffic_split  = 0.5;
        cfg.min_samples    = 30;
        return cfg;
    }
};

TEST_F(ABTestManagerPersistenceTest, StartPersistsEntryToStorage) {
    ABTestManager mgr;
    mgr.setStorageEngine(&storage);

    mgr.startTest(makePersistedConfig("p1"), loader);

    auto r = storage.get("ab_test::p1");
    EXPECT_TRUE(r.has_value());
}

TEST_F(ABTestManagerPersistenceTest, StartLoadsPersistedEntries) {
    // Phase 1: populate storage via a first manager instance.
    {
        ABTestManager mgr1;
        mgr1.setStorageEngine(&storage);
        ABModuleTestConfig cfg = makePersistedConfig("p2");
        mgr1.startTest(cfg, loader);
        // Record 100 outcomes to trigger the periodic persist.
        for (int i = 0; i < 100; ++i) {
          mgr1.recordOutcome("p2", false, true, 3.0);
        }
    }

    // Phase 2: new manager — restore from storage.
    ABTestManager mgr2;
    mgr2.setStorageEngine(&storage);
    mgr2.start();

    auto ctrl = mgr2.getControlMetrics("p2");
    EXPECT_EQ(ctrl.sample_count, 100u);
    EXPECT_NEAR(ctrl.mean_latency_ms, 3.0, 0.01);
}

TEST_F(ABTestManagerPersistenceTest, StartTestReactivatesPersistedOnlyEntry) {
    {
        ABTestManager mgr1;
        mgr1.setStorageEngine(&storage);
        mgr1.startTest(makePersistedConfig("p3"), loader);
        for (int i = 0; i < 100; ++i) {
          mgr1.recordOutcome("p3", false, true, 5.0);
        }
    }

    ABTestManager mgr2;
    mgr2.setStorageEngine(&storage);
    mgr2.start();

    // Re-activate: must succeed AND preserve accumulated metrics.
    bool ok = mgr2.startTest(makePersistedConfig("p3"), loader);
    EXPECT_TRUE(ok);
    auto ctrl = mgr2.getControlMetrics("p3");
    EXPECT_EQ(ctrl.sample_count, 100u);
}

TEST_F(ABTestManagerPersistenceTest, RollbackPersistsTerminalStatus) {
    ABTestManager mgr;
    mgr.setStorageEngine(&storage);
    mgr.startTest(makePersistedConfig("p4"), loader);
    mgr.rollbackTest("p4");

    auto r = storage.get("ab_test::p4");
    ASSERT_TRUE(r.has_value());
    auto j = nlohmann::json::parse(r.value());
    EXPECT_EQ(j.at("status").get<std::string>(), "ROLLED_BACK");
}

// =============================================================================
// MetricsCollector wiring (setMetricsCollector)
// =============================================================================

TEST_F(ABTestManagerTest, SetMetricsCollectorDoesNotCrash) {
    auto& mc = observability::MetricsCollector::getInstance();
    EXPECT_NO_THROW(mgr.setMetricsCollector(&mc));
}

TEST_F(ABTestManagerTest, RecordOutcomeEmitsToMetricsCollector) {
    auto& mc = observability::MetricsCollector::getInstance();
    mc.reset();
    mgr.setMetricsCollector(&mc);
    startTest("t_mc");
    mgr.recordOutcome("t_mc", false, true, 5.0);

    const std::string prom = mc.getPrometheusMetrics();
    EXPECT_NE(prom.find("ab_test"), std::string::npos);

    // Cleanup: reset MetricsCollector so this test does not leak state
    mc.reset();
}

TEST_F(ABTestManagerTest, ABTestMetricRowDefaultValues) {
    ABTestMetricRow row;
    EXPECT_EQ(row.requests,    0u);
    EXPECT_EQ(row.conversions, 0u);
    EXPECT_DOUBLE_EQ(row.success_rate,    0.0);
    EXPECT_DOUBLE_EQ(row.mean_latency_ms, 0.0);
    EXPECT_DOUBLE_EQ(row.latency_p99_ms,  0.0);
    EXPECT_EQ(row.status, ABTestStatus::ACTIVE);
}
