/**
 * @file rotary_embeddings_gpu_cpu.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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
    [[maybe_unused]] const std::vector<std::vector<float>>& embeddings,
    [[maybe_unused]] const std::vector<size_t>& positions
) const {
    throw std::runtime_error(
        "GPU not available: ThemisDB was built without CUDA or HIP support. "
        "Rebuild with -DTHEMIS_ENABLE_CUDA=ON or -DTHEMIS_ENABLE_HIP=ON to enable GPU acceleration."
    );
}

void RotaryEmbeddingGPU::rotateBatchStreamGPU(
    [[maybe_unused]] const float* d_embeddings,
    [[maybe_unused]] const size_t* d_positions,
    [[maybe_unused]] float* d_output,
    [[maybe_unused]] size_t batch_size,
    [[maybe_unused]] void* stream
) const {
    throw std::runtime_error(
        "GPU not available: ThemisDB was built without CUDA or HIP support. "
        "Rebuild with -DTHEMIS_ENABLE_CUDA=ON or -DTHEMIS_ENABLE_HIP=ON to enable GPU acceleration."
    );
}

} // namespace themis
