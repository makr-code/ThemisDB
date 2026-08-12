// Unit tests for the ML-based workload predictor (Phase 4)
// Covers: recording, EMA smoothing, linear-regression forecast, scaling recommendations,
//         edge cases (empty history, single observation, window eviction), thread safety.

#include "performance/workload_predictor.h"
#include "performance/feature_flags.h"
#include <gtest/gtest.h>
#include <algorithm>
#include <atomic>
#include <cstdint>
#include <thread>
#include <vector>

using namespace themis::performance;

namespace {

// ---------------------------------------------------------------------------
// Helper: build a WorkloadSnapshot
// ---------------------------------------------------------------------------
WorkloadSnapshot make_snapshot(uint64_t ts_us,
                                double qps,
                                double cpu,
                                double mem,
                                double avg_lat_us  = 500.0,
                                double p99_lat_us  = 1000.0,
                                uint32_t conns     = 10)
{
    return WorkloadSnapshot{
        .timestamp_us        = ts_us,
        .qps                 = qps,
        .cpu_utilization     = cpu,
        .memory_utilization  = mem,
        .avg_latency_us      = avg_lat_us,
        .p99_latency_us      = p99_lat_us,
        .active_connections  = conns,
    };
}

} // anonymous namespace

// ============================================================================
// Basic API tests
// ============================================================================

TEST(WorkloadPredictorTest, DefaultConstructionSucceeds) {
    WorkloadPredictor predictor;
    EXPECT_EQ(predictor.observation_count(), 0u);
}

TEST(WorkloadPredictorTest, RecordIncreasesCount) {
    WorkloadPredictor predictor;
    predictor.record(make_snapshot(1'000'000, 100.0, 0.3, 0.4));
    EXPECT_EQ(predictor.observation_count(), 1u);
    predictor.record(make_snapshot(2'000'000, 120.0, 0.35, 0.42));
    EXPECT_EQ(predictor.observation_count(), 2u);
}

TEST(WorkloadPredictorTest, ResetClearsHistory) {
    WorkloadPredictor predictor;
    predictor.record(make_snapshot(1'000'000, 100.0, 0.3, 0.4));
    predictor.record(make_snapshot(2'000'000, 120.0, 0.35, 0.42));
    EXPECT_EQ(predictor.observation_count(), 2u);
    predictor.reset();
    EXPECT_EQ(predictor.observation_count(), 0u);
}

// ============================================================================
// Sliding window eviction
// ============================================================================

TEST(WorkloadPredictorTest, SlidingWindowEvictsOldestEntry) {
    WorkloadPredictor::Config cfg;
    cfg.history_window = 5;
    WorkloadPredictor predictor(cfg);

    for (uint64_t i = 0; i < 10; ++i) {
        predictor.record(make_snapshot(i * 1'000'000, static_cast<double>(i * 10), 0.3, 0.4));
    }
    // Window should be capped at 5
    EXPECT_EQ(predictor.observation_count(), 5u);
}

// ============================================================================
// Predict edge cases
// ============================================================================

TEST(WorkloadPredictorTest, PredictWithNoHistoryReturnsZeroForecast) {
    WorkloadPredictor predictor;
    const WorkloadForecast f = predictor.predict(10'000'000ULL);
    EXPECT_DOUBLE_EQ(f.predicted_qps, 0.0);
    EXPECT_DOUBLE_EQ(f.confidence, 0.0);
}

TEST(WorkloadPredictorTest, PredictWithSingleSnapshotReturnsZeroConfidence) {
    WorkloadPredictor predictor;
    predictor.record(make_snapshot(1'000'000, 200.0, 0.5, 0.6));
    const WorkloadForecast f = predictor.predict(10'000'000ULL);
    EXPECT_DOUBLE_EQ(f.predicted_qps, 200.0);
    EXPECT_DOUBLE_EQ(f.confidence, 0.0);
}

// ============================================================================
// Predict correctness – stable workload
// ============================================================================

TEST(WorkloadPredictorTest, StableWorkloadYieldsHighConfidence) {
    WorkloadPredictor predictor;
    // Feed 20 identical snapshots 1 s apart – perfectly stable signal
    for (int i = 0; i < 20; ++i) {
        predictor.record(make_snapshot(
            static_cast<uint64_t>(i) * 1'000'000ULL,
            500.0, 0.5, 0.5, 1000.0));
    }
    const WorkloadForecast f = predictor.predict(5'000'000ULL);
    // Should predict values close to 500 QPS
    EXPECT_NEAR(f.predicted_qps, 500.0, 50.0);
    EXPECT_NEAR(f.predicted_cpu_utilization, 0.5, 0.05);
    // Confidence should be high for a stable signal
    EXPECT_GT(f.confidence, 0.7);
}

// ============================================================================
// Predict correctness – linearly rising workload
// ============================================================================

TEST(WorkloadPredictorTest, RisingWorkloadPredictsIncrease) {
    WorkloadPredictor predictor;
    // QPS rising from 100 to 400 over 20 samples (step +~15 per sample)
    for (int i = 0; i < 20; ++i) {
        const double qps = 100.0 + static_cast<double>(i) * 15.0;
        const double cpu = 0.2  + static_cast<double>(i) * 0.02;
        predictor.record(make_snapshot(
            static_cast<uint64_t>(i) * 1'000'000ULL,
            qps, std::min(cpu, 1.0), 0.4));
    }
    const WorkloadForecast f = predictor.predict(5'000'000ULL); // 5 steps ahead
    // Forecasted QPS should be greater than the last observed value
    EXPECT_GT(f.predicted_qps, 385.0);
}

// ============================================================================
// Forecast timestamp
// ============================================================================

TEST(WorkloadPredictorTest, ForecastTimestampIsLastPlusHorizon) {
    WorkloadPredictor predictor;
    constexpr uint64_t kLast = 50'000'000ULL;
    constexpr uint64_t kHorizon = 10'000'000ULL;
    predictor.record(make_snapshot(40'000'000ULL, 100.0, 0.3, 0.4));
    predictor.record(make_snapshot(kLast, 110.0, 0.32, 0.41));
    const WorkloadForecast f = predictor.predict(kHorizon);
    EXPECT_EQ(f.forecast_timestamp_us, kLast + kHorizon);
}

// ============================================================================
// Predicted values stay within valid ranges
// ============================================================================

TEST(WorkloadPredictorTest, PredictedUtilizationClampedTo01) {
    WorkloadPredictor predictor;
    // Push CPU/mem utilization very close to 1.0 with a rising trend
    for (int i = 0; i < 10; ++i) {
        predictor.record(make_snapshot(
            static_cast<uint64_t>(i) * 1'000'000ULL,
            1000.0, 0.95, 0.95));
    }
    const WorkloadForecast f = predictor.predict(60'000'000ULL);
    EXPECT_LE(f.predicted_cpu_utilization, 1.0);
    EXPECT_GE(f.predicted_cpu_utilization, 0.0);
    EXPECT_LE(f.predicted_memory_utilization, 1.0);
    EXPECT_GE(f.predicted_memory_utilization, 0.0);
}

// ============================================================================
// ScaleRecommendation – no action when utilization is moderate
// ============================================================================

TEST(WorkloadPredictorTest, NoScalingWhenUtilizationModerate) {
    WorkloadPredictor predictor;
    for (int i = 0; i < 10; ++i) {
        predictor.record(make_snapshot(
            static_cast<uint64_t>(i) * 1'000'000ULL,
            400.0, 0.55, 0.55));
    }
    const ScaleRecommendation rec = predictor.recommend_scaling(32, 4096);
    EXPECT_EQ(rec.direction, ScaleDirection::NONE);
    EXPECT_EQ(rec.recommended_thread_pool_size, 32u);
    EXPECT_EQ(rec.recommended_cache_size_mb, 4096u);
}

// ============================================================================
// ScaleRecommendation – scale UP when utilization is high
// ============================================================================

TEST(WorkloadPredictorTest, ScaleUpWhenUtilizationHigh) {
    WorkloadPredictor predictor;
    for (int i = 0; i < 10; ++i) {
        predictor.record(make_snapshot(
            static_cast<uint64_t>(i) * 1'000'000ULL,
            2000.0, 0.90, 0.90));
    }
    const ScaleRecommendation rec = predictor.recommend_scaling(32, 4096);
    EXPECT_EQ(rec.direction, ScaleDirection::UP);
    EXPECT_GT(rec.recommended_thread_pool_size, 32u);
    EXPECT_GT(rec.recommended_cache_size_mb, 4096u);
    EXPECT_FALSE(rec.reason.empty());
}

// ============================================================================
// ScaleRecommendation – scale DOWN when utilization is low
// ============================================================================

TEST(WorkloadPredictorTest, ScaleDownWhenUtilizationLow) {
    WorkloadPredictor predictor;
    for (int i = 0; i < 10; ++i) {
        predictor.record(make_snapshot(
            static_cast<uint64_t>(i) * 1'000'000ULL,
            50.0, 0.10, 0.10));
    }
    const ScaleRecommendation rec = predictor.recommend_scaling(64, 8192);
    EXPECT_EQ(rec.direction, ScaleDirection::DOWN);
    EXPECT_LT(rec.recommended_thread_pool_size, 64u);
    EXPECT_LT(rec.recommended_cache_size_mb, 8192u);
    EXPECT_FALSE(rec.reason.empty());
}

// ============================================================================
// ScaleRecommendation – floor/ceiling respected
// ============================================================================

TEST(WorkloadPredictorTest, ScaleRecommendationRespectsMinMax) {
    WorkloadPredictor::Config cfg;
    cfg.min_thread_pool_size  = 8;
    cfg.max_thread_pool_size  = 16;
    cfg.min_cache_size_mb     = 512;
    cfg.max_cache_size_mb     = 1024;
    WorkloadPredictor predictor(cfg);

    // High utilization
    for (int i = 0; i < 10; ++i) {
        predictor.record(make_snapshot(
            static_cast<uint64_t>(i) * 1'000'000ULL,
            5000.0, 0.95, 0.95));
    }
    const ScaleRecommendation up = predictor.recommend_scaling(12, 768);
    EXPECT_LE(up.recommended_thread_pool_size, 16u);
    EXPECT_LE(up.recommended_cache_size_mb, 1024u);

    predictor.reset();
    // Low utilization
    for (int i = 0; i < 10; ++i) {
        predictor.record(make_snapshot(
            static_cast<uint64_t>(i) * 1'000'000ULL,
            5.0, 0.05, 0.05));
    }
    const ScaleRecommendation down = predictor.recommend_scaling(8, 512);
    EXPECT_GE(down.recommended_thread_pool_size, 8u);
    EXPECT_GE(down.recommended_cache_size_mb, 512u);
}

// ============================================================================
// ScaleRecommendation – no action without sufficient data
// ============================================================================

TEST(WorkloadPredictorTest, NoRecommendationWithoutSufficientData) {
    WorkloadPredictor predictor;
    // Only 1 observation → confidence is 0 → NONE recommendation
    predictor.record(make_snapshot(1'000'000, 2000.0, 0.95, 0.95));
    const ScaleRecommendation rec = predictor.recommend_scaling(32, 4096);
    EXPECT_EQ(rec.direction, ScaleDirection::NONE);
}

// ============================================================================
// Feature flag integration
// ============================================================================

TEST(WorkloadPredictorFeatureFlagTest, FlagIsAccessible) {
    auto& flags = PerformanceFeatureFlags::instance();
    EXPECT_NO_THROW(flags.ml_workload_predictor_enabled());
}

TEST(WorkloadPredictorFeatureFlagTest, FlagCanBeToggled) {
    auto& flags = PerformanceFeatureFlags::instance();
    const bool initial = flags.ml_workload_predictor_enabled();

    flags.set_ml_workload_predictor_enabled(true);
    EXPECT_TRUE(flags.ml_workload_predictor_enabled());

    flags.set_ml_workload_predictor_enabled(false);
    EXPECT_FALSE(flags.ml_workload_predictor_enabled());

    // Restore
    flags.set_ml_workload_predictor_enabled(initial);
}

TEST(WorkloadPredictorFeatureFlagTest, FlagAppearsInGetAllFlags) {
    auto& flags = PerformanceFeatureFlags::instance();
    const auto all = flags.get_all_flags();
    EXPECT_NE(all.find("ml_workload_predictor"), all.end());
}

// ============================================================================
// Thread safety
// ============================================================================

TEST(WorkloadPredictorTest, ConcurrentRecordIsThreadSafe) {
    WorkloadPredictor predictor;
    constexpr int kThreads  = 4;
    constexpr int kPerThread = 50;

    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&predictor, t]() {
            for (int i = 0; i < kPerThread; ++i) {
                const uint64_t ts = static_cast<uint64_t>(t * kPerThread + i) * 1'000'000ULL;
                predictor.record(make_snapshot(ts, 100.0 + i, 0.4, 0.5));
            }
        });
    }
    for (auto& th : threads) th.join();

    // Should not crash; count is bounded by history_window (default 60)
    EXPECT_LE(predictor.observation_count(), predictor.config().history_window);
}

TEST(WorkloadPredictorTest, ConcurrentPredictIsThreadSafe) {
    WorkloadPredictor predictor;
    for (int i = 0; i < 20; ++i) {
        predictor.record(make_snapshot(
            static_cast<uint64_t>(i) * 1'000'000ULL, 200.0 + i, 0.5, 0.5));
    }

    std::atomic<int> success_count{0};
    constexpr int kThreads = 4;
    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&predictor, &success_count]() {
            const auto f = predictor.predict(5'000'000ULL);
            if (f.predicted_qps > 0.0) {
                success_count.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }
    for (auto& th : threads) th.join();
    EXPECT_EQ(success_count.load(), kThreads);
}

// ============================================================================
// Edge case: predict with zero-horizon
// ============================================================================

TEST(WorkloadPredictorTest, PredictZeroHorizonReturnsCurrentEstimate) {
    WorkloadPredictor predictor;
    for (int i = 0; i < 5; ++i) {
        predictor.record(make_snapshot(
            static_cast<uint64_t>(i) * 1'000'000ULL,
            300.0, 0.5, 0.5));
    }
    // horizon_us = 0: forecast_timestamp should equal last snapshot timestamp
    const WorkloadForecast f = predictor.predict(0ULL);
    EXPECT_EQ(f.forecast_timestamp_us, 4'000'000ULL);
    // Predicted QPS should be non-negative (stable signal → near 300)
    EXPECT_GE(f.predicted_qps, 0.0);
    EXPECT_LE(f.predicted_cpu_utilization, 1.0);
}

// ============================================================================
// Feature flag: load_from_config for new key
// ============================================================================

TEST(WorkloadPredictorFeatureFlagTest, LoadFromConfigSetsNewFlag) {
    auto& flags = PerformanceFeatureFlags::instance();
    const bool initial = flags.ml_workload_predictor_enabled();

    // Enable via config map
    flags.load_from_config({{"enable_ml_workload_predictor", true}});
    EXPECT_TRUE(flags.ml_workload_predictor_enabled());

    // Disable via config map
    flags.load_from_config({{"enable_ml_workload_predictor", false}});
    EXPECT_FALSE(flags.ml_workload_predictor_enabled());

    // Restore
    flags.set_ml_workload_predictor_enabled(initial);
}
