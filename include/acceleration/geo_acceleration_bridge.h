/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            geo_acceleration_bridge.h                          ║
  Version:         0.0.8                                              ║
  Last Modified:   2026-04-14 06:49:36                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     129                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • e38662530e  2026-02-24  feat(acceleration): implement GeoAccelerationBridge::popu... ║
    • 1315cab747  2026-02-24  Implement GeoAccelerationBridge::populateGeoDispatch() to... ║
    • 6dd23063fc  2026-02-23  feat(acceleration): integrate GPU backend with geo module... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "acceleration/compute_backend.h"

namespace themis {
namespace acceleration {

/**
 * @brief Bridges the acceleration IGeoBackend interface to the geo module's
 *        ISpatialComputeBackend (GpuBatchBackend).
 *
 * GeoAccelerationBridge makes the geo module's GPU spatial backend visible to
 * the acceleration BackendRegistry.  It adapts the flat array-based
 * batchDistances() / batchPointInPolygon() API of IGeoBackend to the
 * GeometryInfo-based batchIntersects() / exactIntersects() API of the geo
 * module, and delegates to the singleton returned by
 * themis::geo::getGpuSpatialBackend().
 *
 * isAvailable() returns true whenever the underlying geo GPU backend reports
 * availability; the bridge falls back to a CPU haversine implementation when
 * the GPU is absent or the circuit-breaker is open, so the result is always
 * valid.
 *
 * Thread safety: all methods are thread-safe (the underlying GpuBatchBackend
 * is itself thread-safe).
 */
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

    /**
     * @brief Batch Haversine (or Euclidean) distances between point pairs.
     *
     * Computes great-circle distances in kilometres between corresponding
     * (lat1,lon1) and (lat2,lon2) pairs.  The computation runs on CPU using
     * the Haversine formula.  For GPU kernel dispatch use populateGeoDispatch()
     * which returns CUDA launchers when a CUDA device is present.
     *
     * @param useHaversine  When true (default), applies the Haversine formula
     *                      for WGS-84 geodesic distances.  When false, returns
     *                      planar Euclidean distances in the same coordinate
     *                      units (degrees).
     */
    std::vector<float> batchDistances(
        const double* latitudes1,
        const double* longitudes1,
        const double* latitudes2,
        const double* longitudes2,
        size_t count,
        bool useHaversine = true
    ) override;

    /**
     * @brief Batch point-in-polygon tests via the geo GPU spatial backend.
     *
     * Converts the flat coordinate arrays into GeometryInfo objects and
     * delegates to GpuBatchBackend::batchIntersects().  The geo backend
     * automatically falls back to CPU when no GPU device is present.
     *
     * @param polygonCoords  Interleaved [lat, lon] pairs for a single polygon
     *                       applied to all test points.  The polygon is treated
     *                       as a closed ring (the implementation does not
     *                       require the first and last vertex to be identical).
     *                       Mapping: polygonCoords[v*2] → x (latitude),
     *                       polygonCoords[v*2+1] → y (longitude).
     */
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
    /// Haversine distance between two WGS-84 points; result in kilometres.
    static double haversineKm(double lat1, double lon1,
                               double lat2, double lon2) noexcept;
};

} // namespace acceleration
} // namespace themis
