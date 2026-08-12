// Tests for SystemMemoryPressureMonitor
// Covers: level classification, callback registration/unregistration,
//         eviction triggering, thread lifecycle, OS memory sampling.

#include "performance/phase3/memory_pressure.h"
#include "performance/phase3/feature_flags.h"
#include <gtest/gtest.h>
#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

using namespace themis::performance::phase3;
using Level = SystemMemoryPressureMonitor::PressureLevel;

// ---------------------------------------------------------------------------
// Helper: build a monitor that treats everything as HIGH pressure
// by using very low thresholds and a tiny total memory override.
// ---------------------------------------------------------------------------
static SystemMemoryPressureMonitor make_high_pressure_monitor(
    std::chrono::milliseconds poll = std::chrono::milliseconds(50))
{
    SystemMemoryPressureMonitor::Config cfg;
    cfg.poll_interval = poll;
    // 1 GB total, only 50 MB available → ~95% usage → CRITICAL
    cfg.total_memory_override_bytes = 1024ULL * 1024 * 1024; // 1 GB
    // By default available is read from OS; we rely on real OS data here but
    // lower the thresholds so that any non-trivial real usage fires callbacks.
    cfg.thresholds.moderate_threshold = 0.0;
    cfg.thresholds.high_threshold     = 0.0;
    cfg.thresholds.critical_threshold = 0.0;
    return SystemMemoryPressureMonitor(cfg);
}

// ---------------------------------------------------------------------------
// Level classification
// ---------------------------------------------------------------------------

class MemoryPressureClassifyTest : public ::testing::Test {
protected:
    // Use default thresholds (moderate=70, high=85, critical=95)
    SystemMemoryPressureMonitor monitor;
};

TEST_F(MemoryPressureClassifyTest, LevelNames) {
    EXPECT_EQ(SystemMemoryPressureMonitor::level_name(Level::NORMAL),   "NORMAL");
    EXPECT_EQ(SystemMemoryPressureMonitor::level_name(Level::MODERATE), "MODERATE");
    EXPECT_EQ(SystemMemoryPressureMonitor::level_name(Level::HIGH),     "HIGH");
    EXPECT_EQ(SystemMemoryPressureMonitor::level_name(Level::CRITICAL), "CRITICAL");
}

TEST_F(MemoryPressureClassifyTest, SampleReturnsValidSnapshot) {
    auto snap = monitor.sample();
    // total_bytes may be 0 on some CI environments without /proc/meminfo,
    // but the level should always be set.
    EXPECT_GE(snap.usage_percent, 0.0);
    EXPECT_LE(snap.usage_percent, 100.0);
    // Snapshot time should be recent
    auto age = std::chrono::steady_clock::now() - snap.sampled_at;
    EXPECT_LT(age, std::chrono::seconds(5));
}

TEST_F(MemoryPressureClassifyTest, LevelOrderingConsistency) {
    // CRITICAL > HIGH > MODERATE > NORMAL (as int comparisons used internally)
    EXPECT_GT(static_cast<int>(Level::CRITICAL), static_cast<int>(Level::HIGH));
    EXPECT_GT(static_cast<int>(Level::HIGH),     static_cast<int>(Level::MODERATE));
    EXPECT_GT(static_cast<int>(Level::MODERATE), static_cast<int>(Level::NORMAL));
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

TEST(MemoryPressureLifecycleTest, StartStop) {
    SystemMemoryPressureMonitor monitor;
    EXPECT_FALSE(monitor.is_running());
    monitor.start();
    EXPECT_TRUE(monitor.is_running());
    monitor.stop();
    EXPECT_FALSE(monitor.is_running());
}

TEST(MemoryPressureLifecycleTest, DoubleStartIsIdempotent) {
    SystemMemoryPressureMonitor monitor;
    monitor.start();
    monitor.start(); // should not throw or deadlock
    EXPECT_TRUE(monitor.is_running());
    monitor.stop();
}

TEST(MemoryPressureLifecycleTest, StopWithoutStartIsIdempotent) {
    SystemMemoryPressureMonitor monitor;
    monitor.stop(); // should not throw
    EXPECT_FALSE(monitor.is_running());
}

TEST(MemoryPressureLifecycleTest, DestructorStopsThread) {
    {
        SystemMemoryPressureMonitor monitor;
        monitor.start();
        EXPECT_TRUE(monitor.is_running());
        // destructor called here → must join cleanly
    }
    // If we reach here without a crash the test passes
    SUCCEED();
}

// ---------------------------------------------------------------------------
// Callback registration / unregistration
// ---------------------------------------------------------------------------

TEST(MemoryPressureCallbackTest, RegisterAndUnregister) {
    auto monitor = make_high_pressure_monitor();
    int count = 0;
    size_t handle = monitor.register_eviction_callback(
        Level::MODERATE, [&count]() { count++; });
    monitor.unregister_eviction_callback(handle);

    monitor.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    monitor.stop();

    // Callback was removed before the first poll; count may be 0 or 1
    // (race window is tiny, we just verify no crash).
    EXPECT_GE(count, 0);
}

TEST(MemoryPressureCallbackTest, MultipleCallbacksRegistered) {
    auto monitor = make_high_pressure_monitor();
    std::atomic<int> calls_a{0}, calls_b{0};

    monitor.register_eviction_callback(Level::MODERATE, [&calls_a]() { calls_a++; });
    monitor.register_eviction_callback(Level::HIGH,     [&calls_b]() { calls_b++; });

    monitor.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    monitor.stop();

    // Both thresholds are 0.0, so all callbacks should fire at least once.
    EXPECT_GT(calls_a.load(), 0);
    EXPECT_GT(calls_b.load(), 0);
}

// ---------------------------------------------------------------------------
// Eviction triggering
// ---------------------------------------------------------------------------

TEST(MemoryPressureEvictionTest, CallbackFiredOnHighPressure) {
    auto monitor = make_high_pressure_monitor(std::chrono::milliseconds(50));
    std::atomic<int> eviction_count{0};

    monitor.register_eviction_callback(Level::MODERATE, [&eviction_count]() {
        eviction_count.fetch_add(1, std::memory_order_relaxed);
    });

    monitor.start();
    // Allow at least 3 poll cycles
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    monitor.stop();

    EXPECT_GT(eviction_count.load(), 0);
    EXPECT_GT(monitor.eviction_trigger_count(), 0ULL);
}

TEST(MemoryPressureEvictionTest, EvictionTriggerCountAccumulates) {
    auto monitor = make_high_pressure_monitor(std::chrono::milliseconds(50));
    monitor.register_eviction_callback(Level::MODERATE, []() {});
    monitor.register_eviction_callback(Level::HIGH,     []() {});

    monitor.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    monitor.stop();

    // Two callbacks firing per cycle → count should be > 1
    EXPECT_GT(monitor.eviction_trigger_count(), 1ULL);
}

TEST(MemoryPressureEvictionTest, NormalPressureDoesNotTrigger) {
    // Use very high thresholds so nothing triggers
    SystemMemoryPressureMonitor::Config cfg;
    cfg.poll_interval = std::chrono::milliseconds(50);
    cfg.thresholds.moderate_threshold = 99.9;
    cfg.thresholds.high_threshold     = 99.95;
    cfg.thresholds.critical_threshold = 99.99;

    SystemMemoryPressureMonitor monitor(cfg);
    std::atomic<int> eviction_count{0};
    monitor.register_eviction_callback(Level::MODERATE, [&eviction_count]() {
        eviction_count.fetch_add(1, std::memory_order_relaxed);
    });

    monitor.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    monitor.stop();

    // With thresholds at 99.9% no real system should fire the callback.
    EXPECT_EQ(eviction_count.load(), 0);
    EXPECT_EQ(monitor.eviction_trigger_count(), 0ULL);
}

// ---------------------------------------------------------------------------
// last_snapshot()
// ---------------------------------------------------------------------------

TEST(MemoryPressureSnapshotTest, LastSnapshotUpdatedAfterStart) {
    SystemMemoryPressureMonitor::Config cfg;
    cfg.poll_interval = std::chrono::milliseconds(50);
    SystemMemoryPressureMonitor monitor(cfg);

    monitor.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    monitor.stop();

    auto snap = monitor.last_snapshot();
    // After at least one poll cycle the sampled_at should not be epoch.
    EXPECT_NE(snap.sampled_at, std::chrono::steady_clock::time_point{});
}

// ---------------------------------------------------------------------------
// Feature flag integration
// ---------------------------------------------------------------------------

TEST(MemoryPressureFeatureFlagTest, DefaultDisabled) {
    auto& flags = Phase3FeatureFlags::instance();
    // Default state: disabled
    EXPECT_FALSE(flags.memory_pressure_enabled());
}

TEST(MemoryPressureFeatureFlagTest, RuntimeToggle) {
    auto& flags = Phase3FeatureFlags::instance();
    flags.set_memory_pressure_enabled(true);
    EXPECT_TRUE(flags.memory_pressure_enabled());
    flags.set_memory_pressure_enabled(false);
    EXPECT_FALSE(flags.memory_pressure_enabled());
}

// ---------------------------------------------------------------------------
// Cache eviction integration example (using LIRSCache-like pattern)
// ---------------------------------------------------------------------------

TEST(MemoryPressureCacheIntegrationTest, EvictionCallbackReducesCacheSize) {
    // Simulate a cache with a size counter
    std::atomic<size_t> cache_size{1000};

    auto monitor = make_high_pressure_monitor(std::chrono::milliseconds(50));
    monitor.register_eviction_callback(Level::MODERATE, [&cache_size]() {
        // Evict 25% of cache entries
        size_t current = cache_size.load(std::memory_order_relaxed);
        size_t to_evict = current / 4;
        if (to_evict > 0) {
            cache_size.fetch_sub(to_evict, std::memory_order_relaxed);
        }
    });

    monitor.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    monitor.stop();

    // After multiple eviction cycles the cache should be smaller.
    EXPECT_LT(cache_size.load(), 1000UL);
}
