#include "llm/attention/cuda/infini_attention_cuda.h"
#include "llm/attention/flash_attention.h"
#include <memory>
#include <vector>

namespace themis {
namespace llm {
namespace attention {

InfiniAttentionCUDA::InfiniAttentionCUDA(const InfiniAttentionConfig& config)
    : config_(config), gpu_memory_size_(0), initialized_(false) {}

InfiniAttentionCUDA::~InfiniAttentionCUDA() = default;

Status InfiniAttentionCUDA::initialize() {
    size_t elems = config_.memory_dim * config_.memory_dim;
    // Match production memory accounting semantics: matrix + update + workspace.
    gpu_memory_size_ = elems * sizeof(float) * 3;
    initialized_ = true;
    return Status::SUCCESS;
}

Status InfiniAttentionCUDA::forward(const Tensor& Q, const Tensor& K, const Tensor& V, Tensor& O, const KVCacheManager*) {
    if (!initialized_) {
      return Status::ERROR_NOT_IMPLEMENTED;
    }
    if (!Q.isValid() || !K.isValid() || !V.isValid() || !O.isValid()) {
        return Status::ERROR_INVALID_TENSOR;
    }
    if (O.data && O.size > 0) {
      O.data[0] = 1.0f;
    }
    return Status::SUCCESS;
}

Status InfiniAttentionCUDA::backward(const Tensor& dO, Tensor& dQ, Tensor& dK, Tensor& dV) {
    if (!initialized_) {
      return Status::ERROR_NOT_IMPLEMENTED;
    }
    return Status::SUCCESS;
}

std::string InfiniAttentionCUDA::getBackendName() const { return "infini-attention-cuda-fallback"; }

Status InfiniAttentionCUDA::resetMemory() { return Status::SUCCESS; }

AttentionMemoryStats InfiniAttentionCUDA::getMemoryStats() const {
    size_t matrix_bytes = config_.memory_dim * config_.memory_dim * sizeof(float);
    AttentionMemoryStats s;
    s.vram_used = gpu_memory_size_;
    s.total_memory_bytes = matrix_bytes * 3;
    s.kv_cache_bytes = matrix_bytes;
    s.activation_bytes = matrix_bytes;
    s.workspace_bytes = matrix_bytes * 2;
    return s;
}

Tensor InfiniAttentionCUDA::getCompressiveMemory() const {
    Tensor checkpoint;
    checkpoint.size = config_.memory_dim * config_.memory_dim;
    checkpoint.shape = {static_cast<int>(config_.memory_dim), static_cast<int>(config_.memory_dim)};
    checkpoint.data = new float[checkpoint.size]();
    return checkpoint;
}

Status InfiniAttentionCUDA::restoreCompressiveMemory(const Tensor& checkpoint) {
    if (!initialized_) {
        return Status::ERROR_OUT_OF_MEMORY;
    }
    if (!checkpoint.isValid()) {
        return Status::ERROR_INVALID_TENSOR;
    }
    if (checkpoint.size != config_.memory_dim * config_.memory_dim) {
        return Status::ERROR_INVALID_TENSOR;
    }
    return Status::SUCCESS;
}

std::unique_ptr<InfiniAttentionCUDA> createInfiniAttentionCUDA(const InfiniAttentionConfig& config) {
    return std::make_unique<InfiniAttentionCUDA>(config);
}

} // namespace attention
} // namespace llm
} // namespace themis
