/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            rotary_embeddings_gpu.h                            ║
  Version:         0.0.38                                             ║
  Last Modified:   2026-04-13 04:16:09                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     105                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • a629043ab2  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "index/rotary_embeddings.h"
#include <memory>

namespace themis {

/// GPU Backend selection
enum class GPUBackend {
    CUDA,  // NVIDIA CUDA
    HIP,   // AMD ROCm HIP
    CPU    // Fallback to CPU
};

/// GPU-accelerated Rotary Position Embeddings
/// Provides 10-100x speedup for large batch operations (batch_size > 100)
/// Falls back to CPU automatically for small batches or if GPU unavailable
class RotaryEmbeddingGPU : public RotaryEmbedding {
public:
    explicit RotaryEmbeddingGPU(const RotationConfig& config, GPUBackend backend = GPUBackend::CUDA);
    ~RotaryEmbeddingGPU();
    
    // Override batch rotation with GPU acceleration
    std::vector<std::vector<float>> rotateBatch(
        const std::vector<std::vector<float>>& embeddings,
        const std::vector<size_t>& positions
    ) const;
    
    /// GPU-accelerated batch rotation (explicit GPU method)
    std::vector<std::vector<float>> rotateBatchGPU(
        const std::vector<std::vector<float>>& embeddings,
        const std::vector<size_t>& positions
    ) const;
    
    /// Streaming API for large datasets (advanced users)
    /// Operates on device memory directly for maximum performance
    /// @param d_embeddings Device pointer to embeddings (batch_size * hidden_dim floats)
    /// @param d_positions Device pointer to positions (batch_size size_t values)
    /// @param d_output Device pointer to output buffer (batch_size * hidden_dim floats)
    /// @param batch_size Number of embeddings to rotate
    /// @param stream CUDA/HIP stream for async execution (0 = default stream)
    void rotateBatchStreamGPU(
        const float* d_embeddings,
        const size_t* d_positions,
        float* d_output,
        size_t batch_size,
        void* stream = nullptr
    ) const;
    
    /// Check if GPU is available and initialized
    bool isGPUAvailable() const { return gpu_available_; }
    
    /// Get current backend
    GPUBackend getBackend() const { return backend_; }
    
    /// Get batch size threshold for GPU acceleration (default: 100)
    size_t getGPUBatchThreshold() const { return gpu_batch_threshold_; }
    
    /// Set batch size threshold for GPU acceleration
    void setGPUBatchThreshold(size_t threshold) { gpu_batch_threshold_ = threshold; }
    
private:
    GPUBackend backend_;
    mutable bool gpu_available_;
    size_t gpu_batch_threshold_ = 100;  // Use GPU only for batches >= this size
    
    // Opaque pointer to GPU resources (theta cache, context, etc.)
    // Implemented differently for CUDA vs HIP
    struct GPUResources;
    std::unique_ptr<GPUResources> gpu_resources_;
    
    /// Initialize GPU resources
    bool initializeGPU();
    
    /// Clean up GPU resources
    void cleanupGPU();
    
    /// Upload theta cache to GPU device memory
    bool uploadThetaCacheToGPU();
};

} // namespace themis
