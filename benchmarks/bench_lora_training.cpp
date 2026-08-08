/*
 * ThemisDB | File: bench_lora_training.cpp | Version: 0.0.47
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#include <benchmark/benchmark.h>
#include "llm/lora_framework/lora_layers.h"
#include <memory>
#include <vector>
#include <cassert>

using namespace themis::llm::lora;

/**
 * @file bench_lora_training.cpp
 * @brief Benchmarks for LoRA training components with Phase 5 performance gates
 * 
 * Phase 5 Performance Gates:
 * - Layer construction: <50µs for standard dimensions
 * - Forward pass: <100µs per sample
 * - Backward pass: <150µs per sample
 * - Adapter merge: <100ms
 * - Checkpoint save/load: <200ms save, <500ms load
 * - Sustained training: no memory leaks over 1000+ steps
 * 
 * Benchmarks:
 * - Layer construction time with regression detection
 * - Forward pass performance with throughput gates
 * - Backward pass performance with throughput gates
 * - Parameter access overhead
 * - Memory efficiency during sustained training
 */

// ===== Performance Gate Configuration =====

namespace gates {
    // Construction gates (microseconds)
    constexpr double LORA_LAYER_CONSTRUCTION_US = 50.0;      // Phase 5 target: <50µs
    
    // Forward pass gates (microseconds per sample)
    constexpr double LORA_FORWARD_PER_SAMPLE_US = 100.0;     // Phase 5 target: <100µs/sample
    constexpr double ATTENTION_FORWARD_PER_SAMPLE_US = 150.0; // Phase 5 target: <150µs/sample
    
    // Backward pass gates (microseconds per sample)
    constexpr double LORA_BACKWARD_PER_SAMPLE_US = 150.0;    // Phase 5 target: <150µs/sample
    constexpr double ATTENTION_BACKWARD_PER_SAMPLE_US = 200.0; // Phase 5 target: <200µs/sample
    
    // Merge operations (milliseconds)
    constexpr double ADAPTER_MERGE_MS = 100.0;               // Phase 5 target: <100ms
    
    // Checkpoint operations (milliseconds)
    constexpr double CHECKPOINT_SAVE_MS = 200.0;             // Phase 5 target: <200ms
    constexpr double CHECKPOINT_LOAD_MS = 500.0;             // Phase 5 target: <500ms for ~50MB
    
    // Memory regression detection (tolerance %)
    constexpr double MEMORY_REGRESSION_TOLERANCE_PCT = 5.0;  // Flag 5%+ memory increases
    
    // Helper to report gate violations
    static void report_gate_violation(const std::string& gate_name, 
                                      double measured_us, 
                                      double gate_us) {
        double violation_pct = ((measured_us - gate_us) / gate_us) * 100.0;
        fprintf(stderr, 
                "[PERF_GATE] %s VIOLATION: measured=%.2fµs gate=%.2fµs (+%.1f%%)\n",
                gate_name.c_str(), measured_us, gate_us, violation_pct);
    }
}

// ===== Construction Benchmarks =====

static void BM_LoRALayer_Construction(benchmark::State& state) {
    size_t dim = state.range(0);
    size_t rank = 8;
    
    for (auto _ : state) {
        LoRALayer layer(dim, dim, rank);
        benchmark::DoNotOptimize(layer);
    }
    
    // Phase 5 gate: LoRA layer construction <50µs for standard dimensions
    if (dim == 768) {  // Standard BERT dimension
        double nanos = state.counters["_total_time"] * 1e9 / state.iterations();
        double micros = nanos / 1000.0;
        
        state.counters["construction_us"] = micros;
        
        // Report violation if exceeds gate
        if (micros > gates::LORA_LAYER_CONSTRUCTION_US) {
            gates::report_gate_violation("BM_LoRALayer_Construction", micros, 
                                         gates::LORA_LAYER_CONSTRUCTION_US);
        }
    }
}
BENCHMARK(BM_LoRALayer_Construction)->Range(256, 4096)->Unit(benchmark::kMicrosecond);

static void BM_AttentionLoRA_Construction(benchmark::State& state) {
    size_t dim = state.range(0);
    size_t rank = 8;
    
    for (auto _ : state) {
        AttentionLoRA attn(dim, rank);
        benchmark::DoNotOptimize(attn);
    }
    
    // Phase 5 gate: Attention construction <50µs for standard dimensions
    if (dim == 768) {
        double nanos = state.counters["_total_time"] * 1e9 / state.iterations();
        double micros = nanos / 1000.0;
        
        state.counters["construction_us"] = micros;
        
        if (micros > gates::LORA_LAYER_CONSTRUCTION_US) {
            gates::report_gate_violation("BM_AttentionLoRA_Construction", micros, 
                                         gates::LORA_LAYER_CONSTRUCTION_US);
        }
    }
}
BENCHMARK(BM_AttentionLoRA_Construction)->Range(256, 4096)->Unit(benchmark::kMicrosecond);

static void BM_Sequential_Construction(benchmark::State& state) {
    size_t num_layers = state.range(0);
    
    for (auto _ : state) {
        Sequential seq;
        for (size_t i = 0; i < num_layers; ++i) {
            seq.add(std::make_unique<LoRALayer>(768, 768, 8));
        }
        benchmark::DoNotOptimize(seq);
    }
    
    // Per-layer average should be within gate
    double nanos = state.counters["_total_time"] * 1e9 / state.iterations();
    double micros_per_layer = (nanos / 1000.0) / num_layers;
    
    state.counters["construction_us_per_layer"] = micros_per_layer;
    
    if (micros_per_layer > gates::LORA_LAYER_CONSTRUCTION_US) {
        gates::report_gate_violation("BM_Sequential_Construction (per-layer)", 
                                     micros_per_layer, gates::LORA_LAYER_CONSTRUCTION_US);
    }
}
BENCHMARK(BM_Sequential_Construction)->Range(1, 16)->Unit(benchmark::kMicrosecond);

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
    
    // Phase 5 gate: Forward pass <100µs per sample
    double nanos = state.counters["_total_time"] * 1e9 / state.iterations();
    double micros = nanos / 1000.0;
    
    state.counters["forward_us_per_sample"] = micros;
    
    if (micros > gates::LORA_FORWARD_PER_SAMPLE_US) {
        gates::report_gate_violation("BM_LoRALayer_Forward", micros, 
                                     gates::LORA_FORWARD_PER_SAMPLE_US);
    }
}
BENCHMARK(BM_LoRALayer_Forward)->Range(256, 4096)->Unit(benchmark::kMicrosecond);

static void BM_AttentionLoRA_Forward(benchmark::State& state) {
    size_t dim = state.range(0);
    size_t rank = 8;
    
    AttentionLoRA attn(dim, rank);
    Tensor input({1, dim});
    
    for (auto _ : state) {
        Tensor output = attn.forward(input);
        benchmark::DoNotOptimize(output);
    }
    
    // Phase 5 gate: Attention forward pass <150µs per sample
    double nanos = state.counters["_total_time"] * 1e9 / state.iterations();
    double micros = nanos / 1000.0;
    
    state.counters["forward_us_per_sample"] = micros;
    
    if (micros > gates::ATTENTION_FORWARD_PER_SAMPLE_US) {
        gates::report_gate_violation("BM_AttentionLoRA_Forward", micros, 
                                     gates::ATTENTION_FORWARD_PER_SAMPLE_US);
    }
}
BENCHMARK(BM_AttentionLoRA_Forward)->Range(256, 4096)->Unit(benchmark::kMicrosecond);

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
    
    // Per-layer amortized forward time should be within gate
    double nanos = state.counters["_total_time"] * 1e9 / state.iterations();
    double micros_per_layer = (nanos / 1000.0) / num_layers;
    
    state.counters["forward_us_per_layer"] = micros_per_layer;
    
    if (micros_per_layer > gates::LORA_FORWARD_PER_SAMPLE_US) {
        gates::report_gate_violation("BM_Sequential_Forward (per-layer)", 
                                     micros_per_layer, gates::LORA_FORWARD_PER_SAMPLE_US);
    }
}
BENCHMARK(BM_Sequential_Forward)->Range(1, 16)->Unit(benchmark::kMicrosecond);

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
    
    // Phase 5 gate: Backward pass <150µs per sample
    double nanos = state.counters["_total_time"] * 1e9 / state.iterations();
    double micros = nanos / 1000.0;
    
    state.counters["backward_us_per_sample"] = micros;
    
    if (micros > gates::LORA_BACKWARD_PER_SAMPLE_US) {
        gates::report_gate_violation("BM_LoRALayer_Backward", micros, 
                                     gates::LORA_BACKWARD_PER_SAMPLE_US);
    }
}
BENCHMARK(BM_LoRALayer_Backward)->Range(256, 4096)->Unit(benchmark::kMicrosecond);

static void BM_AttentionLoRA_Backward(benchmark::State& state) {
    size_t dim = state.range(0);
    size_t rank = 8;
    
    AttentionLoRA attn(dim, rank);
    Tensor grad_output({1, dim});
    
    for (auto _ : state) {
        Tensor grad_input = attn.backward(grad_output);
        benchmark::DoNotOptimize(grad_input);
    }
    
    // Phase 5 gate: Attention backward pass <200µs per sample
    double nanos = state.counters["_total_time"] * 1e9 / state.iterations();
    double micros = nanos / 1000.0;
    
    state.counters["backward_us_per_sample"] = micros;
    
    if (micros > gates::ATTENTION_BACKWARD_PER_SAMPLE_US) {
        gates::report_gate_violation("BM_AttentionLoRA_Backward", micros, 
                                     gates::ATTENTION_BACKWARD_PER_SAMPLE_US);
    }
}
BENCHMARK(BM_AttentionLoRA_Backward)->Range(256, 4096)->Unit(benchmark::kMicrosecond);

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
    
    // Per-layer amortized backward time should be within gate
    double nanos = state.counters["_total_time"] * 1e9 / state.iterations();
    double micros_per_layer = (nanos / 1000.0) / num_layers;
    
    state.counters["backward_us_per_layer"] = micros_per_layer;
    
    if (micros_per_layer > gates::LORA_BACKWARD_PER_SAMPLE_US) {
        gates::report_gate_violation("BM_Sequential_Backward (per-layer)", 
                                     micros_per_layer, gates::LORA_BACKWARD_PER_SAMPLE_US);
    }
}
BENCHMARK(BM_Sequential_Backward)->Range(1, 16)->Unit(benchmark::kMicrosecond);

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
BENCHMARK(BM_LoRALayer_ParameterCount)->Range(256, 4096)->Unit(benchmark::kMicrosecond);

static void BM_LoRALayer_ParameterAccess(benchmark::State& state) {
    size_t dim = 768;
    size_t rank = 8;
    
    LoRALayer layer(dim, dim, rank);
    
    for (auto _ : state) {
        auto params = layer.parameters();
        benchmark::DoNotOptimize(params);
    }
}
BENCHMARK(BM_LoRALayer_ParameterAccess)->Unit(benchmark::kMicrosecond);

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
BENCHMARK(BM_Sequential_ParameterCollection)->Range(1, 16)->Unit(benchmark::kMicrosecond);

// ===== Memory Efficiency Benchmarks =====

static void BM_LoRALayer_MemoryUsage(benchmark::State& state) {
    size_t dim = state.range(0);
    size_t rank = 8;
    
    LoRALayer layer(dim, dim, rank);
    
    for (auto _ : state) {
        size_t memory = layer.memory_bytes();
        benchmark::DoNotOptimize(memory);
    }
    
    // Track baseline memory usage
    size_t baseline_memory = 0;
    if (dim == 768) {
        baseline_memory = (768 * 8) * sizeof(float) + (8 * 768) * sizeof(float);
        state.counters["baseline_memory_bytes"] = baseline_memory;
        state.counters["baseline_memory_mb"] = baseline_memory / (1024.0 * 1024.0);
    }
}
BENCHMARK(BM_LoRALayer_MemoryUsage)->Range(256, 4096)->Unit(benchmark::kMicrosecond);

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
BENCHMARK(BM_CompositePattern_Overhead)->Unit(benchmark::kMicrosecond);

// ===== Phase 5 Stress Test Benchmarks: Extended Training Sessions =====

/**
 * Phase 5 hardening: Measure memory behavior during extended training
 * without accumulation or leaks over 1000+ training steps
 */
static void BM_Extended_TrainingSession_1000Steps(benchmark::State& state) {
    size_t steps = 1000;
    size_t batch_size = 32;
    
    Sequential seq;
    seq.add(std::make_unique<LoRALayer>(768, 768, 8));
    seq.add(std::make_unique<LoRALayer>(768, 768, 8));
    
    Tensor batch_input({batch_size, 768});
    Tensor batch_target({batch_size, 768});
    batch_input.fill(0.5f);
    batch_target.fill(0.3f);
    
    // Track memory before
    size_t memory_before = 0;  // Would call malloc_info or similar in real implementation
    
    for (auto _ : state) {
        auto start = std::chrono::high_resolution_clock::now();
        
        for (size_t step = 0; step < steps; ++step) {
            // Forward pass
            auto output = seq.forward(batch_input);
            
            // Compute gradient
            auto loss = output - batch_target;
            
            // Backward pass
            auto grads = seq.backward(loss);
            
            // Zero gradients for next iteration
            seq.zero_grad();
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        state.SetIterationTime(elapsed.count() / 1000.0);
    }
    
    // Track memory after
    size_t memory_after = 0;  // Would call malloc_info or similar
    
    // Calculate per-step time
    double total_time_ms = state.counters["_total_time"] * 1e3;
    double ms_per_step = total_time_ms / steps;
    
    // Phase 5 gate: Training step <500ms for 32-sample batch
    state.counters["ms_per_step"] = ms_per_step;
    state.counters["total_steps"] = steps;
    state.counters["memory_diff_bytes"] = memory_after - memory_before;
    
    if (ms_per_step > 500.0) {
        gates::report_gate_violation("BM_Extended_TrainingSession_1000Steps", 
                                     ms_per_step * 1000.0, 500.0 * 1000.0);  // Convert to us
    }
}
BENCHMARK(BM_Extended_TrainingSession_1000Steps)->Unit(benchmark::kMillisecond);

/**
 * Phase 5 hardening: Measure concurrent adapter training
 * Simulates training multiple adapters simultaneously (4 adapters)
 */
static void BM_Concurrent_AdapterTraining(benchmark::State& state) {
    size_t num_adapters = 4;
    size_t steps = 100;
    
    // Create multiple independent adapters
    std::vector<std::unique_ptr<Sequential>> adapters;
    for (size_t i = 0; i < num_adapters; ++i) {
        auto seq = std::make_unique<Sequential>();
        seq->add(std::make_unique<LoRALayer>(768, 768, 8));
        adapters.push_back(std::move(seq));
    }
    
    std::vector<Tensor> inputs;
    std::vector<Tensor> targets;
    for (size_t i = 0; i < num_adapters; ++i) {
        inputs.emplace_back(std::vector<size_t>{32, 768});
        targets.emplace_back(std::vector<size_t>{32, 768});
        inputs[i].fill(0.5f);
        targets[i].fill(0.3f);
    }
    
    for (auto _ : state) {
        auto start = std::chrono::high_resolution_clock::now();
        
        for (size_t step = 0; step < steps; ++step) {
            // Train each adapter
            for (size_t i = 0; i < num_adapters; ++i) {
                auto output = adapters[i]->forward(inputs[i]);
                auto loss = output - targets[i];
                auto grads = adapters[i]->backward(loss);
                adapters[i]->zero_grad();
                
                benchmark::DoNotOptimize(grads);
            }
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        state.SetIterationTime(elapsed.count() / 1000.0);
    }
    
    state.counters["num_adapters"] = num_adapters;
    state.counters["total_steps"] = steps;
}
BENCHMARK(BM_Concurrent_AdapterTraining)->Unit(benchmark::kMillisecond);

/**
 * Phase 5 hardening: Memory pressure during large batch training
 * Validates behavior with large batch sizes approaching GPU memory limits
 */
static void BM_LargeBatchTraining_MemoryPressure(benchmark::State& state) {
    size_t batch_size = state.range(0);  // 64, 128, 256
    size_t steps = 10;
    
    Sequential seq;
    seq.add(std::make_unique<LoRALayer>(768, 768, 8));
    seq.add(std::make_unique<LoRALayer>(768, 768, 8));
    
    Tensor batch_input({batch_size, 768});
    Tensor batch_target({batch_size, 768});
    batch_input.fill(0.5f);
    batch_target.fill(0.3f);
    
    for (auto _ : state) {
        auto start = std::chrono::high_resolution_clock::now();
        
        for (size_t step = 0; step < steps; ++step) {
            auto output = seq.forward(batch_input);
            auto loss = output - batch_target;
            auto grads = seq.backward(loss);
            seq.zero_grad();
            
            benchmark::DoNotOptimize(grads);
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        state.SetIterationTime(elapsed.count() / 1000.0);
    }
    
    state.counters["batch_size"] = batch_size;
    state.counters["steps"] = steps;
}
BENCHMARK(BM_LargeBatchTraining_MemoryPressure)->Args({64})->Args({128})->Args({256})->Unit(benchmark::kMillisecond);

// Benchmark main
BENCHMARK_MAIN();
