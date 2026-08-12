/*
 * Unit tests for the GPU profiler (profiler.h / profiler.cpp).
 *
 * All tests run on CI without GPU hardware.  The profiler stores events
 * internally on CPU-only builds, so the recorded state and the Chrome trace
 * JSON export are always exercised.
 */

#include <gtest/gtest.h>
#include "themis/gpu/profiler.h"
#include "themis/gpu/metrics.h"
#include <string>
#include <thread>
#include <vector>

using namespace themis::gpu;

// Helper: reset the singleton before/after each test.
class GPUProfilerTest : public ::testing::Test {
protected:
    void SetUp()    override { GPUProfiler::GetInstance().reset(); }
    void TearDown() override { GPUProfiler::GetInstance().reset(); }
};

// ---------------------------------------------------------------------------
// Initial state
// ---------------------------------------------------------------------------

TEST_F(GPUProfilerTest, InitialState_NoRanges) {
    EXPECT_TRUE(GPUProfiler::GetInstance().getRanges().empty());
}

TEST_F(GPUProfilerTest, InitialState_ExportEmpty) {
    const std::string json = GPUProfiler::GetInstance().rocm_profiler_export();
    EXPECT_FALSE(json.empty());
    EXPECT_NE(json.find("traceEvents"), std::string::npos);
}

// ---------------------------------------------------------------------------
// beginRange / endRange
// ---------------------------------------------------------------------------

TEST_F(GPUProfilerTest, BeginEndRange_RecordsOneRange) {
    auto& p = GPUProfiler::GetInstance();
    p.beginRange("kernel_launch");
    p.endRange();

    const auto ranges = p.getRanges();
    ASSERT_EQ(ranges.size(), 1u);
    EXPECT_EQ(ranges[0].name, "kernel_launch");
    EXPECT_GT(ranges[0].end_ns, 0u);
    EXPECT_LE(ranges[0].start_ns, ranges[0].end_ns);
}

TEST_F(GPUProfilerTest, BeginEndRange_MultipleRanges) {
    auto& p = GPUProfiler::GetInstance();
    p.beginRange("range_a");
    p.endRange();
    p.beginRange("range_b");
    p.endRange();

    const auto ranges = p.getRanges();
    ASSERT_EQ(ranges.size(), 2u);
    EXPECT_EQ(ranges[0].name, "range_a");
    EXPECT_EQ(ranges[1].name, "range_b");
}

TEST_F(GPUProfilerTest, EndRange_WithoutBegin_IsIgnored) {
    auto& p = GPUProfiler::GetInstance();
    // Must not crash or corrupt state.
    p.endRange();
    EXPECT_TRUE(p.getRanges().empty());
}

TEST_F(GPUProfilerTest, NestedRanges_OrderedCorrectly) {
    auto& p = GPUProfiler::GetInstance();
    p.beginRange("outer");
    p.beginRange("inner");
    p.endRange();  // closes "inner"
    p.endRange();  // closes "outer"

    const auto ranges = p.getRanges();
    ASSERT_EQ(ranges.size(), 2u);
    // Inner closes first.
    EXPECT_EQ(ranges[0].name, "inner");
    EXPECT_EQ(ranges[1].name, "outer");
    // Outer must span at least as long as inner.
    EXPECT_GE(ranges[1].end_ns - ranges[1].start_ns,
              ranges[0].end_ns - ranges[0].start_ns);
}

TEST_F(GPUProfilerTest, BeginRange_CustomColor_Stored) {
    auto& p = GPUProfiler::GetInstance();
    p.beginRange("colored", 0xFFFF0000u);  // red
    p.endRange();

    const auto ranges = p.getRanges();
    ASSERT_EQ(ranges.size(), 1u);
    EXPECT_EQ(ranges[0].color, 0xFFFF0000u);
}

// ---------------------------------------------------------------------------
// markEvent
// ---------------------------------------------------------------------------

TEST_F(GPUProfilerTest, MarkEvent_RecordsInstantRange) {
    auto& p = GPUProfiler::GetInstance();
    p.markEvent("alloc_done");

    const auto ranges = p.getRanges();
    ASSERT_EQ(ranges.size(), 1u);
    EXPECT_EQ(ranges[0].name, "alloc_done");
    // Point event: start == end.
    EXPECT_EQ(ranges[0].start_ns, ranges[0].end_ns);
}

TEST_F(GPUProfilerTest, MarkEvent_MixedWithRanges) {
    auto& p = GPUProfiler::GetInstance();
    p.beginRange("work");
    p.markEvent("checkpoint");
    p.endRange();

    const auto ranges = p.getRanges();
    ASSERT_EQ(ranges.size(), 2u);
    // checkpoint closes first (markEvent is a push+pop in one call)
    EXPECT_EQ(ranges[0].name, "checkpoint");
    EXPECT_EQ(ranges[1].name, "work");
}

// ---------------------------------------------------------------------------
// rocm_profiler_export
// ---------------------------------------------------------------------------

TEST_F(GPUProfilerTest, RocmExport_EmptyWhenNoRanges) {
    const std::string json = GPUProfiler::GetInstance().rocm_profiler_export();
    EXPECT_FALSE(json.empty());
    EXPECT_NE(json.find("traceEvents"), std::string::npos);
    // traceEvents array should be empty (no comma after opening bracket)
    EXPECT_NE(json.find("[]"), std::string::npos);
}

TEST_F(GPUProfilerTest, RocmExport_ContainsDurationRange) {
    auto& p = GPUProfiler::GetInstance();
    p.beginRange("vector_search");
    p.endRange();

    const std::string json = p.rocm_profiler_export();
    EXPECT_NE(json.find("vector_search"), std::string::npos);
    EXPECT_NE(json.find("\"ph\": \"X\""),  std::string::npos);
    EXPECT_NE(json.find("\"ts\":"),         std::string::npos);
    EXPECT_NE(json.find("\"dur\":"),        std::string::npos);
}

TEST_F(GPUProfilerTest, RocmExport_ContainsInstantEvent) {
    auto& p = GPUProfiler::GetInstance();
    p.markEvent("query_start");

    const std::string json = p.rocm_profiler_export();
    EXPECT_NE(json.find("query_start"), std::string::npos);
    EXPECT_NE(json.find("\"ph\": \"i\""), std::string::npos);
}

TEST_F(GPUProfilerTest, RocmExport_MultipleRanges) {
    auto& p = GPUProfiler::GetInstance();
    p.beginRange("range_1");
    p.endRange();
    p.beginRange("range_2");
    p.endRange();
    p.markEvent("event_3");

    const std::string json = p.rocm_profiler_export();
    EXPECT_NE(json.find("range_1"), std::string::npos);
    EXPECT_NE(json.find("range_2"), std::string::npos);
    EXPECT_NE(json.find("event_3"), std::string::npos);
}

TEST_F(GPUProfilerTest, RocmExport_Reset_ClearsRanges) {
    auto& p = GPUProfiler::GetInstance();
    p.beginRange("temp");
    p.endRange();
    p.reset();

    const std::string json = p.rocm_profiler_export();
    EXPECT_EQ(json.find("temp"), std::string::npos);
    EXPECT_NE(json.find("[]"), std::string::npos);
}

// ---------------------------------------------------------------------------
// ScopedGPURange
// ---------------------------------------------------------------------------

TEST_F(GPUProfilerTest, ScopedRange_RecordsOnDestruction) {
    {
        ScopedGPURange scoped("scoped_op");
        // range not yet completed inside the block
    }
    const auto ranges = GPUProfiler::GetInstance().getRanges();
    ASSERT_EQ(ranges.size(), 1u);
    EXPECT_EQ(ranges[0].name, "scoped_op");
    EXPECT_LT(ranges[0].start_ns, ranges[0].end_ns);
}

TEST_F(GPUProfilerTest, ScopedRange_NestedScopes) {
    {
        ScopedGPURange outer("outer_op");
        {
            ScopedGPURange inner("inner_op");
        }
        // inner completed; outer still open
    }
    const auto ranges = GPUProfiler::GetInstance().getRanges();
    ASSERT_EQ(ranges.size(), 2u);
    EXPECT_EQ(ranges[0].name, "inner_op");
    EXPECT_EQ(ranges[1].name, "outer_op");
}

// ---------------------------------------------------------------------------
// GPUMetrics::rocm_profiler_export
// ---------------------------------------------------------------------------

class GPUMetricsRocmExportTest : public ::testing::Test {
protected:
    void SetUp()    override { GPUMetrics::GetInstance().reset(); }
    void TearDown() override { GPUMetrics::GetInstance().reset(); }
};

TEST_F(GPUMetricsRocmExportTest, EmptyExport_ContainsTraceEvents) {
    const std::string json = GPUMetrics::GetInstance().rocm_profiler_export();
    EXPECT_FALSE(json.empty());
    EXPECT_NE(json.find("traceEvents"), std::string::npos);
}

TEST_F(GPUMetricsRocmExportTest, KernelRecord_AppearsInExport) {
    auto& m = GPUMetrics::GetInstance();
    GPUMetrics::KernelRecord rec;
    rec.name        = "hnsw_kernel";
    rec.duration_ns = 50000.0;
    rec.device_id   = 0;
    rec.grid_x      = 64;
    rec.block_x     = 128;
    m.recordKernelDuration(rec);

    const std::string json = m.rocm_profiler_export();
    EXPECT_NE(json.find("hnsw_kernel"), std::string::npos);
    EXPECT_NE(json.find("\"ph\": \"X\""),  std::string::npos);
    EXPECT_NE(json.find("\"args\""),       std::string::npos);
    EXPECT_NE(json.find("\"grid\""),       std::string::npos);
    EXPECT_NE(json.find("\"block\""),      std::string::npos);
}

TEST_F(GPUMetricsRocmExportTest, MultipleKernels_AllPresent) {
    auto& m = GPUMetrics::GetInstance();
    const std::vector<std::string> names = {"kernel_a", "kernel_b", "kernel_c"};
    for (const auto& n : names) {
        GPUMetrics::KernelRecord rec;
        rec.name        = n;
        rec.duration_ns = 1000.0;
        m.recordKernelDuration(rec);
    }
    const std::string json = m.rocm_profiler_export();
    for (const auto& n : names) {
        EXPECT_NE(json.find(n), std::string::npos) << "Missing: " << n;
    }
}

TEST_F(GPUMetricsRocmExportTest, Reset_ClearsKernelExport) {
    auto& m = GPUMetrics::GetInstance();
    GPUMetrics::KernelRecord rec;
    rec.name        = "temp_kernel";
    rec.duration_ns = 100.0;
    m.recordKernelDuration(rec);

    m.reset();
    const std::string json = m.rocm_profiler_export();
    EXPECT_EQ(json.find("temp_kernel"), std::string::npos);
}

// ---------------------------------------------------------------------------
// Thread safety
// ---------------------------------------------------------------------------

TEST_F(GPUProfilerTest, Concurrent_Writes_NoDataRace) {
    auto& p = GPUProfiler::GetInstance();
    constexpr int THREADS = 4, OPS = 20;
    std::vector<std::thread> threads;
    threads.reserve(THREADS);
    for (int t = 0; t < THREADS; ++t) {
        threads.emplace_back([&p] {
            for (int i = 0; i < OPS; ++i) {
                p.beginRange("concurrent_range");
                p.markEvent("concurrent_event");
                p.endRange();
            }
        });
    }
    for (auto& th : threads) th.join();
    // Just verify no crash and state is consistent.
    EXPECT_FALSE(p.rocm_profiler_export().empty());
}
