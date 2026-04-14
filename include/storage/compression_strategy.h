/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            compression_strategy.h                             ║
  Version:         0.0.40                                             ║
  Last Modified:   2026-04-14 06:56:49                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     305                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2386ec7ef9  2026-03-12  feat: implement GPU-accelerated compression (Zstd/Snappy/... ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "utils/compression_metrics.h"
#include "utils/zstd_codec.h"
#include "utils/lossless_vector_compression.h"
#include "storage/gpu_compression.h"
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <optional>

namespace themis {
namespace compression {

/**
 * @brief Compression method types
 */
enum class CompressionMethod {
    NONE,           // No compression
    ZSTD,           // Zstandard general-purpose compression
    LZ4,            // LZ4 fast compression (if available)
    SNAPPY,         // Snappy compression (if available)
    RLE,            // Run-Length Encoding
    DELTA,          // Delta encoding
    DICTIONARY,     // Dictionary encoding
    SPARSE_CSR,     // Sparse vector compression
    ADAPTIVE,       // Auto-select best method

    // GPU-accelerated variants (CUDA/HIP with CPU fallback)
    GPU_ZSTD,       // Zstd via NVIDIA nvCOMP or HIP; CPU fallback
    GPU_SNAPPY,     // Snappy GPU-accelerated variant; CPU fallback
    GPU_LZ4,        // LZ4 with parallel GPU decompression; CPU fallback
};

/**
 * @brief Data type hints for adaptive compression
 */
enum class DataType {
    GENERIC,        // Unknown/generic binary data
    TEXT,           // Text data (UTF-8 strings)
    JSON,           // JSON documents
    VECTOR_DENSE,   // Dense float vectors
    VECTOR_SPARSE,  // Sparse vectors
    INTEGER_SEQ,    // Integer sequences
    CATEGORICAL,    // Categorical/enum data
    TIMESERIES      // Time-series data
};

/**
 * @brief Compression configuration
 */
struct CompressionConfig {
    CompressionMethod method = CompressionMethod::ADAPTIVE;
    DataType data_type = DataType::GENERIC;
    int level = 3;                  // Compression level (1-22 for ZSTD)
    size_t min_size = 128;          // Don't compress smaller data
    float sparse_threshold = 0.95f; // For sparse detection
    bool enable_metrics = true;     // Track performance metrics
    
    // Adaptive thresholds
    float adaptive_ratio_threshold = 1.2f;  // Min ratio to consider successful
    size_t adaptive_sample_size = 1024;     // Bytes to sample for method selection

    // GPU acceleration settings (used for GPU_ZSTD / GPU_SNAPPY / GPU_LZ4)
    themis::storage::GpuCompressionConfig gpu_config; // Forwarded to GpuCompressionManager
};

/**
 * @brief Compression result
 */
struct CompressionResult {
    std::vector<uint8_t> data;
    CompressionMethod method_used;
    size_t original_size;
    float compression_ratio;
    bool success;
    
    CompressionResult() 
        : method_used(CompressionMethod::NONE)
        , original_size(0)
        , compression_ratio(1.0f)
        , success(false)
    {}
};

/**
 * @brief Comprehensive compression strategy manager
 * 
 * Provides unified interface for multiple compression methods with
 * adaptive selection, performance tracking, and fallback strategies.
 */
class CompressionStrategyManager {
public:
    explicit CompressionStrategyManager(const CompressionConfig& config = CompressionConfig{});
    
    /**
     * @brief Compress data with configured or adaptive strategy
     */
    CompressionResult compress(
        const uint8_t* data,
        size_t size,
        std::optional<DataType> hint = std::nullopt
    );
    
    /**
     * @brief Compress vector data (convenience overload)
     */
    CompressionResult compress(
        const std::vector<uint8_t>& data,
        std::optional<DataType> hint = std::nullopt
    ) {
        return compress(data.data(), data.size(), hint);
    }
    
    /**
     * @brief Compress string data (convenience overload)
     */
    CompressionResult compress(
        const std::string& data,
        std::optional<DataType> hint = std::nullopt
    ) {
        return compress(
            reinterpret_cast<const uint8_t*>(data.data()),
            data.size(),
            hint
        );
    }
    
    /**
     * @brief Decompress data
     * 
     * @param data Compressed data
     * @param method Method used for compression
     * @return Decompressed data, or empty vector on failure
     */
    std::vector<uint8_t> decompress(
        const std::vector<uint8_t>& data,
        CompressionMethod method
    );
    
    /**
     * @brief Select optimal compression method for given data
     */
    CompressionMethod select_method(
        const uint8_t* data,
        size_t size,
        DataType type
    );
    
    /**
     * @brief Get compression metrics
     */
    std::string get_metrics() const;
    
    /**
     * @brief Reset metrics
     */
    void reset_metrics();
    
    /**
     * @brief Update configuration
     */
    void set_config(const CompressionConfig& config) {
        config_ = config;
    }
    
    /**
     * @brief Get current configuration
     */
    const CompressionConfig& get_config() const {
        return config_;
    }
    
    /**
     * @brief Convert method enum to string
     */
    static std::string method_to_string(CompressionMethod method);
    
    /**
     * @brief Convert string to method enum
     */
    static std::optional<CompressionMethod> string_to_method(const std::string& str);
    
private:
    // Compression method implementations
    CompressionResult compress_zstd(const uint8_t* data, size_t size);
    CompressionResult compress_rle(const uint8_t* data, size_t size);
    CompressionResult compress_delta(const uint8_t* data, size_t size);
    CompressionResult compress_dictionary(const uint8_t* data, size_t size);

    // GPU-accelerated compression implementations
    CompressionResult compress_gpu_zstd(const uint8_t* data, size_t size);
    CompressionResult compress_gpu_snappy(const uint8_t* data, size_t size);
    CompressionResult compress_gpu_lz4(const uint8_t* data, size_t size);
    
    // Decompression method implementations
    std::vector<uint8_t> decompress_zstd(const std::vector<uint8_t>& data);
    std::vector<uint8_t> decompress_rle(const std::vector<uint8_t>& data);
    std::vector<uint8_t> decompress_delta(const std::vector<uint8_t>& data);
    std::vector<uint8_t> decompress_dictionary(const std::vector<uint8_t>& data);

    // GPU-accelerated decompression implementations
    std::vector<uint8_t> decompress_gpu_zstd(const std::vector<uint8_t>& data);
    std::vector<uint8_t> decompress_gpu_snappy(const std::vector<uint8_t>& data);
    std::vector<uint8_t> decompress_gpu_lz4(const std::vector<uint8_t>& data);
    
    // Helper functions
    DataType detect_data_type(const uint8_t* data, size_t size);
    bool is_mostly_text(const uint8_t* data, size_t size);
    bool is_sparse_data(const uint8_t* data, size_t size);

    // Lazy-init GPU compression manager (created on first GPU method call)
    themis::storage::GpuCompressionManager& gpu_manager();
    
    CompressionConfig config_;
    std::unique_ptr<themis::storage::GpuCompressionManager> gpu_manager_;
};

/**
 * @brief Run-Length Encoding utilities
 */
class RLECodec {
public:
    /**
     * @brief Compress using RLE
     * 
     * Format: [count:varint][value:byte]...
     * Efficient for data with long runs of repeated bytes
     */
    static std::vector<uint8_t> compress(const uint8_t* data, size_t size);
    
    /**
     * @brief Decompress RLE data
     */
    static std::vector<uint8_t> decompress(const std::vector<uint8_t>& data);
    
private:
    static void encode_varint(std::vector<uint8_t>& output, uint32_t value);
    static uint32_t decode_varint(const uint8_t*& ptr);
};

/**
 * @brief Delta encoding utilities
 */
class DeltaCodec {
public:
    /**
     * @brief Compress using delta encoding
     * 
     * Stores first value, then differences between consecutive values.
     * Efficient for monotonic or slowly-changing sequences.
     */
    static std::vector<uint8_t> compress(const uint8_t* data, size_t size);
    
    /**
     * @brief Decompress delta-encoded data
     */
    static std::vector<uint8_t> decompress(const std::vector<uint8_t>& data);
};

/**
 * @brief Simple dictionary encoding utilities
 */
class SimpleDictionaryCodec {
public:
    /**
     * @brief Compress using dictionary encoding
     * 
     * Efficient for data with repeated patterns/blocks.
     * Format: [dict_size:4][dict_entries...][indices...]
     */
    static std::vector<uint8_t> compress(const uint8_t* data, size_t size);
    
    /**
     * @brief Decompress dictionary-encoded data
     */
    static std::vector<uint8_t> decompress(const std::vector<uint8_t>& data);
};

} // namespace compression
} // namespace themis
