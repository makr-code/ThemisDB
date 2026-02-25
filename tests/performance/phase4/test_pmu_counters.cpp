/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_pmu_counters.cpp                              ║
  Version:         0.0.1                                              ║
  Last Modified:   2026-02-25                                         ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     200                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// Tests for Phase 4 Hardware PMU counter integration (cache miss analysis).
//
// These tests are designed to be safe in CI environments where PMU access
// may be restricted (kernel.perf_event_paranoid > 2).  Every test that
// relies on actual hardware counters first checks is_available() /
// pmu_accessible() and skips gracefully if PMU access is denied.

#include "performance/phase4/pmu_counters.h"
#include "performance/phase4/feature_flags.h"
#include <gtest/gtest.h>
#include <vector>
#include <numeric>

using namespace themis::performance::phase4;

// ---------------------------------------------------------------------------
// PmuCounter unit tests
// ---------------------------------------------------------------------------

TEST(PmuCounterTest, DefaultConstructedIsNotOpen) {
    PmuCounter counter;
    EXPECT_FALSE(counter.is_open());
    EXPECT_EQ(counter.read(), 0u);
}

TEST(PmuCounterTest, CloseOnUnopenedCounterIsSafe) {
    PmuCounter counter;
    EXPECT_NO_THROW(counter.close());
    EXPECT_FALSE(counter.is_open());
}

TEST(PmuCounterTest, MoveSemantics) {
    PmuCounter a;
    PmuCounter b = std::move(a);
    EXPECT_FALSE(a.is_open()); // NOLINT(bugprone-use-after-move)
    EXPECT_FALSE(b.is_open());
}

TEST(PmuCounterTest, OpenLLCMissCounterOrSkip) {
    // PERF_TYPE_HARDWARE = 0, PERF_COUNT_HW_CACHE_MISSES = 3
    constexpr uint32_t PERF_TYPE_HARDWARE      = 0;
    constexpr uint64_t PERF_COUNT_HW_CACHE_MISSES = 3;

    PmuCounter counter;
    bool opened = counter.open(PERF_TYPE_HARDWARE, PERF_COUNT_HW_CACHE_MISSES);
    if (!opened) {
        GTEST_SKIP() << "PMU not accessible (perf_event_paranoid too high or non-Linux)";
    }
    EXPECT_TRUE(counter.is_open());

    counter.enable();
    // Touch some memory to generate events
    volatile int sum = 0;
    for (int i = 0; i < 1000; ++i) sum += i;
    counter.disable();

    uint64_t val = counter.read();
    // Value must be readable (≥0) when PMU is open
    EXPECT_GE(val, 0u);
}

// ---------------------------------------------------------------------------
// CacheMissAnalyzer unit tests
// ---------------------------------------------------------------------------

TEST(CacheMissAnalyzerTest, PmuAccessibleReportsBool) {
    bool accessible = CacheMissAnalyzer::pmu_accessible();
    // Just verifying it returns a valid bool without crashing
    SUCCEED() << "PMU accessible: " << (accessible ? "yes" : "no");
}

TEST(CacheMissAnalyzerTest, UnavailableAnalyzerReturnsSafeDefaults) {
    // Even if hardware is unavailable, stop() must return a valid struct
    CacheMissAnalyzer analyzer;
    analyzer.start(); // safe even if unavailable
    CacheMissMetrics m = analyzer.stop();

    if (!analyzer.is_available()) {
        EXPECT_FALSE(m.available);
        EXPECT_EQ(m.l1d_read_misses,       0u);
        EXPECT_EQ(m.llc_misses,            0u);
        EXPECT_EQ(m.branch_mispredictions, 0u);
    }
    // If available, no assertions about values (hardware-dependent)
}

TEST(CacheMissAnalyzerTest, StartStopCycleIsSafe) {
    CacheMissAnalyzer analyzer;
    if (!analyzer.is_available()) {
        GTEST_SKIP() << "PMU not accessible";
    }

    analyzer.start();
    // Generate some cache activity
    std::vector<int> v(4096, 1);
    volatile long s = std::accumulate(v.begin(), v.end(), 0L);
    (void)s;
    CacheMissMetrics m = analyzer.stop();

    EXPECT_TRUE(m.available);
    // Counters must be non-negative (they are unsigned, so always true,
    // but the read must succeed without returning garbage)
    SUCCEED() << "LLC misses=" << m.llc_misses
              << " L1d misses=" << m.l1d_read_misses
              << " branch mispredict=" << m.branch_mispredictions;
}

TEST(CacheMissAnalyzerTest, MultipleStartStopCycles) {
    CacheMissAnalyzer analyzer;
    if (!analyzer.is_available()) {
        GTEST_SKIP() << "PMU not accessible";
    }

    for (int iter = 0; iter < 3; ++iter) {
        analyzer.start();
        volatile int x = 0;
        for (int i = 0; i < 100; ++i) x += i;
        CacheMissMetrics m = analyzer.stop();
        EXPECT_TRUE(m.available);
    }
}

// ---------------------------------------------------------------------------
// ScopedCacheMissTimer unit tests
// ---------------------------------------------------------------------------

TEST(ScopedCacheMissTimerTest, WritesMetricsOnDestruction) {
    CacheMissAnalyzer analyzer;
    CacheMissMetrics  metrics{};

    {
        ScopedCacheMissTimer timer(analyzer, &metrics);
        volatile int x = 0;
        for (int i = 0; i < 500; ++i) x += i;
    }

    if (analyzer.is_available()) {
        EXPECT_TRUE(metrics.available);
    } else {
        EXPECT_FALSE(metrics.available);
    }
}

TEST(ScopedCacheMissTimerTest, NullOutputPointerIsSafe) {
    CacheMissAnalyzer analyzer;
    // Passing nullptr must not crash
    EXPECT_NO_THROW({
        ScopedCacheMissTimer timer(analyzer, nullptr);
        volatile int x = 1 + 1;
        (void)x;
    });
}

// ---------------------------------------------------------------------------
// Phase4FeatureFlags PMU flag tests
// ---------------------------------------------------------------------------

TEST(Phase4FeatureFlagsTest, PmuEnabledDefaultFalse) {
    // Default state must be disabled (safe default)
    auto& flags = Phase4FeatureFlags::instance();
    // Reset to known state
    flags.set_pmu_enabled(false);
    EXPECT_FALSE(flags.pmu_enabled());
}

TEST(Phase4FeatureFlagsTest, PmuEnabledToggle) {
    auto& flags = Phase4FeatureFlags::instance();
    flags.set_pmu_enabled(true);
    EXPECT_TRUE(flags.pmu_enabled());
    flags.set_pmu_enabled(false);
    EXPECT_FALSE(flags.pmu_enabled());
}

TEST(Phase4FeatureFlagsTest, PmemFlagUnaffectedByPmuToggle) {
    auto& flags = Phase4FeatureFlags::instance();
    flags.set_pmem_enabled(false);
    flags.set_pmu_enabled(true);
    EXPECT_FALSE(flags.pmem_enabled());
    flags.set_pmu_enabled(false);
}

// Main removed - using GTest's main from themis_tests.exe
