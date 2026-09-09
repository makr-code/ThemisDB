/**
 * @file nccl_backend.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=7; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=4, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "llm/lora_framework/nccl_backend.h"
#include <spdlog/spdlog.h>
#include <stdexcept>

#ifdef THEMIS_ENABLE_CUDA
#include <cuda_runtime.h>
#endif

namespace themis {
namespace llm {
namespace lora {

NCCLBackend::NCCLBackend(const MultiGPUContext& ctx, int rank, int world_size)
    : ctx_(ctx), rank_(rank), world_size_(world_size), initialized_(false) {
    
#ifdef THEMIS_ENABLE_CUDA
#ifdef THEMIS_ENABLE_NCCL
    cuda_stream_ = nullptr;
#endif
#endif
    
    spdlog::info("NCCLBackend created: rank={}, world_size={}", rank_, world_size_);
}

NCCLBackend::~NCCLBackend() {
    if (initialized_) {
        finalize();
    }
}

NCCLBackend::NCCLBackend(NCCLBackend&& other) noexcept
    : ctx_(other.ctx_), rank_(other.rank_), world_size_(other.world_size_),
      initialized_(other.initialized_) {
#ifdef THEMIS_ENABLE_CUDA
#ifdef THEMIS_ENABLE_NCCL
    nccl_comm_ = other.nccl_comm_;
    nccl_id_ = other.nccl_id_;
    cuda_stream_ = other.cuda_stream_;
    other.nccl_comm_ = nullptr;
    other.cuda_stream_ = nullptr;
#endif
#endif
    other.initialized_ = false;
}

NCCLBackend& NCCLBackend::operator=(NCCLBackend&& other) noexcept {
    if (this != &other) {
        if (initialized_) {
            finalize();
        }
        
        rank_ = other.rank_;
        world_size_ = other.world_size_;
        initialized_ = other.initialized_;
        
#ifdef THEMIS_ENABLE_CUDA
#ifdef THEMIS_ENABLE_NCCL
        nccl_comm_ = other.nccl_comm_;
        nccl_id_ = other.nccl_id_;
        cuda_stream_ = other.cuda_stream_;
        other.nccl_comm_ = nullptr;
        other.cuda_stream_ = nullptr;
#endif
#endif
        other.initialized_ = false;
    }
    return *this;
}

bool NCCLBackend::initialize() {
    if (initialized_) {
        spdlog::warn("NCCLBackend already initialized");
        return true;
    }
    
    if (!is_available()) {
        spdlog::error("NCCL is not available");
        return false;
    }
    
    if (ctx_.gpu_type() != DeviceType::CUDA) {
        spdlog::error("NCCL requires CUDA devices");
        return false;
    }
    
    return initialize_nccl();
}

void NCCLBackend::finalize() {
    if (!initialized_) {
        return;
    }
    
    cleanup_nccl();
    initialized_ = false;
    spdlog::info("NCCLBackend finalized");
}

bool NCCLBackend::allreduce(std::vector<GPUTensor*>& tensors, bool average) {
    if (!initialized_) {
        spdlog::error("NCCLBackend not initialized");
        return false;
    }
    
#ifdef THEMIS_ENABLE_CUDA
#ifdef THEMIS_ENABLE_NCCL
    cudaStream_t stream = static_cast<cudaStream_t>(cuda_stream_);
    
    // Group API for efficient communication — REL-68: check ncclGroupStart return value
    ncclResult_t group_start_err = ncclGroupStart();
    if (group_start_err != ncclSuccess) {
        spdlog::error("NCCL allreduce: ncclGroupStart failed: {}",
                      ncclGetErrorString(group_start_err));
        return false;
    }
    
    for (auto* tensor : tensors) {
        if (!tensor || tensor->device().type != DeviceType::CUDA) {
            spdlog::warn("Skipping non-CUDA tensor in NCCL allreduce");
            continue;
        }
        
        void* send_buf = tensor->gpu_ptr();
        void* recv_buf = tensor->gpu_ptr();  // In-place
        size_t count = tensor->size();
        
        ncclResult_t result = ncclAllReduce(
            send_buf,
            recv_buf,
            count,
            ncclFloat,
            ncclSum,
            nccl_comm_,
            stream
        );
        
        if (result != ncclSuccess) {
            spdlog::error("NCCL allreduce failed: {}", ncclGetErrorString(result));
            // REL-69: check ncclGroupEnd return value on early-exit path
            ncclResult_t group_end_err = ncclGroupEnd();
            if (group_end_err != ncclSuccess) {
                spdlog::warn("NCCL allreduce early-exit: ncclGroupEnd failed: {}",
                             ncclGetErrorString(group_end_err));
            }
            return false;
        }
    }
    
    // REL-70: check ncclGroupEnd return value on success path
    ncclResult_t group_end_err = ncclGroupEnd();
    if (group_end_err != ncclSuccess) {
        spdlog::error("NCCL allreduce: ncclGroupEnd failed: {}",
                      ncclGetErrorString(group_end_err));
        return false;
    }
    
    // Wait for completion — REL-10: check cudaStreamSynchronize return value
    {
        cudaError_t sync_err = cudaStreamSynchronize(stream);
        if (sync_err != cudaSuccess) {
            spdlog::error("NCCL allreduce stream sync failed: {}",
                          cudaGetErrorString(sync_err));
            return false;
        }
    }
    
    // Average if requested
    if (average && world_size_ > 1) {
        float scale = 1.0f / static_cast<float>(world_size_);
        for (auto* tensor : tensors) {
            if (tensor && tensor->device().type == DeviceType::CUDA) {
                tensor->mul_scalar_inplace(scale);
            }
        }
    }
    
    return true;
#else
    spdlog::error("NCCL not enabled at compile time");
    return false;
#endif
#else
    spdlog::error("CUDA not enabled at compile time");
    return false;
#endif
}

bool NCCLBackend::allreduce(GPUTensor& tensor, bool average) {
    std::vector<GPUTensor*> tensors = {&tensor};
    return allreduce(tensors, average);
}

bool NCCLBackend::broadcast(GPUTensor& tensor, int root) {
    static_cast<void>(tensor);
    static_cast<void>(root);
    if (!initialized_) {
        spdlog::error("NCCLBackend not initialized");
        return false;
    }
    
#ifdef THEMIS_ENABLE_CUDA
#ifdef THEMIS_ENABLE_NCCL
    if (tensor.device().type != DeviceType::CUDA) {
        spdlog::error("Tensor must be on CUDA device for NCCL broadcast");
        return false;
    }
    
    cudaStream_t stream = static_cast<cudaStream_t>(cuda_stream_);
    
    ncclResult_t result = ncclBcast(
        tensor.gpu_ptr(),
        tensor.size(),
        ncclFloat,
        root,
        nccl_comm_,
        stream
    );
    
    if (result != ncclSuccess) {
        spdlog::error("NCCL broadcast failed: {}", ncclGetErrorString(result));
        return false;
    }
    
    // REL-11: check cudaStreamSynchronize return value in broadcast
    {
        cudaError_t sync_err = cudaStreamSynchronize(stream);
        if (sync_err != cudaSuccess) {
            spdlog::error("NCCL broadcast stream sync failed: {}",
                          cudaGetErrorString(sync_err));
            return false;
        }
    }
    return true;
#else
    spdlog::error("NCCL not enabled at compile time");
    return false;
#endif
#else
    spdlog::error("CUDA not enabled at compile time");
    return false;
#endif
}

void NCCLBackend::barrier() {
    if (!initialized_) {
        return;
    }
    
    // NCCL doesn't have explicit barrier, use allreduce with dummy data
#ifdef THEMIS_ENABLE_CUDA
#ifdef THEMIS_ENABLE_NCCL
    float dummy = 0.0f;
    cudaStream_t stream = static_cast<cudaStream_t>(cuda_stream_);
    
    ncclResult_t result = ncclAllReduce(
        &dummy,
        &dummy,
        1,
        ncclFloat,
        ncclSum,
        nccl_comm_,
        stream
    );
    
    if (result != ncclSuccess) {
        spdlog::error("NCCL barrier allreduce failed: {}", ncclGetErrorString(result));
    }
    
    // REL-12: check cudaStreamSynchronize return value in barrier
    {
        cudaError_t sync_err = cudaStreamSynchronize(stream);
        if (sync_err != cudaSuccess) {
            spdlog::error("NCCL barrier stream sync failed: {}",
                          cudaGetErrorString(sync_err));
        }
    }
#endif
#endif
}

bool NCCLBackend::is_available() {
#ifdef THEMIS_ENABLE_CUDA
#ifdef THEMIS_ENABLE_NCCL
    return true;
#else
    return false;
#endif
#else
    return false;
#endif
}

std::string NCCLBackend::get_version() {
#ifdef THEMIS_ENABLE_CUDA
#ifdef THEMIS_ENABLE_NCCL
    int version = 0;
    ncclResult_t version_err = ncclGetVersion(&version);
    if (version_err != ncclSuccess) {
        spdlog::warn("Failed to query NCCL version: {}", ncclGetErrorString(version_err));
        return "Unknown (query failed)";
    }
    int major = version / 10000;
    int minor = (version % 10000) / 100;
    int patch = version % 100;
    return std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch);
#else
    return "Not available (NCCL not enabled)";
#endif
#else
    return "Not available (CUDA not enabled)";
#endif
}

bool NCCLBackend::initialize_nccl() {
#ifdef THEMIS_ENABLE_CUDA
#ifdef THEMIS_ENABLE_NCCL
    spdlog::info("Initializing NCCL backend (rank {}/{})", rank_, world_size_);
    
    // Set device — REL-13: check cudaSetDevice return value
    Device device = ctx_.get_device(rank_);
    {
        cudaError_t set_err = cudaSetDevice(device.id);
        if (set_err != cudaSuccess) {
            spdlog::error("NCCL init: cudaSetDevice({}) failed: {}",
                          device.id, cudaGetErrorString(set_err));
            return false;
        }
    }
    
    // Create CUDA stream
    cudaStream_t stream;
    cudaError_t cuda_err = cudaStreamCreate(&stream);
    if (cuda_err != cudaSuccess) {
        spdlog::error("Failed to create CUDA stream: {}", cudaGetErrorString(cuda_err));
        return false;
    }
    cuda_stream_ = stream;
    
    // Generate unique ID on rank 0 (in real implementation, this would be broadcast)
    if (rank_ == 0) {
        ncclResult_t result = ncclGetUniqueId(&nccl_id_);
        if (result != ncclSuccess) {
            spdlog::error("Failed to get NCCL unique ID: {}", ncclGetErrorString(result));
            return false;
        }
        // In real implementation, broadcast nccl_id_ to all ranks via MPI or TCP
    } else {
        // In real implementation, receive nccl_id_ from rank 0
    }
    
    // Initialize NCCL communicator
    ncclResult_t result = ncclCommInitRank(&nccl_comm_, world_size_, nccl_id_, rank_);
    if (result != ncclSuccess) {
        spdlog::error("Failed to initialize NCCL communicator: {}", ncclGetErrorString(result));
        return false;
    }
    
    initialized_ = true;
    spdlog::info("NCCL backend initialized successfully (version: {})", get_version());
    return true;
#else
    spdlog::error("NCCL not enabled at compile time");
    return false;
#endif
#else
    spdlog::error("CUDA not enabled at compile time");
    return false;
#endif
}

void NCCLBackend::cleanup_nccl() {
#ifdef THEMIS_ENABLE_CUDA
#ifdef THEMIS_ENABLE_NCCL
    if (nccl_comm_) {
        ncclResult_t destroy_err = ncclCommDestroy(nccl_comm_);
        if (destroy_err != ncclSuccess) {
            spdlog::warn("NCCL cleanup: ncclCommDestroy failed: {}", ncclGetErrorString(destroy_err));
        }
        nccl_comm_ = nullptr;
    }
    
    if (cuda_stream_) {
        cudaError_t destroy_err = cudaStreamDestroy(static_cast<cudaStream_t>(cuda_stream_));
        if (destroy_err != cudaSuccess) {
            spdlog::warn("NCCL cleanup: cudaStreamDestroy failed: {}", cudaGetErrorString(destroy_err));
        }
        cuda_stream_ = nullptr;
    }
#endif
#endif
}

} // namespace lora
} // namespace llm
} // namespace themis
