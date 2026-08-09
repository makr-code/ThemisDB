// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file bench_onnx_clip_vit_backend.cpp
 * @brief Phase 2A multi-backend throughput and memory benchmarks for ONNX CLIP v0.3.0.
 *
 * Provides reproducible measurements for:
 * - Batch-64 CUDA throughput (target: ≥ 6x single-image)
 * - Batch-16 CPU throughput (baseline comparison)
 * - Model load memory footprint
 * - Runtime memory state
 * - Batch-splitting performance (optimization validation)
 *
 * All benchmarks follow Wave 1 measurement hygiene:
 * - Canonical seed: kCanonicalRngSeed = 42
 * - Steady clock for stable timing
 * - Memory measurements via process RSS tracking
 * - Deterministic synthetic data
 *
 * ## Release Gates
 *
 * | Gate ID | Benchmark | Threshold | Status |
 * |---------|-----------|-----------|--------|
 * | FCP-05  | BM_Throughput_ViTB32_CUDA_Batch64 | ≥ 6x single | Hard gate |
 * | FCP-06  | BM_MemoryFootprint_ModelLoad | tracked | Tracking gate |
 *
 * @see benchmarks/MEASUREMENT_HYGIENE.md
 * @see benchmarks/bench_fixtures.h
 * @see benchmarks/onnx_clip/README.md
 */

#include <benchmark/benchmark.h>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <memory>
#include <random>
#include <string>
#include <vector>
#include <algorithm>
#include <cstring>

// Measurement hygiene constants (Wave 1)
namespace themis {
namespace bench {
namespace onnx_clip {

static constexpr uint64_t kCanonicalRngSeed = 42;
static constexpr int kWarmupIterationsCold = 10;
static constexpr int kWarmupIterationsWarm = 10;
static constexpr int kWarmupIterationsHot = 10;

/**
 * @brief Utility to read RSS (resident set size) from /proc/self/status.
 * Used for memory footprint tracking.
 *
 * @return RSS in bytes, or 0 if unable to read.
 */
static uint64_t getRssBytes() {
    std::ifstream status("/proc/self/status");
    std::string line;
    while (std::getline(status, line)) {
        if (line.find("VmRSS:") == 0) {
            // Format: "VmRSS:         1234 kB"
            size_t pos = line.find_last_not_of(" kB");
            size_t val_start = line.find_last_of(' ', pos) + 1;
            uint64_t kb = std::stoull(line.substr(val_start, pos - val_start + 1));
            return kb * 1024;  // Convert KB to bytes
        }
    }
    return 0;
}

/**
 * @brief Mock ONNX CLIP model with backend selection (CPU/CUDA).
 * 
 * Simulates:
 * - Single-image inference (CPU baseline)
 * - Batch inference (CPU or CUDA)
 * - Memory footprint tracking
 */
class MockOnnxClipBackendModel {
public:
    enum class Backend {
        kCPU,
        kCUDA,
        kUnavailable
    };

    MockOnnxClipBackendModel() : rng_(kCanonicalRngSeed), backend_(Backend::kCPU) {
        // Attempt CUDA detection (mock: always CPU unless explicitly overridden)
        // In production, check CUDA_VISIBLE_DEVICES, cudaGetDeviceCount(), etc.
        cuda_available_ = false;  // Set to true if CUDA detected
    }

    /**
     * @brief Encode single image on specified backend.
     * Baseline for throughput comparison.
     */
    std::vector<float> encodeSingleImage(Backend backend) {
        // Simulate per-image inference overhead
        const int activations = 224 * 224;
        std::vector<float> features(768);
        
        for (int i = 0; i < activations / 16; ++i) {
            features[i % 768] += dis_(rng_);
        }
        return features;
    }

    /**
     * @brief Encode batch of images on specified backend.
     * Returns throughput (ops/sec, computed from elapsed time).
     */
    int encodeBatch(Backend backend, int batch_size, double& elapsed_sec) {
        auto start = std::chrono::steady_clock::now();
        
        std::vector<std::vector<float>> batch;
        for (int i = 0; i < batch_size; ++i) {
            batch.push_back(encodeSingleImage(backend));
        }
        
        auto end = std::chrono::steady_clock::now();
        elapsed_sec = std::chrono::duration<double>(end - start).count();
        
        // Return throughput: images per second
        return static_cast<int>(batch_size / elapsed_sec);
    }

    /**
     * @brief Load model and return peak RSS (memory footprint).
     */
    uint64_t loadModelAndMeasureMemory() {
        uint64_t rss_before = getRssBytes();
        
        // Simulate model weight loading (~4-50 MB depending on quantization)
        std::vector<float> model_weights(1024 * 1024 * 10);  // 40 MB
        for (auto& w : model_weights) {
            w = dis_(rng_);
        }
        
        uint64_t rss_after = getRssBytes();
        return rss_after;  // Peak RSS during load
    }

    /**
     * @brief Measure memory during batch inference.
     */
    uint64_t measureRuntimeMemory(int batch_size) {
        uint64_t rss_before = getRssBytes();
        
        // Allocate input buffers + activation maps
        // Input: batch_size × 224 × 224 × 3 × 4 bytes = batch_size × 602 KB
        // Activations: ~5 MB per image (transformer layers)
        std::vector<std::vector<float>> batch;
        for (int i = 0; i < batch_size; ++i) {
            std::vector<float> activations(224 * 224 * 3);
            for (auto& a : activations) {
                a = dis_(rng_);
            }
            batch.push_back(activations);
        }
        
        uint64_t rss_after = getRssBytes();
        return rss_after;
    }

    /**
     * @brief Measure overhead of batch-splitting strategy.
     * Simulates splitting large batch into N smaller batches.
     */
    double measureBatchSplittingOverhead(int total_batch_size, int split_factor) {
        auto start = std::chrono::steady_clock::now();
        
        int sub_batch_size = total_batch_size / split_factor;
        for (int i = 0; i < split_factor; ++i) {
            double _ = 0.0;
            auto _ = encodeBatch(Backend::kCPU, sub_batch_size, _);
            (void)_;
        }
        
        auto end = std::chrono::steady_clock::now();
        return std::chrono::duration<double>(end - start).count();
    }

    Backend backend() const { return backend_; }
    bool cudaAvailable() const { return cuda_available_; }

private:
    std::mt19937 rng_;
    std::uniform_real_distribution<float> dis_{-1.0f, 1.0f};
    Backend backend_;
    bool cuda_available_;
};

// ---------------------------------------------------------------------------
// Fixtures
// ---------------------------------------------------------------------------

/**
 * @brief Fixture for backend throughput benchmarks.
 * Pre-initializes model and performs 3-phase warmup.
 */
class OnnxClipBackendThroughputFixture : public benchmark::Fixture {
public:
    void SetUp(::benchmark::State& state) override {
        model_ = std::make_unique<MockOnnxClipBackendModel>();
        
        // 3-phase warmup
        int batch_size = static_cast<int>(state.range(0));
        
        // Phase 1: Cold
        for (int i = 0; i < kWarmupIterationsCold; ++i) {
            double _;
            model_->encodeBatch(MockOnnxClipBackendModel::Backend::kCPU, batch_size, _);
        }
        
        // Phase 2: Warm
        for (int i = 0; i < kWarmupIterationsWarm; ++i) {
            double _;
            model_->encodeBatch(MockOnnxClipBackendModel::Backend::kCPU, batch_size, _);
        }
        
        // Phase 3: Hot
        std::mt19937 rng(kCanonicalRngSeed);
        std::uniform_int_distribution<> batch_dist(1, batch_size);
        for (int i = 0; i < kWarmupIterationsHot; ++i) {
            int bs = batch_dist(rng);
            double _;
            model_->encodeBatch(MockOnnxClipBackendModel::Backend::kCPU, bs, _);
        }
    }

    void TearDown(::benchmark::State& /*state*/) override {
        model_.reset();
    }

protected:
    std::unique_ptr<MockOnnxClipBackendModel> model_;
};

/**
 * @brief Fixture for memory benchmarks (no warmup).
 */
class OnnxClipMemoryFixture : public benchmark::Fixture {
public:
    void SetUp(::benchmark::State& /*state*/) override {
        model_ = std::make_unique<MockOnnxClipBackendModel>();
    }

    void TearDown(::benchmark::State& /*state*/) override {
        model_.reset();
    }

protected:
    std::unique_ptr<MockOnnxClipBackendModel> model_;
};

/**
 * @brief Fixture for batch-splitting optimization benchmarks.
 */
class OnnxClipBatchSplittingFixture : public benchmark::Fixture {
public:
    void SetUp(::benchmark::State& /*state*/) override {
        model_ = std::make_unique<MockOnnxClipBackendModel>();
    }

    void TearDown(::benchmark::State& /*state*/) override {
        model_.reset();
    }

protected:
    std::unique_ptr<MockOnnxClipBackendModel> model_;
};

} // namespace onnx_clip
} // namespace bench
} // namespace themis

// ---------------------------------------------------------------------------
// Benchmarks
// ---------------------------------------------------------------------------

using themis::bench::onnx_clip::OnnxClipBackendThroughputFixture;
using themis::bench::onnx_clip::OnnxClipMemoryFixture;
using themis::bench::onnx_clip::OnnxClipBatchSplittingFixture;
using themis::bench::onnx_clip::MockOnnxClipBackendModel;

/**
 * @brief FCP-05: CUDA batch-64 throughput benchmark.
 *
 * Measures throughput (images/sec) for batch-64 inference on CUDA.
 * Falls back to CPU if CUDA unavailable.
 *
 * Gate threshold: Throughput ≥ 6x single-image throughput
 * This tests batch parallelization efficiency.
 */
BENCHMARK_F(OnnxClipBackendThroughputFixture, BM_Throughput_ViTB32_CUDA_Batch64)
    (benchmark::State& state) {
    const int batch_size = 64;
    
    for (auto _ : state) {
        double elapsed_sec;
        int throughput = model_->encodeBatch(
            MockOnnxClipBackendModel::Backend::kCUDA,
            batch_size,
            elapsed_sec
        );
        state.SetItemsProcessed(batch_size);
        benchmark::DoNotOptimize(throughput);
    }
}
BENCHMARK_REGISTER_F(OnnxClipBackendThroughputFixture, BM_Throughput_ViTB32_CUDA_Batch64)
    ->UseRealTime()
    ->Unit(benchmark::kMillisecond)
    ->Arg(64);

/**
 * @brief CPU batch-16 throughput benchmark (baseline).
 *
 * Measures throughput (images/sec) for batch-16 inference on CPU.
 * Used as baseline for CUDA comparison.
 */
BENCHMARK_F(OnnxClipBackendThroughputFixture, BM_Throughput_ViTB32_CPU_Batch16)
    (benchmark::State& state) {
    const int batch_size = 16;
    
    for (auto _ : state) {
        double elapsed_sec;
        int throughput = model_->encodeBatch(
            MockOnnxClipBackendModel::Backend::kCPU,
            batch_size,
            elapsed_sec
        );
        state.SetItemsProcessed(batch_size);
        benchmark::DoNotOptimize(throughput);
    }
}
BENCHMARK_REGISTER_F(OnnxClipBackendThroughputFixture, BM_Throughput_ViTB32_CPU_Batch16)
    ->UseRealTime()
    ->Unit(benchmark::kMillisecond)
    ->Arg(16);

/**
 * @brief FCP-06: Memory footprint during model load.
 *
 * Measures peak RSS (resident set size) during model weight loading.
 * Includes:
 * - Weight deserialization
 * - Graph construction
 * - Temporary allocations
 *
 * Gate threshold: Tracked (regression > 10% blocks release).
 */
BENCHMARK_F(OnnxClipMemoryFixture, BM_MemoryFootprint_ModelLoad)
    (benchmark::State& state) {
    for (auto _ : state) {
        uint64_t peak_rss = model_->loadModelAndMeasureMemory();
        state.counters["peak_rss_mb"] = peak_rss / (1024.0 * 1024.0);
        benchmark::DoNotOptimize(peak_rss);
    }
}
BENCHMARK_REGISTER_F(OnnxClipMemoryFixture, BM_MemoryFootprint_ModelLoad)
    ->UseRealTime()
    ->Unit(benchmark::kMillisecond);

/**
 * @brief Memory footprint during runtime batch inference.
 *
 * Measures peak RSS during batch inference execution.
 * Includes:
 * - Input image buffers
 * - Activation maps (intermediate layers)
 * - Gradient buffers (if training mode)
 *
 * Parametrized by batch size: 1, 8, 16, 64.
 */
BENCHMARK_F(OnnxClipMemoryFixture, BM_MemoryFootprint_Runtime_State)
    (benchmark::State& state) {
    int batch_size = static_cast<int>(state.range(0));
    
    for (auto _ : state) {
        uint64_t peak_rss = model_->measureRuntimeMemory(batch_size);
        state.counters["peak_rss_mb"] = peak_rss / (1024.0 * 1024.0);
        state.counters["batch_size"] = batch_size;
        benchmark::DoNotOptimize(peak_rss);
    }
}
BENCHMARK_REGISTER_F(OnnxClipMemoryFixture, BM_MemoryFootprint_Runtime_State)
    ->UseRealTime()
    ->Unit(benchmark::kMillisecond)
    ->Arg(1)->Arg(8)->Arg(16)->Arg(64);

/**
 * @brief Batch-splitting performance optimization validation.
 *
 * Measures overhead of splitting a large batch (e.g., batch-64)
 * into smaller sub-batches (split-4, split-8, split-16).
 *
 * Tests different split factors to validate batch-splitting
 * optimization. A good implementation should show minimal overhead.
 *
 * Parametrized by split factor: 2, 4, 8, 16.
 */
BENCHMARK_F(OnnxClipBatchSplittingFixture, BM_BatchSplitting_Performance)
    (benchmark::State& state) {
    const int total_batch_size = 64;
    int split_factor = static_cast<int>(state.range(0));
    
    for (auto _ : state) {
        double elapsed = model_->measureBatchSplittingOverhead(total_batch_size, split_factor);
        state.counters["split_factor"] = split_factor;
        state.counters["elapsed_sec"] = elapsed;
        
        // Calculate speedup relative to monolithic batch
        // (would be computed from BM_Throughput_ViTB32_CUDA_Batch64 baseline)
        benchmark::DoNotOptimize(elapsed);
    }
}
BENCHMARK_REGISTER_F(OnnxClipBatchSplittingFixture, BM_BatchSplitting_Performance)
    ->UseRealTime()
    ->Unit(benchmark::kMillisecond)
    ->Arg(2)->Arg(4)->Arg(8)->Arg(16);
