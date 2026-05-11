/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            geometric_distances.h                              ║
  Version:         1.0.0                                              ║
  Last Modified:   2026-04-27                                         ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file geometric_distances.h
 * @brief Unified geometric distance API for ThemisDB.
 *
 * This header is the single canonical include for all distance metrics used
 * across the codebase.  It consolidates three previously scattered families:
 *
 *   1. **Vector distances** (L2, cosine, inner product) — re-exported from
 *      `utils/simd_distance.h`.  All variants are SIMD-accelerated
 *      (AVX-512 / AVX2 / ARM NEON) with scalar fallbacks.
 *
 *   2. **Geospatial distances** (Haversine) — inline implementations in
 *      `themis::geo` namespace.  These were previously duplicated in at
 *      least 8 translation units; new code MUST use this header.
 *
 *   3. **Manhattan distance** — scalar implementation for arbitrary-dimension
 *      float vectors.  Not SIMD-optimised; use L2 or inner-product for
 *      performance-critical paths.
 *
 * ### Migration guide (existing duplicate implementations) — all migrated as of v1.9.0
 *
 * | Old symbol | Location | Status |
 * |---|---|---|
 * | `CPUVectorBackend::computeL2Distance` | cpu_backend.cpp | ✅ migrated → `simd::l2_distance_sq` |
 * | `CPUVectorBackend::computeCosineDistance` | cpu_backend.cpp | ✅ migrated → `simd::cosine_distance` |
 * | `CPUGeoBackend::haversineDistance` | cpu_backend.cpp | ✅ migrated → `geo::haversine_km` |
 * | `opengl_haversine_km` / `vulkan_haversine_km` | graphics_backends.cpp | ✅ migrated → `geo::haversine_km` |
 * | `SecondaryIndexManager::haversineDistance` | secondary_index.cpp | ✅ migrated → `geo::haversine_km` |
 * | `SpatialIndexManager::haversineDistance` | spatial_index.cpp | ✅ migrated → `geo::haversine_m` |
 * | `GeoAccelerationBridge::haversineKm` | geo_acceleration_bridge.cpp | ✅ migrated → `geo::haversine_km` |
 * | `haversineDistanceM()` (geo/\*.cpp) | various | ✅ canonical in geo/geo_math.h (no change needed) |
 *
 * ### Consolidation status
 * - All acceleration, index, and geo files fully migrated (v1.9.0)
 */

#pragma once

#include "geo/geo_math.h"
#include "utils/simd_distance.h"

#include <cmath>
#include <cstddef>

// ============================================================================
// Re-export of SIMD vector distance API (themis::simd namespace)
// ============================================================================
//
// The following symbols are defined in utils/simd_distance.h and implemented
// in utils/simd_distance.cpp.  They are available here via the include above;
// no additional declarations are required.  Refer to simd_distance.h for the
// full API contract.
//
//   float  simd::l2_distance(a, b, dim)          — sqrt(Σ(aᵢ−bᵢ)²)
//   float  simd::l2_distance_sq(a, b, dim)       — Σ(aᵢ−bᵢ)²  (no sqrt)
//   void   simd::batch_l2_distance_sq(...)       — vectorised batch variant
//   float  simd::inner_product(a, b, dim)        — Σ aᵢ·bᵢ
//   float  simd::cosine_distance(a, b, dim)      — 1 − cosine_similarity
//   float  simd::cosine_similarity(a, b, dim)    — dot/(‖a‖·‖b‖)
//   void   simd::batch_cosine_similarity(...)    — vectorised batch variant

namespace themis {

// ============================================================================
// Geospatial distances — themis::geo namespace
// ============================================================================

namespace geo {

/// Earth mean radius in metres (WGS-84 approximation).
constexpr double EARTH_RADIUS_M  = 6'371'000.0;
/// Earth mean radius in kilometres (WGS-84 approximation).
constexpr double EARTH_RADIUS_KM = 6'371.0;

/**
 * @brief Haversine great-circle distance in **kilometres**.
 *
 * Inputs are in decimal degrees (WGS-84).  The function is accurate to within
 * ±0.5 % for distances up to ~10 000 km and handles the anti-meridian
 * correctly via the haversine formula.
 *
 * @param lat1  Latitude of point A  (degrees, −90 … +90).
 * @param lon1  Longitude of point A (degrees, −180 … +180).
 * @param lat2  Latitude of point B.
 * @param lon2  Longitude of point B.
 * @return Great-circle distance in kilometres (always ≥ 0).
 */
using ::themis::geo::haversine_km;

/**
 * @brief Haversine great-circle distance in **metres**.
 *
 * Thin wrapper over haversine_km(); use this when the rest of the call-site
 * operates in metres (e.g. the geo/\*.cpp clustering code that previously used
 * file-local `haversineDistanceM()`).
 *
 * @return Great-circle distance in metres (always ≥ 0).
 */
[[nodiscard]] inline double haversine_m(
    double lat1, double lon1,
    double lat2, double lon2) noexcept
{
    return haversine_km(lat1, lon1, lat2, lon2) * 1000.0;
}

} // namespace geo

// ============================================================================
// Manhattan (L1) distance — themis namespace
// ============================================================================

/**
 * @brief Manhattan (L1) distance between two float vectors of length @p dim.
 *
 * Returns Σ |aᵢ − bᵢ|.  Scalar implementation; for high-throughput search
 * paths prefer simd::l2_distance_sq (which is SIMD-accelerated).
 *
 * @param a    First vector (length @p dim).
 * @param b    Second vector (length @p dim).
 * @param dim  Vector dimensionality.
 * @return Non-negative L1 distance.
 */
[[nodiscard]] inline float manhattan_distance(
    const float* a, const float* b, std::size_t dim) noexcept
{
    float sum = 0.0f;
    for (std::size_t i = 0; i < dim; ++i) {
        const float d = a[i] - b[i];
        sum += d < 0.0f ? -d : d;  // std::abs not constexpr in all C++17 environments
    }
    return sum;
}

} // namespace themis
