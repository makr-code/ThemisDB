/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            bench_lora_training.cpp                            ║
  Version:         0.0.44                                             ║
  Last Modified:   2026-04-15 05:31:49                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     293                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include <benchmark/benchmark.h>
#include "llm/lora_framework/lora_layers.h"
#include <memory>
#include <vector>

using namespace themis::llm::lora;

/**
 * @file bench_lora_training.cpp
 * @brief Benchmarks for LoRA training components
 * 
 * Benchmarks:
 * - Layer construction time
 * - Forward pass performance
 * - Backward pass performance
 * - Parameter access overhead
 * - Memory efficiency
 */

// ===== Construction Benchmarks =====

static void BM_LoRALayer_Construction(benchmark::State& state) {
    size_t dim = state.range(0);
    size_t rank = 8;
    
    for (auto _ : state) {
        LoRALayer layer(dim, dim, rank);
        benchmark::DoNotOptimize(layer);
    }
}
BENCHMARK(BM_LoRALayer_Construction)->Range(256, 4096);

static void BM_AttentionLoRA_Construction(benchmark::State& state) {
    size_t dim = state.range(0);
    size_t rank = 8;
    
    for (auto _ : state) {
        AttentionLoRA attn(dim, rank);
        benchmark::DoNotOptimize(attn);
    }
}
BENCHMARK(BM_AttentionLoRA_Construction)->Range(256, 4096);

static void BM_Sequential_Construction(benchmark::State& state) {
    size_t num_layers = state.range(0);
    
    for (auto _ : state) {
        Sequential seq;
        for (size_t i = 0; i < num_layers; ++i) {
            seq.add(std::make_unique<LoRALayer>(768, 768, 8));
        }
        benchmark::DoNotOptimize(seq);
    }
}
BENCHMARK(BM_Sequential_Construction)->Range(1, 16);

// ===== Forward Pass Benchmarks =====

static void BM_LoRALayer_Forward(benchmark::State& state) {
    size_t dim = state.range(0);
    size_t rank = 8;
    
    LoRALayer layer(dim, dim, rank);
    Tensor input({1, dim});
    
    for (auto _ : state) {
        Tensor output = layer.forward(input);
        benchmark::DoNotOptimize(output);
    }
}
BENCHMARK(BM_LoRALayer_Forward)->Range(256, 4096);

static void BM_AttentionLoRA_Forward(benchmark::State& state) {
    size_t dim = state.range(0);
    size_t rank = 8;
    
    AttentionLoRA attn(dim, rank);
    Tensor input({1, dim});
    
    for (auto _ : state) {
        Tensor output = attn.forward(input);
        benchmark::DoNotOptimize(output);
    }
}
BENCHMARK(BM_AttentionLoRA_Forward)->Range(256, 4096);

static void BM_Sequential_Forward(benchmark::State& state) {
    size_t num_layers = state.range(0);
    
    Sequential seq;
    for (size_t i = 0; i < num_layers; ++i) {
        seq.add(std::make_unique<LoRALayer>(768, 768, 8));
    }
    
    Tensor input({1, 768});
    
    for (auto _ : state) {
        Tensor output = seq.forward(input);
        benchmark::DoNotOptimize(output);
    }
}
BENCHMARK(BM_Sequential_Forward)->Range(1, 16);

// ===== Backward Pass Benchmarks =====

static void BM_LoRALayer_Backward(benchmark::State& state) {
    size_t dim = state.range(0);
    size_t rank = 8;
    
    LoRALayer layer(dim, dim, rank);
    Tensor grad_output({1, dim});
    
    for (auto _ : state) {
        Tensor grad_input = layer.backward(grad_output);
        benchmark::DoNotOptimize(grad_input);
    }
}
BENCHMARK(BM_LoRALayer_Backward)->Range(256, 4096);

static void BM_AttentionLoRA_Backward(benchmark::State& state) {
    size_t dim = state.range(0);
    size_t rank = 8;
    
    AttentionLoRA attn(dim, rank);
    Tensor grad_output({1, dim});
    
    for (auto _ : state) {
        Tensor grad_input = attn.backward(grad_output);
        benchmark::DoNotOptimize(grad_input);
    }
}
BENCHMARK(BM_AttentionLoRA_Backward)->Range(256, 4096);

static void BM_Sequential_Backward(benchmark::State& state) {
    size_t num_layers = state.range(0);
    
    Sequential seq;
    for (size_t i = 0; i < num_layers; ++i) {
        seq.add(std::make_unique<LoRALayer>(768, 768, 8));
    }
    
    Tensor grad_output({1, 768});
    
    for (auto _ : state) {
        Tensor grad_input = seq.backward(grad_output);
        benchmark::DoNotOptimize(grad_input);
    }
}
BENCHMARK(BM_Sequential_Backward)->Range(1, 16);

// ===== Parameter Management Benchmarks =====

static void BM_LoRALayer_ParameterCount(benchmark::State& state) {
    size_t dim = state.range(0);
    size_t rank = 8;
    
    LoRALayer layer(dim, dim, rank);
    
    for (auto _ : state) {
        size_t count = layer.parameter_count();
        benchmark::DoNotOptimize(count);
    }
}
BENCHMARK(BM_LoRALayer_ParameterCount)->Range(256, 4096);

static void BM_LoRALayer_ParameterAccess(benchmark::State& state) {
    size_t dim = 768;
    size_t rank = 8;
    
    LoRALayer layer(dim, dim, rank);
    
    for (auto _ : state) {
        auto params = layer.parameters();
        benchmark::DoNotOptimize(params);
    }
}
BENCHMARK(BM_LoRALayer_ParameterAccess);

static void BM_Sequential_ParameterCollection(benchmark::State& state) {
    size_t num_layers = state.range(0);
    
    Sequential seq;
    for (size_t i = 0; i < num_layers; ++i) {
        seq.add(std::make_unique<LoRALayer>(768, 768, 8));
    }
    
    for (auto _ : state) {
        auto params = seq.parameters();
        benchmark::DoNotOptimize(params);
    }
}
BENCHMARK(BM_Sequential_ParameterCollection)->Range(1, 16);

// ===== Memory Efficiency Benchmarks =====

static void BM_LoRALayer_MemoryUsage(benchmark::State& state) {
    size_t dim = state.range(0);
    size_t rank = 8;
    
    LoRALayer layer(dim, dim, rank);
    
    for (auto _ : state) {
        size_t memory = layer.memory_bytes();
        benchmark::DoNotOptimize(memory);
    }
}
BENCHMARK(BM_LoRALayer_MemoryUsage)->Range(256, 4096);

static void BM_Compare_LoRAvsFullFinetuning(benchmark::State& state) {
    size_t dim = state.range(0);
    
    // LoRA parameters: (dim * rank) + (rank * dim)
    size_t lora_params = (dim * 8) + (8 * dim);
    
    // Full fine-tuning parameters: dim * dim
    size_t full_params = dim * dim;
    
    float reduction = 100.0f * (1.0f - static_cast<float>(lora_params) / full_params);
    
    state.counters["LoRA_Params"] = lora_params;
    state.counters["Full_Params"] = full_params;
    state.counters["Reduction_%"] = reduction;
    
    for (auto _ : state) {
        benchmark::DoNotOptimize(lora_params);
    }
}
BENCHMARK(BM_Compare_LoRAvsFullFinetuning)->Range(256, 4096);

// ===== Rank Impact Benchmarks =====

static void BM_LoRALayer_RankImpact(benchmark::State& state) {
    size_t dim = 768;
    size_t rank = state.range(0);
    
    LoRALayer layer(dim, dim, rank);
    
    state.counters["Params"] = layer.parameter_count();
    state.counters["Memory_MB"] = layer.memory_bytes() / (1024.0 * 1024.0);
    
    for (auto _ : state) {
        benchmark::DoNotOptimize(layer);
    }
}
BENCHMARK(BM_LoRALayer_RankImpact)->Range(4, 64);

// ===== Composite Pattern Overhead =====

static void BM_CompositePattern_Overhead(benchmark::State& state) {
    // Direct layer
    LoRALayer direct_layer(768, 768, 8);
    
    // Wrapped in Sequential
    Sequential seq;
    seq.add(std::make_unique<LoRALayer>(768, 768, 8));
    
    Tensor input({1, 768});
    
    for (auto _ : state) {
        // Measure overhead of composite pattern
        Tensor output1 = direct_layer.forward(input);
        Tensor output2 = seq.forward(input);
        benchmark::DoNotOptimize(output1);
        benchmark::DoNotOptimize(output2);
    }
}
BENCHMARK(BM_CompositePattern_Overhead);

// Benchmark main
BENCHMARK_MAIN();
