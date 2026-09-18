/**
 * @file hip_backend.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

class HIPVectorBackend : public IVectorBackend {
public:
    struct HIPConfig {
        int deviceId = 0;              // GPU device ID to use
        int waveSize = 0;              // 0=auto, 32=Wave32 (RDNA), 64=Wave64 (CDNA)
        bool enableRocBLAS = false;    // Use rocBLAS for matrix operations
        size_t maxVRAM_MB = 0;         // Maximum VRAM to use (0=auto)
        bool enableProfiling = false;  // Enable HIP event profiling
    };
    
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
    /**
     * @brief HIPVector Backend.
     * @param[in] config Input parameter.
     * @return Return value.
     */
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
     * @brief Get Device Info.
     * @return Return value.
     */
    DeviceInfo getDeviceInfo() const;
    
    /**
     * @brief Set Config.
     * @param[in] config Input parameter.
     */
    void setConfig(const HIPConfig& config);
    
    /**
     * @brief Get Config.
     * @return Return value.
     */
    HIPConfig getConfig() const;
    
    /**
     * @brief Get Available Devices.
     * @return Return value.
     */
    static std::vector<DeviceInfo> getAvailableDevices();
    
    /**
     * @brief Get HIPVersion.
     * @return Return value.
     */
    static std::string getHIPVersion();
    
    /**
     * @brief Get ROCm Version.
     * @return Return value.
     */
    static std::string getROCmVersion();

    ANNKernelDispatch populateANNDispatch() const override;

private:
    std::unique_ptr<HIPBackendImpl> impl_;
};

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

    GeoKernelDispatch populateGeoDispatch() const override;

private:
    bool initialized_ = false;

#ifdef THEMIS_ENABLE_HIP
    raii::HipStream stream_;
#endif
};

} // namespace acceleration
} // namespace themis
