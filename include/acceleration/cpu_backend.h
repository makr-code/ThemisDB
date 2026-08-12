/**
 * @file cpu_backend.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "acceleration/compute_backend.h"
#include "acceleration/tensor_core_matmul.h"

namespace themis {
namespace acceleration {

// CPU fallback implementation for vector operations
class CPUVectorBackend : public IVectorBackend {
public:
    CPUVectorBackend() = default;
    ~CPUVectorBackend() override = default;
    
    // IComputeBackend interface
    const char* name() const noexcept override { return "CPU"; }
    BackendType type() const noexcept override { return BackendType::CPU; }
    bool isAvailable() const noexcept override { return true; }
    
    BackendCapabilities getCapabilities() const override {
        BackendCapabilities caps;
        caps.supportsVectorOps = true;
        caps.supportsGraphOps = false;
        caps.supportsGeoOps = false;
        caps.supportsBatchProcessing = true;
        caps.supportsAsync = false;
        caps.supportedPrecisions = PrecisionMode::FP32;
        caps.supportedMetrics = metricBit(DistanceMetric::L2)
                              | metricBit(DistanceMetric::COSINE)
                              | metricBit(DistanceMetric::INNER_PRODUCT);
        caps.deviceName = "CPU (Fallback)";
        return caps;
    }

    bool initialize() override {
        clearError();
        return true;
    }
    void shutdown() override {}

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

    ANNKernelDispatch populateANNDispatch() const override;
    
protected:
    // Allow derived classes to call these helper methods
    float computeL2Distance(const float* a, const float* b, size_t dim) const;
    float computeCosineDistance(const float* a, const float* b, size_t dim) const;
};

// CPU fallback implementation for graph operations
class CPUGraphBackend : public IGraphBackend {
public:
    CPUGraphBackend() = default;
    ~CPUGraphBackend() override = default;
    
    // IComputeBackend interface
    const char* name() const noexcept override { return "CPU"; }
    BackendType type() const noexcept override { return BackendType::CPU; }
    bool isAvailable() const noexcept override { return true; }
    
    BackendCapabilities getCapabilities() const override {
        BackendCapabilities caps;
        caps.supportsVectorOps = false;
        caps.supportsGraphOps = true;
        caps.supportsGeoOps = false;
        caps.supportsBatchProcessing = true;
        caps.supportsAsync = false;
        caps.supportedPrecisions = PrecisionMode::FP32;
        caps.supportedMetrics = 0; // graph ops do not use distance metrics
        caps.deviceName = "CPU (Fallback)";
        return caps;
    }

    bool initialize() override {
        clearError();
        return true;
    }
    void shutdown() override {}

    // IGraphBackend interface
    std::vector<std::vector<uint32_t>> batchBFS(
        const uint32_t* adjacency,
        size_t numVertices,
        const uint32_t* startVertices,
        size_t numStarts,
        uint32_t maxDepth
    ) override;
    
    std::vector<std::vector<uint32_t>> batchShortestPath(
        const uint32_t* adjacency,
        const float* weights,
        size_t numVertices,
        const uint32_t* startVertices,
        const uint32_t* endVertices,
        size_t numPairs
    ) override;
};

// CPU fallback implementation for geo operations
class CPUGeoBackend : public IGeoBackend {
public:
    CPUGeoBackend() = default;
    ~CPUGeoBackend() override = default;
    
    // IComputeBackend interface
    const char* name() const noexcept override { return "CPU"; }
    BackendType type() const noexcept override { return BackendType::CPU; }
    bool isAvailable() const noexcept override { return true; }
    
    BackendCapabilities getCapabilities() const override {
        BackendCapabilities caps;
        caps.supportsVectorOps = false;
        caps.supportsGraphOps = false;
        caps.supportsGeoOps = true;
        caps.supportsBatchProcessing = true;
        caps.supportsAsync = false;
        caps.supportedPrecisions = PrecisionMode::FP32;
        caps.supportedMetrics = 0; // geo ops do not use ANN distance metrics
        caps.deviceName = "CPU (Fallback)";
        return caps;
    }
    
    bool initialize() override {
        clearError();
        return true;
    }
    void shutdown() override {}
    
    // IGeoBackend interface
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

protected:
    double haversineDistance(double lat1, double lon1, double lat2, double lon2) const;
    double vincentyDistance(double lat1, double lon1, double lat2, double lon2) const;
};

// CPU fallback implementation for FP16/BF16 matrix operations.
// On CPU all precisions are executed as FP32.
class CPUMatrixBackend : public IMatrixBackend {
public:
    CPUMatrixBackend() = default;
    ~CPUMatrixBackend() override = default;

    const char* name() const noexcept override { return "CPU"; }
    BackendType type() const noexcept override { return BackendType::CPU; }
    bool isAvailable() const noexcept override { return true; }

    BackendCapabilities getCapabilities() const override {
        BackendCapabilities caps;
        caps.supportsMatrixOps     = true;
        caps.supportsBatchProcessing = true;
        caps.supportsAsync         = false;
        caps.supportedPrecisions   = PrecisionMode::FP32;
        caps.deviceName            = "CPU (Fallback)";
        return caps;
    }

    bool initialize() override {
        clearError();
        return true;
    }
    void shutdown() override {}

    // IMatrixBackend interface
    int matmul(const MatrixKernelParams& params, void* opaque_stream = nullptr) override;

    MatrixKernelDispatch populateMatrixDispatch() const override;
};

} // namespace acceleration
} // namespace themis
