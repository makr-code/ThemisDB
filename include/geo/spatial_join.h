/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            spatial_join.h                                     ║
  Version:         0.0.1                                              ║
  Last Modified:   2026-03-02 03:53:01                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     64                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • a6b9a1ca8  2026-02-24  feat(geo): implement spatial JOIN for nearby point pairs ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "utils/geo/ewkb.h"

#include <cstddef>
#include <string>
#include <vector>

namespace themis {
namespace geo {

/// A single spatial join result: a matching pair of keys and their geodesic distance.
struct SpatialJoinPair {
    std::string key_a;   ///< Key from the outer (left) collection.
    std::string key_b;   ///< Key from the inner (right) collection.
    double distance_m;   ///< Geodesic (Haversine) distance between the two geometries in metres.
};

/// Configuration for a spatial join operation.
struct SpatialJoinConfig {
    /// Maximum number of result pairs to materialise (default 1 000 000).
    /// A warning is logged when the limit is reached.
    std::size_t max_pairs = 1'000'000;
};

/**
 * @brief Find all pairs (A, B) from two geometry collections where the
 *        geodesic distance between A and B is ≤ threshold_m.
 *
 * Uses an R-tree index built on the inner (right) collection to obtain
 * MBR-level candidates, then verifies each candidate with an exact
 * Haversine distance computation.  Only Point geometries are currently
 * supported for exact distance computation; for non-Point geometries the
 * centroid is used.
 *
 * The result is not ordered.  At most `config.max_pairs` entries are
 * returned; a warning is logged if the limit is reached.
 *
 * @param outer        Left collection: vector of (key, geometry) pairs.
 * @param inner        Right collection: vector of (key, geometry) pairs.
 * @param threshold_m  Maximum distance in metres (must be > 0).
 * @param config       Optional configuration (e.g. max_pairs limit).
 * @return             Vector of matching (key_a, key_b, distance_m) triples.
 */
std::vector<SpatialJoinPair> spatialJoin(
    const std::vector<std::pair<std::string, GeometryInfo>>& outer,
    const std::vector<std::pair<std::string, GeometryInfo>>& inner,
    double threshold_m,
    const SpatialJoinConfig& config = SpatialJoinConfig{});

/**
 * @brief Compute the Haversine geodesic distance between two WGS84 points.
 *
 * @param lon1  Longitude of point 1 in degrees (WGS84).
 * @param lat1  Latitude  of point 1 in degrees (WGS84).
 * @param lon2  Longitude of point 2 in degrees (WGS84).
 * @param lat2  Latitude  of point 2 in degrees (WGS84).
 * @return Distance in metres.
 */
double haversineDistanceM(double lon1, double lat1,
                          double lon2, double lat2) noexcept;

} // namespace geo
} // namespace themis
