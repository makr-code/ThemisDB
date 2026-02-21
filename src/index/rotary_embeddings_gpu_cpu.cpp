/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            rotary_embeddings_gpu_cpu.cpp                      ║
  Version:         0.0.18                                             ║
  Last Modified:   2026-02-21 18:44:08                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     97                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
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
    throw std::runtime_error(
        "GPU not available: ThemisDB was built without CUDA or HIP support. "
        "Rebuild with -DTHEMIS_ENABLE_CUDA=ON or -DTHEMIS_ENABLE_HIP=ON to enable GPU acceleration."
    );
}

} // namespace themis
