/**
 * @file bench_stream_protocol.cpp
 * @brief Google Benchmark suite for Stream Protocol (v1.3.0 Phase 2)
 * 
 * This benchmark file provides performance testing for:
 * - Network throughput (MB/s)
 * - Round-trip latency
 * - Compression performance and ratios
 * - Encryption overhead
 * - Chunk serialization/deserialization speed
 */

#include <benchmark/benchmark.h>
#include "sharding/stream_protocol.h"
#include <random>
#include <vector>

using namespace themisdb::streaming;

// ============================================================================
// Helper Functions
// ============================================================================

/**
 * @brief Generate random data for benchmarking
 */
static std::vector<uint8_t> generateRandomData(size_t size) {
    std::vector<uint8_t> data(size);
    std::mt19937 gen(42);  // Fixed seed for reproducibility
    std::uniform_int_distribution<> dis(0, 255);
    
    for (auto& byte : data) {
        byte = static_cast<uint8_t>(dis(gen));
    }
    
    return data;
}

/**
 * @brief Generate compressible data (repeated patterns)
 */
static std::vector<uint8_t> generateCompressibleData(size_t size) {
    std::vector<uint8_t> data(size);
    for (size_t i = 0; i < size; ++i) {
        data[i] = static_cast<uint8_t>(i % 256);
    }
    return data;
}

// ============================================================================
// Chunk Serialization Benchmarks
// ============================================================================

/**
 * @benchmark Chunk serialization performance
 */
static void BM_StreamProtocol_ChunkSerialization(benchmark::State& state) {
    StreamChunk chunk;
    chunk.file_offset = 1024;
    chunk.chunk_index = 5;
    chunk.uncompressed_size = state.range(0);
    chunk.compressed_size = state.range(0);
    chunk.data = generateRandomData(state.range(0));
    chunk.checksum = 0x12345678;
    
    for (auto _ : state) {
        auto serialized = chunk.serialize();
        benchmark::DoNotOptimize(serialized);
    }
    
    state.SetBytesProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_StreamProtocol_ChunkSerialization)
    ->Arg(1024)    // 1 KB
    ->Arg(4096)    // 4 KB
    ->Arg(65536)   // 64 KB
    ->Arg(1048576) // 1 MB
    ->Unit(benchmark::kMicrosecond);

/**
 * @benchmark Chunk deserialization performance
 */
static void BM_StreamProtocol_ChunkDeserialization(benchmark::State& state) {
    StreamChunk chunk;
    chunk.file_offset = 1024;
    chunk.chunk_index = 5;
    chunk.uncompressed_size = state.range(0);
    chunk.compressed_size = state.range(0);
    chunk.data = generateRandomData(state.range(0));
    chunk.checksum = 0x12345678;
    
    auto serialized = chunk.serialize();
    
    for (auto _ : state) {
        auto deserialized = StreamChunk::deserialize(serialized);
        benchmark::DoNotOptimize(deserialized);
    }
    
    state.SetBytesProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_StreamProtocol_ChunkDeserialization)
    ->Arg(1024)
    ->Arg(4096)
    ->Arg(65536)
    ->Arg(1048576)
    ->Unit(benchmark::kMicrosecond);

// ============================================================================
// Compression Performance Benchmarks
// ============================================================================

/**
 * @benchmark LZ4 compression throughput
 */
static void BM_StreamProtocol_LZ4Compression(benchmark::State& state) {
    auto data = generateCompressibleData(state.range(0));
    
    for (auto _ : state) {
        auto compressed = StreamCompressor::compress(data, CompressionAlgorithm::LZ4);
        benchmark::DoNotOptimize(compressed);
    }
    
    state.SetBytesProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_StreamProtocol_LZ4Compression)
    ->Range(1024, 1048576)  // 1 KB to 1 MB
    ->Unit(benchmark::kMicrosecond);

/**
 * @benchmark LZ4 decompression throughput
 */
static void BM_StreamProtocol_LZ4Decompression(benchmark::State& state) {
    auto data = generateCompressibleData(state.range(0));
    auto compressed = StreamCompressor::compress(data, CompressionAlgorithm::LZ4);
    
    for (auto _ : state) {
        auto decompressed = StreamCompressor::decompress(
            compressed, CompressionAlgorithm::LZ4, data.size());
        benchmark::DoNotOptimize(decompressed);
    }
    
    state.SetBytesProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_StreamProtocol_LZ4Decompression)
    ->Range(1024, 1048576)
    ->Unit(benchmark::kMicrosecond);

/**
 * @benchmark Zstd compression throughput
 */
static void BM_StreamProtocol_ZstdCompression(benchmark::State& state) {
    auto data = generateCompressibleData(state.range(0));
    
    for (auto _ : state) {
        auto compressed = StreamCompressor::compress(data, CompressionAlgorithm::ZSTD);
        benchmark::DoNotOptimize(compressed);
    }
    
    state.SetBytesProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_StreamProtocol_ZstdCompression)
    ->Range(1024, 1048576)
    ->Unit(benchmark::kMicrosecond);

/**
 * @benchmark Zstd decompression throughput
 */
static void BM_StreamProtocol_ZstdDecompression(benchmark::State& state) {
    auto data = generateCompressibleData(state.range(0));
    auto compressed = StreamCompressor::compress(data, CompressionAlgorithm::ZSTD);
    
    for (auto _ : state) {
        auto decompressed = StreamCompressor::decompress(
            compressed, CompressionAlgorithm::ZSTD, data.size());
        benchmark::DoNotOptimize(decompressed);
    }
    
    state.SetBytesProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_StreamProtocol_ZstdDecompression)
    ->Range(1024, 1048576)
    ->Unit(benchmark::kMicrosecond);

/**
 * @benchmark Compression ratio measurement
 */
static void BM_StreamProtocol_CompressionRatio(benchmark::State& state) {
    auto data = generateCompressibleData(1048576); // 1 MB
    
    size_t original_size = 0;
    size_t compressed_size = 0;
    
    for (auto _ : state) {
        auto compressed = StreamCompressor::compress(data, CompressionAlgorithm::LZ4);
        original_size += data.size();
        compressed_size += compressed.size();
        benchmark::DoNotOptimize(compressed);
    }
    
    double ratio = static_cast<double>(original_size) / compressed_size;
    state.counters["CompressionRatio"] = ratio;
    state.SetBytesProcessed(state.iterations() * data.size());
}
BENCHMARK(BM_StreamProtocol_CompressionRatio)->Unit(benchmark::kMillisecond);

// ============================================================================
// Encryption Performance Benchmarks
// ============================================================================

/**
 * @benchmark AES-256-GCM encryption throughput
 */
static void BM_StreamProtocol_Encryption(benchmark::State& state) {
    auto data = generateRandomData(state.range(0));
    std::vector<uint8_t> key(32, 0xAB); // 256-bit key
    std::vector<uint8_t> iv(12, 0xCD);  // 96-bit IV
    
    for (auto _ : state) {
        auto encrypted = StreamEncryptor::encrypt(data, key, iv);
        benchmark::DoNotOptimize(encrypted);
    }
    
    state.SetBytesProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_StreamProtocol_Encryption)
    ->Range(1024, 1048576)
    ->Unit(benchmark::kMicrosecond);

/**
 * @benchmark AES-256-GCM decryption throughput
 */
static void BM_StreamProtocol_Decryption(benchmark::State& state) {
    auto data = generateRandomData(state.range(0));
    std::vector<uint8_t> key(32, 0xAB);
    std::vector<uint8_t> iv(12, 0xCD);
    
    auto encrypted = StreamEncryptor::encrypt(data, key, iv);
    
    for (auto _ : state) {
        auto decrypted = StreamEncryptor::decrypt(encrypted, key, iv);
        benchmark::DoNotOptimize(decrypted);
    }
    
    state.SetBytesProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_StreamProtocol_Decryption)
    ->Range(1024, 1048576)
    ->Unit(benchmark::kMicrosecond);

/**
 * @benchmark Encryption overhead measurement
 */
static void BM_StreamProtocol_EncryptionOverhead(benchmark::State& state) {
    auto data = generateRandomData(65536); // 64 KB
    std::vector<uint8_t> key(32, 0xAB);
    std::vector<uint8_t> iv(12, 0xCD);
    
    for (auto _ : state) {
        auto encrypted = StreamEncryptor::encrypt(data, key, iv);
        benchmark::DoNotOptimize(encrypted);
    }
    
    state.SetBytesProcessed(state.iterations() * data.size());
}
BENCHMARK(BM_StreamProtocol_EncryptionOverhead)->Unit(benchmark::kMicrosecond);

// ============================================================================
// Network Throughput Simulation Benchmarks
// ============================================================================

/**
 * @benchmark Simulated network throughput with compression
 */
static void BM_StreamProtocol_NetworkThroughputWithCompression(benchmark::State& state) {
    auto data = generateCompressibleData(state.range(0));
    
    for (auto _ : state) {
        // Simulate: Compress -> Serialize -> Deserialize -> Decompress
        auto compressed = StreamCompressor::compress(data, CompressionAlgorithm::LZ4);
        
        StreamChunk chunk;
        chunk.data = compressed;
        chunk.uncompressed_size = data.size();
        chunk.compressed_size = compressed.size();
        
        auto serialized = chunk.serialize();
        auto deserialized = StreamChunk::deserialize(serialized);
        
        if (deserialized.has_value()) {
            auto decompressed = StreamCompressor::decompress(
                deserialized->data, CompressionAlgorithm::LZ4, data.size());
            benchmark::DoNotOptimize(decompressed);
        }
    }
    
    state.SetBytesProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_StreamProtocol_NetworkThroughputWithCompression)
    ->Arg(65536)    // 64 KB
    ->Arg(262144)   // 256 KB
    ->Arg(1048576)  // 1 MB
    ->Unit(benchmark::kMillisecond);

/**
 * @benchmark Simulated network throughput with encryption
 */
static void BM_StreamProtocol_NetworkThroughputWithEncryption(benchmark::State& state) {
    auto data = generateRandomData(state.range(0));
    std::vector<uint8_t> key(32, 0xAB);
    std::vector<uint8_t> iv(12, 0xCD);
    
    for (auto _ : state) {
        // Simulate: Encrypt -> Serialize -> Deserialize -> Decrypt
        auto encrypted = StreamEncryptor::encrypt(data, key, iv);
        
        StreamChunk chunk;
        chunk.data = encrypted;
        chunk.uncompressed_size = data.size();
        chunk.compressed_size = encrypted.size();
        
        auto serialized = chunk.serialize();
        auto deserialized = StreamChunk::deserialize(serialized);
        
        if (deserialized.has_value()) {
            auto decrypted = StreamEncryptor::decrypt(deserialized->data, key, iv);
            benchmark::DoNotOptimize(decrypted);
        }
    }
    
    state.SetBytesProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_StreamProtocol_NetworkThroughputWithEncryption)
    ->Arg(65536)
    ->Arg(262144)
    ->Arg(1048576)
    ->Unit(benchmark::kMillisecond);

/**
 * @benchmark Full pipeline: compress + encrypt
 */
static void BM_StreamProtocol_FullPipeline(benchmark::State& state) {
    auto data = generateCompressibleData(state.range(0));
    std::vector<uint8_t> key(32, 0xAB);
    std::vector<uint8_t> iv(12, 0xCD);
    
    for (auto _ : state) {
        // Compress
        auto compressed = StreamCompressor::compress(data, CompressionAlgorithm::LZ4);
        
        // Encrypt
        auto encrypted = StreamEncryptor::encrypt(compressed, key, iv);
        
        // Serialize
        StreamChunk chunk;
        chunk.data = encrypted;
        chunk.uncompressed_size = data.size();
        chunk.compressed_size = compressed.size();
        
        auto serialized = chunk.serialize();
        benchmark::DoNotOptimize(serialized);
    }
    
    state.SetBytesProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_StreamProtocol_FullPipeline)
    ->Arg(65536)
    ->Arg(262144)
    ->Arg(1048576)
    ->Unit(benchmark::kMillisecond);

// ============================================================================
// Rate Limiter Benchmarks
// ============================================================================

/**
 * @benchmark Rate limiter overhead
 */
static void BM_StreamProtocol_RateLimiterOverhead(benchmark::State& state) {
    auto rate_limiter = std::make_shared<StreamRateLimiter>(100 * 1024 * 1024); // 100 MB/s
    
    for (auto _ : state) {
        bool allowed = rate_limiter->allowTransfer(65536);
        benchmark::DoNotOptimize(allowed);
    }
}
BENCHMARK(BM_StreamProtocol_RateLimiterOverhead)->Unit(benchmark::kNanosecond);

/**
 * @benchmark Rate limiter with varying transfer sizes
 */
static void BM_StreamProtocol_RateLimiterVaryingSizes(benchmark::State& state) {
    auto rate_limiter = std::make_shared<StreamRateLimiter>(100 * 1024 * 1024);
    size_t transfer_size = state.range(0);
    
    for (auto _ : state) {
        bool allowed = rate_limiter->allowTransfer(transfer_size);
        benchmark::DoNotOptimize(allowed);
    }
    
    state.SetBytesProcessed(state.iterations() * transfer_size);
}
BENCHMARK(BM_StreamProtocol_RateLimiterVaryingSizes)
    ->Arg(1024)
    ->Arg(65536)
    ->Arg(1048576)
    ->Unit(benchmark::kNanosecond);

// ============================================================================
// Latency Benchmarks
// ============================================================================

/**
 * @benchmark Round-trip latency simulation (minimal processing)
 */
static void BM_StreamProtocol_RoundTripLatency(benchmark::State& state) {
    StreamChunk chunk;
    chunk.data = generateRandomData(4096); // 4 KB chunk
    chunk.uncompressed_size = 4096;
    chunk.compressed_size = 4096;
    chunk.checksum = 0x12345678;
    
    for (auto _ : state) {
        // Send: Serialize
        auto serialized = chunk.serialize();
        
        // Receive: Deserialize
        auto deserialized = StreamChunk::deserialize(serialized);
        
        // ACK: Serialize response
        StreamChunk ack_chunk;
        ack_chunk.chunk_index = chunk.chunk_index;
        auto ack_serialized = ack_chunk.serialize();
        
        benchmark::DoNotOptimize(ack_serialized);
    }
}
BENCHMARK(BM_StreamProtocol_RoundTripLatency)->Unit(benchmark::kMicrosecond);

// Main function for Google Benchmark
BENCHMARK_MAIN();
