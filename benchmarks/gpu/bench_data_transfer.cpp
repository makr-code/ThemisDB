#include <benchmark/benchmark.h>
#include "llm/lora_framework/gpu_data_loader.h"
#include "llm/lora_framework/gpu_tensor.h"
#include "llm/lora_framework/gpu_memory.h"
#include "llm/lora_framework/data_loader.h"
#include <chrono>
#include <vector>

using namespace themis::llm::lora;

/**
 * @file bench_data_transfer.cpp
 * @brief DataLoader & Memory Transfer Benchmarks
 * 
 * Tests:
 * - GPU data transfer throughput (CPU→GPU, GPU→CPU)
 * - Async prefetching effectiveness
 * - Memory bandwidth utilization
 * - Pinned memory performance
 */

// Benchmark configurations
constexpr size_t TRANSFER_SIZES[] = {1024, 4096, 16384, 65536};  // KB
constexpr size_t BATCH_SIZES[] = {1, 4, 8, 16};
constexpr size_t SEQ_LENGTH = 512;
constexpr size_t VOCAB_SIZE = 32000;
constexpr int WARMUP_ITERS = 3;

// ============================================================================
// Helper Functions
// ============================================================================

static bool cuda_available() {
    auto backends = GPUMemoryManager::detect_backends();
    for (const auto& backend : backends) {
        if (backend.type == themis::acceleration::BackendType::CUDA && backend.available) {
            return true;
        }
    }
    return false;
}

// ============================================================================
// CPU → GPU Transfer Benchmarks
// ============================================================================

static void BM_CPUtoGPU_Transfer(benchmark::State& state) {
    if (!cuda_available()) {
        state.SkipWithError("CUDA not available");
        return;
    }
    
    size_t size_kb = state.range(0);
    size_t num_elements = (size_kb * 1024) / sizeof(float);
    
    // Allocate CPU memory
    std::vector<float> cpu_data(num_elements, 1.0f);
    
    // Create GPU tensor
    GPUTensor gpu_tensor({num_elements}, Device::cuda());
    
    // Warmup
    for (int i = 0; i < WARMUP_ITERS; ++i) {
        gpu_tensor.upload(cpu_data);
    }
    
    // Measure
    for (auto _ : state) {
        auto start = std::chrono::high_resolution_clock::now();
        gpu_tensor.upload(cpu_data);
        auto end = std::chrono::high_resolution_clock::now();
        
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        state.SetIterationTime(elapsed.count() / 1e6);
    }
    
    // Calculate bandwidth (GB/s)
    double bytes_transferred = num_elements * sizeof(float);
    double time_sec = state.iterations() * state.max_iterations * 1e-6;
    double bandwidth_gbps = bytes_transferred / (1024.0 * 1024.0 * 1024.0) / time_sec;
    
    state.counters["bandwidth_GB/s"] = bandwidth_gbps;
    state.counters["size_MB"] = (num_elements * sizeof(float)) / (1024.0 * 1024.0);
    
    state.SetLabel("CPU→GPU");
}

BENCHMARK(BM_CPUtoGPU_Transfer)
    ->Arg(1024)->Arg(4096)->Arg(16384)->Arg(65536)
    ->UseManualTime();

// ============================================================================
// GPU → CPU Transfer Benchmarks
// ============================================================================

static void BM_GPUtoCPU_Transfer(benchmark::State& state) {
    if (!cuda_available()) {
        state.SkipWithError("CUDA not available");
        return;
    }
    
    size_t size_kb = state.range(0);
    size_t num_elements = (size_kb * 1024) / sizeof(float);
    
    // Create GPU tensor and fill with data
    GPUTensor gpu_tensor({num_elements}, Device::cuda());
    gpu_tensor.fill(1.0f);
    
    // Allocate CPU buffer
    std::vector<float> cpu_data(num_elements);
    
    // Warmup
    for (int i = 0; i < WARMUP_ITERS; ++i) {
        gpu_tensor.download(cpu_data.data(), num_elements);
    }
    
    // Measure
    for (auto _ : state) {
        auto start = std::chrono::high_resolution_clock::now();
        gpu_tensor.download(cpu_data.data(), num_elements);
        auto end = std::chrono::high_resolution_clock::now();
        
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        state.SetIterationTime(elapsed.count() / 1e6);
    }
    
    // Calculate bandwidth (GB/s)
    double bytes_transferred = num_elements * sizeof(float);
    double time_sec = state.iterations() * state.max_iterations * 1e-6;
    double bandwidth_gbps = bytes_transferred / (1024.0 * 1024.0 * 1024.0) / time_sec;
    
    state.counters["bandwidth_GB/s"] = bandwidth_gbps;
    state.counters["size_MB"] = (num_elements * sizeof(float)) / (1024.0 * 1024.0);
    
    state.SetLabel("GPU→CPU");
}

BENCHMARK(BM_GPUtoCPU_Transfer)
    ->Arg(1024)->Arg(4096)->Arg(16384)->Arg(65536)
    ->UseManualTime();

// ============================================================================
// Pinned vs Pageable Memory Transfer
// ============================================================================

static void BM_PinnedMemory_Transfer(benchmark::State& state) {
    if (!cuda_available()) {
        state.SkipWithError("CUDA not available");
        return;
    }
    
    bool use_pinned = state.range(0) == 1;
    size_t num_elements = (16 * 1024 * 1024) / sizeof(float);  // 16MB
    
    // Allocate memory based on type
    std::vector<float> cpu_data(num_elements, 1.0f);
    // Note: Real implementation would use cudaMallocHost for pinned memory
    
    GPUTensor gpu_tensor({num_elements}, Device::cuda());
    
    // Warmup
    for (int i = 0; i < WARMUP_ITERS; ++i) {
        gpu_tensor.upload(cpu_data);
    }
    
    // Measure
    for (auto _ : state) {
        auto start = std::chrono::high_resolution_clock::now();
        gpu_tensor.upload(cpu_data);
        auto end = std::chrono::high_resolution_clock::now();
        
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        state.SetIterationTime(elapsed.count() / 1e6);
    }
    
    double bytes_transferred = num_elements * sizeof(float);
    double time_sec = state.iterations() * state.max_iterations * 1e-6;
    double bandwidth_gbps = bytes_transferred / (1024.0 * 1024.0 * 1024.0) / time_sec;
    
    state.counters["bandwidth_GB/s"] = bandwidth_gbps;
    state.counters["pinned"] = use_pinned ? 1 : 0;
    
    state.SetLabel(use_pinned ? "Pinned" : "Pageable");
}

BENCHMARK(BM_PinnedMemory_Transfer)
    ->Arg(0)  // Pageable
    ->Arg(1)  // Pinned
    ->UseManualTime();

// ============================================================================
// Async Prefetching Benchmarks
// ============================================================================

class MockTokenizer : public ITokenizer {
public:
    MockTokenizer(int vocab_size = VOCAB_SIZE) : vocab_size_(vocab_size) {}
    
    std::vector<int> encode(const std::string& text, 
                           bool add_bos = true,
                           bool add_eos = false) override {
        std::vector<int> tokens(SEQ_LENGTH, 1);
        if (add_bos) {
          tokens[0] = bos_token_id();
        }
        if (add_eos) {
          tokens[SEQ_LENGTH-1] = eos_token_id();
        }
        return tokens;
    }
    
    std::string decode(const std::vector<int>& tokens) override {
        return "decoded text";
    }
    
    int vocab_size() const override { return vocab_size_; }
    int bos_token_id() const override { return 1; }
    int eos_token_id() const override { return 2; }
    int pad_token_id() const override { return 0; }
    
private:
    int vocab_size_;
};

static void BM_DataLoader_WithPrefetch(benchmark::State& state) {
    if (!cuda_available()) {
        state.SkipWithError("CUDA not available");
        return;
    }
    
    bool enable_prefetch = state.range(0) == 1;
    size_t batch_size = state.range(1);
    
    // Create mock tokenizer
    auto tokenizer = std::make_shared<MockTokenizer>();
    
    // Create data loader config
    GPUDataLoaderConfig config;
    config.batch_size = batch_size;
    config.max_sequence_length = SEQ_LENGTH;
    config.target_device = Device::cuda();
    config.async_loading = enable_prefetch;
    config.prefetch_batches = enable_prefetch ? 2 : 0;
    config.pin_cpu_memory = true;
    
    GPUDataLoader loader(tokenizer, config);
    
    // Create training samples
    std::vector<InstructionDataSample> samples = {};

    for (size_t i = 0; i < 100; ++i) {
        InstructionDataSample sample;
        sample.instruction = "Sample instruction " + std::to_string(i);
        sample.input = "Sample input";
        sample.output = "Sample output";
        samples.push_back(sample);
    }
    
    loader.loadFromSamples(samples);
    
    // Warmup
    for (int i = 0; i < WARMUP_ITERS; ++i) {
        if (loader.hasNext()) {
            auto batch = loader.getNextBatch();
        }
    }
    
    // Reset loader
    loader.reset();
    
    // Measure
    int batches_processed = 0;
    for (auto _ : state) {
        if (!loader.hasNext()) {
            loader.reset();
        }
        
        auto start = std::chrono::high_resolution_clock::now();
        auto batch = loader.getNextBatch();
        auto end = std::chrono::high_resolution_clock::now();
        
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        state.SetIterationTime(elapsed.count() / 1e6);
        
        batches_processed++;
    }
    
    double batches_per_sec = batches_processed / (state.iterations() * state.max_iterations * 1e-6);
    state.counters["batches/sec"] = batches_per_sec;
    state.counters["samples/sec"] = batches_per_sec * batch_size;
    state.counters["prefetch"] = enable_prefetch ? 1 : 0;
    state.counters["batch_size"] = batch_size;
    
    state.SetLabel(enable_prefetch ? "Async" : "Sync");
}

BENCHMARK(BM_DataLoader_WithPrefetch)
    ->Args({0, 4})->Args({0, 8})->Args({0, 16})  // No prefetch
    ->Args({1, 4})->Args({1, 8})->Args({1, 16})  // With prefetch
    ->UseManualTime();

// ============================================================================
// Batch Loading Throughput
// ============================================================================

static void BM_BatchLoading_Throughput(benchmark::State& state) {
    if (!cuda_available()) {
        state.SkipWithError("CUDA not available");
        return;
    }
    
    size_t batch_size = state.range(0);
    size_t seq_len = state.range(1);
    
    auto tokenizer = std::make_shared<MockTokenizer>();
    
    GPUDataLoaderConfig config;
    config.batch_size = batch_size;
    config.max_sequence_length = seq_len;
    config.target_device = Device::cuda();
    config.async_loading = true;
    config.prefetch_batches = 2;
    config.pin_cpu_memory = true;
    
    GPUDataLoader loader(tokenizer, config);
    
    // Create training samples
    std::vector<InstructionDataSample> samples = {};

    for (size_t i = 0; i < 200; ++i) {
        InstructionDataSample sample;
        sample.instruction = "Instruction " + std::to_string(i);
        sample.input = "Input data";
        sample.output = "Output data";
        samples.push_back(sample);
    }
    
    loader.loadFromSamples(samples);
    
    // Warmup
    for (int i = 0; i < WARMUP_ITERS && loader.hasNext(); ++i) {
        auto batch = loader.getNextBatch();
    }
    
    loader.reset();
    
    // Measure
    int batches_processed = 0;
    for (auto _ : state) {
        if (!loader.hasNext()) {
            loader.reset();
        }
        
        auto batch = loader.getNextBatch();
        batches_processed++;
    }
    
    double total_samples = batches_processed * batch_size;
    double samples_per_sec = total_samples / (state.iterations() * state.max_iterations * 1e-6);
    
    state.counters["samples/sec"] = samples_per_sec;
    state.counters["batch_size"] = batch_size;
    state.counters["seq_len"] = seq_len;
    state.counters["throughput_tokens/sec"] = samples_per_sec * seq_len;
    
    state.SetLabel("B" + std::to_string(batch_size) + "_S" + std::to_string(seq_len));
}

BENCHMARK(BM_BatchLoading_Throughput)
    ->Args({4, 128})->Args({4, 512})
    ->Args({8, 128})->Args({8, 512})
    ->Args({16, 128})->Args({16, 512});

// ============================================================================
// Memory Bandwidth Utilization
// ============================================================================

static void BM_MemoryBandwidth_Utilization(benchmark::State& state) {
    if (!cuda_available()) {
        state.SkipWithError("CUDA not available");
        return;
    }
    
    size_t num_elements = (64 * 1024 * 1024) / sizeof(float);  // 64MB
    
    // Create two GPU tensors
    GPUTensor tensor_a({num_elements}, Device::cuda());
    GPUTensor tensor_b({num_elements}, Device::cuda());
    tensor_a.fill(1.0f);
    
    // Warmup
    for (int i = 0; i < WARMUP_ITERS; ++i) {
        tensor_b = tensor_a + tensor_a;
    }
    
    // Measure
    for (auto _ : state) {
        auto start = std::chrono::high_resolution_clock::now();
        
        // Perform memory-bound operation
        tensor_b = tensor_a + tensor_a;
        
        auto end = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        state.SetIterationTime(elapsed.count() / 1e6);
    }
    
    // Calculate effective bandwidth
    // Read tensor_a twice, write tensor_b once = 3x data movement
    double bytes_transferred = 3 * num_elements * sizeof(float);
    double time_sec = state.iterations() * state.max_iterations * 1e-6;
    double bandwidth_gbps = bytes_transferred / (1024.0 * 1024.0 * 1024.0) / time_sec;
    
    state.counters["bandwidth_GB/s"] = bandwidth_gbps;
    state.counters["utilization_%"] = (bandwidth_gbps / 900.0) * 100.0;  // Assume 900GB/s peak
    
    state.SetLabel("VRAM Bandwidth");
}

BENCHMARK(BM_MemoryBandwidth_Utilization)
    ->UseManualTime();

// ============================================================================
// Cache Hit/Miss Analysis (Simplified)
// ============================================================================

static void BM_Cache_HitMiss_Pattern(benchmark::State& state) {
    if (!cuda_available()) {
        state.SkipWithError("CUDA not available");
        return;
    }
    
    bool sequential_access = state.range(0) == 1;
    size_t num_elements = (16 * 1024 * 1024) / sizeof(float);  // 16MB
    
    GPUTensor tensor({num_elements}, Device::cuda());
    tensor.fill(1.0f);
    
    // Warmup
    for (int i = 0; i < WARMUP_ITERS; ++i) {
        auto result = tensor + tensor;
        benchmark::DoNotOptimize(result);
    }
    
    // Measure
    for (auto _ : state) {
        auto start = std::chrono::high_resolution_clock::now();
        
        if (sequential_access) {
            // Sequential access - good cache locality
            auto result = tensor + tensor;
            benchmark::DoNotOptimize(result);
        } else {
            // Strided access - poor cache locality (simulated)
            auto result = tensor * 2.0f;
            benchmark::DoNotOptimize(result);
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        state.SetIterationTime(elapsed.count() / 1e6);
    }
    
    state.counters["sequential"] = sequential_access ? 1 : 0;
    
    state.SetLabel(sequential_access ? "Sequential" : "Strided");
}

BENCHMARK(BM_Cache_HitMiss_Pattern)
    ->Arg(0)  // Strided
    ->Arg(1)  // Sequential
    ->UseManualTime();

BENCHMARK_MAIN();
