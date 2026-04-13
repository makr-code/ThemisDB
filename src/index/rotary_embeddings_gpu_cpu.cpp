/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            rotary_embeddings_gpu_cpu.cpp                      ║
  Version:         0.0.38                                             ║
  Last Modified:   2026-04-13 04:26:15                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     93                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// CPU-only fallback implementation of RotaryEmbeddingGPU
// Compiled when CUDA and HIP are not available

#include "index/rotary_embeddings_gpu.h"
#include <stdexcept>

namespace themis {

// Empty GPU resources for CPU-only build
struct RotaryEmbeddingGPU::GPUResources {
    // No GPU resources needed for CPU-only build
};

RotaryEmbeddingGPU::RotaryEmbeddingGPU(const RotationConfig& config, GPUBackend backend)
    : RotaryEmbedding(config)
    , backend_(backend)
    , gpu_available_(false)
    , gpu_resources_(std::make_unique<GPUResources>())
{
    // GPU not available in CPU-only build
    // Will always fall back to CPU
}

RotaryEmbeddingGPU::~RotaryEmbeddingGPU() {
    // Nothing to clean up in CPU-only build
}

bool RotaryEmbeddingGPU::initializeGPU() {
    return false;
}

void RotaryEmbeddingGPU::cleanupGPU() {
    // Nothing to clean up
}

bool RotaryEmbeddingGPU::uploadThetaCacheToGPU() {
    return false;
}

std::vector<std::vector<float>> RotaryEmbeddingGPU::rotateBatch(
    const std::vector<std::vector<float>>& embeddings,
    const std::vector<size_t>& positions
) const {
    // Always use CPU implementation
    return RotaryEmbedding::rotateBatch(embeddings, positions);
}

std::vector<std::vector<float>> RotaryEmbeddingGPU::rotateBatchGPU(
    const std::vector<std::vector<float>>& embeddings,
    const std::vector<size_t>& positions
) const {
    (void)embeddings;
    (void)positions;
    throw std::runtime_error(
        "GPU not available: ThemisDB was built without CUDA or HIP support. "
        "Rebuild with -DTHEMIS_ENABLE_CUDA=ON or -DTHEMIS_ENABLE_HIP=ON to enable GPU acceleration."
    );
}

void RotaryEmbeddingGPU::rotateBatchStreamGPU(
    const float* d_embeddings,
    const size_t* d_positions,
    float* d_output,
    size_t batch_size,
    void* stream
) const {
    (void)d_embeddings;
    (void)d_positions;
    (void)d_output;
    (void)batch_size;
    (void)stream;
    throw std::runtime_error(
        "GPU not available: ThemisDB was built without CUDA or HIP support. "
        "Rebuild with -DTHEMIS_ENABLE_CUDA=ON or -DTHEMIS_ENABLE_HIP=ON to enable GPU acceleration."
    );
}

} // namespace themis
