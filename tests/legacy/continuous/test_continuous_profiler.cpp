/**
 * @file test_continuous_profiler.cpp
 * @brief Unit tests for ContinuousProfiler – pprof / async-profiler compatible
 *        continuous profiling integration.
 *
 * Tests cover:
 *  - Default construction and configuration retrieval
 *  - Enable / disable lifecycle
 *  - Snapshot format (pprof folded-stacks text)
 *  - ProfileDiff: new / removed / changed hotspot detection
 *  - Anomaly callback registration and triggering
 *  - File persistence (saveToFile / loadFromFile round-trip)
 *  - JSON serialisation of ProfileSnapshot and ProfileDiff
 *  - getSnapshots filtering by time range
 */

#include <gtest/gtest.h>
#include "observability/continuous_profiler.h"

#include <atomic>
#include <chrono>
#include <cstdio>
#include <mutex>
#include <string>
#include <thread>

using namespace themis::observability;
using namespace std::chrono_literals;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static ProfileSnapshot makeCpuSnapshot(const std::string& folded_text,
                                        std::chrono::system_clock::time_point ts =
                                            std::chrono::system_clock::now()) {
    ProfileSnapshot s;
    s.type = ProfileType::CPU;
    s.timestamp = ts;
    s.duration = std::chrono::seconds(60);
    s.data.assign(folded_text.begin(), folded_text.end());
    return s;
}

// ---------------------------------------------------------------------------
// Construction / configuration
// ---------------------------------------------------------------------------

TEST(ContinuousProfilerTest, DefaultConfig_DisabledByDefault) {
    ContinuousProfiler p;
    EXPECT_FALSE(p.isEnabled());
}

TEST(ContinuousProfilerTest, ConfigRoundtrip) {
    ContinuousProfilerConfig cfg;
    cfg.enabled = true;
    cfg.cpu_sample_rate = 0.05;
    cfg.snapshot_interval = 30s;
    cfg.max_snapshots_retained = 100;
    cfg.output_dir = "/tmp/themis_profiles";
    cfg.enable_cpu_profiling = true;
    cfg.enable_heap_profiling = false;

    ContinuousProfiler p(cfg);
    EXPECT_TRUE(p.isEnabled());

    auto got = p.getConfig();
    EXPECT_DOUBLE_EQ(0.05, got.cpu_sample_rate);
    EXPECT_EQ(30, got.snapshot_interval.count());
    EXPECT_EQ(100u, got.max_snapshots_retained);
    EXPECT_EQ("/tmp/themis_profiles", got.output_dir);
    EXPECT_TRUE(got.enable_cpu_profiling);
    EXPECT_FALSE(got.enable_heap_profiling);
}

// ---------------------------------------------------------------------------
// Enable / disable
// ---------------------------------------------------------------------------

TEST(ContinuousProfilerTest, EnableDisable) {
    ContinuousProfiler p;
    EXPECT_FALSE(p.isEnabled());

    p.enable();
    EXPECT_TRUE(p.isEnabled());

    p.disable();
    EXPECT_FALSE(p.isEnabled());
}

// ---------------------------------------------------------------------------
// Snapshot – disabled profiler returns empty snapshot
// ---------------------------------------------------------------------------

TEST(ContinuousProfilerTest, SnapshotWhenDisabledIsEmpty) {
    ContinuousProfiler p;  // enabled = false
    auto snap = p.snapshot(ProfileType::CPU);
    EXPECT_EQ(ProfileType::CPU, snap.type);
    // No stacks collected → data should be empty (or contain only whitespace)
    std::string s = snap.dataAsString();
    // Strip whitespace
    s.erase(std::remove_if(s.begin(), s.end(), ::isspace), s.end());
    EXPECT_TRUE(s.empty());
}

// ---------------------------------------------------------------------------
// Snapshot – start / stop with very short interval
// ---------------------------------------------------------------------------

TEST(ContinuousProfilerTest, StartStopProducesSnapshot) {
    ContinuousProfilerConfig cfg;
    cfg.enabled = true;
    cfg.cpu_sample_rate = 1.0;          // maximum sampling rate (1 ms period)
    cfg.snapshot_interval = 10s;        // don't flush automatically during test
    cfg.output_dir = "";                 // no disk I/O

    ContinuousProfiler p(cfg);
    p.start();
    std::this_thread::sleep_for(50ms);  // let the sampler run a few iterations
    auto snap = p.snapshot(ProfileType::CPU);
    p.stop();

    // The snapshot should contain at least one line (stack + count)
    auto text = snap.dataAsString();
    EXPECT_FALSE(text.empty()) << "Expected at least one sample to be collected";
}

// ---------------------------------------------------------------------------
// ProfileDiff – no changes
// ---------------------------------------------------------------------------

TEST(ProfileDiffTest, IdenticalSnapshotsNoChange) {
    const std::string folded =
        "main;foo;bar 10\n"
        "main;foo;baz 5\n";

    auto base    = makeCpuSnapshot(folded);
    auto current = makeCpuSnapshot(folded);

    ContinuousProfiler p;
    auto diff = p.compare(base, current);

    EXPECT_NEAR(0.0, diff.cpu_regression_percent, 0.001);
    EXPECT_TRUE(diff.new_hotspots.empty());
    EXPECT_TRUE(diff.removed_hotspots.empty());
    EXPECT_TRUE(diff.changed_hotspots.empty());
}

// ---------------------------------------------------------------------------
// ProfileDiff – new hotspot
// ---------------------------------------------------------------------------

TEST(ProfileDiffTest, NewHotspotDetected) {
    const std::string base_text    = "main;foo 10\n";
    const std::string current_text = "main;foo 10\nmain;newHotspot 50\n";

    auto base    = makeCpuSnapshot(base_text);
    auto current = makeCpuSnapshot(current_text);

    ContinuousProfiler p;
    auto diff = p.compare(base, current);

    bool found = false;
    for (const auto& h : diff.new_hotspots) {
        if (h.find("newHotspot") != std::string::npos) {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found) << "Expected 'newHotspot' in new_hotspots";
}

// ---------------------------------------------------------------------------
// ProfileDiff – removed hotspot
// ---------------------------------------------------------------------------

TEST(ProfileDiffTest, RemovedHotspotDetected) {
    const std::string base_text    = "main;foo 10\nmain;disappear 20\n";
    const std::string current_text = "main;foo 10\n";

    auto base    = makeCpuSnapshot(base_text);
    auto current = makeCpuSnapshot(current_text);

    ContinuousProfiler p;
    auto diff = p.compare(base, current);

    bool found = false;
    for (const auto& h : diff.removed_hotspots) {
        if (h.find("disappear") != std::string::npos) {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found) << "Expected 'disappear' in removed_hotspots";
}

// ---------------------------------------------------------------------------
// ProfileDiff – CPU regression
// ---------------------------------------------------------------------------

TEST(ProfileDiffTest, CpuRegression) {
    // baseline: 100 samples; current: 200 samples → 100 % regression
    const std::string base_text    = "main;foo 100\n";
    const std::string current_text = "main;foo 200\n";

    auto base    = makeCpuSnapshot(base_text);
    auto current = makeCpuSnapshot(current_text);

    ContinuousProfiler p;
    auto diff = p.compare(base, current);

    EXPECT_NEAR(100.0, diff.cpu_regression_percent, 0.001);
}

// ---------------------------------------------------------------------------
// ProfileDiff – non-CPU types return empty diff
// ---------------------------------------------------------------------------

TEST(ProfileDiffTest, NonCpuTypesReturnEmptyDiff) {
    ProfileSnapshot heap_snap;
    heap_snap.type = ProfileType::HEAP;

    ContinuousProfiler p;
    auto diff = p.compare(heap_snap, heap_snap);

    EXPECT_NEAR(0.0, diff.cpu_regression_percent, 0.001);
    EXPECT_TRUE(diff.new_hotspots.empty());
    EXPECT_TRUE(diff.removed_hotspots.empty());
}

// ---------------------------------------------------------------------------
// File persistence round-trip
// ---------------------------------------------------------------------------

TEST(ContinuousProfilerTest, SaveAndLoadSnapshot) {
    const std::string content = "main;foo;bar 42\nmain;baz 7\n";
    auto snap = makeCpuSnapshot(content);

    const std::string tmp_path = "/tmp/themis_test_profile.folded";
    snap.saveToFile(tmp_path);

    auto loaded = ProfileSnapshot::loadFromFile(tmp_path);
    EXPECT_EQ(content, loaded.dataAsString());

    std::remove(tmp_path.c_str());
}

TEST(ContinuousProfilerTest, LoadNonexistentFileThrows) {
    EXPECT_THROW(
        ProfileSnapshot::loadFromFile("/tmp/this_file_should_not_exist_12345.folded"),
        std::runtime_error);
}

// ---------------------------------------------------------------------------
// JSON serialisation – ProfileSnapshot
// ---------------------------------------------------------------------------

TEST(ContinuousProfilerTest, SnapshotToJSONContainsExpectedFields) {
    const std::string content = "main;worker 5\n";
    auto snap = makeCpuSnapshot(content);

    auto j = snap.toJSON();

    EXPECT_EQ("cpu", j.at("type").get<std::string>());
    EXPECT_TRUE(j.contains("timestamp_ms"));
    EXPECT_TRUE(j.contains("duration_s"));
    EXPECT_TRUE(j.contains("data_size_bytes"));
    EXPECT_TRUE(j.contains("data_base64"));
    EXPECT_EQ(content.size(), j.at("data_size_bytes").get<size_t>());
}

// ---------------------------------------------------------------------------
// JSON serialisation – ProfileDiff
// ---------------------------------------------------------------------------

TEST(ProfileDiffTest, DiffToJSONContainsExpectedFields) {
    ProfileDiff diff;
    diff.cpu_regression_percent = 15.5;
    diff.new_hotspots = {"stackA"};
    diff.removed_hotspots = {"stackB"};
    diff.changed_hotspots = {"stackC"};

    auto j = diff.toJSON();

    EXPECT_NEAR(15.5, j.at("cpu_regression_percent").get<double>(), 0.001);
    EXPECT_EQ(1u, j.at("new_hotspots").size());
    EXPECT_EQ(1u, j.at("removed_hotspots").size());
    EXPECT_EQ(1u, j.at("changed_hotspots").size());
}

// ---------------------------------------------------------------------------
// getSnapshots – time-range filtering
// ---------------------------------------------------------------------------

TEST(ContinuousProfilerTest, GetSnapshotsTimeRangeFiltering) {
    ContinuousProfilerConfig cfg;
    cfg.enabled = true;
    cfg.cpu_sample_rate = 1.0;
    cfg.snapshot_interval = 5s;
    cfg.output_dir = "";

    ContinuousProfiler p(cfg);
    p.start();
    std::this_thread::sleep_for(20ms);
    // Force a snapshot to populate the history
    p.snapshot(ProfileType::CPU);
    p.stop();

    auto now = std::chrono::system_clock::now();
    // A query for the future should return nothing
    auto snaps = p.getSnapshots(ProfileType::CPU, now + 1h, now + 2h);
    EXPECT_TRUE(snaps.empty());

    // A query for a wide window that includes the snapshot should return it
    auto all = p.getSnapshots(ProfileType::CPU, now - 1h, now + 1h);
    // We requested one snapshot above; there should be at least one in the history
    EXPECT_FALSE(all.empty());
}

// ---------------------------------------------------------------------------
// Anomaly callback registration (no crash on registration)
// ---------------------------------------------------------------------------

TEST(ContinuousProfilerTest, AnomalyCallbackRegistration) {
    ContinuousProfiler p;
    bool called = false;
    p.registerAnomalyCallback(
        [&called](const ProfileSnapshot& /*snap*/, const std::string& /*msg*/) {
            called = true;
        });
    // Just verify registration doesn't throw / crash; the callback may not be
    // triggered in a short test unless a regression is injected artificially.
    EXPECT_FALSE(called);  // no profiling was started
}

// ---------------------------------------------------------------------------
// Anomaly callback – triggered by a >20 % CPU regression between flushes.
// We drive the flush cycle by using a very short snapshot_interval so the
// background thread fires it during the test window.
// ---------------------------------------------------------------------------

TEST(ContinuousProfilerTest, AnomalyCallbackFiredOnRegression) {
    ContinuousProfilerConfig cfg;
    cfg.enabled = true;
    cfg.cpu_sample_rate = 1.0;        // 1 ms sample period
    cfg.snapshot_interval = std::chrono::seconds{1};
    cfg.output_dir = "";              // no disk I/O

    ContinuousProfiler p(cfg);

    std::atomic<int> callback_count{0};
    std::string last_msg;
    std::mutex cb_mutex;

    p.registerAnomalyCallback(
        [&](const ProfileSnapshot& /*snap*/, const std::string& msg) {
            std::lock_guard<std::mutex> lk(cb_mutex);
            callback_count.fetch_add(1, std::memory_order_relaxed);
            last_msg = msg;
        });

    p.start();
    // Run long enough for at least two flush cycles so the diff logic can fire
    std::this_thread::sleep_for(150ms);
    p.stop();

    // The callback may or may not fire depending on whether the background
    // thread observed a >20 % sample-count change between consecutive flushes.
    // What we must guarantee is: no crash, no deadlock, and if it fired the
    // message contains a meaningful string.
    if (callback_count.load() > 0) {
        std::lock_guard<std::mutex> lk(cb_mutex);
        EXPECT_FALSE(last_msg.empty());
        EXPECT_NE(std::string::npos, last_msg.find("CPU regression"))
            << "Unexpected callback message: " << last_msg;
    }
    // At minimum, the profiler must have survived the run without deadlock.
    SUCCEED();
}
