/**
 * @file geo_math.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.7
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include <cmath>
#include <array>

namespace themis {
namespace geo {

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------

/// Earth mean radius in metres (WGS-84 sphere approximation, IUGG 2015).
inline constexpr double kEarthRadiusM  = 6'371'008.8;

/// Earth mean radius in kilometres.
inline constexpr double kEarthRadiusKm = 6'371.0088;

/// π (double precision).
inline constexpr double kPi = 3.14159265358979323846;

// ---------------------------------------------------------------------------
// Haversine distance
// ---------------------------------------------------------------------------

/**
 * @brief Compute the great-circle distance in **metres** between two WGS-84
 *        points using the Haversine formula.
 *
 * @param lon1  Longitude of point 1 in decimal degrees (−180 … +180).
 * @param lat1  Latitude  of point 1 in decimal degrees ( −90 … +90).
 * @param lon2  Longitude of point 2 in decimal degrees.
 * @param lat2  Latitude  of point 2 in decimal degrees.
 * @return Distance in metres (≥ 0).
 *
 * Precision: ±0.5 % for distances up to ~20 000 km.
 * For sub-metre accuracy use Vincenty or Karney's method.
 */
[[nodiscard]] inline double haversineDistanceM(
        double lon1, double lat1,
        double lon2, double lat2) noexcept
{
    const double rlat1  = lat1 * kPi / 180.0;
    const double rlat2  = lat2 * kPi / 180.0;
    const double dlat   = (lat2 - lat1) * kPi / 180.0;
    const double dlon   = (lon2 - lon1) * kPi / 180.0;

    const double sinDlat = std::sin(dlat * 0.5);
    const double sinDlon = std::sin(dlon * 0.5);
    const double a = sinDlat * sinDlat
                   + std::cos(rlat1) * std::cos(rlat2) * sinDlon * sinDlon;
    const double c = 2.0 * std::atan2(std::sqrt(a), std::sqrt(1.0 - a));
    return kEarthRadiusM * c;
}

/**
 * @brief Compute the great-circle distance in **kilometres** between two
 *        WGS-84 points.
 *
 * Convenience wrapper around haversineDistanceM().
 */
[[nodiscard]] inline double haversineDistanceKm(
        double lon1, double lat1,
        double lon2, double lat2) noexcept
{
    return haversineDistanceM(lon1, lat1, lon2, lat2) / 1000.0;
}

/**
 * @brief Alias kept for legacy callers that used the `haversine_km` name
 *        (parameters in lat/lon order).
 *
 * @param lat1  Latitude  of point 1 in decimal degrees.
 * @param lon1  Longitude of point 1 in decimal degrees.
 * @param lat2  Latitude  of point 2 in decimal degrees.
 * @param lon2  Longitude of point 2 in decimal degrees.
 * @return Distance in kilometres.
 */
[[nodiscard]] inline double haversine_km(
        double lat1, double lon1,
        double lat2, double lon2) noexcept
{
    return haversineDistanceKm(lon1, lat1, lon2, lat2);
}

// ---------------------------------------------------------------------------
// Forward (initial) bearing
// ---------------------------------------------------------------------------

/**
 * @brief Compute the initial bearing (azimuth) from point 1 to point 2.
 *
 * @return Bearing in decimal degrees, clockwise from north, in [0, 360).
 */
[[nodiscard]] inline double bearingDeg(
        double lon1, double lat1,
        double lon2, double lat2) noexcept
{
    const double rlat1 = lat1 * kPi / 180.0;
    const double rlat2 = lat2 * kPi / 180.0;
    const double dlon  = (lon2 - lon1) * kPi / 180.0;

    const double y = std::sin(dlon) * std::cos(rlat2);
    const double x = std::cos(rlat1) * std::sin(rlat2)
                   - std::sin(rlat1) * std::cos(rlat2) * std::cos(dlon);
    const double bearing = std::atan2(y, x) * 180.0 / kPi;
    return std::fmod(bearing + 360.0, 360.0);
}

// ---------------------------------------------------------------------------
// Point-in-polygon (ray-casting, flat-earth / small polygon approximation)
// ---------------------------------------------------------------------------

/**
 * @brief Test whether point (px, py) lies inside a closed polygon.
 *
 * Uses the ray-casting (Jordan curve) algorithm.  Suitable for small polygons
 * on a projected (flat-earth) coordinate plane or for geographic polygons that
 * do not span a hemisphere.
 *
 * @param px       X (longitude) coordinate of the test point.
 * @param py       Y (latitude)  coordinate of the test point.
 * @param polygon  Alternating x/y pairs of polygon vertices.  The last vertex
 *                 is implicitly connected to the first.  Must have even size ≥ 6
 *                 (i.e. at least 3 vertices).
 * @param n        Total number of doubles in `polygon` (= 2 × vertex count).
 * @return `true` if the point is inside (or on the boundary of) the polygon.
 */
[[nodiscard]] inline bool pointInPolygon(
        double px, double py,
        const double* polygon, std::size_t n) noexcept
{
    bool inside = false;
    const std::size_t verts = n / 2;
    for (std::size_t i = 0, j = verts - 1; i < verts; j = i++) {
        const double xi = polygon[2 * i];
        const double yi = polygon[2 * i + 1];
        const double xj = polygon[2 * j];
        const double yj = polygon[2 * j + 1];
        if (((yi > py) != (yj > py)) &&
            (px < (xj - xi) * (py - yi) / (yj - yi) + xi))
        {
            inside = !inside;
        }
    }
    return inside;
}

} // namespace geo
} // namespace themis
