/**
 * @file bench_flash_attention.cpp
 * @brief Benchmark for Flash Attention v3 performance
 * 
 * Measures:
 * - Throughput (TFLOPs)
 * - Latency (ms)
 * - Memory usage (MB)
 * - Speedup vs standard attention
 */

#include <benchmark/benchmark.h>
#include "llm/attention/flash_attention.h"
#include "llm/attention/kv_cache_manager.h"
#include <vector>
#include <random>

#ifndef THEMIS_ENABLE_GPU

static void BM_FlashAttention_GPUDisabled(benchmark::State& state) {
    for (auto _ : state) {
        state.SkipWithError("Flash-attention benchmarks are disabled in this build");
        break;
    }
}

// Disabled: flash attention CUDA kernel requires GPU runner | Deadline: v1.9.0 | Issue: #5
BENCHMARK(BM_FlashAttention_GPUDisabled);
BENCHMARK_MAIN();

#else

using namespace themis::llm::attention;

namespace {

// Helper to create random tensors
Tensor createRandomTensor(const std::vector<int>& shape, float scale = 1.0f) {
    Tensor t;
    t.shape = shape;
    t.size = 1;
    for (int dim : shape) {
        t.size *= dim;
    }
    t.data = new float[t.size];
    
    std::mt19937 gen(42);  // Fixed seed for reproducibility
    std::uniform_real_distribution<float> dist(-scale, scale);
    
    for (size_t i = 0; i < t.size; ++i) {
        t.data[i] = dist(gen);
    }
    
    return t;
}

void freeTensor(Tensor& t) {
    if (t.data) {
        delete[] t.data;
        t.data = nullptr;
    }
}

} // anonymous namespace

// ============================================================================
// Small Attention Benchmarks (2K context)
// ============================================================================

static void BM_FlashAttention_Small_CPU(benchmark::State& state) {
    FlashAttentionConfig config;
    config.batch_size = 1;
    config.seq_len = state.range(0);
    config.num_heads = 32;
    config.head_dim = 128;
    config.use_causal_mask = true;
    
    FlashAttention flash_attn(Backend::CPU, config);
    
    Tensor Q = createRandomTensor({1, config.seq_len, 32, 128}, 0.1f);
    Tensor K = createRandomTensor({1, config.seq_len, 32, 128}, 0.1f);
    Tensor V = createRandomTensor({1, config.seq_len, 32, 128}, 0.1f);
    Tensor O = createRandomTensor({1, config.seq_len, 32, 128}, 0.0f);
    
    for (auto _ : state) {
        Status status = flash_attn.forward(Q, K, V, O);
        benchmark::DoNotOptimize(status);
    }
    
    // Calculate throughput
    int64_t flops = 2LL * config.batch_size * config.num_heads * 
                    config.seq_len * config.seq_len * config.head_dim;
    state.SetItemsProcessed(state.iterations() * flops);
    state.SetLabel("CPU");
    
    freeTensor(Q);
    freeTensor(K);
    freeTensor(V);
    freeTensor(O);
}

BENCHMARK(BM_FlashAttention_Small_CPU)
    ->Arg(256)
    ->Arg(512)
    ->Arg(1024)
    ->Arg(2048)
    ->Unit(benchmark::kMillisecond);

// ============================================================================
// Medium Attention Benchmarks (4K context)
// ============================================================================

static void BM_FlashAttention_Medium_CPU(benchmark::State& state) {
    FlashAttentionConfig config;
    config.batch_size = state.range(0);
    config.seq_len = 4096;
    config.num_heads = 32;
    config.head_dim = 128;
    
    FlashAttention flash_attn(Backend::CPU, config);
    
    int batch = state.range(0);
    Tensor Q = createRandomTensor({batch, 4096, 32, 128}, 0.1f);
    Tensor K = createRandomTensor({batch, 4096, 32, 128}, 0.1f);
    Tensor V = createRandomTensor({batch, 4096, 32, 128}, 0.1f);
    Tensor O = createRandomTensor({batch, 4096, 32, 128}, 0.0f);
    
    for (auto _ : state) {
        Status status = flash_attn.forward(Q, K, V, O);
        benchmark::DoNotOptimize(status);
    }
    
    int64_t flops = 2LL * batch * 32 * 4096 * 4096 * 128;
    state.SetItemsProcessed(state.iterations() * flops);
    state.SetLabel("CPU_batch" + std::to_string(batch));
    
    freeTensor(Q);
    freeTensor(K);
    freeTensor(V);
    freeTensor(O);
}

BENCHMARK(BM_FlashAttention_Medium_CPU)
    ->Arg(1)
    ->Arg(4)
    ->Arg(8)
    ->Unit(benchmark::kMillisecond);

// ============================================================================
// KV-Cache Benchmarks
// ============================================================================

static void BM_KVCache_Allocation(benchmark::State& state) {
    FlashAttentionConfig config;
    config.num_kv_blocks = state.range(0);
    config.kv_block_size = 16;
    
    for (auto _ : state) {
        KVCacheManager cache_mgr(config);
        benchmark::DoNotOptimize(cache_mgr);
    }
    
    state.SetLabel("blocks=" + std::to_string(state.range(0)));
}

BENCHMARK(BM_KVCache_Allocation)
    ->Arg(128)
    ->Arg(512)
    ->Arg(2048)
    ->Arg(4096)
    ->Unit(benchmark::kMicrosecond);

static void BM_KVCache_SequenceAllocation(benchmark::State& state) {
    FlashAttentionConfig config;
    config.num_kv_blocks = 4096;
    config.kv_block_size = 16;
    
    KVCacheManager cache_mgr(config);
    int tokens_per_seq = state.range(0);
    
    uint64_t seq_id = 0;
    for (auto _ : state) {
        BlockTable table = cache_mgr.allocateSequence(seq_id++, tokens_per_seq);
        benchmark::DoNotOptimize(table);
        cache_mgr.freeSequence(seq_id - 1);
    }
    
    state.SetLabel("tokens=" + std::to_string(tokens_per_seq));
}

BENCHMARK(BM_KVCache_SequenceAllocation)
    ->Arg(256)
    ->Arg(512)
    ->Arg(1024)
    ->Arg(2048)
    ->Unit(benchmark::kMicrosecond);

static void BM_KVCache_PrefixSharing(benchmark::State& state) {
    FlashAttentionConfig config;
    config.num_kv_blocks = 4096;
    config.kv_block_size = 16;
    
    KVCacheManager cache_mgr(config);
    
    // Allocate parent sequence
    uint64_t parent_id = 1;
    cache_mgr.allocateSequence(parent_id, 1024);
    
    int prefix_length = state.range(0);
    uint64_t child_id = 2;
    
    for (auto _ : state) {
        cache_mgr.sharePrefix(child_id++, parent_id, prefix_length);
        cache_mgr.freeSequence(child_id - 1);
    }
    
    state.SetLabel("prefix=" + std::to_string(prefix_length));
}

BENCHMARK(BM_KVCache_PrefixSharing)
    ->Arg(64)
    ->Arg(128)
    ->Arg(256)
    ->Arg(512)
    ->Unit(benchmark::kMicrosecond);

// ============================================================================
// Backend Selection Benchmark
// ============================================================================

static void BM_BackendSelection(benchmark::State& state) {
    for (auto _ : state) {
        Backend backend = FlashAttention::selectBestBackend();
        benchmark::DoNotOptimize(backend);
    }
}

BENCHMARK(BM_BackendSelection)->Unit(benchmark::kNanosecond);

// ============================================================================
// Memory Statistics Benchmark
// ============================================================================

static void BM_KVCache_MemoryStats(benchmark::State& state) {
    FlashAttentionConfig config;
    config.num_kv_blocks = 4096;
    config.kv_block_size = 16;
    
    KVCacheManager cache_mgr(config);
    
    // Allocate some sequences
    for (int i = 0; i < 10; ++i) {
        cache_mgr.allocateSequence(i, 128);
    }
    
    for (auto _ : state) {
        AttentionMemoryStats stats = cache_mgr.getStats();
        benchmark::DoNotOptimize(stats);
    }
}

BENCHMARK(BM_KVCache_MemoryStats)->Unit(benchmark::kNanosecond);

// ============================================================================
// Configuration Benchmark
// ============================================================================

static void BM_ConfigCreation(benchmark::State& state) {
    for (auto _ : state) {
        FlashAttentionConfig config;
        config.batch_size = 32;
        config.seq_len = 4096;
        config.num_heads = 32;
        config.head_dim = 128;
        benchmark::DoNotOptimize(config);
    }
}

BENCHMARK(BM_ConfigCreation)->Unit(benchmark::kNanosecond);

BENCHMARK_MAIN();

#endif  // THEMIS_ENABLE_GPU
