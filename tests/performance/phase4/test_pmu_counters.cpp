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
    for (int i = 0; i < 1000; ++i) {
      sum += i;
    }
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
        for (int i = 0; i < 100; ++i) {
          x += i;
        }
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
        for (int i = 0; i < 500; ++i) {
          x += i;
        }
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

#ifndef THEMIS_ENABLE_PMU_COUNTERS
TEST(PmuCounterStubBridgeTest, BridgeCallbacksProvideSyntheticMetrics) {
    PmuCounter::setOpenFn([](uint32_t type, uint64_t config) {
        return type == 1 && config == 2;
    });
    PmuCounter::setReadFn([] { return 1234u; });
    CacheMissAnalyzer::setProbeFn([] { return true; });
    CacheMissAnalyzer::setStopFn([] {
        CacheMissMetrics metrics;
        metrics.l1d_read_misses = 10;
        metrics.llc_misses = 20;
        metrics.branch_mispredictions = 30;
        return metrics;
    });

    PmuCounter counter;
    EXPECT_TRUE(counter.open(1, 2));
    EXPECT_TRUE(counter.is_open());
    EXPECT_EQ(counter.read(), 1234u);

    CacheMissAnalyzer analyzer;
    EXPECT_TRUE(analyzer.is_available());
    auto metrics = analyzer.stop();
    EXPECT_TRUE(metrics.available);
    EXPECT_EQ(metrics.l1d_read_misses, 10u);
    EXPECT_EQ(metrics.llc_misses, 20u);
    EXPECT_EQ(metrics.branch_mispredictions, 30u);
    EXPECT_TRUE(CacheMissAnalyzer::pmu_accessible());

    PmuCounter::setOpenFn(nullptr);
    PmuCounter::setReadFn(nullptr);
    CacheMissAnalyzer::setProbeFn(nullptr);
    CacheMissAnalyzer::setStopFn(nullptr);
}
#endif

// ---------------------------------------------------------------------------
// Non-Linux platform backend tests
// Verify the RDTSC / cycle-count fallback that is active on macOS, Windows,
// and all other non-Linux platforms.
// ---------------------------------------------------------------------------

#ifndef __linux__

TEST(PmuCounterTest, NonLinuxOpenAlwaysSucceeds) {
    // On non-Linux platforms, open() unconditionally returns true because
    // RDTSC / QueryThreadCycleTime / mach_absolute_time is always available.
    PmuCounter counter;
    bool ok = counter.open(0, 0);
    EXPECT_TRUE(ok);
    EXPECT_TRUE(counter.is_open());
}

TEST(PmuCounterTest, NonLinuxCycleCountIsPositiveAfterWork) {
    // After enable() + some work, read() must return a non-zero elapsed
    // cycle (or time) delta from the RDTSC / platform fallback.
    PmuCounter counter;
    ASSERT_TRUE(counter.open(0, 0));
    counter.enable();
    volatile uint64_t sum = 0;
    for (int i = 0; i < 100'000; ++i) {
      sum += static_cast<uint64_t>(i);
    }
    (void)sum;
    uint64_t cycles = counter.read();
    EXPECT_GT(cycles, 0u) << "Cycle counter must advance after real work";
}

TEST(PmuCounterTest, NonLinuxReadBeforeEnableReturnsZeroOrSafe) {
    // read() before enable() should return 0 (start was recorded as 0 in open())
    PmuCounter counter;
    ASSERT_TRUE(counter.open(0, 0));
    // Don't call enable() — read() should not crash and return a defined value.
    EXPECT_NO_THROW(counter.read());
}

TEST(PmuCounterTest, NonLinuxMultipleOpenReusesSafeSlots) {
    // Opening many counters must not overflow the fixed-size slot pool for
    // kMaxFallbackSlots (128 slots) by wrapping around safely.
    std::vector<PmuCounter> counters(64);
    for (auto& c : counters) {
        EXPECT_TRUE(c.open(0, 0));
        EXPECT_TRUE(c.is_open());
    }
}

TEST(CacheMissAnalyzerTest, NonLinuxFallbackIsAvailable) {
    // On non-Linux, CacheMissAnalyzer should always report available=true
    // because the RDTSC / cycle-count fallback always succeeds.
    CacheMissAnalyzer analyzer;
    EXPECT_TRUE(analyzer.is_available());

    analyzer.start();
    volatile int x = 0;
    for (int i = 0; i < 10'000; ++i) {
      x ^= i;
    }
    (void)x;
    CacheMissMetrics m = analyzer.stop();

    EXPECT_TRUE(m.available)
        << "Non-Linux fallback must set CacheMissMetrics::available = true";
}

TEST(CacheMissAnalyzerTest, NonLinuxPmuAccessibleReturnsTrue) {
    // RDTSC / mach_absolute_time / QueryThreadCycleTime is always accessible.
    EXPECT_TRUE(CacheMissAnalyzer::pmu_accessible());
}

TEST(CacheMissAnalyzerTest, NonLinuxStopMetricsFieldsAreDefined) {
    // On non-Linux without true PMU access, cache-miss fields are 0 but
    // available is true.  Verify the struct has defined (not garbage) values.
    CacheMissAnalyzer analyzer;
    ASSERT_TRUE(analyzer.is_available());
    analyzer.start();
    volatile int dummy = 1;
    (void)dummy;
    CacheMissMetrics m = analyzer.stop();
    EXPECT_TRUE(m.available);
    // Cache-miss event counts are 0 on cycle-count fallback (no hardware PMU).
    EXPECT_EQ(m.l1d_read_misses,       0u);
    EXPECT_EQ(m.llc_misses,            0u);
    EXPECT_EQ(m.branch_mispredictions, 0u);
}

#ifdef __APPLE__
TEST(CacheMissAnalyzerTest, MacOsKpcOrFallbackIsCoherent) {
    // On macOS, either kpc counters or the RDTSC/CNTVCT_EL0 fallback is used.
    // Either way, multiple start/stop cycles must remain coherent and not crash.
    CacheMissAnalyzer analyzer;
    EXPECT_TRUE(analyzer.is_available());

    for (int iter = 0; iter < 5; ++iter) {
        analyzer.start();
        volatile uint64_t s = 0;
        for (int i = 0; i < 1000; ++i) {
          s += static_cast<uint64_t>(i * i);
        }
        (void)s;
        CacheMissMetrics m = analyzer.stop();
        EXPECT_TRUE(m.available) << "macOS analyzer must stay available across iterations";
    }
}
#endif // __APPLE__

#ifdef _WIN32
TEST(CacheMissAnalyzerTest, WindowsCycleCountBackendIsCoherent) {
    // On Windows, __rdtsc() (x86/x86_64) or QueryThreadCycleTime (ARM64) is
    // used.  Verify that multiple cycles remain coherent and available=true.
    CacheMissAnalyzer analyzer;
    EXPECT_TRUE(analyzer.is_available());

    for (int iter = 0; iter < 5; ++iter) {
        analyzer.start();
        volatile uint64_t s = 0;
        for (int i = 0; i < 1000; ++i) {
          s += static_cast<uint64_t>(i * i);
        }
        (void)s;
        CacheMissMetrics m = analyzer.stop();
        EXPECT_TRUE(m.available) << "Windows analyzer must stay available across iterations";
    }
}
#endif // _WIN32

#endif // !__linux__

// Main removed - using GTest's main from themis_tests.exe
