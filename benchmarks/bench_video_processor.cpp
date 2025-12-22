/**
 * @file bench_video_processor.cpp
 * @brief Google Benchmark suite for Video Processor (v1.3.0 Phase 2)
 * 
 * This benchmark file provides performance testing for:
 * - Video processing throughput (FPS)
 * - Keyframe extraction speed
 * - Scene detection performance
 * - Thumbnail generation speed
 * - Multiple format handling performance
 */

#include <benchmark/benchmark.h>
#include "content/video_processor.h"
#include <vector>
#include <random>

using namespace themis::content;

// ============================================================================
// Helper Functions
// ============================================================================

/**
 * @brief Generate minimal MP4 video data for benchmarking
 */
static std::vector<uint8_t> generateMinimalMp4(size_t size_kb) {
    std::vector<uint8_t> data;
    data.reserve(size_kb * 1024);
    
    // ftyp box (file type box)
    std::vector<uint8_t> ftyp = {
        0x00, 0x00, 0x00, 0x20, // box size
        'f', 't', 'y', 'p',      // box type
        'i', 's', 'o', 'm',      // major brand
        0x00, 0x00, 0x02, 0x00,  // minor version
        'i', 's', 'o', 'm', 'i', 's', 'o', '2',
        'a', 'v', 'c', '1', 'm', 'p', '4', '1'
    };
    
    data.insert(data.end(), ftyp.begin(), ftyp.end());
    
    // Pad to requested size
    data.resize(size_kb * 1024, 0x00);
    
    return data;
}

// ============================================================================
// Processing Throughput Benchmarks
// ============================================================================

/**
 * @benchmark Video metadata extraction throughput
 */
static void BM_VideoProcessor_MetadataExtraction(benchmark::State& state) {
    VideoProcessor processor;
    PluginConfig config;
    processor.initialize(config);
    
    size_t file_size_kb = state.range(0);
    auto video_data = generateMinimalMp4(file_size_kb);
    
    ExtractionOptions options;
    options.extract_metadata = true;
    
    for (auto _ : state) {
        auto result = processor.extract(video_data, "video/mp4", options);
        benchmark::DoNotOptimize(result);
    }
    
    state.SetBytesProcessed(state.iterations() * video_data.size());
    state.SetItemsProcessed(state.iterations());
    
    processor.shutdown();
}
BENCHMARK(BM_VideoProcessor_MetadataExtraction)
    ->Args({100})    // 100 KB
    ->Args({1024})   // 1 MB
    ->Args({10240})  // 10 MB
    ->Unit(benchmark::kMillisecond);

/**
 * @benchmark Full video processing with all features
 */
static void BM_VideoProcessor_FullProcessing(benchmark::State& state) {
    VideoProcessor processor;
    PluginConfig config;
    config.set("keyframes.max_count", 10);
    config.set("scene_detection.enabled", true);
    processor.initialize(config);
    
    size_t file_size_kb = state.range(0);
    auto video_data = generateMinimalMp4(file_size_kb);
    
    ExtractionOptions options;
    options.extract_metadata = true;
    options.extract_keyframes = true;
    options.extract_scenes = true;
    options.generate_thumbnail = true;
    
    for (auto _ : state) {
        auto result = processor.extract(video_data, "video/mp4", options);
        benchmark::DoNotOptimize(result);
    }
    
    state.SetBytesProcessed(state.iterations() * video_data.size());
    state.SetItemsProcessed(state.iterations());
    
    processor.shutdown();
}
BENCHMARK(BM_VideoProcessor_FullProcessing)
    ->Args({100})
    ->Args({1024})
    ->Args({5120})
    ->Unit(benchmark::kMillisecond);

// ============================================================================
// Keyframe Extraction Benchmarks
// ============================================================================

/**
 * @benchmark Keyframe extraction speed
 */
static void BM_VideoProcessor_KeyframeExtraction(benchmark::State& state) {
    VideoProcessor processor;
    PluginConfig config;
    config.set("keyframes.max_count", state.range(1));
    processor.initialize(config);
    
    size_t file_size_kb = state.range(0);
    auto video_data = generateMinimalMp4(file_size_kb);
    
    ExtractionOptions options;
    options.extract_keyframes = true;
    
    for (auto _ : state) {
        auto result = processor.extract(video_data, "video/mp4", options);
        benchmark::DoNotOptimize(result);
    }
    
    state.SetBytesProcessed(state.iterations() * video_data.size());
    state.SetItemsProcessed(state.iterations());
    
    processor.shutdown();
}
BENCHMARK(BM_VideoProcessor_KeyframeExtraction)
    ->Args({1024, 5})     // 1 MB, 5 keyframes
    ->Args({1024, 10})    // 1 MB, 10 keyframes
    ->Args({1024, 20})    // 1 MB, 20 keyframes
    ->Args({5120, 10})    // 5 MB, 10 keyframes
    ->Unit(benchmark::kMillisecond);

/**
 * @benchmark Keyframe extraction with varying video sizes
 */
static void BM_VideoProcessor_KeyframeScalability(benchmark::State& state) {
    VideoProcessor processor;
    PluginConfig config;
    config.set("keyframes.max_count", 10);
    processor.initialize(config);
    
    size_t file_size_kb = state.range(0);
    auto video_data = generateMinimalMp4(file_size_kb);
    
    ExtractionOptions options;
    options.extract_keyframes = true;
    
    for (auto _ : state) {
        auto result = processor.extract(video_data, "video/mp4", options);
        benchmark::DoNotOptimize(result);
    }
    
    state.SetBytesProcessed(state.iterations() * video_data.size());
    
    processor.shutdown();
}
BENCHMARK(BM_VideoProcessor_KeyframeScalability)
    ->Range(100, 10240)  // 100 KB to 10 MB
    ->Unit(benchmark::kMillisecond);

// ============================================================================
// Scene Detection Benchmarks
// ============================================================================

/**
 * @benchmark Scene detection performance
 */
static void BM_VideoProcessor_SceneDetection(benchmark::State& state) {
    VideoProcessor processor;
    PluginConfig config;
    config.set("scene_detection.enabled", true);
    config.set("scene_detection.threshold", 0.3);
    processor.initialize(config);
    
    size_t file_size_kb = state.range(0);
    auto video_data = generateMinimalMp4(file_size_kb);
    
    ExtractionOptions options;
    options.extract_scenes = true;
    
    for (auto _ : state) {
        auto result = processor.extract(video_data, "video/mp4", options);
        benchmark::DoNotOptimize(result);
    }
    
    state.SetBytesProcessed(state.iterations() * video_data.size());
    state.SetItemsProcessed(state.iterations());
    
    processor.shutdown();
}
BENCHMARK(BM_VideoProcessor_SceneDetection)
    ->Args({1024})
    ->Args({5120})
    ->Args({10240})
    ->Unit(benchmark::kMillisecond);

/**
 * @benchmark Scene detection with different thresholds
 */
static void BM_VideoProcessor_SceneDetectionThreshold(benchmark::State& state) {
    VideoProcessor processor;
    PluginConfig config;
    config.set("scene_detection.enabled", true);
    config.set("scene_detection.threshold", state.range(1) / 100.0);
    processor.initialize(config);
    
    size_t file_size_kb = state.range(0);
    auto video_data = generateMinimalMp4(file_size_kb);
    
    ExtractionOptions options;
    options.extract_scenes = true;
    
    for (auto _ : state) {
        auto result = processor.extract(video_data, "video/mp4", options);
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations());
    
    processor.shutdown();
}
BENCHMARK(BM_VideoProcessor_SceneDetectionThreshold)
    ->Args({5120, 20})   // 5 MB, threshold 0.2
    ->Args({5120, 30})   // 5 MB, threshold 0.3
    ->Args({5120, 40})   // 5 MB, threshold 0.4
    ->Unit(benchmark::kMillisecond);

// ============================================================================
// Thumbnail Generation Benchmarks
// ============================================================================

/**
 * @benchmark Thumbnail generation speed
 */
static void BM_VideoProcessor_ThumbnailGeneration(benchmark::State& state) {
    VideoProcessor processor;
    PluginConfig config;
    config.set("thumbnail.max_width", state.range(1));
    config.set("thumbnail.max_height", state.range(1) * 3 / 4);  // 4:3 aspect ratio
    processor.initialize(config);
    
    size_t file_size_kb = state.range(0);
    auto video_data = generateMinimalMp4(file_size_kb);
    
    ExtractionOptions options;
    options.generate_thumbnail = true;
    
    for (auto _ : state) {
        auto result = processor.extract(video_data, "video/mp4", options);
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations());
    
    processor.shutdown();
}
BENCHMARK(BM_VideoProcessor_ThumbnailGeneration)
    ->Args({1024, 160})   // 1 MB, 160px width
    ->Args({1024, 320})   // 1 MB, 320px width
    ->Args({1024, 640})   // 1 MB, 640px width
    ->Args({5120, 320})   // 5 MB, 320px width
    ->Unit(benchmark::kMillisecond);

/**
 * @benchmark Thumbnail generation with various sizes
 */
static void BM_VideoProcessor_ThumbnailSizeVariations(benchmark::State& state) {
    VideoProcessor processor;
    PluginConfig config;
    config.set("thumbnail.max_width", state.range(0));
    config.set("thumbnail.max_height", state.range(0) * 3 / 4);
    processor.initialize(config);
    
    auto video_data = generateMinimalMp4(1024);  // 1 MB video
    
    ExtractionOptions options;
    options.generate_thumbnail = true;
    
    for (auto _ : state) {
        auto result = processor.extract(video_data, "video/mp4", options);
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations());
    
    processor.shutdown();
}
BENCHMARK(BM_VideoProcessor_ThumbnailSizeVariations)
    ->Arg(80)    // 80px
    ->Arg(160)   // 160px
    ->Arg(320)   // 320px
    ->Arg(640)   // 640px
    ->Arg(1280)  // 1280px
    ->Unit(benchmark::kMillisecond);

// ============================================================================
// Multiple Format Handling Benchmarks
// ============================================================================

/**
 * @benchmark MP4 format processing
 */
static void BM_VideoProcessor_MP4Format(benchmark::State& state) {
    VideoProcessor processor;
    PluginConfig config;
    processor.initialize(config);
    
    auto video_data = generateMinimalMp4(1024);
    
    ExtractionOptions options;
    options.extract_metadata = true;
    
    for (auto _ : state) {
        auto result = processor.extract(video_data, "video/mp4", options);
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations());
    
    processor.shutdown();
}
BENCHMARK(BM_VideoProcessor_MP4Format)->Unit(benchmark::kMillisecond);

/**
 * @benchmark WebM format processing
 */
static void BM_VideoProcessor_WebMFormat(benchmark::State& state) {
    VideoProcessor processor;
    PluginConfig config;
    processor.initialize(config);
    
    auto video_data = generateMinimalMp4(1024);
    
    ExtractionOptions options;
    options.extract_metadata = true;
    
    for (auto _ : state) {
        auto result = processor.extract(video_data, "video/webm", options);
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations());
    
    processor.shutdown();
}
BENCHMARK(BM_VideoProcessor_WebMFormat)->Unit(benchmark::kMillisecond);

/**
 * @benchmark MKV format processing
 */
static void BM_VideoProcessor_MKVFormat(benchmark::State& state) {
    VideoProcessor processor;
    PluginConfig config;
    processor.initialize(config);
    
    auto video_data = generateMinimalMp4(1024);
    
    ExtractionOptions options;
    options.extract_metadata = true;
    
    for (auto _ : state) {
        auto result = processor.extract(video_data, "video/x-matroska", options);
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations());
    
    processor.shutdown();
}
BENCHMARK(BM_VideoProcessor_MKVFormat)->Unit(benchmark::kMillisecond);

// ============================================================================
// Parallel Processing Benchmarks
// ============================================================================

/**
 * @benchmark Concurrent video processing
 */
static void BM_VideoProcessor_ConcurrentProcessing(benchmark::State& state) {
    const int num_threads = state.range(0);
    
    for (auto _ : state) {
        state.PauseTiming();
        std::vector<VideoProcessor> processors(num_threads);
        std::vector<std::vector<uint8_t>> videos(num_threads);
        
        for (int i = 0; i < num_threads; ++i) {
            PluginConfig config;
            processors[i].initialize(config);
            videos[i] = generateMinimalMp4(1024);
        }
        state.ResumeTiming();
        
        // Process videos concurrently
        #pragma omp parallel for if(num_threads > 1)
        for (int i = 0; i < num_threads; ++i) {
            ExtractionOptions options;
            options.extract_metadata = true;
            auto result = processors[i].extract(videos[i], "video/mp4", options);
            benchmark::DoNotOptimize(result);
        }
        
        state.PauseTiming();
        for (auto& proc : processors) {
            proc.shutdown();
        }
        state.ResumeTiming();
    }
    
    state.SetItemsProcessed(state.iterations() * num_threads);
}
BENCHMARK(BM_VideoProcessor_ConcurrentProcessing)
    ->Arg(1)
    ->Arg(2)
    ->Arg(4)
    ->Arg(8)
    ->Unit(benchmark::kMillisecond);

// ============================================================================
// Plugin Lifecycle Benchmarks
// ============================================================================

/**
 * @benchmark Plugin initialization overhead
 */
static void BM_VideoProcessor_InitializationOverhead(benchmark::State& state) {
    for (auto _ : state) {
        VideoProcessor processor;
        PluginConfig config;
        
        auto start = std::chrono::high_resolution_clock::now();
        processor.initialize(config);
        auto end = std::chrono::high_resolution_clock::now();
        
        processor.shutdown();
        
        auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        state.SetIterationTime(elapsed.count() / 1e9);
    }
}
BENCHMARK(BM_VideoProcessor_InitializationOverhead)
    ->UseManualTime()
    ->Unit(benchmark::kMicrosecond);

/**
 * @benchmark Health check overhead
 */
static void BM_VideoProcessor_HealthCheckOverhead(benchmark::State& state) {
    VideoProcessor processor;
    PluginConfig config;
    processor.initialize(config);
    
    for (auto _ : state) {
        bool healthy = processor.healthCheck();
        benchmark::DoNotOptimize(healthy);
    }
    
    processor.shutdown();
}
BENCHMARK(BM_VideoProcessor_HealthCheckOverhead)->Unit(benchmark::kNanosecond);

/**
 * @benchmark Statistics retrieval overhead
 */
static void BM_VideoProcessor_StatisticsOverhead(benchmark::State& state) {
    VideoProcessor processor;
    PluginConfig config;
    processor.initialize(config);
    
    for (auto _ : state) {
        auto stats = processor.getStatistics();
        benchmark::DoNotOptimize(stats);
    }
    
    processor.shutdown();
}
BENCHMARK(BM_VideoProcessor_StatisticsOverhead)->Unit(benchmark::kNanosecond);

// Main function for Google Benchmark
BENCHMARK_MAIN();
