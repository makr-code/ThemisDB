/**
 * @file geo_acceleration_bridge.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "acceleration/compute_backend.h"

namespace themis {
namespace acceleration {

class GeoAccelerationBridge : public IGeoBackend {
public:
    GeoAccelerationBridge();
    ~GeoAccelerationBridge() override = default;

    // -----------------------------------------------------------------------
    // IComputeBackend
    // -----------------------------------------------------------------------
    const char* name() const noexcept override { return "GeoAccelerationBridge"; }
    BackendType type() const noexcept override;
    bool isAvailable() const noexcept override;
    BackendCapabilities getCapabilities() const override;
    bool initialize() override;
    void shutdown() override;

    // -----------------------------------------------------------------------
    // IGeoBackend
    // -----------------------------------------------------------------------

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

    // -----------------------------------------------------------------------
    // GeoKernelDispatch — wired to bridge_geo_distance / bridge_geo_containment
    // GeoKernelDispatch — returns CUDA launchers when CUDA is available,
    // CPU launchers otherwise so the dispatch table is always fully populated.
    // -----------------------------------------------------------------------
    GeoKernelDispatch populateGeoDispatch() const override;

private:
    /**
     * @brief Haversine Km.
     * @param[in] lat1 Input parameter.
     * @param[in] lon1 Input parameter.
     * @param[in] lat2 Input parameter.
     * @param[in] lon2 Input parameter.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    static double haversineKm(double lat1, double lon1,
                               double lat2, double lon2) noexcept;
};

} // namespace acceleration
} // namespace themis
