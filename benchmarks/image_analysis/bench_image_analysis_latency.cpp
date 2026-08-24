/**
 * @file bench_image_analysis_latency.cpp
 * @brief Latency Distribution Benchmarks for AI Image Analysis
 * 
 * Measures P50, P95, P99 latency and compares against industry standards.
 * Includes cold start vs warm cache analysis.
 * 
 * @author ThemisDB Team
 * @date December 2025
 */

#include <benchmark/benchmark.h>
#include "plugins/image_analysis_interface.h"
#include <vector>
#include <memory>
#include <cstdint>
#include <algorithm>
#include <numeric>
#include <cmath>

using namespace themis::plugins::image;

// ============================================================================
// Constants
// ============================================================================

constexpr int EMBEDDING_DIMENSION = 512;
constexpr size_t HASH_SAMPLE_SIZE = 1000;
constexpr int HASH_MULTIPLIER = 31;
constexpr int EMBEDDING_VALUE_RANGE = 1000;

// GPU performance multiplier vs CPU
constexpr int GPU_SPEEDUP_FACTOR = 10;
constexpr int COLD_START_MULTIPLIER = 2;

// Industry Performance Targets (documented in BENCHMARK_ANALYSIS_AI_IMAGERY.md)
// Embedding (224×224): P50<20ms (GPU), P50<180ms (CPU), P95<35ms (GPU), P95<320ms (CPU), P99<50ms (GPU), P99<450ms (CPU)
// Caption: P50<100ms (GPU), P95<160ms (GPU), P99<220ms (GPU)
// Batch 32 (GPU): 80-200ms total, 2.5-6.25ms per image
// Performance degradation target: <5% over sustained load

// ============================================================================
// Utility Functions
// ============================================================================

/**
 * @brief Simulate computation work for realistic latency
 */
inline void simulate_computation(size_t work_units) {
    volatile double dummy = 0.0;
    for (size_t i = 0; i < work_units; ++i) {
        dummy += std::sqrt(static_cast<double>(i)) * std::sin(static_cast<double>(i));
    }
}

// ============================================================================
// Test Data Generation
// ============================================================================

constexpr uint8_t JPEG_SOI_MARKER_1 = 0xFF;
constexpr uint8_t JPEG_SOI_MARKER_2 = 0xD8;
constexpr uint8_t JPEG_PSEUDO_COMPRESSION_SEED = 37;

std::vector<uint8_t> generate_test_image(size_t width, size_t height) {
    size_t compressed_size = (width * height * 3) / 5;
    std::vector<uint8_t> jpeg_data(compressed_size);
    
    jpeg_data[0] = JPEG_SOI_MARKER_1;
    jpeg_data[1] = JPEG_SOI_MARKER_2;
    
    for (size_t i = 2; i < compressed_size; ++i) {
        jpeg_data[i] = static_cast<uint8_t>((i * JPEG_PSEUDO_COMPRESSION_SEED) % 256);
    }
    
    return jpeg_data;
}

// ============================================================================
// Enhanced Mock Plugin with Realistic Latency Simulation
// ============================================================================

class RealisticLatencyPlugin : public IImageAnalysisBackend {
public:
    RealisticLatencyPlugin() : initialized_(false), warmup_done_(false) {}
    
    PluginInfo getInfo() const override {
        return {
            .name = "RealisticLatencyPlugin",
            .version = "1.0.0",
            .description = "Plugin with realistic latency simulation",
            .author = "ThemisDB Team",
            .license = "Apache-2.0",
            .model_name = "latency-test-clip",
            .model_version = "1.0",
            .supported_formats = {"jpeg", "png"},
            .capabilities = {
                .supports_embedding = true,
                .supports_captioning = true,
                .supports_batch_processing = true,
                .thread_safe = true,
                .supported_backends = {BackendType::CPU, BackendType::CUDA},
                .min_memory_mb = 512,
                .recommended_memory_mb = 1024
            }
        };
    }
    
    bool initialize(const PluginConfig& config, BackendType backend) override {
        backend_ = backend;
        initialized_ = true;
        warmup_done_ = false;
        return true;
    }
    
    void shutdown() override {
        initialized_ = false;
        warmup_done_ = false;
    }
    
    bool isReady() const override {
        return initialized_;
    }
    
    BackendType getBackend() const override {
        return backend_;
    }
    
    EmbeddingResult generateEmbedding(
        const std::vector<uint8_t>& image_data,
        const ImageMetadata* metadata
    ) override {
        EmbeddingResult result;
        result.success = true;
        result.dimension = EMBEDDING_DIMENSION;
        result.embedding.resize(EMBEDDING_DIMENSION);
        
        // Simulate realistic computation with latency variation
        size_t work_units = image_data.size() / 1024;
        
        // Add extra latency for cold start
        if (!warmup_done_) {
            work_units *= COLD_START_MULTIPLIER;
            warmup_done_ = true;
        }
        
        // Simulate GPU vs CPU latency difference
        if (backend_ == BackendType::CUDA) {
            work_units /= GPU_SPEEDUP_FACTOR;
        }
        
        // Actual computation simulation
        simulate_computation(work_units);
        
        // Generate embedding
        uint64_t hash = 0;
        for (size_t i = 0; i < std::min(image_data.size(), HASH_SAMPLE_SIZE); i += 10) {
            hash = hash * HASH_MULTIPLIER + image_data[i];
        }
        
        for (size_t i = 0; i < EMBEDDING_DIMENSION; ++i) {
            result.embedding[i] = static_cast<float>((hash + i) % EMBEDDING_VALUE_RANGE) / 
                                 static_cast<float>(EMBEDDING_VALUE_RANGE) - 0.5f;
        }
        
        // Normalize
        float norm = 0.0f;
        for (float v : result.embedding) {
            norm += v * v;
        }
        norm = std::sqrt(norm);
        
        if (norm > 0) {
            for (float& v : result.embedding) {
                v /= norm;
            }
        }
        
        result.model_name = "latency-test-clip";
        result.inference_time_ms = 15;
        
        return result;
    }
    
    CaptionResult generateCaption(
        const std::vector<uint8_t>& image_data,
        const ImageMetadata* metadata,
        int max_length
    ) override {
        CaptionResult result;
        result.success = true;
        
        // Caption generation is more expensive than embedding
        size_t work_units = (image_data.size() / 512) * max_length;
        
        if (!warmup_done_) {
            work_units *= COLD_START_MULTIPLIER;
            warmup_done_ = true;
        }
        
        if (backend_ == BackendType::CUDA) {
            work_units /= GPU_SPEEDUP_FACTOR;
        }
        
        simulate_computation(work_units);
        
        result.caption = "A realistic test image for latency benchmarking";
        result.confidence = 0.88f;
        result.model_name = "latency-test-caption";
        result.inference_time_ms = 80;
        
        return result;
    }
    
    std::vector<EmbeddingResult> generateEmbeddingBatch(
        const std::vector<std::vector<uint8_t>>& images
    ) override {
        std::vector<EmbeddingResult> results;
        results.reserve(images.size());
        
        for (const auto& img : images) {
            results.push_back(generateEmbedding(img, nullptr));
        }
        
        return results;
    }
    
    bool healthCheck() const override {
        return initialized_;
    }
    
    nlohmann::json getStatistics() const override {
        return {
            {"backend", static_cast<int>(backend_)},
            {"is_ready", initialized_},
            {"warmup_done", warmup_done_}
        };
    }
    
    void warmup() override {
        std::vector<uint8_t> dummy_image = generate_test_image(224, 224);
        generateEmbedding(dummy_image, nullptr);
        warmup_done_ = true;
    }
    
private:
    bool initialized_;
    bool warmup_done_;
    BackendType backend_;
};

// ============================================================================
// Latency Distribution Benchmarks
// ============================================================================

// Benchmark: Cold Start vs Warm Cache (Embedding)
static void BM_Embedding_ColdStartVsWarm(benchmark::State& state) {
    bool cold_start = (state.range(0) == 0);
    
    RealisticLatencyPlugin plugin;
    PluginConfig config;
    plugin.initialize(config, BackendType::CPU);
    
    if (!cold_start) {
        plugin.warmup();
    }
    
    auto image = generate_test_image(224, 224);
    
    for (auto _ : state) {
        auto result = plugin.generateEmbedding(image, nullptr);
        benchmark::DoNotOptimize(result);
        
        // Reinitialize for cold start test
        if (cold_start) {
            plugin.shutdown();
            plugin.initialize(config, BackendType::CPU);
        }
    }
    
    state.SetLabel(cold_start ? "ColdStart" : "WarmCache");
}
BENCHMARK(BM_Embedding_ColdStartVsWarm)
    ->Arg(0)  // Cold start
    ->Arg(1)  // Warm cache
    ->Unit(benchmark::kMillisecond);

// Benchmark: Latency Distribution for Standard Image Size
static void BM_Embedding_LatencyDistribution_224(benchmark::State& state) {
    RealisticLatencyPlugin plugin;
    PluginConfig config;
    plugin.initialize(config, BackendType::CPU);
    plugin.warmup();
    
    auto image = generate_test_image(224, 224);
    
    std::vector<double> latencies;
    latencies.reserve(state.max_iterations);
    
    for (auto _ : state) {
        auto start = std::chrono::high_resolution_clock::now();
        auto result = plugin.generateEmbedding(image, nullptr);
        auto end = std::chrono::high_resolution_clock::now();
        
        benchmark::DoNotOptimize(result);
        
        auto duration_ms = std::chrono::duration<double, std::milli>(end - start).count();
        latencies.push_back(duration_ms);
    }
    
    // Calculate percentiles
    if (!latencies.empty()) {
        std::sort(latencies.begin(), latencies.end());
        size_t p50_idx = latencies.size() * 50 / 100;
        size_t p95_idx = latencies.size() * 95 / 100;
        size_t p99_idx = latencies.size() * 99 / 100;
        
        state.counters["P50_ms"] = latencies[p50_idx];
        state.counters["P95_ms"] = latencies[p95_idx];
        state.counters["P99_ms"] = latencies[p99_idx];
        
        // Industry targets: P50<20ms, P95<35ms, P99<50ms on GPU
        // For CPU: P50<180ms, P95<320ms, P99<450ms
        double avg = std::accumulate(latencies.begin(), latencies.end(), 0.0) / latencies.size();
        state.counters["Avg_ms"] = avg;
    }
}
BENCHMARK(BM_Embedding_LatencyDistribution_224)
    ->Iterations(1000)
    ->Unit(benchmark::kMillisecond);

// Benchmark: GPU vs CPU Latency Comparison
static void BM_Embedding_GPUvsCPU_Latency(benchmark::State& state) {
    BackendType backend = (state.range(0) == 0) ? BackendType::CPU : BackendType::CUDA;
    
    RealisticLatencyPlugin plugin;
    PluginConfig config;
    plugin.initialize(config, backend);
    plugin.warmup();
    
    auto image = generate_test_image(384, 384);
    
    std::vector<double> latencies;
    latencies.reserve(state.max_iterations);
    
    for (auto _ : state) {
        auto start = std::chrono::high_resolution_clock::now();
        auto result = plugin.generateEmbedding(image, nullptr);
        auto end = std::chrono::high_resolution_clock::now();
        
        benchmark::DoNotOptimize(result);
        
        auto duration_ms = std::chrono::duration<double, std::milli>(end - start).count();
        latencies.push_back(duration_ms);
    }
    
    if (!latencies.empty()) {
        std::sort(latencies.begin(), latencies.end());
        state.counters["P50_ms"] = latencies[latencies.size() * 50 / 100];
        state.counters["P95_ms"] = latencies[latencies.size() * 95 / 100];
        state.counters["P99_ms"] = latencies[latencies.size() * 99 / 100];
    }
    
    state.SetLabel(backend == BackendType::CPU ? "CPU" : "CUDA");
}
BENCHMARK(BM_Embedding_GPUvsCPU_Latency)
    ->Arg(0)  // CPU
    ->Arg(1)  // CUDA
    ->Iterations(500)
    ->Unit(benchmark::kMillisecond);

// Benchmark: Caption Latency Distribution
static void BM_Caption_LatencyDistribution(benchmark::State& state) {
    RealisticLatencyPlugin plugin;
    PluginConfig config;
    plugin.initialize(config, BackendType::CPU);
    plugin.warmup();
    
    auto image = generate_test_image(224, 224);
    
    std::vector<double> latencies;
    latencies.reserve(state.max_iterations);
    
    for (auto _ : state) {
        auto start = std::chrono::high_resolution_clock::now();
        auto result = plugin.generateCaption(image, nullptr, 50);
        auto end = std::chrono::high_resolution_clock::now();
        
        benchmark::DoNotOptimize(result);
        
        auto duration_ms = std::chrono::duration<double, std::milli>(end - start).count();
        latencies.push_back(duration_ms);
    }
    
    if (!latencies.empty()) {
        std::sort(latencies.begin(), latencies.end());
        state.counters["P50_ms"] = latencies[latencies.size() * 50 / 100];
        state.counters["P95_ms"] = latencies[latencies.size() * 95 / 100];
        state.counters["P99_ms"] = latencies[latencies.size() * 99 / 100];
        
        // Industry target for caption: P50<100ms, P95<160ms, P99<220ms on GPU
        double avg = std::accumulate(latencies.begin(), latencies.end(), 0.0) / latencies.size();
        state.counters["Avg_ms"] = avg;
    }
}
BENCHMARK(BM_Caption_LatencyDistribution)
    ->Iterations(500)
    ->Unit(benchmark::kMillisecond);

// Benchmark: Batch Size vs Latency Per Image
static void BM_Batch_LatencyPerImage(benchmark::State& state) {
    size_t batch_size = state.range(0);
    
    RealisticLatencyPlugin plugin;
    PluginConfig config;
    plugin.initialize(config, BackendType::CUDA);
    plugin.warmup();
    
    std::vector<std::vector<uint8_t>> batch;
    for (size_t i = 0; i < batch_size; ++i) {
        batch.push_back(generate_test_image(224, 224));
    }
    
    std::vector<double> latencies_per_image;
    latencies_per_image.reserve(state.max_iterations);
    
    for (auto _ : state) {
        auto start = std::chrono::high_resolution_clock::now();
        auto results = plugin.generateEmbeddingBatch(batch);
        auto end = std::chrono::high_resolution_clock::now();
        
        benchmark::DoNotOptimize(results);
        
        auto total_ms = std::chrono::duration<double, std::milli>(end - start).count();
        latencies_per_image.push_back(total_ms / batch_size);
    }
    
    if (!latencies_per_image.empty()) {
        std::sort(latencies_per_image.begin(), latencies_per_image.end());
        state.counters["P50_ms_per_img"] = latencies_per_image[latencies_per_image.size() * 50 / 100];
        state.counters["P95_ms_per_img"] = latencies_per_image[latencies_per_image.size() * 95 / 100];
        
        double avg = std::accumulate(latencies_per_image.begin(), latencies_per_image.end(), 0.0) 
                     / latencies_per_image.size();
        state.counters["Avg_ms_per_img"] = avg;
        
        // Calculate efficiency ratio (batch vs individual)
        state.counters["batch_size"] = batch_size;
    }
}
BENCHMARK(BM_Batch_LatencyPerImage)
    ->Arg(1)
    ->Arg(4)
    ->Arg(8)
    ->Arg(16)
    ->Arg(32)
    ->Iterations(200)
    ->Unit(benchmark::kMillisecond);

// Benchmark: Image Size Impact on Latency
static void BM_ImageSize_LatencyImpact(benchmark::State& state) {
    size_t image_size = state.range(0);
    
    RealisticLatencyPlugin plugin;
    PluginConfig config;
    plugin.initialize(config, BackendType::CPU);
    plugin.warmup();
    
    auto image = generate_test_image(image_size, image_size);
    
    std::vector<double> latencies;
    latencies.reserve(state.max_iterations);
    
    for (auto _ : state) {
        auto start = std::chrono::high_resolution_clock::now();
        auto result = plugin.generateEmbedding(image, nullptr);
        auto end = std::chrono::high_resolution_clock::now();
        
        benchmark::DoNotOptimize(result);
        
        auto duration_ms = std::chrono::duration<double, std::milli>(end - start).count();
        latencies.push_back(duration_ms);
    }
    
    if (!latencies.empty()) {
        std::sort(latencies.begin(), latencies.end());
        state.counters["P50_ms"] = latencies[latencies.size() * 50 / 100];
        state.counters["P95_ms"] = latencies[latencies.size() * 95 / 100];
        state.counters["image_size"] = image_size;
        state.counters["megapixels"] = (image_size * image_size) / 1000000.0;
    }
}
BENCHMARK(BM_ImageSize_LatencyImpact)
    ->Arg(224)
    ->Arg(384)
    ->Arg(512)
    ->Arg(1024)
    ->Iterations(300)
    ->Unit(benchmark::kMillisecond);

// Benchmark: Sustained Load Latency (Testing for Performance Degradation)
static void BM_SustainedLoad_LatencyStability(benchmark::State& state) {
    RealisticLatencyPlugin plugin;
    PluginConfig config;
    plugin.initialize(config, BackendType::CPU);
    plugin.warmup();
    
    auto image = generate_test_image(224, 224);
    
    std::vector<double> early_latencies;
    std::vector<double> late_latencies;
    
    size_t iteration = 0;
    for (auto _ : state) {
        auto start = std::chrono::high_resolution_clock::now();
        auto result = plugin.generateEmbedding(image, nullptr);
        auto end = std::chrono::high_resolution_clock::now();
        
        benchmark::DoNotOptimize(result);
        
        auto duration_ms = std::chrono::duration<double, std::milli>(end - start).count();
        
        // Collect early and late samples
        if (iteration < 100) {
            early_latencies.push_back(duration_ms);
        } else if (iteration >= state.max_iterations - 100) {
            late_latencies.push_back(duration_ms);
        }
        
        iteration++;
    }
    
    // Compare early vs late latencies to detect degradation
    if (!early_latencies.empty() && !late_latencies.empty()) {
        double early_avg = std::accumulate(early_latencies.begin(), early_latencies.end(), 0.0) 
                          / early_latencies.size();
        double late_avg = std::accumulate(late_latencies.begin(), late_latencies.end(), 0.0) 
                         / late_latencies.size();
        
        state.counters["Early_Avg_ms"] = early_avg;
        state.counters["Late_Avg_ms"] = late_avg;
        state.counters["Degradation_pct"] = ((late_avg - early_avg) / early_avg) * 100.0;
        
        // Target: <5% degradation over sustained load
    }
}
BENCHMARK(BM_SustainedLoad_LatencyStability)
    ->Iterations(1000)
    ->Unit(benchmark::kMillisecond);

BENCHMARK_MAIN();
