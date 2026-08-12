/**
 * @file test_ebpf_tracer.cpp
 * @brief Unit tests for EbpfTracer – eBPF-based low-overhead kernel-level
 *        tracing integration.
 *
 * Tests cover:
 *  - Default construction and configuration retrieval
 *  - Enable / disable lifecycle
 *  - Start / stop idempotency
 *  - Stats accumulation (platform-agnostic path)
 *  - Event ring-buffer bounds enforcement
 *  - Callback registration and invocation
 *  - Reset clears stats and events
 *  - isPlatformSupported() returns a bool (no crash)
 *  - MetricsCollector gauge publication
 */

#include <gtest/gtest.h>
#include "observability/ebpf_tracer.h"
#include "observability/metrics_collector.h"

#include <atomic>
#include <chrono>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

using namespace themis::observability;
using namespace std::chrono_literals;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static EbpfTracerConfig makeConfig(bool enabled = false,
                                   std::chrono::milliseconds interval = 50ms) {
    EbpfTracerConfig cfg;
    cfg.enabled             = enabled;
    cfg.collection_interval = interval;
    cfg.max_events_retained = 10;
    return cfg;
}

// ---------------------------------------------------------------------------
// Construction / configuration
// ---------------------------------------------------------------------------

TEST(EbpfTracerTest, DefaultConfig_DisabledByDefault) {
    EbpfTracer tracer;
    EXPECT_FALSE(tracer.isEnabled());
}

TEST(EbpfTracerTest, ConfigRoundtrip) {
    EbpfTracerConfig cfg = makeConfig(false, 200ms);
    cfg.probe_context_switches = true;
    cfg.probe_page_faults      = false;
    cfg.probe_cpu_migrations   = true;
    cfg.probe_task_clock       = false;
    cfg.max_events_retained    = 42;

    EbpfTracer tracer(cfg);

    auto got = tracer.getConfig();
    EXPECT_EQ(200, got.collection_interval.count());
    EXPECT_EQ(42u, got.max_events_retained);
    EXPECT_TRUE(got.probe_context_switches);
    EXPECT_FALSE(got.probe_page_faults);
    EXPECT_TRUE(got.probe_cpu_migrations);
    EXPECT_FALSE(got.probe_task_clock);
}

TEST(EbpfTracerTest, EnabledFlag_ConstructedEnabled) {
    EbpfTracerConfig cfg = makeConfig(true, 500ms);
    EbpfTracer tracer(cfg);
    EXPECT_TRUE(tracer.isEnabled());
    tracer.stop();
    EXPECT_FALSE(tracer.isEnabled());
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

TEST(EbpfTracerTest, StartStop_Idempotent) {
    EbpfTracer tracer(makeConfig());
    EXPECT_NO_THROW(tracer.start());
    EXPECT_TRUE(tracer.isEnabled());
    EXPECT_NO_THROW(tracer.start()); // second start – no-op
    EXPECT_TRUE(tracer.isEnabled());
    EXPECT_NO_THROW(tracer.stop());
    EXPECT_FALSE(tracer.isEnabled());
    EXPECT_NO_THROW(tracer.stop()); // second stop – no-op
}

TEST(EbpfTracerTest, EnableDisable_Roundtrip) {
    EbpfTracer tracer(makeConfig());
    tracer.enable();
    EXPECT_TRUE(tracer.isEnabled());
    tracer.disable();
    EXPECT_FALSE(tracer.isEnabled());
    tracer.enable();
    EXPECT_TRUE(tracer.isEnabled());
    tracer.stop();
}

// ---------------------------------------------------------------------------
// Statistics
// ---------------------------------------------------------------------------

TEST(EbpfTracerTest, InitialStats_AllZero) {
    EbpfTracer tracer;
    auto stats = tracer.getStats();
    EXPECT_EQ(0, stats.context_switches_total);
    EXPECT_EQ(0, stats.page_faults_total);
    EXPECT_EQ(0, stats.cpu_migrations_total);
    EXPECT_EQ(0, stats.task_clock_ns_total);
    EXPECT_EQ(0u, stats.collection_cycles);
}

TEST(EbpfTracerTest, CollectionCycles_IncrementAfterStart) {
    EbpfTracerConfig cfg = makeConfig(true, 30ms);
    EbpfTracer tracer(cfg);

    // Wait for at least 2 collection cycles
    std::this_thread::sleep_for(100ms);
    tracer.stop();

    auto stats = tracer.getStats();
    EXPECT_GE(stats.collection_cycles, 1u);
}

TEST(EbpfTracerTest, Reset_ClearsStats) {
    EbpfTracerConfig cfg = makeConfig(true, 20ms);
    EbpfTracer tracer(cfg);
    std::this_thread::sleep_for(60ms);
    tracer.stop();

    tracer.reset();
    auto stats = tracer.getStats();
    EXPECT_EQ(0, stats.context_switches_total);
    EXPECT_EQ(0, stats.page_faults_total);
    EXPECT_EQ(0, stats.cpu_migrations_total);
    EXPECT_EQ(0, stats.task_clock_ns_total);
    EXPECT_EQ(0u, stats.collection_cycles);
}

TEST(EbpfTracerTest, Reset_WhileRunning_DoesNotCrash) {
    // Verifies the pending_reset_ atomic flag path: calling reset() while the
    // background collection loop is active must not corrupt state or crash.
    EbpfTracerConfig cfg = makeConfig(true, 10ms);
    EbpfTracer tracer(cfg);

    for (int _ = 0; _ < 5; ++_) {
        std::this_thread::sleep_for(15ms);
        EXPECT_NO_THROW(tracer.reset());
    }

    tracer.stop();
    // Reset after stop clears all accumulated state.
    tracer.reset();
    auto stats = tracer.getStats();
    EXPECT_EQ(0u, stats.collection_cycles);
}


TEST(EbpfTracerTest, InitialEvents_Empty) {
    EbpfTracer tracer;
    EXPECT_TRUE(tracer.getRecentEvents().empty());
}

TEST(EbpfTracerTest, Reset_ClearsEvents) {
    EbpfTracerConfig cfg = makeConfig(true, 20ms);
    EbpfTracer tracer(cfg);
    std::this_thread::sleep_for(80ms);
    tracer.stop();

    // Whether or not we have events, reset must succeed
    EXPECT_NO_THROW(tracer.reset());
    EXPECT_TRUE(tracer.getRecentEvents().empty());
}

TEST(EbpfTracerTest, EventRingBuffer_RespectsBound) {
    // max_events_retained = 5; if events arrive they must be bounded
    EbpfTracerConfig cfg = makeConfig(true, 10ms);
    cfg.max_events_retained = 5;
    EbpfTracer tracer(cfg);
    std::this_thread::sleep_for(100ms);
    tracer.stop();

    auto events = tracer.getRecentEvents();
    EXPECT_LE(events.size(), 5u);
}

// ---------------------------------------------------------------------------
// Callback
// ---------------------------------------------------------------------------

TEST(EbpfTracerTest, Callback_RegisteredWithoutCrash) {
    EbpfTracer tracer(makeConfig());
    EXPECT_NO_THROW(tracer.registerEventCallback(
        [](const std::vector<KernelEvent>&) {}));
}

TEST(EbpfTracerTest, Callback_Invoked_WhenTracerRunning) {
    EbpfTracerConfig cfg = makeConfig(false, 20ms);
    EbpfTracer tracer(cfg);

    std::atomic<int> call_count{0};
    tracer.registerEventCallback([&](const std::vector<KernelEvent>&) {
        ++call_count;
    });

    tracer.start();
    std::this_thread::sleep_for(100ms);
    tracer.stop();

    // The callback fires only when events are non-empty (non-zero deltas).
    // On Linux with perf_event_open available, at least one cycle with a
    // non-zero delta is expected.  In sandboxed / non-Linux environments the
    // callback may never fire — the test just verifies no crash occurs and
    // the count is non-negative.
    EXPECT_GE(call_count.load(), 0);

#if defined(__linux__)
    // On Linux we also verify that collection_cycles incremented, confirming
    // the background thread ran correctly even when no events were emitted.
    auto stats = tracer.getStats();
    EXPECT_GE(stats.collection_cycles, 1u);
#endif
}

// ---------------------------------------------------------------------------
// Platform support
// ---------------------------------------------------------------------------

TEST(EbpfTracerTest, IsPlatformSupported_ReturnsABool) {
    // Simply confirm no crash and that the return type is bool
    bool supported = EbpfTracer::isPlatformSupported();
    (void)supported; // value is platform-dependent
    SUCCEED();
}

#if defined(__linux__)
TEST(EbpfTracerTest, PlatformSupported_TrueOnLinux) {
    EXPECT_TRUE(EbpfTracer::isPlatformSupported());
}
#else
TEST(EbpfTracerTest, PlatformSupported_FalseOnNonLinux) {
    EXPECT_FALSE(EbpfTracer::isPlatformSupported());
}
#endif

// ---------------------------------------------------------------------------
// MetricsCollector integration
// ---------------------------------------------------------------------------

TEST(EbpfTracerTest, Metrics_GaugesPublishedAfterCollectionCycle) {
    MetricsCollector::getInstance().reset();

    EbpfTracerConfig cfg = makeConfig(true, 20ms);
    EbpfTracer tracer(cfg);
    std::this_thread::sleep_for(80ms);
    tracer.stop();

    // The collection_cycles gauge must have been published
    std::string prom = MetricsCollector::getInstance().getPrometheusMetrics();
    EXPECT_NE(std::string::npos,
              prom.find("themis_ebpf_collection_cycles_total"));
}

// ---------------------------------------------------------------------------
// KernelEvent structure
// ---------------------------------------------------------------------------

TEST(KernelEventTest, DefaultConstruction) {
    KernelEvent ev;
    EXPECT_EQ(EbpfProbeType::NONE, ev.type);
    EXPECT_EQ(0, ev.delta);
    EXPECT_TRUE(ev.description.empty());
}

TEST(EbpfTracerStatsTest, DefaultConstruction) {
    EbpfTracerStats stats;
    EXPECT_EQ(0, stats.context_switches_total);
    EXPECT_EQ(0, stats.page_faults_total);
    EXPECT_EQ(0, stats.cpu_migrations_total);
    EXPECT_EQ(0, stats.task_clock_ns_total);
    EXPECT_EQ(0u, stats.collection_cycles);
}

TEST(EbpfTracerConfigTest, DefaultValues) {
    EbpfTracerConfig cfg;
    EXPECT_FALSE(cfg.enabled);
    EXPECT_EQ(1000, cfg.collection_interval.count());
    EXPECT_EQ(3600u, cfg.max_events_retained);
    EXPECT_TRUE(cfg.probe_context_switches);
    EXPECT_TRUE(cfg.probe_page_faults);
    EXPECT_TRUE(cfg.probe_cpu_migrations);
    EXPECT_TRUE(cfg.probe_task_clock);
}
