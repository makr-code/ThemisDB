/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            hip_backend.h                                      ║
  Version:         0.0.21                                             ║
  Last Modified:   2026-02-21 19:19:57                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     171                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 0d722b04c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "acceleration/compute_backend.h"
#include <string>
#include <memory>

namespace themis {
namespace acceleration {

// Forward declarations for HIP types (avoid including HIP headers in public API)
struct HIPBackendImpl;

/**
 * HIP Backend for AMD GPU Acceleration
 * 
 * Provides GPU-accelerated vector operations using AMD ROCm/HIP platform.
 * Compatible with AMD Radeon GPUs (RDNA2, RDNA3) and AMD Instinct GPUs (CDNA).
 * 
 * Features:
 * - L2, Cosine, and Inner Product distance computation on GPU
 * - Batch KNN search with parallel top-k selection
 * - Asynchronous operations via HIP streams
 * - Architecture-specific optimizations (Wave32/Wave64)
 * 
 * Hardware Requirements:
 * - AMD GPU with ROCm support (RDNA2+, CDNA)
 * - Minimum 8GB VRAM recommended
 * - ROCm 5.0+ installed
 * 
 * @see docs/GPU_SUPPORT_ROADMAP.md for setup instructions
 * @see https://rocm.docs.amd.com/ for ROCm documentation
 */
class HIPVectorBackend : public IVectorBackend {
public:
    /**
     * Configuration options for HIP backend
     */
    struct HIPConfig {
        int deviceId = 0;              // GPU device ID to use
        int waveSize = 0;              // 0=auto, 32=Wave32 (RDNA), 64=Wave64 (CDNA)
        bool enableRocBLAS = false;    // Use rocBLAS for matrix operations
        size_t maxVRAM_MB = 0;         // Maximum VRAM to use (0=auto)
        bool enableProfiling = false;  // Enable HIP event profiling
    };
    
    /**
     * Device information structure
     */
    struct DeviceInfo {
        std::string name;
        int computeUnits = 0;
        size_t totalMemory = 0;
        int waveSize = 0;
        std::string gcnArchName;
        bool supportsInt8 = false;
        bool supportsFP16 = false;
    };
    
    HIPVectorBackend();
    explicit HIPVectorBackend(const HIPConfig& config);
    ~HIPVectorBackend() override;
    
    // IComputeBackend interface
    const char* name() const noexcept override;
    BackendType type() const noexcept override;
    bool isAvailable() const noexcept override;
    BackendCapabilities getCapabilities() const override;
    bool initialize() override;
    void shutdown() override;
    
    // IVectorBackend interface
    std::vector<float> computeDistances(
        const float* queries,
        size_t numQueries,
        size_t dim,
        const float* vectors,
        size_t numVectors,
        bool useL2 = true
    ) override;
    
    std::vector<std::vector<std::pair<uint32_t, float>>> batchKnnSearch(
        const float* queries,
        size_t numQueries,
        size_t dim,
        const float* vectors,
        size_t numVectors,
        size_t k,
        bool useL2 = true
    ) override;
    
    // Extended metric-aware batch KNN search
    enum class DistanceMetric {
        L2,
        COSINE,
        INNER_PRODUCT
    };
    
    std::vector<std::vector<std::pair<uint32_t, float>>> batchKnnSearchWithMetric(
        const float* queries,
        size_t numQueries,
        size_t dim,
        const float* vectors,
        size_t numVectors,
        size_t k,
        DistanceMetric metric
    );
    
    // HIP-specific methods
    
    /**
     * Get device information
     */
    DeviceInfo getDeviceInfo() const;
    
    /**
     * Set HIP configuration
     */
    void setConfig(const HIPConfig& config);
    
    /**
     * Get current configuration
     */
    HIPConfig getConfig() const;
    
    /**
     * Query available HIP devices
     */
    static std::vector<DeviceInfo> getAvailableDevices();
    
    /**
     * Get HIP version info
     */
    static std::string getHIPVersion();
    
    /**
     * Get ROCm version info
     */
    static std::string getROCmVersion();
    
private:
    std::unique_ptr<HIPBackendImpl> impl_;
};

} // namespace acceleration
} // namespace themis
