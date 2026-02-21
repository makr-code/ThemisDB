/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            cpu_backend.h                                      ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-02-21 10:35:50                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     178                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 10d6fc4a6  2026-01-25  feat(build): Add missing cmake configurations for Voice A... ║
    • 5c4835268  2026-01-14  Fix 11 compilation errors: missing macros, interface mism... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "acceleration/compute_backend.h"

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
        caps.deviceName = "CPU (Fallback)";
        return caps;
    }
    
    bool initialize() override { return true; }
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
        caps.deviceName = "CPU (Fallback)";
        return caps;
    }
    
    bool initialize() override { return true; }
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
        caps.deviceName = "CPU (Fallback)";
        return caps;
    }
    
    bool initialize() override { return true; }
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

protected:
    double haversineDistance(double lat1, double lon1, double lat2, double lon2) const;
    double vincentyDistance(double lat1, double lon1, double lat2, double lon2) const;
};

} // namespace acceleration
} // namespace themis
