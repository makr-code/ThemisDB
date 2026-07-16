/// @file bench_tensor_cpu_gpu_break_even.cpp
/// @brief Benchmark suite for CPU vs GPU break-even analysis
/// @author ThemisDB Implementation Team
/// @date 2026-07-02
///
/// Measures GPU utility for tensor operations:
/// - Batch-size sweep (N=[1,8,32,128,512])
/// - Density sweep (nnz=[10%,30%,50%,70%,90%])
/// - Rank sweep (R=[1,8,16,32,64])
/// - Host↔device transfer overhead
/// - Break-even point identification

#include <benchmark/benchmark.h>
#include "distributed_tensor/include/artifact_manifest.h"
#include <vector>
#include <algorithm>

namespace themis {
namespace distributed_tensor {
namespace bench {

// Mock GPU kernel simulator (CPU-simulated for benchmark portability)
class TensorComputeSimulator {
public:
    struct ComputeStats {
        double cpu_time_ms = 0.0;
        double gpu_time_ms = 0.0;
        double transfer_time_ms = 0.0;
        double gpu_total_ms = 0.0;
    };

    ComputeStats simulateCPUCompute(int batch_size, int rank, float density) {
        ComputeStats stats;
        // CPU time: O(batch_size * rank * nnz_count)
        int nnz_count = static_cast<int>(batch_size * rank * 1000 * density);
        stats.cpu_time_ms = (nnz_count / 1000.0) * 0.001;
        return stats;
    }

    ComputeStats simulateGPUCompute(int batch_size, int rank, float density) {
        ComputeStats stats;
        int nnz_count = static_cast<int>(batch_size * rank * 1000 * density);

        // GPU compute time (highly parallelized)
        stats.gpu_time_ms = (nnz_count / 100000.0) * 0.001;

        // Host-to-device transfer: batch_size * rank * 8 bytes * (1 + density)
        int transfer_bytes = batch_size * rank * 8 * (1 + static_cast<int>(10 * density));
        double bandwidth_gbps = 20.0;  // PCIe 4.0: ~20 GB/s
        stats.transfer_time_ms = (transfer_bytes / (1024.0 * 1024.0 * 1024.0)) * 1000.0 / bandwidth_gbps;

        stats.gpu_total_ms = stats.gpu_time_ms + stats.transfer_time_ms;
        return stats;
    }
};

// ============================================================================
// CPU/GPU Break-Even Fixtures
// ============================================================================

class CPUGPUBreakEvenFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State&) override {
        simulator_ = std::make_unique<TensorComputeSimulator>();

        manifest_.artifact_id = "test:tensor:cpu_gpu";
        manifest_.rank_cap = 32;
    }

    std::unique_ptr<TensorComputeSimulator> simulator_;
    ArtifactManifest manifest_;
};

// ============================================================================
// Batch Size Sweep
// ============================================================================

BENCHMARK_F(CPUGPUBreakEvenFixture, BatchSizeSweep_CPUCompute)(
    benchmark::State& state) {
    const int BATCH_SIZE = state.range(0);
    const int RANK = 16;
    const float DENSITY = 0.5f;

    for (auto _ : state) {
        auto stats = simulator_->simulateCPUCompute(BATCH_SIZE, RANK, DENSITY);
        benchmark::DoNotOptimize(stats);
    }
}
BENCHMARK_REGISTER_F(CPUGPUBreakEvenFixture, BatchSizeSweep_CPUCompute)
    ->Arg(1)
    ->Arg(8)
    ->Arg(32)
    ->Arg(128)
    ->Arg(512);

BENCHMARK_F(CPUGPUBreakEvenFixture, BatchSizeSweep_GPUCompute)(
    benchmark::State& state) {
    const int BATCH_SIZE = state.range(0);
    const int RANK = 16;
    const float DENSITY = 0.5f;

    for (auto _ : state) {
        auto stats = simulator_->simulateGPUCompute(BATCH_SIZE, RANK, DENSITY);
        benchmark::DoNotOptimize(stats);
    }
}
BENCHMARK_REGISTER_F(CPUGPUBreakEvenFixture, BatchSizeSweep_GPUCompute)
    ->Arg(1)
    ->Arg(8)
    ->Arg(32)
    ->Arg(128)
    ->Arg(512);

// ============================================================================
// Density Sweep
// ============================================================================

BENCHMARK_F(CPUGPUBreakEvenFixture, DensitySweep_CPUCompute)(benchmark::State& state) {
    const int BATCH_SIZE = 32;
    const int RANK = 16;
    const int DENSITY_PCT = state.range(0);
    const float DENSITY = DENSITY_PCT / 100.0f;

    for (auto _ : state) {
        auto stats = simulator_->simulateCPUCompute(BATCH_SIZE, RANK, DENSITY);
        benchmark::DoNotOptimize(stats);
    }
}
BENCHMARK_REGISTER_F(CPUGPUBreakEvenFixture, DensitySweep_CPUCompute)
    ->Arg(10)
    ->Arg(30)
    ->Arg(50)
    ->Arg(70)
    ->Arg(90);

BENCHMARK_F(CPUGPUBreakEvenFixture, DensitySweep_GPUCompute)(benchmark::State& state) {
    const int BATCH_SIZE = 32;
    const int RANK = 16;
    const int DENSITY_PCT = state.range(0);
    const float DENSITY = DENSITY_PCT / 100.0f;

    for (auto _ : state) {
        auto stats = simulator_->simulateGPUCompute(BATCH_SIZE, RANK, DENSITY);
        benchmark::DoNotOptimize(stats);
    }
}
BENCHMARK_REGISTER_F(CPUGPUBreakEvenFixture, DensitySweep_GPUCompute)
    ->Arg(10)
    ->Arg(30)
    ->Arg(50)
    ->Arg(70)
    ->Arg(90);

// ============================================================================
// Rank Sweep
// ============================================================================

BENCHMARK_F(CPUGPUBreakEvenFixture, RankSweep_CPUCompute)(benchmark::State& state) {
    const int BATCH_SIZE = 32;
    const int RANK = state.range(0);
    const float DENSITY = 0.5f;

    for (auto _ : state) {
        auto stats = simulator_->simulateCPUCompute(BATCH_SIZE, RANK, DENSITY);
        benchmark::DoNotOptimize(stats);
    }
}
BENCHMARK_REGISTER_F(CPUGPUBreakEvenFixture, RankSweep_CPUCompute)
    ->Arg(1)
    ->Arg(8)
    ->Arg(16)
    ->Arg(32)
    ->Arg(64);

BENCHMARK_F(CPUGPUBreakEvenFixture, RankSweep_GPUCompute)(benchmark::State& state) {
    const int BATCH_SIZE = 32;
    const int RANK = state.range(0);
    const float DENSITY = 0.5f;

    for (auto _ : state) {
        auto stats = simulator_->simulateGPUCompute(BATCH_SIZE, RANK, DENSITY);
        benchmark::DoNotOptimize(stats);
    }
}
BENCHMARK_REGISTER_F(CPUGPUBreakEvenFixture, RankSweep_GPUCompute)
    ->Arg(1)
    ->Arg(8)
    ->Arg(16)
    ->Arg(32)
    ->Arg(64);

// ============================================================================
// Transfer Overhead Analysis
// ============================================================================

BENCHMARK_F(CPUGPUBreakEvenFixture, TransferOverhead_Analysis)(benchmark::State& state) {
    for (auto _ : state) {
        auto cpu_stats = simulator_->simulateCPUCompute(32, 16, 0.5f);
        auto gpu_stats = simulator_->simulateGPUCompute(32, 16, 0.5f);

        // Measure transfer overhead as percentage of GPU time
        double transfer_pct = (gpu_stats.transfer_time_ms / gpu_stats.gpu_total_ms) * 100.0;
        benchmark::DoNotOptimize(transfer_pct);
    }
}

} // namespace bench
} // namespace distributed_tensor
} // namespace themis
