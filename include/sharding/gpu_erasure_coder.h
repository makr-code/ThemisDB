/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            gpu_erasure_coder.h                                ║
  Version:         0.0.45                                             ║
  Last Modified:   2026-04-15 07:09:30                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     268                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * ThemisDB GPU-Accelerated Erasure Coding
 * 
 * CUDA/OpenCL acceleration for Reed-Solomon erasure coding operations
 * providing 10-50× speedup on large data blocks.
 * 
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "sharding/redundancy_strategy.h"
#include <memory>
#include <vector>
#include <cstdint>

namespace themis {
namespace sharding {

/**
 * GPU Acceleration Type
 */
enum class AccelerationType {
    CPU_ONLY,       // CPU-only fallback
    GPU_CUDA,       // NVIDIA CUDA
    GPU_OPENCL,     // OpenCL (AMD/Intel/NVIDIA)
    AUTO            // Auto-detect best available
};

/**
 * GPU Configuration for Erasure Coding
 */
struct GPUConfig {
    int device_id = 0;              // GPU device ID
    size_t batch_size = 64;         // Number of operations to batch
    bool async_compute = true;      // Non-blocking GPU compute
    bool fallback_cpu = true;       // CPU fallback if GPU busy/unavailable
    size_t min_size_for_gpu = 1024 * 1024;  // 1MB minimum for GPU acceleration
    
    // Memory management
    bool use_pinned_memory = true;  // Use pinned host memory for faster transfers
    size_t pinned_buffer_size = 64 * 1024 * 1024;  // 64MB pinned buffer
    
    // Performance tuning
    int cuda_streams = 4;           // Number of CUDA streams for async ops
    size_t max_gpu_memory_mb = 2048;  // Maximum GPU memory to use (MB)
};

/**
 * Forward declarations for platform-specific implementations
 */
class GPUErasureCoderImpl;

/**
 * GPU-Accelerated Erasure Coder
 * 
 * Provides GPU acceleration for Reed-Solomon erasure coding operations
 * with automatic CPU fallback and batching support.
 */
class GPUErasureCoder : public ErasureCoder {
public:
    /**
     * Create GPU erasure coder with specified acceleration type
     * 
     * @param accel_type Acceleration type (CUDA/OpenCL/Auto)
     * @param config GPU configuration
     * @param algorithm Erasure coding algorithm (default: Reed-Solomon)
     */
    explicit GPUErasureCoder(
        AccelerationType accel_type = AccelerationType::AUTO,
        const GPUConfig& config = GPUConfig{},
        ErasureCodingAlgorithm algorithm = ErasureCodingAlgorithm::REED_SOLOMON
    );
    
    ~GPUErasureCoder() override;
    
    // Disable copy, allow move
    GPUErasureCoder(const GPUErasureCoder&) = delete;
    GPUErasureCoder& operator=(const GPUErasureCoder&) = delete;
    GPUErasureCoder(GPUErasureCoder&&) noexcept;
    GPUErasureCoder& operator=(GPUErasureCoder&&) noexcept;
    
    /**
     * Encode data into data + parity chunks using GPU acceleration
     * 
     * @param data Input data to encode
     * @param data_shards Number of data chunks
     * @param parity_shards Number of parity chunks
     * @return Vector of encoded chunks (data + parity)
     */
    std::vector<std::vector<uint8_t>> encode(
        const std::vector<uint8_t>& data,
        uint32_t data_shards,
        uint32_t parity_shards
    ) override;
    
    /**
     * Decode/recover original data from available chunks using GPU acceleration
     * 
     * @param available_chunks Map from chunk index to chunk data
     * @param missing_indices Indices of missing chunks
     * @param data_shards Number of data chunks
     * @param parity_shards Number of parity chunks
     * @return Recovered original data
     */
    std::vector<uint8_t> decode(
        const std::map<uint32_t, std::vector<uint8_t>>& available_chunks,
        const std::vector<uint32_t>& missing_indices,
        uint32_t data_shards,
        uint32_t parity_shards
    ) override;
    
    /**
     * Batch encode multiple data blocks
     * More efficient than encoding one at a time
     * 
     * @param data_blocks Vector of data blocks to encode
     * @param data_shards Number of data chunks per block
     * @param parity_shards Number of parity chunks per block
     * @return Vector of encoded chunk vectors (one per input block)
     */
    std::vector<std::vector<std::vector<uint8_t>>> batchEncode(
        const std::vector<std::vector<uint8_t>>& data_blocks,
        uint32_t data_shards,
        uint32_t parity_shards
    );
    
    /**
     * Check if GPU acceleration is available and initialized
     */
    bool isGPUAvailable() const;
    
    /**
     * Get current acceleration type being used
     */
    AccelerationType getAccelerationType() const;
    
    /**
     * Get GPU configuration
     */
    const GPUConfig& getConfig() const { return config_; }
    
    /**
     * Force CPU fallback (for testing/debugging)
     */
    void forceCPUFallback(bool enable);
    
    /**
     * Get performance statistics
     */
    struct PerformanceStats {
        uint64_t total_encodes = 0;
        uint64_t total_decodes = 0;
        uint64_t gpu_encodes = 0;
        uint64_t gpu_decodes = 0;
        uint64_t cpu_fallbacks = 0;
        uint64_t bytes_encoded = 0;
        uint64_t bytes_decoded = 0;
        double avg_gpu_encode_ms = 0.0;
        double avg_gpu_decode_ms = 0.0;
        double avg_cpu_encode_ms = 0.0;
        double avg_cpu_decode_ms = 0.0;
    };
    
    PerformanceStats getStats() const { return stats_; }
    void resetStats();

private:
    AccelerationType accel_type_;
    GPUConfig config_;
    ErasureCodingAlgorithm algorithm_;
    
    // Platform-specific implementation (CUDA or OpenCL)
    std::unique_ptr<GPUErasureCoderImpl> impl_;
    
    // CPU fallback coder
    std::unique_ptr<ErasureCoder> cpu_coder_;
    
    // Force CPU mode flag
    bool force_cpu_ = false;
    
    // Performance statistics
    mutable PerformanceStats stats_;
    
    // Initialize GPU implementation
    bool initializeGPU();
    
    // Determine if we should use GPU for given data size
    bool shouldUseGPU(size_t data_size) const;
};

/**
 * Platform-specific implementation interface
 * Implemented separately for CUDA (.cu) and OpenCL (.cpp)
 */
class GPUErasureCoderImpl {
public:
    virtual ~GPUErasureCoderImpl() = default;
    
    virtual bool initialize(const GPUConfig& config) = 0;
    virtual void shutdown() = 0;
    
    virtual std::vector<std::vector<uint8_t>> encode(
        const std::vector<uint8_t>& data,
        uint32_t data_shards,
        uint32_t parity_shards
    ) = 0;
    
    virtual std::vector<uint8_t> decode(
        const std::map<uint32_t, std::vector<uint8_t>>& available_chunks,
        const std::vector<uint32_t>& missing_indices,
        uint32_t data_shards,
        uint32_t parity_shards
    ) = 0;
    
    virtual std::vector<std::vector<std::vector<uint8_t>>> batchEncode(
        const std::vector<std::vector<uint8_t>>& data_blocks,
        uint32_t data_shards,
        uint32_t parity_shards
    ) = 0;
    
    virtual bool isAvailable() const = 0;
};

/**
 * Factory function to create GPU erasure coder with best available backend
 */
std::unique_ptr<ErasureCoder> createGPUErasureCoder(
    AccelerationType accel_type = AccelerationType::AUTO,
    const GPUConfig& config = GPUConfig{},
    ErasureCodingAlgorithm algorithm = ErasureCodingAlgorithm::REED_SOLOMON
);

} // namespace sharding
} // namespace themis

// Backward compatibility
namespace themisdb {
namespace sharding {
using themis::sharding::AccelerationType;
using themis::sharding::GPUConfig;
using themis::sharding::GPUErasureCoder;
using themis::sharding::createGPUErasureCoder;
}
}
