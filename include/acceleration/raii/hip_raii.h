/*
 * ThemisDB | File: hip_raii.h | Version: 0.0.47 | Last Modified: 2026-05-31 12:17:24
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 323
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * PR History (last 5): #2072 feat(acceleration): Vulkan ... (2026-03-11)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#pragma once

// RAII wrappers for HIP resources (AMD ROCm)
// Provides automatic resource cleanup and exception safety
// Header-only implementation for ease of use

#ifdef THEMIS_ENABLE_HIP
#include <hip/hip_runtime.h>
#include <memory>
#include <stdexcept>
#include <string>

namespace themis {
namespace acceleration {
namespace raii {

// ============================================================================
// HIP Stream RAII Wrapper
// ============================================================================

class HipStream {
public:
    HipStream() : stream_(nullptr), owned_(false) {}
    
    // Create a new stream
    explicit HipStream(bool createNow, unsigned int flags = 0) 
        : stream_(nullptr), owned_(false) {
        if (createNow) {
            create(flags);
        }
    }
    
    // Wrap an existing stream (non-owning)
    static HipStream wrap(hipStream_t stream) {
        HipStream wrapper;
        wrapper.stream_ = stream;
        wrapper.owned_ = false;
        return wrapper;
    }
    
    // No copy
    HipStream(const HipStream&) = delete;
    HipStream& operator=(const HipStream&) = delete;
    
    // Move semantics
    HipStream(HipStream&& other) noexcept 
        : stream_(other.stream_), owned_(other.owned_) {
        other.stream_ = nullptr;
        other.owned_ = false;
    }
    
    HipStream& operator=(HipStream&& other) noexcept {
        if (this != &other) {
            destroy();
            stream_ = other.stream_;
            owned_ = other.owned_;
            other.stream_ = nullptr;
            other.owned_ = false;
        }
        return *this;
    }
    
    ~HipStream() {
        destroy();
    }
    
    // Create stream with specified flags
    void create(unsigned int flags = 0) {
        if (stream_ && owned_) {
            destroy();
        }
        
        hipError_t err = hipStreamCreate(&stream_);
        if (err != hipSuccess) {
            throw std::runtime_error(
                std::string("Failed to create HIP stream: ") + 
                hipGetErrorString(err)
            );
        }
        owned_ = true;
    }
    
    // Create with priority
    void createWithPriority(int priority, unsigned int flags = hipStreamNonBlocking) {
        if (stream_ && owned_) {
            destroy();
        }
        
        hipError_t err = hipStreamCreateWithPriority(&stream_, flags, priority);
        if (err != hipSuccess) {
            throw std::runtime_error(
                std::string("Failed to create HIP stream with priority: ") + 
                hipGetErrorString(err)
            );
        }
        owned_ = true;
    }
    
    // Synchronize stream
    void synchronize() {
        if (stream_) {
            hipError_t err = hipStreamSynchronize(stream_);
            if (err != hipSuccess) {
                throw std::runtime_error(
                    std::string("HIP stream synchronization failed: ") + 
                    hipGetErrorString(err)
                );
            }
        }
    }
    
    // Check if stream is valid
    bool valid() const { return stream_ != nullptr; }
    
    // Get underlying stream handle
    hipStream_t get() const { return stream_; }
    
    // Release ownership (caller must manage lifetime)
    hipStream_t release() {
        owned_ = false;
        hipStream_t tmp = stream_;
        stream_ = nullptr;
        return tmp;
    }
    
private:
    void destroy() {
        if (stream_ && owned_) {
            hipStreamDestroy(stream_);
        }
        stream_ = nullptr;
        owned_ = false;
    }
    
    hipStream_t stream_;
    bool owned_;
};

// ============================================================================
// HIP Device Memory RAII Wrapper
// ============================================================================

class HipDeviceMemory {
public:
    HipDeviceMemory() : ptr_(nullptr), size_(0) {}
    
    explicit HipDeviceMemory(size_t size) : ptr_(nullptr), size_(0) {
        if (size > 0) {
            allocate(size);
        }
    }
    
    // No copy
    HipDeviceMemory(const HipDeviceMemory&) = delete;
    HipDeviceMemory& operator=(const HipDeviceMemory&) = delete;
    
    // Move semantics
    HipDeviceMemory(HipDeviceMemory&& other) noexcept 
        : ptr_(other.ptr_), size_(other.size_) {
        other.ptr_ = nullptr;
        other.size_ = 0;
    }
    
    HipDeviceMemory& operator=(HipDeviceMemory&& other) noexcept {
        if (this != &other) {
            free();
            ptr_ = other.ptr_;
            size_ = other.size_;
            other.ptr_ = nullptr;
            other.size_ = 0;
        }
        return *this;
    }
    
    ~HipDeviceMemory() {
        free();
    }
    
    void allocate(size_t size) {
        if (ptr_) {
            free();
        }
        
        if (size == 0) {
            return;
        }
        
        hipError_t err = hipMalloc(&ptr_, size);
        if (err != hipSuccess) {
            throw std::runtime_error(
                std::string("Failed to allocate HIP device memory (") + 
                std::to_string(size) + " bytes): " + 
                hipGetErrorString(err)
            );
        }
        size_ = size;
    }
    
    void copyFrom(const void* host, size_t size, hipStream_t stream = 0) {
        if (!ptr_) {
            throw std::runtime_error("Cannot copy to unallocated HIP memory");
        }
        if (size > size_) {
            throw std::runtime_error("Copy size exceeds allocated size");
        }
        
        hipError_t err;
        if (stream) {
            err = hipMemcpyAsync(ptr_, host, size, hipMemcpyHostToDevice, stream);
        } else {
            err = hipMemcpy(ptr_, host, size, hipMemcpyHostToDevice);
        }
        
        if (err != hipSuccess) {
            throw std::runtime_error(
                std::string("Failed to copy to HIP device memory: ") + 
                hipGetErrorString(err)
            );
        }
    }
    
    void copyTo(void* host, size_t size, hipStream_t stream = 0) const {
        if (!ptr_) {
            throw std::runtime_error("Cannot copy from unallocated HIP memory");
        }
        if (size > size_) {
            throw std::runtime_error("Copy size exceeds allocated size");
        }
        
        hipError_t err;
        if (stream) {
            err = hipMemcpyAsync(host, ptr_, size, hipMemcpyDeviceToHost, stream);
        } else {
            err = hipMemcpy(host, ptr_, size, hipMemcpyDeviceToHost);
        }
        
        if (err != hipSuccess) {
            throw std::runtime_error(
                std::string("Failed to copy from HIP device memory: ") + 
                hipGetErrorString(err)
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
            hipFree(ptr_);
            ptr_ = nullptr;
            size_ = 0;
        }
    }
    
    void* ptr_;
    size_t size_;
};

// ============================================================================
// Scoped HIP Device Setter
// ============================================================================

class ScopedHipDevice {
public:
    explicit ScopedHipDevice(int deviceId) : previousDevice_(-1) {
        hipError_t err = hipGetDevice(&previousDevice_);
        if (err != hipSuccess) {
            throw std::runtime_error(
                std::string("Failed to get current HIP device: ") + 
                hipGetErrorString(err)
            );
        }
        
        err = hipSetDevice(deviceId);
        if (err != hipSuccess) {
            throw std::runtime_error(
                std::string("Failed to set HIP device to ") + 
                std::to_string(deviceId) + ": " + 
                hipGetErrorString(err)
            );
        }
    }
    
    ~ScopedHipDevice() {
        if (previousDevice_ >= 0) {
            hipSetDevice(previousDevice_);
        }
    }
    
    // No copy or move
    ScopedHipDevice(const ScopedHipDevice&) = delete;
    ScopedHipDevice& operator=(const ScopedHipDevice&) = delete;
    ScopedHipDevice(ScopedHipDevice&&) = delete;
    ScopedHipDevice& operator=(ScopedHipDevice&&) = delete;
    
private:
    int previousDevice_;
};

} // namespace raii
} // namespace acceleration
} // namespace themis

#endif // THEMIS_ENABLE_HIP
