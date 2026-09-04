/**
 * @file bench_lossy_vs_lossless.cpp (simplified v1.3.0)
 * @brief Benchmarks comparing lossy vs lossless compression patterns
 * 
 * Note: Advanced compression algorithms are for v1.4.0. This demonstrates
 * basic compression and dequantization patterns.
 */

#include <benchmark/benchmark.h>
#include <vector>
#include <random>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <zlib.h>
#include <cstring>

// ===== Basic Data Generation =====

static std::vector<float> generateRandomVector(size_t size) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.0, 100.0);
    
    std::vector<float> v(size);
    for (auto& x : v) {
        x = dis(gen);
    }
    return v;
}

static std::vector<uint8_t> vectorToBytes(const std::vector<float>& vec) {
    std::vector<uint8_t> bytes = {};

    bytes.reserve(vec.size() * sizeof(float));
    
    for (float v : vec) {
        uint32_t bits = 0;
        std::memcpy(&bits, &v, sizeof(float));
        
        bytes.push_back((bits >> 0) & 0xFF);
        bytes.push_back((bits >> 8) & 0xFF);
        bytes.push_back((bits >> 16) & 0xFF);
        bytes.push_back((bits >> 24) & 0xFF);
    }
    
    return bytes;
}

// ===== Compression Benchmarks =====

static void BM_Compression_Zlib_Small(benchmark::State& state) {
    std::vector<uint8_t> data = vectorToBytes(generateRandomVector(100));
    std::vector<uint8_t> compressed(compressBound(data.size()));
    uLongf compressedSize = compressed.size();
    
    for (auto _ : state) {
        compressedSize = compressed.size();
        compress(compressed.data(), &compressedSize, data.data(), data.size());
        benchmark::DoNotOptimize(compressedSize);
    }
    
    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(data.size()));
}
BENCHMARK(BM_Compression_Zlib_Small);

static void BM_Compression_Zlib_Medium(benchmark::State& state) {
    std::vector<uint8_t> data = vectorToBytes(generateRandomVector(1000));
    std::vector<uint8_t> compressed(compressBound(data.size()));
    uLongf compressedSize = compressed.size();
    
    for (auto _ : state) {
        compressedSize = compressed.size();
        compress(compressed.data(), &compressedSize, data.data(), data.size());
        benchmark::DoNotOptimize(compressedSize);
    }
    
    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(data.size()));
}
BENCHMARK(BM_Compression_Zlib_Medium);

static void BM_Compression_Zlib_Large(benchmark::State& state) {
    std::vector<uint8_t> data = vectorToBytes(generateRandomVector(10000));
    std::vector<uint8_t> compressed(compressBound(data.size()));
    uLongf compressedSize = compressed.size();
    
    for (auto _ : state) {
        compressedSize = compressed.size();
        compress(compressed.data(), &compressedSize, data.data(), data.size());
        benchmark::DoNotOptimize(compressedSize);
    }
    
    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(data.size()));
}
BENCHMARK(BM_Compression_Zlib_Large);

// ===== Decompression Benchmarks =====

static void BM_Decompression_Zlib(benchmark::State& state) {
    std::vector<uint8_t> original = vectorToBytes(generateRandomVector(1000));
    std::vector<uint8_t> compressed(compressBound(original.size()));
    uLongf compressedSize = compressed.size();
    compress(compressed.data(), &compressedSize, original.data(), original.size());
    
    std::vector<uint8_t> decompressed(original.size());
    uLongf decompressedSize = decompressed.size();
    
    for (auto _ : state) {
        decompressedSize = original.size();
        uncompress(decompressed.data(), &decompressedSize, 
                  compressed.data(), compressedSize);
        benchmark::DoNotOptimize(decompressedSize);
    }
    
    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(decompressed.size()));
}
BENCHMARK(BM_Decompression_Zlib);

// ===== Quantization Simulation Benchmarks =====

static void BM_Quantization_Int8(benchmark::State& state) {
    std::vector<float> vec = generateRandomVector(1000);
    
    // Find min/max
    float min_val = *std::min_element(vec.begin(), vec.end());
    float max_val = *std::max_element(vec.begin(), vec.end());
    float scale = (max_val - min_val) / 255.0f;
    
    std::vector<int8_t> quantized(vec.size());
    
    for (auto _ : state) {
        for (size_t i = 0; i < vec.size(); ++i) {
            int8_t q = static_cast<int8_t>((vec[i] - min_val) / scale);
            quantized[i] = q;
        }
        benchmark::DoNotOptimize(quantized);
    }
    
    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(vec.size()) * static_cast<int64_t>(sizeof(float)));
}
BENCHMARK(BM_Quantization_Int8);

static void BM_Dequantization_Int8(benchmark::State& state) {
    std::vector<int8_t> quantized(1000);
    for (int i = 0; i < 1000; ++i) {
        quantized[i] = i % 256;
    }
    
    float min_val = 0.0f;
    float scale = 100.0f / 255.0f;
    
    std::vector<float> recovered(quantized.size());
    
    for (auto _ : state) {
        for (size_t i = 0; i < quantized.size(); ++i) {
            recovered[i] = min_val + quantized[i] * scale;
        }
        benchmark::DoNotOptimize(recovered);
    }
    
    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(quantized.size()) * static_cast<int64_t>(sizeof(int8_t)));
}
BENCHMARK(BM_Dequantization_Int8);

// ===== Memory Footprint Estimation =====

static void BM_CompressionRatio_Analysis(benchmark::State& state) {
    std::vector<float> original = generateRandomVector(10000);
    std::vector<uint8_t> data = vectorToBytes(original);
    
    // Test different compression scenarios
    for (auto _ : state) {
        // Uncompressed size
        size_t uncompressed_bytes = data.size();
        
        // Quantized size (int8 instead of float32)
        size_t quantized_bytes = original.size() * sizeof(int8_t);
        
        // Compressed size estimate (typical is 30-50% for random data)
        std::vector<uint8_t> compressed(compressBound(data.size()));
        uLongf compressedSize = compressed.size();
        compress(compressed.data(), &compressedSize, data.data(), data.size());
        
        double ratio_quantized = static_cast<double>(quantized_bytes) / static_cast<double>(uncompressed_bytes);
        double ratio_compressed = static_cast<double>(compressedSize) / static_cast<double>(uncompressed_bytes);
        
        benchmark::DoNotOptimize(ratio_quantized);
        benchmark::DoNotOptimize(ratio_compressed);
    }
}
BENCHMARK(BM_CompressionRatio_Analysis);

// ===== Main =====

int main(int argc, char** argv) {
    ::benchmark::Initialize(&argc, argv);
    if (::benchmark::ReportUnrecognizedArguments(argc, argv)) {
        return 1;
    }
    
    ::benchmark::RunSpecifiedBenchmarks();
    ::benchmark::Shutdown();
    
    return 0;
}
