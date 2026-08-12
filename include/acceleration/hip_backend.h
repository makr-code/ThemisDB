/**
 * @file hip_backend.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "acceleration/compute_backend.h"
#include "acceleration/kernel_invocation.h"
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

    /**
     * Populate the frozen ANN kernel dispatch table for this backend.
     *
     * When THEMIS_ENABLE_HIP is defined the returned table references the
     * kernel launchers compiled in src/acceleration/hip/ann_kernels.hip.
     * When HIP is unavailable all slots are null and the BackendRegistry
     * falls back to the CPU table.
     */
    ANNKernelDispatch populateANNDispatch() const override;

private:
    std::unique_ptr<HIPBackendImpl> impl_;
};

/**
 * HIP Geospatial Backend for AMD GPU Acceleration
 *
 * Provides GPU-accelerated geospatial operations using the AMD ROCm/HIP
 * platform.  Implements the IGeoBackend interface and exposes the frozen
 * GeoKernelDispatch table wired to the launchers compiled in
 * src/acceleration/hip/geo_kernels.hip.
 *
 * When THEMIS_ENABLE_HIP is not defined the backend reports itself as
 * unavailable and all operations are no-ops, allowing the BackendRegistry
 * to fall back to the CPU geo backend.
 */
class HIPGeoBackend : public IGeoBackend {
public:
    HIPGeoBackend() = default;
    ~HIPGeoBackend() override;

    const char* name() const noexcept override { return "HIP"; }
    BackendType type() const noexcept override { return BackendType::HIP; }
    bool isAvailable() const noexcept override;

    BackendCapabilities getCapabilities() const override;
    bool initialize() override;
    void shutdown() override;

    std::vector<float> batchDistances(
        const double* latitudes1,
        const double* longitudes1,
        const double* latitudes2,
        const double* longitudes2,
        size_t count,
        bool useHaversine = true
    ) override;

    std::vector<bool> batchPointInPolygon(
        const double* pointLats,
        const double* pointLons,
        size_t numPoints,
        const double* polygonCoords,
        size_t numPolygonVertices
    ) override;

    /**
     * Populate the frozen geo kernel dispatch table for this backend.
     *
     * When THEMIS_ENABLE_HIP is defined the returned table references the
     * kernel launchers compiled in src/acceleration/hip/geo_kernels.hip.
     * When HIP is unavailable all slots are null and the BackendRegistry
     * falls back to the CPU geo table.
     */
    GeoKernelDispatch populateGeoDispatch() const override;

private:
    bool initialized_ = false;

#ifdef THEMIS_ENABLE_HIP
    raii::HipStream stream_;
#endif
};

} // namespace acceleration
} // namespace themis
