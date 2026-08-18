/**
 * @file gpu_resource_handles.cpp
 * @brief Implementation of GPU Resource Handle RAII Wrappers
 *
 * Implements automatic lifecycle management for GPU streams, events,
 * and other CUDA/HIP resources.
 *
 * @version 1.0
 * @date 2026-08-18
 */

#include "gpu/gpu_resource_handles.h"

#ifdef THEMIS_ENABLE_CUDA
#include <cuda_runtime.h>
#include <cublas_v2.h>
#endif

#ifdef THEMIS_ENABLE_HIP
#include <hip/hip_runtime.h>
#include <hipblas/hipblas.h>
#endif

#include "gpu/gpu_error.h"

namespace themis {
namespace gpu {

// ============================================================================
// GPUStreamHandle Implementation
// ============================================================================

GPUStreamHandle::GPUStreamHandle() : stream_(nullptr) {
#ifdef THEMIS_ENABLE_CUDA
    cudaError_t err = cudaStreamCreate(&stream_);
    if (err != cudaSuccess) {
        THEMIS_ERROR("cudaStreamCreate failed: {}", cudaGetErrorString(err));
        throw std::runtime_error("Failed to create CUDA stream");
    }
#elif defined(THEMIS_ENABLE_HIP)
    hipError_t err = hipStreamCreate(&stream_);
    if (err != hipSuccess) {
        THEMIS_ERROR("hipStreamCreate failed: {}", hipGetErrorString(err));
        throw std::runtime_error("Failed to create HIP stream");
    }
#else
    // CPU-only mode: stub implementation
    stream_ = nullptr;
#endif
}

GPUStreamHandle::~GPUStreamHandle() noexcept {
    destroy();
}

void GPUStreamHandle::destroy() noexcept {
    if (!stream_) return;

#ifdef THEMIS_ENABLE_CUDA
    cudaError_t err = cudaStreamDestroy(stream_);
    if (err != cudaSuccess) {
        THEMIS_WARN("cudaStreamDestroy failed: {}", cudaGetErrorString(err));
    }
#elif defined(THEMIS_ENABLE_HIP)
    hipError_t err = hipStreamDestroy(stream_);
    if (err != hipSuccess) {
        THEMIS_WARN("hipStreamDestroy failed: {}", hipGetErrorString(err));
    }
#endif

    stream_ = nullptr;
}

void GPUStreamHandle::synchronize() {
#ifdef THEMIS_ENABLE_CUDA
    cudaError_t err = cudaStreamSynchronize(stream_);
    if (err != cudaSuccess) {
        THEMIS_ERROR("cudaStreamSynchronize failed: {}", cudaGetErrorString(err));
        throw std::runtime_error("Failed to synchronize CUDA stream");
    }
#elif defined(THEMIS_ENABLE_HIP)
    hipError_t err = hipStreamSynchronize(stream_);
    if (err != hipSuccess) {
        THEMIS_ERROR("hipStreamSynchronize failed: {}", hipGetErrorString(err));
        throw std::runtime_error("Failed to synchronize HIP stream");
    }
#endif
}

// ============================================================================
// GPUEventHandle Implementation
// ============================================================================

GPUEventHandle::GPUEventHandle() : event_(nullptr) {
#ifdef THEMIS_ENABLE_CUDA
    cudaError_t err = cudaEventCreate(&event_);
    if (err != cudaSuccess) {
        THEMIS_ERROR("cudaEventCreate failed: {}", cudaGetErrorString(err));
        throw std::runtime_error("Failed to create CUDA event");
    }
#elif defined(THEMIS_ENABLE_HIP)
    hipError_t err = hipEventCreate(&event_);
    if (err != hipSuccess) {
        THEMIS_ERROR("hipEventCreate failed: {}", hipGetErrorString(err));
        throw std::runtime_error("Failed to create HIP event");
    }
#else
    event_ = nullptr;
#endif
}

GPUEventHandle::~GPUEventHandle() noexcept {
    destroy();
}

void GPUEventHandle::destroy() noexcept {
    if (!event_) return;

#ifdef THEMIS_ENABLE_CUDA
    cudaError_t err = cudaEventDestroy(event_);
    if (err != cudaSuccess) {
        THEMIS_WARN("cudaEventDestroy failed: {}", cudaGetErrorString(err));
    }
#elif defined(THEMIS_ENABLE_HIP)
    hipError_t err = hipEventDestroy(event_);
    if (err != hipSuccess) {
        THEMIS_WARN("hipEventDestroy failed: {}", hipGetErrorString(err));
    }
#endif

    event_ = nullptr;
}

void GPUEventHandle::record(cudaStream_t stream) {
#ifdef THEMIS_ENABLE_CUDA
    cudaError_t err = cudaEventRecord(event_, stream);
    if (err != cudaSuccess) {
        THEMIS_ERROR("cudaEventRecord failed: {}", cudaGetErrorString(err));
        throw std::runtime_error("Failed to record CUDA event");
    }
#elif defined(THEMIS_ENABLE_HIP)
    hipError_t err = hipEventRecord(event_, stream);
    if (err != hipSuccess) {
        THEMIS_ERROR("hipEventRecord failed: {}", hipGetErrorString(err));
        throw std::runtime_error("Failed to record HIP event");
    }
#endif
}

bool GPUEventHandle::isCompleted() noexcept {
#ifdef THEMIS_ENABLE_CUDA
    cudaError_t err = cudaEventQuery(event_);
    return err == cudaSuccess;
#elif defined(THEMIS_ENABLE_HIP)
    hipError_t err = hipEventQuery(event_);
    return err == hipSuccess;
#else
    return true;
#endif
}

void GPUEventHandle::wait() {
#ifdef THEMIS_ENABLE_CUDA
    cudaError_t err = cudaEventSynchronize(event_);
    if (err != cudaSuccess) {
        THEMIS_ERROR("cudaEventSynchronize failed: {}", cudaGetErrorString(err));
        throw std::runtime_error("Failed to synchronize CUDA event");
    }
#elif defined(THEMIS_ENABLE_HIP)
    hipError_t err = hipEventSynchronize(event_);
    if (err != hipSuccess) {
        THEMIS_ERROR("hipEventSynchronize failed: {}", hipGetErrorString(err));
        throw std::runtime_error("Failed to synchronize HIP event");
    }
#endif
}

// ============================================================================
// GPUKernelTimeoutGuard Implementation
// ============================================================================

GPUKernelTimeoutGuard::GPUKernelTimeoutGuard(
    cudaStream_t stream,
    std::chrono::milliseconds timeout)
    : stream_(stream),
      timeout_(timeout),
      start_time_(std::chrono::steady_clock::now()) {
    // Start monitor thread
    monitor_thread_ = std::thread([this]() { monitorThread(); });
}

GPUKernelTimeoutGuard::~GPUKernelTimeoutGuard() noexcept {
    // Mark completion and wait for monitor thread to finish
    completed_.store(true, std::memory_order_release);

    if (monitor_thread_.joinable()) {
        monitor_thread_.join();
    }

    // If timeout occurred, attempt cleanup
    if (timed_out_.load(std::memory_order_acquire)) {
        cleanupStream();
    }
}

std::chrono::milliseconds GPUKernelTimeoutGuard::getRemainingBudget() const noexcept {
    auto elapsed = std::chrono::steady_clock::now() - start_time_;
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed);

    if (elapsed_ms >= timeout_) {
        return std::chrono::milliseconds(0);
    }
    return timeout_ - elapsed_ms;
}

void GPUKernelTimeoutGuard::monitorThread() noexcept {
    // Poll for timeout with 1ms granularity
    auto poll_interval = std::chrono::milliseconds(1);

    while (!completed_.load(std::memory_order_acquire)) {
        auto remaining = getRemainingBudget();
        if (remaining.count() <= 0) {
            timed_out_.store(true, std::memory_order_release);
            cleanupStream();
            return;
        }

        std::this_thread::sleep_for(poll_interval);
    }

    // Kernel completed in time
    timed_out_.store(false, std::memory_order_release);
}

void GPUKernelTimeoutGuard::cleanupStream() noexcept {
    if (!stream_) return;

#ifdef THEMIS_ENABLE_CUDA
    cudaError_t err = cudaStreamDestroy(stream_);
    if (err != cudaSuccess) {
        THEMIS_WARN("Failed to destroy stream after timeout: {}",
                    cudaGetErrorString(err));
    }
#elif defined(THEMIS_ENABLE_HIP)
    hipError_t err = hipStreamDestroy(stream_);
    if (err != hipSuccess) {
        THEMIS_WARN("Failed to destroy stream after timeout: {}",
                    hipGetErrorString(err));
    }
#endif

    stream_ = nullptr;
}

}  // namespace gpu
}  // namespace themis
