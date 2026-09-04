/**
 * @file vram_secure_clear.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "security/vram_secure_clear.h"
#include <cstring>
#include <spdlog/spdlog.h>
#include <vector>

// Conditional CUDA/HIP includes
#ifdef THEMIS_ENABLE_CUDA
#include <cuda_runtime.h>
#endif

#ifdef THEMIS_ENABLE_HIP
#include <hip/hip_runtime.h>
#endif

namespace themis {
namespace security {

constexpr uint8_t VRAMSecureClear::PATTERNS[];

bool VRAMSecureClear::secureClearCUDA(void* ptr, size_t size_bytes) {
    return secureClearCUDA(ptr, size_bytes, Config{});
}

bool VRAMSecureClear::secureClearCUDA(void* ptr, size_t size_bytes, const Config& config) {
#ifdef THEMIS_ENABLE_CUDA
    if (ptr == nullptr || size_bytes == 0) {
        return true;  // Nothing to clear
    }
    
    if (config.audit_log) {
        spdlog::debug("VRAM secure clear: {} bytes at {}, {} passes", 
                     size_bytes, ptr, config.num_passes);
    }
    
    // Perform multi-pass clearing with different patterns
    for (int pass = 0; pass < config.num_passes; ++pass) {
        uint8_t pattern = PATTERNS[pass % (sizeof(PATTERNS) / sizeof(PATTERNS[0]))];
        
        cudaError_t err = cudaMemset(ptr, pattern, size_bytes);
        if (err != cudaSuccess) {
            spdlog::error("VRAM secure clear failed at pass {}: {}", 
                         pass, cudaGetErrorString(err));
            return false;
        }
        
        // Ensure completion before next pass
        err = cudaDeviceSynchronize();
        if (err != cudaSuccess) {
            spdlog::error("VRAM secure clear sync failed at pass {}: {}", 
                         pass, cudaGetErrorString(err));
            return false;
        }
    }
    
    // Optional verification
    if (config.verify_clear) {
        std::vector<uint8_t> verify_buffer(std::min(size_bytes, size_t(4096)));
        cudaError_t err = cudaMemcpy(verify_buffer.data(), ptr, 
                                     verify_buffer.size(), cudaMemcpyDeviceToHost);
        if (err == cudaSuccess) {
            // Check that buffer contains the last pattern
            uint8_t expected = PATTERNS[(config.num_passes - 1) % 
                                       (sizeof(PATTERNS) / sizeof(PATTERNS[0]))];
            bool all_cleared = true;
            for (size_t i = 0; i <static_cast<int>(verify_buffer.size()); ++i) {
                if (verify_buffer[i] != expected) {
                    all_cleared = false;
                    break;
                }
            }
            
            if (!all_cleared) {
                spdlog::warn("VRAM secure clear verification failed - memory not fully cleared");
            }
        }
    }
    
    if (config.audit_log) {
        spdlog::debug("VRAM secure clear completed: {} bytes", size_bytes);
    }
    
    return true;
#else
    (void)ptr;
    (void)size_bytes;
    (void)config;
    spdlog::warn("VRAM secure clear called but CUDA not enabled");
    return false;
#endif
}

bool VRAMSecureClear::secureClearHIP(void* ptr, size_t size_bytes) {
    return secureClearHIP(ptr, size_bytes, Config{});
}

bool VRAMSecureClear::secureClearHIP(void* ptr, size_t size_bytes, const Config& config) {
#ifdef THEMIS_ENABLE_HIP
    if (ptr == nullptr || size_bytes == 0) {
        return true;  // Nothing to clear
    }
    
    if (config.audit_log) {
        spdlog::debug("VRAM secure clear (HIP): {} bytes at {}, {} passes", 
                     size_bytes, ptr, config.num_passes);
    }
    
    // Perform multi-pass clearing with different patterns
    for (int pass = 0; pass < config.num_passes; ++pass) {
        uint8_t pattern = PATTERNS[pass % (sizeof(PATTERNS) / sizeof(PATTERNS[0]))];
        
        hipError_t err = hipMemset(ptr, pattern, size_bytes);
        if (err != hipSuccess) {
            spdlog::error("VRAM secure clear (HIP) failed at pass {}: {}", 
                         pass, hipGetErrorString(err));
            return false;
        }
        
        // Ensure completion before next pass
        err = hipDeviceSynchronize();
        if (err != hipSuccess) {
            spdlog::error("VRAM secure clear (HIP) sync failed at pass {}: {}", 
                         pass, hipGetErrorString(err));
            return false;
        }
    }
    
    // Optional verification
    if (config.verify_clear) {
        std::vector<uint8_t> verify_buffer(std::min(size_bytes, size_t(4096)));
        hipError_t err = hipMemcpy(verify_buffer.data(), ptr, 
                                   verify_buffer.size(), hipMemcpyDeviceToHost);
        if (err == hipSuccess) {
            uint8_t expected = PATTERNS[(config.num_passes - 1) % 
                                       (sizeof(PATTERNS) / sizeof(PATTERNS[0]))];
            bool all_cleared = true;
            for (size_t i = 0; i <static_cast<int>(verify_buffer.size()); ++i) {
                if (verify_buffer[i] != expected) {
                    all_cleared = false;
                    break;
                }
            }
            
            if (!all_cleared) {
                spdlog::warn("VRAM secure clear (HIP) verification failed");
            }
        }
    }
    
    if (config.audit_log) {
        spdlog::debug("VRAM secure clear (HIP) completed: {} bytes", size_bytes);
    }
    
    return true;
#else
    (void)ptr;
    (void)size_bytes;
    (void)config;
    spdlog::warn("VRAM secure clear (HIP) called but HIP not enabled");
    return false;
#endif
}

void VRAMSecureClear::secureClearCPU(void* ptr, size_t size_bytes) {
    secureClearCPU(ptr, size_bytes, Config{});
}

void VRAMSecureClear::secureClearCPU(void* ptr, size_t size_bytes, const Config& config) {
    if (ptr == nullptr || size_bytes == 0) {
        return;
    }
    
    if (config.audit_log) {
        spdlog::debug("CPU secure clear: {} bytes at {}, {} passes", 
                     size_bytes, ptr, config.num_passes);
    }
    
    // Use volatile to prevent compiler optimization
    volatile uint8_t* vptr = static_cast<volatile uint8_t*>(ptr);
    
    // Multi-pass overwrite
    for (int pass = 0; pass < config.num_passes; ++pass) {
        uint8_t pattern = PATTERNS[pass % (sizeof(PATTERNS) / sizeof(PATTERNS[0]))];
        for (size_t i = 0; i < size_bytes; ++i) {
            vptr[i] = pattern;
        }
    }
    
    if (config.audit_log) {
        spdlog::debug("CPU secure clear completed: {} bytes", size_bytes);
    }
}

} // namespace security
} // namespace themis
