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
// recordKernelDuration
// ---------------------------------------------------------------------------

TEST_F(GPUMetricsTest, KernelDuration_AppearsInSnapshot) {
    auto& m = GPUMetrics::GetInstance();
    GPUMetrics::KernelRecord rec;
    rec.name        = "vector_distance_kernel";
    rec.duration_ns = 12345.0;
    rec.device_id   = 0;
    m.recordKernelDuration(rec);

    const auto snap = m.snapshot();
    bool found = false;
    for (const auto& s : snap) {
        if (s.name.find("themis_gpu_kernel_duration_ns") != std::string::npos &&
            s.name.find("vector_distance_kernel") != std::string::npos) {
            EXPECT_DOUBLE_EQ(s.value, 12345.0);
            EXPECT_EQ(s.type, "gauge");
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(GPUMetricsTest, KernelDuration_MultipleKernels) {
    auto& m = GPUMetrics::GetInstance();
    for (int i = 0; i < 3; ++i) {
        GPUMetrics::KernelRecord rec;
        rec.name        = "kernel_" + std::to_string(i);
        rec.duration_ns = static_cast<double>(i + 1) * 1000.0;
        rec.device_id   = 0;
        m.recordKernelDuration(rec);
    }
    EXPECT_FALSE(m.snapshot().empty());
}

// ---------------------------------------------------------------------------
// nsight_export
// ---------------------------------------------------------------------------

TEST_F(GPUMetricsTest, NsightExport_EmptyWhenNoKernels) {
    const std::string json = GPUMetrics::GetInstance().nsight_export();
    EXPECT_FALSE(json.empty());
    // Must contain the version marker and an empty Kernels array.
    EXPECT_NE(json.find("NsightComputeVersion"), std::string::npos);
    EXPECT_NE(json.find("\"Kernels\": []"), std::string::npos);
}

TEST_F(GPUMetricsTest, NsightExport_ContainsKernelEntry) {
    auto& m = GPUMetrics::GetInstance();
    GPUMetrics::KernelRecord rec;
    rec.name        = "hnsw_distance_kernel";
    rec.duration_ns = 56789.0;
    rec.device_id   = 1;
    rec.grid_x      = 128;
    rec.block_x     = 256;
    m.recordKernelDuration(rec);

    const std::string json = m.nsight_export();
    EXPECT_NE(json.find("hnsw_distance_kernel"),   std::string::npos);
    EXPECT_NE(json.find("56789"),                   std::string::npos);
    EXPECT_NE(json.find("NsightComputeVersion"),    std::string::npos);
    EXPECT_NE(json.find("Duration (ns)"),           std::string::npos);
    EXPECT_NE(json.find("Grid Size"),               std::string::npos);
    EXPECT_NE(json.find("Block Size"),              std::string::npos);
}

TEST_F(GPUMetricsTest, NsightExport_MultipleKernels_ValidJson) {
    auto& m = GPUMetrics::GetInstance();
    const std::vector<std::string> names = {
        "embedding_kernel", "attention_kernel", "pooling_kernel"};
    for (std::size_t i = 0; i < names.size(); ++i) {
        GPUMetrics::KernelRecord rec;
        rec.name        = names[i];
        rec.duration_ns = static_cast<double>((i + 1) * 10000);
        rec.device_id   = 0;
        m.recordKernelDuration(rec);
    }
    const std::string json = m.nsight_export();
    for (const auto& n : names) {
        EXPECT_NE(json.find(n), std::string::npos) << "Missing kernel: " << n;
    }
}

TEST_F(GPUMetricsTest, NsightExport_Reset_ClearsKernels) {
    auto& m = GPUMetrics::GetInstance();
    GPUMetrics::KernelRecord rec;
    rec.name        = "temp_kernel";
    rec.duration_ns = 100.0;
    m.recordKernelDuration(rec);

    m.reset();
    const std::string json = m.nsight_export();
    EXPECT_EQ(json.find("temp_kernel"), std::string::npos);
    EXPECT_NE(json.find("\"Kernels\": []"), std::string::npos);
}

// ---------------------------------------------------------------------------
// Thermal and power telemetry
// ---------------------------------------------------------------------------

TEST_F(GPUMetricsTest, Temperature_GaugeUpdated) {
    auto& m = GPUMetrics::GetInstance();
    m.setTemperature(0, 72.5);
    const auto snap = m.snapshot();
    bool found = false;
    for (const auto& s : snap) {
        if (s.name.find("themis_gpu_temperature_celsius") != std::string::npos &&
            s.name.find("\"0\"") != std::string::npos) {
            EXPECT_DOUBLE_EQ(s.value, 72.5);
            EXPECT_EQ(s.type, "gauge");
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(GPUMetricsTest, Temperature_MultipleDevices) {
    auto& m = GPUMetrics::GetInstance();
    m.setTemperature(0, 68.0);
    m.setTemperature(1, 75.0);
    const auto snap = m.snapshot();
    bool found0 = false, found1 = false;
    for (const auto& s : snap) {
        if (s.name.find("themis_gpu_temperature_celsius") != std::string::npos) {
            if (s.name.find("\"0\"") != std::string::npos) { EXPECT_DOUBLE_EQ(s.value, 68.0); found0 = true; }
            if (s.name.find("\"1\"") != std::string::npos) { EXPECT_DOUBLE_EQ(s.value, 75.0); found1 = true; }
        }
    }
    EXPECT_TRUE(found0);
    EXPECT_TRUE(found1);
}

TEST_F(GPUMetricsTest, Temperature_OverwritesPreviousValue) {
    auto& m = GPUMetrics::GetInstance();
    m.setTemperature(0, 60.0);
    m.setTemperature(0, 85.0);
    const auto snap = m.snapshot();
    for (const auto& s : snap) {
        if (s.name.find("themis_gpu_temperature_celsius") != std::string::npos &&
            s.name.find("\"0\"") != std::string::npos) {
            EXPECT_DOUBLE_EQ(s.value, 85.0);
        }
    }
}

TEST_F(GPUMetricsTest, PowerDraw_GaugeUpdated) {
    auto& m = GPUMetrics::GetInstance();
    m.setPowerDraw(0, 250.0);
    const auto snap = m.snapshot();
    bool found = false;
    for (const auto& s : snap) {
        if (s.name.find("themis_gpu_power_draw_watts") != std::string::npos &&
            s.name.find("\"0\"") != std::string::npos) {
            EXPECT_DOUBLE_EQ(s.value, 250.0);
            EXPECT_EQ(s.type, "gauge");
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(GPUMetricsTest, PowerDraw_MultipleDevices) {
    auto& m = GPUMetrics::GetInstance();
    m.setPowerDraw(0, 200.0);
    m.setPowerDraw(1, 350.0);
    const auto snap = m.snapshot();
    bool found0 = false, found1 = false;
    for (const auto& s : snap) {
        if (s.name.find("themis_gpu_power_draw_watts") != std::string::npos) {
            if (s.name.find("\"0\"") != std::string::npos) { EXPECT_DOUBLE_EQ(s.value, 200.0); found0 = true; }
            if (s.name.find("\"1\"") != std::string::npos) { EXPECT_DOUBLE_EQ(s.value, 350.0); found1 = true; }
        }
    }
    EXPECT_TRUE(found0);
    EXPECT_TRUE(found1);
}

TEST_F(GPUMetricsTest, PowerLimit_GaugeUpdated) {
    auto& m = GPUMetrics::GetInstance();
    m.setPowerLimit(0, 400.0);
    const auto snap = m.snapshot();
    bool found = false;
    for (const auto& s : snap) {
        if (s.name.find("themis_gpu_power_limit_watts") != std::string::npos &&
            s.name.find("\"0\"") != std::string::npos) {
            EXPECT_DOUBLE_EQ(s.value, 400.0);
            EXPECT_EQ(s.type, "gauge");
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(GPUMetricsTest, ThermalPower_ResetClearsAll) {
    auto& m = GPUMetrics::GetInstance();
    m.setTemperature(0, 80.0);
    m.setPowerDraw(0, 300.0);
    m.setPowerLimit(0, 400.0);
    m.reset();
    const auto snap = m.snapshot();
    for (const auto& s : snap) {
        EXPECT_EQ(s.name.find("themis_gpu_temperature_celsius"), std::string::npos);
        EXPECT_EQ(s.name.find("themis_gpu_power_draw_watts"),    std::string::npos);
        EXPECT_EQ(s.name.find("themis_gpu_power_limit_watts"),   std::string::npos);
    }
}

TEST_F(GPUMetricsTest, ThermalPower_ConcurrentWrites_NoDataRace) {
    auto& m = GPUMetrics::GetInstance();
    constexpr int THREADS = 4, OPS = 50;
    std::vector<std::thread> threads;
    threads.reserve(THREADS);
    for (int t = 0; t < THREADS; ++t) {
        threads.emplace_back([&m, t] {
            for (int i = 0; i < OPS; ++i) {
                m.setTemperature(t, static_cast<double>(60 + i));
                m.setPowerDraw(t, static_cast<double>(100 + i));
                m.setPowerLimit(t, 400.0);
            }
        });
    }
    for (auto& th : threads) th.join();
    EXPECT_FALSE(m.snapshot().empty());
}

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
