/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            cuda_raii.h                                        ║
  Version:         0.0.39                                             ║
  Last Modified:   2026-04-13 20:19:47                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     296                                            ║
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

// RAII wrappers for CUDA resources
// Provides automatic resource cleanup and exception safety
// Header-only implementation for ease of use

#ifdef THEMIS_ENABLE_CUDA
#include <cuda_runtime.h>
#include <memory>
#include <stdexcept>
#include <string>

namespace themis {
namespace acceleration {
namespace raii {

// ============================================================================
// CUDA Stream RAII Wrapper
// ============================================================================

class CudaStream {
public:
    CudaStream() : stream_(nullptr), owned_(false) {}
    
    // Create a new stream
    explicit CudaStream(bool createNow, unsigned int flags = cudaStreamDefault) 
        : stream_(nullptr), owned_(false) {
        if (createNow) {
            create(flags);
        }
    }
    
    // Wrap an existing stream (non-owning)
    static CudaStream wrap(cudaStream_t stream) {
        CudaStream wrapper;
        wrapper.stream_ = stream;
        wrapper.owned_ = false;
        return wrapper;
    }
    
    // No copy
    CudaStream(const CudaStream&) = delete;
    CudaStream& operator=(const CudaStream&) = delete;
    
    // Move semantics
    CudaStream(CudaStream&& other) noexcept 
        : stream_(other.stream_), owned_(other.owned_) {
        other.stream_ = nullptr;
        other.owned_ = false;
    }
    
    CudaStream& operator=(CudaStream&& other) noexcept {
        if (this != &other) {
            destroy();
            stream_ = other.stream_;
            owned_ = other.owned_;
            other.stream_ = nullptr;
            other.owned_ = false;
        }
        return *this;
    }
    
    ~CudaStream() {
        destroy();
    }
    
    // Create stream with specified flags
    void create(unsigned int flags = cudaStreamDefault) {
        if (stream_ && owned_) {
            destroy();
        }
        
        cudaError_t err = cudaStreamCreate(&stream_);
        if (err != cudaSuccess) {
            throw std::runtime_error(
                std::string("Failed to create CUDA stream: ") + 
                cudaGetErrorString(err)
            );
        }
        owned_ = true;
    }
    
    // Create with priority
    void createWithPriority(int priority, unsigned int flags = cudaStreamNonBlocking) {
        if (stream_ && owned_) {
            destroy();
        }
        
        cudaError_t err = cudaStreamCreateWithPriority(&stream_, flags, priority);
        if (err != cudaSuccess) {
            throw std::runtime_error(
                std::string("Failed to create CUDA stream with priority: ") + 
                cudaGetErrorString(err)
            );
        }
        owned_ = true;
    }
    
    // Synchronize stream
    void synchronize() {
        if (stream_) {
            cudaError_t err = cudaStreamSynchronize(stream_);
            if (err != cudaSuccess) {
                throw std::runtime_error(
                    std::string("CUDA stream synchronization failed: ") + 
                    cudaGetErrorString(err)
                );
            }
        }
    }
    
    // Check if stream is valid
    bool valid() const { return stream_ != nullptr; }
    
    // Get underlying stream handle
    cudaStream_t get() const { return stream_; }
    
    // Release ownership (caller must manage lifetime)
    cudaStream_t release() {
        owned_ = false;
        cudaStream_t tmp = stream_;
        stream_ = nullptr;
        return tmp;
    }
    
private:
    void destroy() {
        if (stream_ && owned_) {
            cudaStreamDestroy(stream_);
        }
        stream_ = nullptr;
        owned_ = false;
    }
    
    cudaStream_t stream_;
    bool owned_;
};

// ============================================================================
// CUDA Device Memory RAII Wrapper
// ============================================================================

class CudaDeviceMemory {
public:
    CudaDeviceMemory() : ptr_(nullptr), size_(0) {}
    
    explicit CudaDeviceMemory(size_t size) : ptr_(nullptr), size_(0) {
        if (size > 0) {
            allocate(size);
        }
    }
    
    // No copy
    CudaDeviceMemory(const CudaDeviceMemory&) = delete;
    CudaDeviceMemory& operator=(const CudaDeviceMemory&) = delete;
    
    // Move semantics
    CudaDeviceMemory(CudaDeviceMemory&& other) noexcept 
        : ptr_(other.ptr_), size_(other.size_) {
        other.ptr_ = nullptr;
        other.size_ = 0;
    }
    
    CudaDeviceMemory& operator=(CudaDeviceMemory&& other) noexcept {
        if (this != &other) {
            free();
            ptr_ = other.ptr_;
            size_ = other.size_;
            other.ptr_ = nullptr;
            other.size_ = 0;
        }
        return *this;
    }
    
    ~CudaDeviceMemory() {
        free();
    }
    
    void allocate(size_t size) {
        if (ptr_) {
            free();
        }
        
        if (size == 0) {
            return;
        }
        
        cudaError_t err = cudaMalloc(&ptr_, size);
        if (err != cudaSuccess) {
            throw std::runtime_error(
                std::string("Failed to allocate CUDA device memory (") + 
                std::to_string(size) + " bytes): " + 
                cudaGetErrorString(err)
            );
        }
        size_ = size;
    }
    
    void copyFrom(const void* host, size_t size, cudaStream_t stream = 0) {
        if (!ptr_) {
            throw std::runtime_error("Cannot copy to unallocated CUDA memory");
        }
        if (size > size_) {
            throw std::runtime_error("Copy size exceeds allocated size");
        }
        
        cudaError_t err;
        if (stream) {
            err = cudaMemcpyAsync(ptr_, host, size, cudaMemcpyHostToDevice, stream);
        } else {
            err = cudaMemcpy(ptr_, host, size, cudaMemcpyHostToDevice);
        }
        
        if (err != cudaSuccess) {
            throw std::runtime_error(
                std::string("Failed to copy to CUDA device memory: ") + 
                cudaGetErrorString(err)
            );
        }
    }
    
    void copyTo(void* host, size_t size, cudaStream_t stream = 0) const {
        if (!ptr_) {
            throw std::runtime_error("Cannot copy from unallocated CUDA memory");
        }
        if (size > size_) {
            throw std::runtime_error("Copy size exceeds allocated size");
        }
        
        cudaError_t err;
        if (stream) {
            err = cudaMemcpyAsync(host, ptr_, size, cudaMemcpyDeviceToHost, stream);
        } else {
            err = cudaMemcpy(host, ptr_, size, cudaMemcpyDeviceToHost);
        }
        
        if (err != cudaSuccess) {
            throw std::runtime_error(
                std::string("Failed to copy from CUDA device memory: ") + 
                cudaGetErrorString(err)
            );
        }
    }
    
    bool valid() const { return ptr_ != nullptr; }
    void* get() const { return ptr_; }
    size_t size() const { return size_; }
    
    void* release() {
        void* tmp = ptr_;
        ptr_ = nullptr;
        size_ = 0;
        return tmp;
    }
    
private:
    void free() {
        if (ptr_) {
            cudaFree(ptr_);
            ptr_ = nullptr;
            size_ = 0;
        }
    }
    
    void* ptr_;
    size_t size_;
};

} // namespace raii
} // namespace acceleration
} // namespace themis

#endif // THEMIS_ENABLE_CUDA
