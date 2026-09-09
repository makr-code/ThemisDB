/**
 * @file rccl_backend.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=7; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=4, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "llm/lora_framework/rccl_backend.h"
#include <spdlog/spdlog.h>
#include <stdexcept>

#ifdef THEMIS_ENABLE_HIP
#include <hip/hip_runtime.h>
#endif

namespace themis {
namespace llm {
namespace lora {

RCCLBackend::RCCLBackend(const MultiGPUContext& ctx, int rank, int world_size)
    : ctx_(ctx), rank_(rank), world_size_(world_size), initialized_(false) {
    
#ifdef THEMIS_ENABLE_HIP
#ifdef THEMIS_ENABLE_RCCL
    hip_stream_ = nullptr;
#endif
#endif
    
    spdlog::info("RCCLBackend created: rank={}, world_size={}", rank_, world_size_);
}

RCCLBackend::~RCCLBackend() {
    if (initialized_) {
        finalize();
    }
}

RCCLBackend::RCCLBackend(RCCLBackend&& other) noexcept
    : ctx_(other.ctx_), rank_(other.rank_), world_size_(other.world_size_),
      initialized_(other.initialized_) {
#ifdef THEMIS_ENABLE_HIP
#ifdef THEMIS_ENABLE_RCCL
    rccl_comm_ = other.rccl_comm_;
    rccl_id_ = other.rccl_id_;
    hip_stream_ = other.hip_stream_;
    other.rccl_comm_ = nullptr;
    other.hip_stream_ = nullptr;
#endif
#endif
    other.initialized_ = false;
}

RCCLBackend& RCCLBackend::operator=(RCCLBackend&& other) noexcept {
    if (this != &other) {
        if (initialized_) {
            finalize();
        }
        
        rank_ = other.rank_;
        world_size_ = other.world_size_;
        initialized_ = other.initialized_;
        
#ifdef THEMIS_ENABLE_HIP
#ifdef THEMIS_ENABLE_RCCL
        rccl_comm_ = other.rccl_comm_;
        rccl_id_ = other.rccl_id_;
        hip_stream_ = other.hip_stream_;
        other.rccl_comm_ = nullptr;
        other.hip_stream_ = nullptr;
#endif
#endif
        other.initialized_ = false;
    }
    return *this;
}

bool RCCLBackend::initialize() {
    if (initialized_) {
        spdlog::warn("RCCLBackend already initialized");
        return true;
    }
    
    if (!is_available()) {
        spdlog::error("RCCL is not available");
        return false;
    }
    
    if (ctx_.gpu_type() != DeviceType::HIP) {
        spdlog::error("RCCL requires HIP devices");
        return false;
    }
    
    return initialize_rccl();
}

void RCCLBackend::finalize() {
    if (!initialized_) {
        return;
    }
    
    cleanup_rccl();
    initialized_ = false;
    spdlog::info("RCCLBackend finalized");
}

bool RCCLBackend::allreduce(std::vector<GPUTensor*>& tensors, bool average) {
    if (!initialized_) {
        spdlog::error("RCCLBackend not initialized");
        return false;
    }
    
#ifdef THEMIS_ENABLE_HIP
#ifdef THEMIS_ENABLE_RCCL
    hipStream_t stream = static_cast<hipStream_t>(hip_stream_);
    
    // Group API for efficient communication — REL-71: check ncclGroupStart return value
    ncclResult_t group_start_err = ncclGroupStart();
    if (group_start_err != ncclSuccess) {
        spdlog::error("RCCL allreduce: ncclGroupStart failed: {}",
                      ncclGetErrorString(group_start_err));
        return false;
    }
    
    for (auto* tensor : tensors) {
        if (!tensor || tensor->device().type != DeviceType::HIP) {
            spdlog::warn("Skipping non-HIP tensor in RCCL allreduce");
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
            rccl_comm_,
            stream
        );
        
        if (result != ncclSuccess) {
            spdlog::error("RCCL allreduce failed: {}", ncclGetErrorString(result));
            ncclResult_t group_end_err = ncclGroupEnd();
            if (group_end_err != ncclSuccess) {
                spdlog::warn("RCCL allreduce early-exit: ncclGroupEnd failed: {}",
                             ncclGetErrorString(group_end_err));
            }
            return false;
        }
    }
    
    // REL-72: check ncclGroupEnd return value on success path
    ncclResult_t group_end_err = ncclGroupEnd();
    if (group_end_err != ncclSuccess) {
        spdlog::error("RCCL allreduce: ncclGroupEnd failed: {}",
                      ncclGetErrorString(group_end_err));
        return false;
    }
    
    // Wait for completion — REL-14: check hipStreamSynchronize return value
    {
        hipError_t sync_err = hipStreamSynchronize(stream);
        if (sync_err != hipSuccess) {
            spdlog::error("RCCL allreduce stream sync failed: {}",
                          hipGetErrorString(sync_err));
            return false;
        }
    }
    
    // Average if requested
    if (average && world_size_ > 1) {
        float scale = 1.0f / static_cast<float>(world_size_);
        for (auto* tensor : tensors) {
            if (tensor && tensor->device().type == DeviceType::HIP) {
                tensor->mul_scalar_inplace(scale);
            }
        }
    }
    
    return true;
#else
    spdlog::error("RCCL not enabled at compile time");
    return false;
#endif
#else
    spdlog::error("HIP not enabled at compile time");
    return false;
#endif
}

bool RCCLBackend::allreduce(GPUTensor& tensor, bool average) {
    std::vector<GPUTensor*> tensors = {&tensor};
    return allreduce(tensors, average);
}

bool RCCLBackend::broadcast(GPUTensor& tensor, int root) {
    static_cast<void>(tensor);
    static_cast<void>(root);
    if (!initialized_) {
        spdlog::error("RCCLBackend not initialized");
        return false;
    }
    
#ifdef THEMIS_ENABLE_HIP
#ifdef THEMIS_ENABLE_RCCL
    if (tensor.device().type != DeviceType::HIP) {
        spdlog::error("Tensor must be on HIP device for RCCL broadcast");
        return false;
    }
    
    hipStream_t stream = static_cast<hipStream_t>(hip_stream_);
    
    ncclResult_t result = ncclBcast(
        tensor.gpu_ptr(),
        tensor.size(),
        ncclFloat,
        root,
        rccl_comm_,
        stream
    );
    
    if (result != ncclSuccess) {
        spdlog::error("RCCL broadcast failed: {}", ncclGetErrorString(result));
        return false;
    }
    
    // REL-15: check hipStreamSynchronize return value in broadcast
    {
        hipError_t sync_err = hipStreamSynchronize(stream);
        if (sync_err != hipSuccess) {
            spdlog::error("RCCL broadcast stream sync failed: {}",
                          hipGetErrorString(sync_err));
            return false;
        }
    }
    return true;
#else
    spdlog::error("RCCL not enabled at compile time");
    return false;
#endif
#else
    spdlog::error("HIP not enabled at compile time");
    return false;
#endif
}

void RCCLBackend::barrier() {
    if (!initialized_) {
        return;
    }
    
    // RCCL doesn't have explicit barrier, use allreduce with dummy data
#ifdef THEMIS_ENABLE_HIP
#ifdef THEMIS_ENABLE_RCCL
    float dummy = 0.0f;
    hipStream_t stream = static_cast<hipStream_t>(hip_stream_);
    
    ncclResult_t result = ncclAllReduce(
        &dummy,
        &dummy,
        1,
        ncclFloat,
        ncclSum,
        rccl_comm_,
        stream
    );
    
    if (result != ncclSuccess) {
        spdlog::error("RCCL barrier allreduce failed: {}", ncclGetErrorString(result));
    }
    
    // REL-16: check hipStreamSynchronize return value in barrier
    {
        hipError_t sync_err = hipStreamSynchronize(stream);
        if (sync_err != hipSuccess) {
            spdlog::error("RCCL barrier stream sync failed: {}",
                          hipGetErrorString(sync_err));
        }
    }
#endif
#endif
}

bool RCCLBackend::is_available() {
#ifdef THEMIS_ENABLE_HIP
#ifdef THEMIS_ENABLE_RCCL
    return true;
#else
    return false;
#endif
#else
    return false;
#endif
}

std::string RCCLBackend::get_version() {
#ifdef THEMIS_ENABLE_HIP
#ifdef THEMIS_ENABLE_RCCL
    int version = 0;
    ncclResult_t version_err = ncclGetVersion(&version);
    if (version_err != ncclSuccess) {
        spdlog::warn("Failed to query RCCL version: {}", ncclGetErrorString(version_err));
        return "Unknown (query failed)";
    }
    int major = version / 10000;
    int minor = (version % 10000) / 100;
    int patch = version % 100;
    return std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch);
#else
    return "Not available (RCCL not enabled)";
#endif
#else
    return "Not available (HIP not enabled)";
#endif
}

bool RCCLBackend::initialize_rccl() {
#ifdef THEMIS_ENABLE_HIP
#ifdef THEMIS_ENABLE_RCCL
    spdlog::info("Initializing RCCL backend (rank {}/{})", rank_, world_size_);
    
    // Set device — REL-17: check hipSetDevice return value
    Device device = ctx_.get_device(rank_);
    {
        hipError_t set_err = hipSetDevice(device.id);
        if (set_err != hipSuccess) {
            spdlog::error("RCCL init: hipSetDevice({}) failed: {}",
                          device.id, hipGetErrorString(set_err));
            return false;
        }
    }
    
    // Create HIP stream
    hipStream_t stream;
    hipError_t hip_err = hipStreamCreate(&stream);
    if (hip_err != hipSuccess) {
        spdlog::error("Failed to create HIP stream: {}", hipGetErrorString(hip_err));
        return false;
    }
    hip_stream_ = stream;
    
    // Generate unique ID on rank 0 (in real implementation, this would be broadcast)
    if (rank_ == 0) {
        ncclResult_t result = ncclGetUniqueId(&rccl_id_);
        if (result != ncclSuccess) {
            spdlog::error("Failed to get RCCL unique ID: {}", ncclGetErrorString(result));
            return false;
        }
        // In real implementation, broadcast rccl_id_ to all ranks via MPI or TCP
    } else {
        // In real implementation, receive rccl_id_ from rank 0
    }
    
    // Initialize RCCL communicator
    ncclResult_t result = ncclCommInitRank(&rccl_comm_, world_size_, rccl_id_, rank_);
    if (result != ncclSuccess) {
        spdlog::error("Failed to initialize RCCL communicator: {}", ncclGetErrorString(result));
        return false;
    }
    
    initialized_ = true;
    spdlog::info("RCCL backend initialized successfully (version: {})", get_version());
    return true;
#else
    spdlog::error("RCCL not enabled at compile time");
    return false;
#endif
#else
    spdlog::error("HIP not enabled at compile time");
    return false;
#endif
}

void RCCLBackend::cleanup_rccl() {
#ifdef THEMIS_ENABLE_HIP
#ifdef THEMIS_ENABLE_RCCL
    if (rccl_comm_) {
        ncclResult_t destroy_err = ncclCommDestroy(rccl_comm_);
        if (destroy_err != ncclSuccess) {
            spdlog::warn("RCCL cleanup: ncclCommDestroy failed: {}", ncclGetErrorString(destroy_err));
        }
        rccl_comm_ = nullptr;
    }
    
    if (hip_stream_) {
        hipError_t destroy_err = hipStreamDestroy(static_cast<hipStream_t>(hip_stream_));
        if (destroy_err != hipSuccess) {
            spdlog::warn("RCCL cleanup: hipStreamDestroy failed: {}", hipGetErrorString(destroy_err));
        }
        hip_stream_ = nullptr;
    }
#endif
#endif
}

} // namespace lora
} // namespace llm
} // namespace themis
