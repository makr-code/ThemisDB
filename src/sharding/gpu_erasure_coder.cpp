/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            gpu_erasure_coder.cpp                              ║
  Version:         0.0.45                                             ║
  Last Modified:   2026-04-15 07:14:24                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     311                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * ThemisDB GPU-Accelerated Erasure Coding Implementation
 * 
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sharding/gpu_erasure_coder.h"
#include <spdlog/spdlog.h>
#include <chrono>

namespace themis {
namespace sharding {

// Forward declare platform-specific implementations
#ifdef THEMIS_ENABLE_CUDA
std::unique_ptr<GPUErasureCoderImpl> createCUDAErasureCoder(
    const GPUConfig& config,
    ErasureCodingAlgorithm algorithm
);
#endif

#ifdef THEMIS_ENABLE_OPENCL
std::unique_ptr<GPUErasureCoderImpl> createOpenCLErasureCoder(
    const GPUConfig& config,
    ErasureCodingAlgorithm algorithm
);
#endif

// ═══════════════════════════════════════════════════════════
// GPUErasureCoder Implementation
// ═══════════════════════════════════════════════════════════

GPUErasureCoder::GPUErasureCoder(
    AccelerationType accel_type,
    const GPUConfig& config,
    ErasureCodingAlgorithm algorithm
)
    : accel_type_(accel_type)
    , config_(config)
    , algorithm_(algorithm)
{
    // Initialize CPU fallback coder
    cpu_coder_ = ErasureCoder::create(algorithm);
    
    // Try to initialize GPU if requested
    if (accel_type_ != AccelerationType::CPU_ONLY) {
        if (!initializeGPU()) {
            spdlog::warn("GPU initialization failed, falling back to CPU");
            accel_type_ = AccelerationType::CPU_ONLY;
        }
    }
}

GPUErasureCoder::~GPUErasureCoder() = default;

GPUErasureCoder::GPUErasureCoder(GPUErasureCoder&&) noexcept = default;
GPUErasureCoder& GPUErasureCoder::operator=(GPUErasureCoder&&) noexcept = default;

bool GPUErasureCoder::initializeGPU() {
    // Auto-detect best available GPU backend
    if (accel_type_ == AccelerationType::AUTO) {
#ifdef THEMIS_ENABLE_CUDA
        accel_type_ = AccelerationType::GPU_CUDA;
        spdlog::info("Auto-detected CUDA support");
#elif defined(THEMIS_ENABLE_OPENCL)
        accel_type_ = AccelerationType::GPU_OPENCL;
        spdlog::info("Auto-detected OpenCL support");
#else
        spdlog::warn("No GPU support compiled in, using CPU");
        accel_type_ = AccelerationType::CPU_ONLY;
        return false;
#endif
    }
    
    // Create platform-specific implementation
    try {
        switch (accel_type_) {
#ifdef THEMIS_ENABLE_CUDA
            case AccelerationType::GPU_CUDA:
                impl_ = createCUDAErasureCoder(config_, algorithm_);
                break;
#endif
                
#ifdef THEMIS_ENABLE_OPENCL
            case AccelerationType::GPU_OPENCL:
                impl_ = createOpenCLErasureCoder(config_, algorithm_);
                break;
#endif
                
            default:
                spdlog::error("Requested GPU acceleration type not available");
                return false;
        }
        
        if (!impl_ || !impl_->initialize(config_)) {
            spdlog::error("Failed to initialize GPU erasure coder implementation");
            impl_.reset();
            return false;
        }
        
        spdlog::info("GPU erasure coder initialized successfully (device {})", 
                     config_.device_id);
        return true;
        
    } catch (const std::exception& e) {
        spdlog::error("Exception during GPU initialization: {}", e.what());
        impl_.reset();
        return false;
    }
}

bool GPUErasureCoder::shouldUseGPU(size_t data_size) const {
    // Don't use GPU if forced to CPU or GPU not available
    if (force_cpu_ || !impl_ || !impl_->isAvailable()) {
        return false;
    }
    
    // Only use GPU for large enough data blocks to amortize transfer overhead
    return data_size >= config_.min_size_for_gpu;
}

std::vector<std::vector<uint8_t>> GPUErasureCoder::encode(
    const std::vector<uint8_t>& data,
    uint32_t data_shards,
    uint32_t parity_shards
) {
    auto start = std::chrono::high_resolution_clock::now();
    
    stats_.total_encodes++;
    stats_.bytes_encoded += data.size();
    
    std::vector<std::vector<uint8_t>> result;
    
    // Decide whether to use GPU or CPU
    if (shouldUseGPU(data.size())) {
        try {
            result = impl_->encode(data, data_shards, parity_shards);
            stats_.gpu_encodes++;
            
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration<double, std::milli>(end - start).count();
            stats_.avg_gpu_encode_ms = 
                (stats_.avg_gpu_encode_ms * (stats_.gpu_encodes - 1) + duration) / stats_.gpu_encodes;
            
            return result;
            
        } catch (const std::exception& e) {
            spdlog::warn("GPU encode failed: {}, falling back to CPU", e.what());
            stats_.cpu_fallbacks++;
        }
    }
    
    // CPU fallback
    result = cpu_coder_->encode(data, data_shards, parity_shards);
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration<double, std::milli>(end - start).count();
    auto cpu_encodes = stats_.total_encodes - stats_.gpu_encodes;
    if (cpu_encodes > 0) {
        stats_.avg_cpu_encode_ms = 
            (stats_.avg_cpu_encode_ms * (cpu_encodes - 1) + duration) / cpu_encodes;
    }
    
    return result;
}

std::vector<uint8_t> GPUErasureCoder::decode(
    const std::map<uint32_t, std::vector<uint8_t>>& available_chunks,
    const std::vector<uint32_t>& missing_indices,
    uint32_t data_shards,
    uint32_t parity_shards
) {
    auto start = std::chrono::high_resolution_clock::now();
    
    stats_.total_decodes++;
    
    // Estimate data size from first chunk
    size_t estimated_size = 0;
    if (!available_chunks.empty()) {
        estimated_size = available_chunks.begin()->second.size() * data_shards;
        stats_.bytes_decoded += estimated_size;
    }
    
    std::vector<uint8_t> result;
    
    // Decide whether to use GPU or CPU
    if (shouldUseGPU(estimated_size)) {
        try {
            result = impl_->decode(available_chunks, missing_indices, 
                                  data_shards, parity_shards);
            stats_.gpu_decodes++;
            
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration<double, std::milli>(end - start).count();
            stats_.avg_gpu_decode_ms = 
                (stats_.avg_gpu_decode_ms * (stats_.gpu_decodes - 1) + duration) / stats_.gpu_decodes;
            
            return result;
            
        } catch (const std::exception& e) {
            spdlog::warn("GPU decode failed: {}, falling back to CPU", e.what());
            stats_.cpu_fallbacks++;
        }
    }
    
    // CPU fallback
    result = cpu_coder_->decode(available_chunks, missing_indices, 
                                data_shards, parity_shards);
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration<double, std::milli>(end - start).count();
    auto cpu_decodes = stats_.total_decodes - stats_.gpu_decodes;
    if (cpu_decodes > 0) {
        stats_.avg_cpu_decode_ms = 
            (stats_.avg_cpu_decode_ms * (cpu_decodes - 1) + duration) / cpu_decodes;
    }
    
    return result;
}

std::vector<std::vector<std::vector<uint8_t>>> GPUErasureCoder::batchEncode(
    const std::vector<std::vector<uint8_t>>& data_blocks,
    uint32_t data_shards,
    uint32_t parity_shards
) {
    // Calculate total data size
    size_t total_size = 0;
    for (const auto& block : data_blocks) {
        total_size += block.size();
    }
    
    // Use GPU batch operation if available and worthwhile
    if (shouldUseGPU(total_size) && impl_) {
        try {
            return impl_->batchEncode(data_blocks, data_shards, parity_shards);
        } catch (const std::exception& e) {
            spdlog::warn("GPU batch encode failed: {}, falling back to CPU", e.what());
            stats_.cpu_fallbacks++;
        }
    }
    
    // CPU fallback: encode each block individually
    std::vector<std::vector<std::vector<uint8_t>>> results;
    results.reserve(data_blocks.size());
    
    for (const auto& block : data_blocks) {
        results.push_back(encode(block, data_shards, parity_shards));
    }
    
    return results;
}

bool GPUErasureCoder::isGPUAvailable() const {
    return impl_ && impl_->isAvailable() && !force_cpu_;
}

AccelerationType GPUErasureCoder::getAccelerationType() const {
    if (force_cpu_ || !impl_ || !impl_->isAvailable()) {
        return AccelerationType::CPU_ONLY;
    }
    return accel_type_;
}

void GPUErasureCoder::forceCPUFallback(bool enable) {
    force_cpu_ = enable;
    if (enable) {
        spdlog::info("Forcing CPU fallback mode");
    }
}

void GPUErasureCoder::resetStats() {
    stats_ = PerformanceStats{};
}

// ═══════════════════════════════════════════════════════════
// Factory Function
// ═══════════════════════════════════════════════════════════

std::unique_ptr<ErasureCoder> createGPUErasureCoder(
    AccelerationType accel_type,
    const GPUConfig& config,
    ErasureCodingAlgorithm algorithm
) {
    return std::make_unique<GPUErasureCoder>(accel_type, config, algorithm);
}

} // namespace sharding
} // namespace themis
