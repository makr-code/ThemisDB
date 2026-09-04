/**
 * @file gpu_kernel_manager.cpp
 * @brief Implementation of GPU kernel execution manager with move semantics
 * @version 0.1.0
 * @note Gap Fix: CWE-457, CWE-415, CWE-672
 */

#include "gpu/gpu_kernel_manager.h"
#include <cuda_runtime.h>
#include <utility>
#include <cstring>
#include <stdexcept>
#include <sstream>

namespace themis {
namespace gpu {

// Forward declaration implementation
struct CudaKernelHandle {
    CUfunction function;
    CUmodule module;
    void* kernel_ptr;
};

// =============================================================================
// GPUKernelManager Implementation
// =============================================================================

GPUKernelManager::GPUKernelManager(const std::string& kernel_name, 
                                   int device_id, 
                                   const Config& config)
    : device_id_(device_id), 
      config_(config), 
      kernel_name_(kernel_name),
      is_moved_from_(false) {
    
    if (device_id < 0) {
        throw std::invalid_argument("device_id must be >= 0");
    }

    // Validate GPU device availability
    int device_count = {};
    cudaError_t err = cudaGetDeviceCount(&device_count);
    if (err != cudaSuccess || device_id >= device_count) {
        throw std::runtime_error("Invalid device ID: " + std::string(cudaGetErrorString(err)));
    }

    // Set device for this context
    err = cudaSetDevice(device_id);
    if (err != cudaSuccess) {
        throw std::runtime_error("Failed to set CUDA device: " + std::string(cudaGetErrorString(err)));
    }

    // Allocate and initialize kernel handle
    try {
        handle_ = std::make_unique<CudaKernelHandle>();
        handle_->kernel_ptr = nullptr;  // Placeholder; actual kernel loading would go here
    } catch (const std::bad_alloc& e) {
        cleanup();
        throw std::runtime_error("Failed to allocate GPU kernel handle: " + std::string(e.what()));
    }
}

GPUKernelManager::~GPUKernelManager() noexcept {
    cleanup();
}

GPUKernelManager::GPUKernelManager(GPUKernelManager&& other) noexcept
    : handle_(std::move(other.handle_)),
      config_(other.config_),
      device_id_(other.device_id_),
      kernel_name_(std::move(other.kernel_name_)),
      is_moved_from_(false) {
    
    // Mark source as moved-from to prevent double cleanup
    other.is_moved_from_ = true;
    other.device_id_ = -1;
    other.config_ = Config{};
}

GPUKernelManager& GPUKernelManager::operator=(GPUKernelManager&& other) noexcept {
    // Self-assignment check via moved-from state
    if (this == &other || other.is_moved_from_) {
        return *this;
    }

    // Release current resources before acquiring new ones
    cleanup();

    // Transfer ownership
    handle_ = std::move(other.handle_);
    config_ = other.config_;
    device_id_ = other.device_id_;
    kernel_name_ = std::move(other.kernel_name_);
    is_moved_from_ = false;

    // Mark source as moved-from
    other.is_moved_from_ = true;
    other.device_id_ = -1;
    other.config_ = Config{};

    return *this;
}

void GPUKernelManager::launch(const void* args) const {
    if (is_moved_from_) {
        throw std::logic_error("Cannot launch kernel on moved-from manager");
    }
    if (!handle_) {
        throw std::logic_error("Invalid kernel handle");
    }

    // Queue kernel launch (simplified; actual CUDA launch would go here)
    // CudaCheckError(cudaLaunchKernel(...));
}

bool GPUKernelManager::wait([[maybe_unused]] uint32_t timeout_ms) const {
    if (is_moved_from_) {
        throw std::logic_error("Cannot wait on moved-from manager");
    }
    if (!handle_) {
        throw std::logic_error("Invalid kernel handle");
    }

    // Synchronize on device
    cudaError_t err = cudaDeviceSynchronize();
    if (err != cudaSuccess) {
        throw std::runtime_error("Kernel synchronization failed: " + std::string(cudaGetErrorString(err)));
    }

    return true;
}

bool GPUKernelManager::is_running() const noexcept {
    if (is_moved_from_ || !handle_) {
        return false;
    }
    // Check stream status
    return true;  // Placeholder
}

bool GPUKernelManager::is_moved_from() const noexcept {
    return is_moved_from_;
}

bool GPUKernelManager::is_valid() const noexcept {
    return handle_ != nullptr && !is_moved_from_;
}

int GPUKernelManager::device_id() const noexcept {
    return is_moved_from_ ? -1 : device_id_;
}

const std::string& GPUKernelManager::kernel_name() const noexcept {
    static const std::string empty;
    return is_moved_from_ ? empty : kernel_name_;
}

void GPUKernelManager::cleanup() noexcept {
    if (handle_) {
        try {
            cudaSetDevice(device_id_);
            // Cleanup CUDA resources
            handle_.reset();
        } catch (...) {
            // Log but don't throw from cleanup
        }
    }
    handle_.reset();
}

// =============================================================================
// KernelArgumentBuffer Implementation
// =============================================================================

KernelArgumentBuffer::KernelArgumentBuffer(size_t size, int device_id)
    : size_(size), device_id_(device_id), is_moved_from_(false),
      device_ptr_(nullptr), host_ptr_(nullptr) {
    
    if (size == 0) {
        throw std::invalid_argument("Buffer size must be > 0");
    }

    // Allocate pinned host memory
    cudaError_t err = cudaMallocHost(&host_ptr_, size);
    if (err != cudaSuccess) {
        throw std::runtime_error("Failed to allocate host memory: " + std::string(cudaGetErrorString(err)));
    }

    // Allocate device memory
    err = cudaMalloc(&device_ptr_, size);
    if (err != cudaSuccess) {
        cudaFreeHost(host_ptr_);
        host_ptr_ = nullptr;
        throw std::runtime_error("Failed to allocate device memory: " + std::string(cudaGetErrorString(err)));
    }
}

KernelArgumentBuffer::~KernelArgumentBuffer() noexcept {
    cleanup();
}

KernelArgumentBuffer::KernelArgumentBuffer(KernelArgumentBuffer&& other) noexcept
    : device_ptr_(other.device_ptr_), 
      host_ptr_(other.host_ptr_),
      size_(other.size_),
      device_id_(other.device_id_),
      is_moved_from_(false) {
    
    // Mark source as moved-from
    other.device_ptr_ = nullptr;
    other.host_ptr_ = nullptr;
    other.size_ = 0;
    other.is_moved_from_ = true;
}

KernelArgumentBuffer& KernelArgumentBuffer::operator=(KernelArgumentBuffer&& other) noexcept {
    if (this == &other) {
        return *this;
    }

    cleanup();

    device_ptr_ = other.device_ptr_;
    host_ptr_ = other.host_ptr_;
    size_ = other.size_;
    device_id_ = other.device_id_;
    is_moved_from_ = false;

    other.device_ptr_ = nullptr;
    other.host_ptr_ = nullptr;
    other.size_ = 0;
    other.is_moved_from_ = true;

    return *this;
}

void* KernelArgumentBuffer::device_ptr() noexcept {
    return is_moved_from_ ? nullptr : device_ptr_;
}

const void* KernelArgumentBuffer::device_ptr() const noexcept {
    return is_moved_from_ ? nullptr : device_ptr_;
}

void* KernelArgumentBuffer::host_ptr() noexcept {
    return is_moved_from_ ? nullptr : host_ptr_;
}

void KernelArgumentBuffer::upload() const {
    if (is_moved_from_) {
        throw std::logic_error("Cannot upload from moved-from buffer");
    }

    cudaError_t err = cudaMemcpy(device_ptr_, host_ptr_, size_, cudaMemcpyHostToDevice);
    if (err != cudaSuccess) {
        throw std::runtime_error("Failed to upload to device: " + std::string(cudaGetErrorString(err)));
    }
}

void KernelArgumentBuffer::download() const {
    if (is_moved_from_) {
        throw std::logic_error("Cannot download from moved-from buffer");
    }

    cudaError_t err = cudaMemcpy(host_ptr_, device_ptr_, size_, cudaMemcpyDeviceToHost);
    if (err != cudaSuccess) {
        throw std::runtime_error("Failed to download from device: " + std::string(cudaGetErrorString(err)));
    }
}

size_t KernelArgumentBuffer::size() const noexcept {
    return is_moved_from_ ? 0 : size_;
}

bool KernelArgumentBuffer::is_valid() const noexcept {
    return !is_moved_from_ && device_ptr_ != nullptr;
}

void KernelArgumentBuffer::cleanup() noexcept {
    if (!is_moved_from_) {
        if (device_ptr_) {
            cudaFree(device_ptr_);
            device_ptr_ = nullptr;
        }
        if (host_ptr_) {
            cudaFreeHost(host_ptr_);
            host_ptr_ = nullptr;
        }
    }
    size_ = 0;
}

} // namespace gpu
} // namespace themis
