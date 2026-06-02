/*
 * ThemisDB | File: geo_faiss_knn.h | Version: 0.0.9 | Last Modified: 2026-05-31 12:17:24
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 140
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * PR History (last 5): #5166 Complete GPU geospatial res... (2026-05-19)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#pragma once

/**
 * @file geo_faiss_knn.h
 * @brief FAISS-GPU bridge for GPU-accelerated spatial k-NN queries.
 *
 * `GeoFaissKnn` provides nearest-neighbour and within-radius geo queries
 * over a pre-built point dataset by projecting WGS-84 coordinates into
 * ECEF 3D space (float32) and delegating to the FAISS GPU FLAT_L2 index.
 *
 * **Coordinate projection**
 * Points are projected to the unit sphere in ECEF:
 *   x = cos(lat) × cos(lon)
 *   y = cos(lat) × sin(lon)
 *   z = sin(lat)
 * Euclidean (chord) distance in this space approximates geodesic distance
 * with < 0.5 % error for distances ≤ 5000 km, which is sufficient for
 * k-NN cluster assignment and ST_DWITHIN nearest-N queries.
 *
 * **Fallback behaviour**
 * When CUDA is not available the CPU FAISS FLAT_L2 index is used instead,
 * maintaining API compatibility.  The `getBackendName()` method reports
 * whether GPU or CPU execution is active.
 *
 * **Thread safety**
 * `build()` is not thread-safe.  `knnSearch()` and `radiusSearch()` may
 * be called concurrently from multiple threads after `build()` completes.
 *
 * @see include/acceleration/faiss_gpu_backend.h
 * @see include/geo/spatial_join.h
 */

#include "utils/geo/ewkb.h"

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace themis {
namespace geo {

/// A single k-NN or radius-search result entry.
struct GeoKnnResult {
    std::size_t index;    ///< Index into the dataset passed to build().
    double      dist_m;   ///< Approximate geodesic distance in metres.
};

/**
 * @brief GPU-accelerated spatial k-NN index backed by FAISS FLAT_L2.
 *
 * Build the index once with `build()`, then issue any number of
 * `knnSearch()` / `radiusSearch()` calls.
 */
class GeoFaissKnn {
public:
    /**
     * @brief Configuration for the FAISS geo index.
     */
    struct Config {
        /// CUDA device to use (ignored when CUDA is not available).
        int cuda_device_id = 0;
        /// Force CPU execution even when GPU is present.
        bool force_cpu = false;
    };

    explicit GeoFaissKnn(const Config& cfg = Config{});
    ~GeoFaissKnn();

    GeoFaissKnn(const GeoFaissKnn&)            = delete;
    GeoFaissKnn& operator=(const GeoFaissKnn&) = delete;
    GeoFaissKnn(GeoFaissKnn&&) noexcept;
    GeoFaissKnn& operator=(GeoFaissKnn&&) noexcept;

    /**
     * @brief (Re-)build the FAISS index from a set of WGS-84 points.
     *
     * Only `Point` geometries contribute to the index.  Non-Point entries
     * are silently skipped; their original positions in `dataset` are
     * preserved so that result `index` values correctly refer back to the
     * input vector.
     *
     * @param dataset   Input WGS-84 Point geometries.
     * @return true on success; false if the index could not be built
     *         (e.g. no valid points, allocation failure).
     */
    bool build(const std::vector<GeometryInfo>& dataset);

    /**
     * @brief Find the k nearest neighbours to a query point.
     *
     * @param query   Query point (must be a WGS-84 Point geometry).
     * @param k       Number of neighbours to return.
     * @return Vector of up to k results, sorted ascending by dist_m.
     *         Empty when the index is not built or query is not a Point.
     */
    std::vector<GeoKnnResult> knnSearch(
        const GeometryInfo& query, std::size_t k) const;

    /**
     * @brief Find all points within @p radius_m metres of a query point.
     *
     * Internally this performs a k-NN search with k = dataset size and
     * filters by the chord-distance threshold corresponding to @p radius_m.
     * For large datasets prefer setting a realistic upper bound on k.
     *
     * @param query     Query point (must be a WGS-84 Point geometry).
     * @param radius_m  Search radius in metres (must be > 0).
     * @param max_results  Maximum number of results to return (0 = unlimited).
     * @return Vector of results, sorted ascending by dist_m.
     */
    std::vector<GeoKnnResult> radiusSearch(
        const GeometryInfo& query, double radius_m,
        std::size_t max_results = 0) const;

    /// Returns true when the index has been successfully built.
    bool isBuilt() const noexcept;

    /// Returns the number of indexed points.
    std::size_t size() const noexcept;

    /// Returns "faiss_gpu" or "faiss_cpu" depending on execution mode.
    const char* getBackendName() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace geo
} // namespace themis
