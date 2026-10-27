#include <benchmark/benchmark.h>
#include <chrono>
#include <random>
#include <cmath>
#include <memory>
#include <vector>
#include "llm/ssm_stub_plugin.h"
#include "llm/infini_attention_cpu.h"
#include "llm/ssm_drift_metrics.h"
#include "llm/context_quality_metrics.h"

using namespace themis::llm;

// ============================================================================
// Shared constants and fixtures
// ============================================================================

namespace themis::bench {
    constexpr uint64_t kCanonicalRngSeed = 42;
}

namespace {

    // Synthetic latency model: time proportional to (seq_len^1.5) * hidden_dim
    // Mimics O(N^2) attention complexity with constant factor tuning
    double synthesize_infini_cpu_latency_ms(int seq_len, int hidden_dim = 768) {
        // CPU Infini-attention with compressive memory:
        // - Linear attention section: O(N * D)
        // - Compressive attention section: O(1 * D) amortized
        // Total: O(N * D) + overhead
        double base_us = (seq_len * hidden_dim * 1e-3);  // microseconds
        double overhead_us = (10.0 * std::log(seq_len + 1));  // logarithmic overhead
        return (base_us + overhead_us) / 1000.0;  // convert to ms
    }

    // Synthetic VRAM model: KV cache + attention state
    // FP16 KV cache: 2 bytes * 2 (K+V) * seq_len * hidden_dim / 64 (block size)
    // Compressive state: ~5% of KV cache (Infini compressive K/V)
    double synthesize_vram_footprint_mb(int seq_len, int hidden_dim = 768) {
        double kv_cache_bytes = 2.0 * 2 * seq_len * hidden_dim;  // FP16 K + V
        double compressive_bytes = kv_cache_bytes * 0.05;
        return (kv_cache_bytes + compressive_bytes) / (1024 * 1024);
    }

    // Synthetic throughput: tokens generated per second
    // Model: decode rate inversely proportional to latency per token
    double synthesize_tokens_per_second(double latency_ms) {
        // Assume: 1 token generation = latency_ms milliseconds
        // Throughput = 1000 / latency_ms tokens/sec
        if (latency_ms <= 0.0) {
          return 0.0;
        }
        return 1000.0 / latency_ms;
    }

}  // namespace

// ============================================================================
// Benchmark: CPU Infini-attention correctness baseline
// ============================================================================

/**
 * SIMULATION NOTE (Stub Path):
 * Purpose: Measure CPU Infini-attention synthetic latency and VRAM footprint during Phase 1
 * Activation: Benchmark execution; InfiniAttentionCpuFallback invoked
 * Production Delta: Actual CUDA kernel measurements will replace these synthetic metrics in Phase 2
 * Removal Plan: Superseded by P2-D02 CUDA kernel benchmarks (Q4/2026)
 */
static void bench_infini_attention_cpu_baseline(benchmark::State& state) {
    int seq_len = state.range(0);
    int hidden_dim = 768;

    themis::llm::attention::InfiniAttentionCPU::Config cfg;
    cfg.hidden_dim = hidden_dim;
    cfg.seq_len = seq_len;
    cfg.memory_size = 4096;
    cfg.use_fp64 = false;

    auto infini_cpu = std::make_unique<themis::llm::attention::InfiniAttentionCPU>(cfg);
    if (!infini_cpu->initialize()) {
        state.SkipWithError("InfiniAttentionCPU initialization failed");
    }

    Eigen::MatrixXf query = Eigen::MatrixXf::Zero(seq_len, hidden_dim);
    Eigen::MatrixXf key = Eigen::MatrixXf::Zero(seq_len, hidden_dim);
    Eigen::MatrixXf value = Eigen::MatrixXf::Zero(seq_len, hidden_dim);
    Eigen::MatrixXf output = Eigen::MatrixXf::Zero(seq_len, hidden_dim);

    double latency_ms = synthesize_infini_cpu_latency_ms(seq_len, hidden_dim);
    benchmark::DoNotOptimize(latency_ms);

    for (auto _ : state) {
        benchmark::DoNotOptimize(infini_cpu->forward(query, key, value, output));
        auto stats = infini_cpu->getMemoryStats();
        benchmark::DoNotOptimize(stats);
        latency_ms = synthesize_infini_cpu_latency_ms(seq_len, hidden_dim);
        state.counters["latency_ms"] = latency_ms;
    }

    state.counters["seq_len"] = seq_len;
    state.counters["vram_mb"] = synthesize_vram_footprint_mb(seq_len, hidden_dim);
    state.counters["throughput_tokens_per_sec"] = synthesize_tokens_per_second(latency_ms);
}

BENCHMARK(bench_infini_attention_cpu_baseline)
    ->Arg(512)
    ->Arg(2048)
    ->Arg(8192)
    ->Unit(benchmark::kMillisecond)
    ->Iterations(10);

// ============================================================================
// Benchmark: SSM stub plugin state checkpoint/resume baseline
// ============================================================================

/**
 * SIMULATION NOTE (Stub Path):
 * Purpose: Measure synthetic SSM state checkpoint/resume cycle during Phase 1
 * Activation: Benchmark execution; SyntheticSSMStub invoked
 * Production Delta: Real Mamba SSM backend will have different latency/VRAM profile in Phase 2+
 * Removal Plan: Superseded by real Mamba plugin (P1-D03 replaced in P2-D03 with real model)
 */
static void bench_ssm_stub_checkpoint_resume(benchmark::State& state) {
    int num_sessions = state.range(0);
    int state_size_kb = state.range(1);

    // Create synthetic SSM stub plugin
    auto ssm_stub = std::make_unique<SyntheticSSMStub>();

    // Create in-memory state store
    auto state_store = std::make_unique<InMemorySSMStateStore>();

    // Benchmark: checkpoint + resume cycle
    for (auto _ : state) {
        for (int i = 0; i < num_sessions; ++i) {
            // Simulate checkpoint: serialize state
            std::vector<uint8_t> serialized(state_size_kb * 1024, 0xAB);
            benchmark::DoNotOptimize(serialized);

            // Simulate resume: deserialize state
            std::vector<uint8_t> deserialized(state_size_kb * 1024, 0xCD);
            benchmark::DoNotOptimize(deserialized);
        }
    }

    state.counters["num_sessions"] = num_sessions;
    state.counters["state_size_kb"] = state_size_kb;
    state.counters["throughput_checkpoints_per_sec"] = num_sessions * 10.0 / 1000.0;  // synthetic
}

BENCHMARK(bench_ssm_stub_checkpoint_resume)
    ->Args({1, 512})
    ->Args({1, 2048})
    ->Args({10, 512})
    ->Args({10, 2048})
    ->Unit(benchmark::kMillisecond)
    ->Iterations(5);

// ============================================================================
// Benchmark: Context quality metric collection (Agentic Memory L1/L2/L3)
// ============================================================================

/**
 * Measurement Point: Phase 1 observability baseline for Agentic Memory layer transitions.
 * Purpose: Establish baseline cost of context quality scoring before Phase 3 optimization.
 */
static void bench_context_quality_metrics_computation(benchmark::State& state) {
    int num_samples = state.range(0);

    for (auto _ : state) {
        for (int i = 0; i < num_samples; ++i) {
            // Synthetic context quality metrics
            ContextQualityMetrics metrics;
            metrics.l1_tokens = 512;
            metrics.l2_tokens = 1024;
            metrics.l3_tokens = 2048;
            metrics.state_retention_score = 0.85f;
            metrics.factual_drift_estimate = 0.15f;
            metrics.tokens_since_last_retrieval = 50;

            // Evaluate decision thresholds
            bool should_refresh = metrics.shouldRefreshRAG();
            bool is_transformer_quality = metrics.isTransformerQuality();
            bool is_infini_quality = metrics.isInfiniQuality();
            bool is_ssm_quality = metrics.isSSMQuality();

            benchmark::DoNotOptimize(should_refresh);
            benchmark::DoNotOptimize(is_transformer_quality);
            benchmark::DoNotOptimize(is_infini_quality);
            benchmark::DoNotOptimize(is_ssm_quality);
        }
    }

    state.counters["num_samples"] = num_samples;
    state.counters["metrics_per_sec"] = num_samples * 1e3;  // synthetic rate
}

BENCHMARK(bench_context_quality_metrics_computation)
    ->Arg(100)
    ->Arg(1000)
    ->Arg(10000)
    ->Unit(benchmark::kMillisecond)
    ->Iterations(10);

// ============================================================================
// Benchmark: Drift metric collection (Prometheus instrumentation)
// ============================================================================

/**
 * Measurement Point: Phase 1 observability baseline for factual drift scoring.
 * Purpose: Establish baseline cost of Prometheus metric export before Phase 3 distributed observability.
 */
static void bench_drift_metrics_prometheus_export(benchmark::State& state) {
    int num_metrics = state.range(0);

    auto& drift_collector = themis::llm::metrics::SSMDriftMetrics::instance();

    for (auto _ : state) {
        for (int i = 0; i < num_metrics; ++i) {
            drift_collector.recordFactualDriftScore("bench-session", 0.35);
            drift_collector.recordSSMStateCheckpoint("bench-session", 4096u);
            drift_collector.recordHybridRouterDecision("transformer");
            benchmark::DoNotOptimize(drift_collector.exportPrometheus());
        }
    }

    state.counters["num_metrics"] = num_metrics;
    state.counters["metrics_per_sec"] = num_metrics * 1e3;  // synthetic rate
}

BENCHMARK(bench_drift_metrics_prometheus_export)
    ->Arg(100)
    ->Arg(1000)
    ->Arg(10000)
    ->Unit(benchmark::kMillisecond)
    ->Iterations(5);

// ============================================================================
// Main entry point
// ============================================================================

BENCHMARK_MAIN();
