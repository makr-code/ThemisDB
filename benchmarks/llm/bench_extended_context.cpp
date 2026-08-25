/**
 * @file bench_extended_context.cpp
 * @brief Performance benchmarks for Extended Context Window (32K-128K) and RoPE/YARN Scaling
 * 
 * Benchmarks the performance characteristics of extended context including:
 * - Context scaling overhead (4K vs 32K vs 128K)
 * - RoPE/YARN scaling methods comparison
 * - Memory allocation/deallocation performance
 * - Thread-safety overhead with LoRA adapters
 * 
 * @author ThemisDB Team / GitHub Copilot
 * @date January 2026
 */

#include <benchmark/benchmark.h>
#include "llm/model_loader.h"
#include "llm/grafana_metrics.h"
#include <vector>
#include <random>

using namespace themis::llm;
using namespace themis::llm::monitoring;

// ═══════════════════════════════════════════════════════════
// Benchmark Configuration
// ═══════════════════════════════════════════════════════════

static constexpr size_t HIDDEN_SIZE = 4096;
static constexpr size_t N_LAYERS = 32;
static constexpr float DTYPE_SIZE = 2.0f;  // FP16

// ═══════════════════════════════════════════════════════════
// Helper Functions
// ═══════════════════════════════════════════════════════════

size_t calculateKVCacheSize(size_t n_ctx) {
    // KV_Cache = n_ctx × n_layers × hidden_size × 2 (key + value) × dtype_size
    return static_cast<size_t>(
        n_ctx * N_LAYERS * HIDDEN_SIZE * 2 * DTYPE_SIZE
    );
}

double calculateScalingFactor(size_t original_ctx, size_t max_ctx) {
    return static_cast<double>(max_ctx) / static_cast<double>(original_ctx);
}

// ═══════════════════════════════════════════════════════════
// Memory Estimation Benchmarks
// ═══════════════════════════════════════════════════════════

static void BM_MemoryEstimation_4K(benchmark::State& state) {
    const size_t n_ctx = 4096;
    
    for (auto _ : state) {
        size_t cache_size = calculateKVCacheSize(n_ctx);
        benchmark::DoNotOptimize(cache_size);
    }
    
    state.SetLabel("Context: 4K");
    state.counters["cache_mb"] = calculateKVCacheSize(n_ctx) / (1024.0 * 1024.0);
}
BENCHMARK(BM_MemoryEstimation_4K);

static void BM_MemoryEstimation_8K(benchmark::State& state) {
    const size_t n_ctx = 8192;
    
    for (auto _ : state) {
        size_t cache_size = calculateKVCacheSize(n_ctx);
        benchmark::DoNotOptimize(cache_size);
    }
    
    state.SetLabel("Context: 8K");
    state.counters["cache_mb"] = calculateKVCacheSize(n_ctx) / (1024.0 * 1024.0);
}
BENCHMARK(BM_MemoryEstimation_8K);

static void BM_MemoryEstimation_16K(benchmark::State& state) {
    const size_t n_ctx = 16384;
    
    for (auto _ : state) {
        size_t cache_size = calculateKVCacheSize(n_ctx);
        benchmark::DoNotOptimize(cache_size);
    }
    
    state.SetLabel("Context: 16K");
    state.counters["cache_mb"] = calculateKVCacheSize(n_ctx) / (1024.0 * 1024.0);
}
BENCHMARK(BM_MemoryEstimation_16K);

static void BM_MemoryEstimation_32K(benchmark::State& state) {
    const size_t n_ctx = 32768;
    
    for (auto _ : state) {
        size_t cache_size = calculateKVCacheSize(n_ctx);
        benchmark::DoNotOptimize(cache_size);
    }
    
    state.SetLabel("Context: 32K");
    state.counters["cache_mb"] = calculateKVCacheSize(n_ctx) / (1024.0 * 1024.0);
}
BENCHMARK(BM_MemoryEstimation_32K);

static void BM_MemoryEstimation_64K(benchmark::State& state) {
    const size_t n_ctx = 65536;
    
    for (auto _ : state) {
        size_t cache_size = calculateKVCacheSize(n_ctx);
        benchmark::DoNotOptimize(cache_size);
    }
    
    state.SetLabel("Context: 64K");
    state.counters["cache_mb"] = calculateKVCacheSize(n_ctx) / (1024.0 * 1024.0);
}
BENCHMARK(BM_MemoryEstimation_64K);

static void BM_MemoryEstimation_128K(benchmark::State& state) {
    const size_t n_ctx = 131072;
    
    for (auto _ : state) {
        size_t cache_size = calculateKVCacheSize(n_ctx);
        benchmark::DoNotOptimize(cache_size);
    }
    
    state.SetLabel("Context: 128K");
    state.counters["cache_mb"] = calculateKVCacheSize(n_ctx) / (1024.0 * 1024.0);
}
BENCHMARK(BM_MemoryEstimation_128K);

// ═══════════════════════════════════════════════════════════
// RoPE Scaling Calculation Benchmarks
// ═══════════════════════════════════════════════════════════

static void BM_RoPEScaling_Linear(benchmark::State& state) {
    const size_t original_ctx = 4096;
    const size_t max_ctx = state.range(0);
    
    for (auto _ : state) {
        float scale_factor = static_cast<float>(original_ctx) / static_cast<float>(max_ctx);
        benchmark::DoNotOptimize(scale_factor);
    }
    
    state.counters["scaling_factor"] = calculateScalingFactor(original_ctx, max_ctx);
}
BENCHMARK(BM_RoPEScaling_Linear)->Arg(8192)->Arg(16384)->Arg(32768)->Arg(65536)->Arg(131072);

static void BM_RoPEScaling_NTK(benchmark::State& state) {
    const size_t original_ctx = 4096;
    const size_t max_ctx = state.range(0);
    
    for (auto _ : state) {
        float scaling_ratio = static_cast<float>(max_ctx) / static_cast<float>(original_ctx);
        float freq_base = 10000.0f * std::pow(scaling_ratio, 0.5f);
        benchmark::DoNotOptimize(freq_base);
    }
    
    state.counters["scaling_factor"] = calculateScalingFactor(original_ctx, max_ctx);
}
BENCHMARK(BM_RoPEScaling_NTK)->Arg(8192)->Arg(16384)->Arg(32768)->Arg(65536)->Arg(131072);

static void BM_RoPEScaling_YARN(benchmark::State& state) {
    const size_t original_ctx = 4096;
    const size_t max_ctx = state.range(0);
    
    // YaRN parameters
    const float ext_factor = 1.0f;
    const float attn_factor = 1.0f;
    const float beta_fast = 32.0f;
    const float beta_slow = 1.0f;
    
    for (auto _ : state) {
        float scale_factor = static_cast<float>(original_ctx) / static_cast<float>(max_ctx);
        
        // Simplified YaRN calculation (actual implementation is more complex)
        float yarn_scale = scale_factor * ext_factor * attn_factor;
        float freq_adjustment = (beta_fast / beta_slow);
        float final_scale = yarn_scale * (1.0f + freq_adjustment * 0.01f);
        
        benchmark::DoNotOptimize(final_scale);
    }
    
    state.counters["scaling_factor"] = calculateScalingFactor(original_ctx, max_ctx);
}
BENCHMARK(BM_RoPEScaling_YARN)->Arg(8192)->Arg(16384)->Arg(32768)->Arg(65536)->Arg(131072);

// ═══════════════════════════════════════════════════════════
// Memory Allocation Benchmarks
// ═══════════════════════════════════════════════════════════

static void BM_MemoryAllocation_KVCache(benchmark::State& state) {
    const size_t n_ctx = state.range(0);
    const size_t cache_size = calculateKVCacheSize(n_ctx);
    
    for (auto _ : state) {
        std::vector<uint8_t> cache(cache_size);
        benchmark::DoNotOptimize(cache.data());
        benchmark::ClobberMemory();
    }
    
    state.SetBytesProcessed(state.iterations() * cache_size);
    state.counters["cache_mb"] = cache_size / (1024.0 * 1024.0);
}
BENCHMARK(BM_MemoryAllocation_KVCache)
    ->Arg(4096)
    ->Arg(8192)
    ->Arg(16384)
    ->Arg(32768)
    ->Arg(65536);

static void BM_MemoryDeallocation_KVCache(benchmark::State& state) {
    const size_t n_ctx = state.range(0);
    const size_t cache_size = calculateKVCacheSize(n_ctx);
    
    for (auto _ : state) {
        state.PauseTiming();
        std::vector<uint8_t> cache(cache_size);
        state.ResumeTiming();
        
        cache.clear();
        cache.shrink_to_fit();
        
        benchmark::ClobberMemory();
    }
    
    state.counters["cache_mb"] = cache_size / (1024.0 * 1024.0);
}
BENCHMARK(BM_MemoryDeallocation_KVCache)
    ->Arg(4096)
    ->Arg(8192)
    ->Arg(16384)
    ->Arg(32768)
    ->Arg(65536);

// ═══════════════════════════════════════════════════════════
// Metrics Recording Benchmarks
// ═══════════════════════════════════════════════════════════

static void BM_MetricsRecording_ContextLength(benchmark::State& state) {
    PrometheusExporter exporter;
    LLMMetricsCollector metrics(&exporter);
    
    const std::string model_id = "bench-model";
    const size_t context_length = state.range(0);
    
    for (auto _ : state) {
        metrics.recordContextLength(model_id, context_length);
    }
    
    state.counters["context_length"] = context_length;
}
BENCHMARK(BM_MetricsRecording_ContextLength)
    ->Arg(4096)
    ->Arg(16384)
    ->Arg(32768)
    ->Arg(131072);

static void BM_MetricsRecording_RAMUsage(benchmark::State& state) {
    PrometheusExporter exporter;
    LLMMetricsCollector metrics(&exporter);
    
    const std::string model_id = "bench-model";
    const size_t ram_used = 12288;
    const size_t ram_total = 65536;
    
    for (auto _ : state) {
        metrics.recordRAMUsage(model_id, ram_used, ram_total);
    }
}
BENCHMARK(BM_MetricsRecording_RAMUsage);

static void BM_MetricsRecording_VRAMUsage(benchmark::State& state) {
    PrometheusExporter exporter;
    LLMMetricsCollector metrics(&exporter);
    
    const std::string model_id = "bench-model";
    const size_t vram_used = 16384;
    const size_t vram_total = 24576;
    
    for (auto _ : state) {
        metrics.recordVRAMUsage(model_id, vram_used, vram_total);
    }
}
BENCHMARK(BM_MetricsRecording_VRAMUsage);

static void BM_MetricsRecording_YARNParameters(benchmark::State& state) {
    PrometheusExporter exporter;
    LLMMetricsCollector metrics(&exporter);
    
    const std::string model_id = "bench-model";
    
    for (auto _ : state) {
        metrics.recordYARNParameters(model_id, 1.0, 1.0, 32.0, 1.0);
    }
}
BENCHMARK(BM_MetricsRecording_YARNParameters);

static void BM_MetricsRecording_LoRASwitch(benchmark::State& state) {
    PrometheusExporter exporter;
    LLMMetricsCollector metrics(&exporter);
    
    const std::string model_id = "bench-model";
    const std::string from = "adapter_a";
    const std::string to = "adapter_b";
    const double duration_ms = 150.0;
    
    for (auto _ : state) {
        metrics.recordLoRAAdapterSwitch(model_id, from, to, duration_ms);
    }
}
BENCHMARK(BM_MetricsRecording_LoRASwitch);

// ═══════════════════════════════════════════════════════════
// Context Scaling Comparison Benchmarks
// ═══════════════════════════════════════════════════════════

static void BM_ContextScaling_Comparison(benchmark::State& state) {
    const size_t n_ctx = state.range(0);
    const size_t original_ctx = 4096;
    
    // Simulate context processing overhead
    std::vector<double> dummy_data(n_ctx);
    std::mt19937 gen(42);
    std::uniform_real_distribution<> dis(0.0, 1.0);
    
    for (auto _ : state) {
        state.PauseTiming();
        for (size_t i = 0; i < n_ctx; ++i) {
            dummy_data[i] = dis(gen);
        }
        state.ResumeTiming();
        
        // Simulate some processing
        double sum = 0.0;
        for (size_t i = 0; i < n_ctx; ++i) {
            sum += dummy_data[i];
        }
        benchmark::DoNotOptimize(sum);
    }
    
    double scaling_factor = static_cast<double>(n_ctx) / static_cast<double>(original_ctx);
    state.counters["scaling_factor"] = scaling_factor;
    state.counters["context_size"] = n_ctx;
    state.SetItemsProcessed(state.iterations() * n_ctx);
}
BENCHMARK(BM_ContextScaling_Comparison)
    ->Arg(4096)    // 1x (native)
    ->Arg(8192)    // 2x
    ->Arg(16384)   // 4x
    ->Arg(32768)   // 8x
    ->Arg(65536)   // 16x
    ->Arg(131072); // 32x

// ═══════════════════════════════════════════════════════════
// Thread-Safety Overhead Benchmarks
// ═══════════════════════════════════════════════════════════

static void BM_ThreadSafety_MutexLock(benchmark::State& state) {
    std::mutex mtx;
    int counter = 0;
    
    for (auto _ : state) {
        std::lock_guard<std::mutex> lock(mtx);
        ++counter;
        benchmark::DoNotOptimize(counter);
    }
    
    state.SetLabel("Mutex overhead");
}
BENCHMARK(BM_ThreadSafety_MutexLock);

static void BM_ThreadSafety_NoLock(benchmark::State& state) {
    int counter = 0;
    
    for (auto _ : state) {
        ++counter;
        benchmark::DoNotOptimize(counter);
    }
    
    state.SetLabel("No lock baseline");
}
BENCHMARK(BM_ThreadSafety_NoLock);

// ═══════════════════════════════════════════════════════════
// Metrics Export Benchmarks
// ═══════════════════════════════════════════════════════════

static void BM_MetricsExport_Small(benchmark::State& state) {
    PrometheusExporter exporter;
    LLMMetricsCollector metrics(&exporter);
    
    // Record a few metrics
    metrics.recordContextLength("model-1", 32768);
    metrics.recordRAMUsage("model-1", 12288, 65536);
    metrics.recordVRAMUsage("model-1", 16384, 24576);
    
    for (auto _ : state) {
        std::string output = exporter.exportMetrics();
        benchmark::DoNotOptimize(output);
    }
    
    state.SetLabel("Small export (3 models)");
}
BENCHMARK(BM_MetricsExport_Small);

static void BM_MetricsExport_Large(benchmark::State& state) {
    PrometheusExporter exporter;
    LLMMetricsCollector metrics(&exporter);
    
    // Record many metrics for multiple models
    for (int i = 0; i < 10; ++i) {
        std::string model_id = "model-" + std::to_string(i);
        metrics.recordContextLength(model_id, 32768);
        metrics.recordRAMUsage(model_id, 12288, 65536);
        metrics.recordVRAMUsage(model_id, 16384, 24576);
        metrics.recordYARNParameters(model_id, 1.0, 1.0, 32.0, 1.0);
        metrics.recordMemoryPressure(model_id, 75.0);
    }
    
    for (auto _ : state) {
        std::string output = exporter.exportMetrics();
        benchmark::DoNotOptimize(output);
    }
    
    state.SetLabel("Large export (10 models × 5 metrics)");
}
BENCHMARK(BM_MetricsExport_Large);

// ═══════════════════════════════════════════════════════════
// Comparative Analysis Benchmarks
// ═══════════════════════════════════════════════════════════

static void BM_EndToEnd_4K_vs_32K(benchmark::State& state) {
    const bool use_extended = state.range(0);
    const size_t n_ctx = use_extended ? 32768 : 4096;
    
    PrometheusExporter exporter;
    LLMMetricsCollector metrics(&exporter);
    
    for (auto _ : state) {
        // Simulate complete workflow
        metrics.recordContextLength("model", n_ctx);
        metrics.recordContextCacheSize("model", calculateKVCacheSize(n_ctx) / (1024 * 1024));
        metrics.recordRAMUsage("model", 12288, 65536);
        metrics.recordVRAMUsage("model", 16384, 24576);
        
        std::string output = exporter.exportMetrics();
        benchmark::DoNotOptimize(output);
    }
    
    state.SetLabel(use_extended ? "32K Extended" : "4K Native");
    state.counters["context_size"] = n_ctx;
}
BENCHMARK(BM_EndToEnd_4K_vs_32K)->Arg(0)->Arg(1);

// ═══════════════════════════════════════════════════════════
// Main
// ═══════════════════════════════════════════════════════════

BENCHMARK_MAIN();
