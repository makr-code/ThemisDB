/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_gpu_metrics.cpp                               ║
  Version:         0.0.24                                             ║
  Last Modified:   2026-02-22 08:12:33                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     242                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 00c723d27  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 03329d86d  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31e8b8df0  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 0d722b04c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include <gtest/gtest.h>
#include "themis/gpu/metrics.h"
#include <thread>
#include <vector>

using namespace themis::gpu;

// Helper: reset the singleton before each test.
class GPUMetricsTest : public ::testing::Test {
protected:
    void SetUp()    override { GPUMetrics::GetInstance().reset(); }
    void TearDown() override { GPUMetrics::GetInstance().reset(); }
};

// ---------------------------------------------------------------------------
// Initial state
// ---------------------------------------------------------------------------

TEST_F(GPUMetricsTest, InitialState_SnapshotEmpty) {
    EXPECT_TRUE(GPUMetrics::GetInstance().snapshot().empty());
}

// ---------------------------------------------------------------------------
// recordAllocSuccess
// ---------------------------------------------------------------------------

TEST_F(GPUMetricsTest, AllocSuccess_AppearsInSnapshot) {
    auto& m = GPUMetrics::GetInstance();
    m.recordAllocSuccess(1024, "tenant_a");
    const auto snap = m.snapshot();
    EXPECT_FALSE(snap.empty());
    bool found = false;
    for (const auto& s : snap) {
        if (s.name.find("themis_gpu_alloc_total") != std::string::npos &&
            s.name.find("success") != std::string::npos) {
            EXPECT_EQ(s.value, 1.0);
            EXPECT_EQ(s.type, "counter");
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(GPUMetricsTest, AllocSuccess_MultipleCallsAccumulate) {
    auto& m = GPUMetrics::GetInstance();
    for (int i = 0; i < 5; ++i) {
        m.recordAllocSuccess(512, "t1");
    }
    const auto snap = m.snapshot();
    for (const auto& s : snap) {
        if (s.name.find("themis_gpu_alloc_total") != std::string::npos &&
            s.name.find("success") != std::string::npos &&
            s.name.find("t1") != std::string::npos) {
            EXPECT_EQ(s.value, 5.0);
        }
    }
}

// ---------------------------------------------------------------------------
// recordAllocFail variants
// ---------------------------------------------------------------------------

TEST_F(GPUMetricsTest, AllocFailGlobal_AppearsInSnapshot) {
    auto& m = GPUMetrics::GetInstance();
    m.recordAllocFailGlobal(2048, "t2");
    const auto snap = m.snapshot();
    bool found = false;
    for (const auto& s : snap) {
        if (s.name.find("fail_global_limit") != std::string::npos) {
            found = true;
            EXPECT_EQ(s.value, 1.0);
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(GPUMetricsTest, AllocFailTenant_AppearsWithTenantLabel) {
    auto& m = GPUMetrics::GetInstance();
    m.recordAllocFailTenant(4096, "heavy_tenant");
    const auto snap = m.snapshot();
    bool found = false;
    for (const auto& s : snap) {
        if (s.name.find("fail_tenant_quota") != std::string::npos &&
            s.name.find("heavy_tenant") != std::string::npos) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

// ---------------------------------------------------------------------------
// recordDealloc
// ---------------------------------------------------------------------------

TEST_F(GPUMetricsTest, Dealloc_CounterIncremented) {
    auto& m = GPUMetrics::GetInstance();
    m.recordDealloc(512);
    m.recordDealloc(1024);
    const auto snap = m.snapshot();
    for (const auto& s : snap) {
        if (s.name.find("themis_gpu_dealloc_total") != std::string::npos &&
            s.name.find("bytes") == std::string::npos) {
            EXPECT_EQ(s.value, 2.0);
        }
    }
}

// ---------------------------------------------------------------------------
// recordFallback
// ---------------------------------------------------------------------------

TEST_F(GPUMetricsTest, Fallback_ReasonLabelPresent) {
    auto& m = GPUMetrics::GetInstance();
    m.recordFallback("oom");
    m.recordFallback("circuit_open");
    const auto snap = m.snapshot();
    bool found_oom = false, found_circ = false;
    for (const auto& s : snap) {
        if (s.name.find("fallback_total") != std::string::npos) {
            if (s.name.find("oom") != std::string::npos)          found_oom  = true;
            if (s.name.find("circuit_open") != std::string::npos) found_circ = true;
        }
    }
    EXPECT_TRUE(found_oom);
    EXPECT_TRUE(found_circ);
}

// ---------------------------------------------------------------------------
// recordCircuitOpen
// ---------------------------------------------------------------------------

TEST_F(GPUMetricsTest, CircuitOpen_CounterIncremented) {
    auto& m = GPUMetrics::GetInstance();
    m.recordCircuitOpen();
    m.recordCircuitOpen();
    const auto snap = m.snapshot();
    for (const auto& s : snap) {
        if (s.name.find("circuit_open_total") != std::string::npos) {
            EXPECT_EQ(s.value, 2.0);
        }
    }
}

// ---------------------------------------------------------------------------
// setVRAMAllocated / setVRAMPeak
// ---------------------------------------------------------------------------

TEST_F(GPUMetricsTest, VRAMGauge_Updated) {
    auto& m = GPUMetrics::GetInstance();
    m.setVRAMAllocated(4ULL * 1024 * 1024 * 1024);
    m.setVRAMPeak(8ULL * 1024 * 1024 * 1024);
    const auto snap = m.snapshot();
    bool found_alloc = false, found_peak = false;
    for (const auto& s : snap) {
        if (s.name.find("vram_allocated_bytes") != std::string::npos) {
            EXPECT_EQ(s.type, "gauge");
            found_alloc = true;
        }
        if (s.name.find("vram_peak_bytes") != std::string::npos) {
            EXPECT_EQ(s.type, "gauge");
            found_peak = true;
        }
    }
    EXPECT_TRUE(found_alloc);
    EXPECT_TRUE(found_peak);
}

TEST_F(GPUMetricsTest, VRAMGauge_OverwritesPreviousValue) {
    auto& m = GPUMetrics::GetInstance();
    m.setVRAMAllocated(1000);
    m.setVRAMAllocated(2000);
    const auto snap = m.snapshot();
    for (const auto& s : snap) {
        if (s.name.find("vram_allocated_bytes") != std::string::npos &&
            s.name.find("tenant") == std::string::npos) {
            // Latest value wins.
            EXPECT_EQ(s.value, 2000.0);
        }
    }
}

// ---------------------------------------------------------------------------
// reset
// ---------------------------------------------------------------------------

TEST_F(GPUMetricsTest, Reset_ClearsAllSamples) {
    auto& m = GPUMetrics::GetInstance();
    m.recordAllocSuccess(1024);
    m.recordFallback("oom");
    EXPECT_FALSE(m.snapshot().empty());
    m.reset();
    EXPECT_TRUE(m.snapshot().empty());
}

// ---------------------------------------------------------------------------
// Thread safety
// ---------------------------------------------------------------------------

TEST_F(GPUMetricsTest, Concurrent_Writes_NoDataRace) {
    auto& m = GPUMetrics::GetInstance();
    constexpr int THREADS = 8, OPS = 50;
    std::vector<std::thread> threads;
    threads.reserve(THREADS);
    for (int t = 0; t < THREADS; ++t) {
        threads.emplace_back([&m, t] {
            for (int i = 0; i < OPS; ++i) {
                m.recordAllocSuccess(static_cast<uint64_t>(i),
                                     "t" + std::to_string(t));
                m.recordFallback("oom");
            }
        });
    }
    for (auto& th : threads) th.join();
    // Just verify it didn't crash and snapshot is non-empty.
    EXPECT_FALSE(m.snapshot().empty());
}
