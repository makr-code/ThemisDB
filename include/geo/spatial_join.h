/**
 * @file spatial_join.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "utils/geo/ewkb.h"

#include <cstddef>
#include <memory>
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
 * @brief Lazy iterator for a spatial join operation.
 *
 * Yields one (key_a, key_b, distance_m) pair at a time without materialising
 * the full result set in memory.  The inner collection is indexed once in the
 * constructor (O(n_inner) build cost); each call to `advance()` performs a
 * Haversine exact-distance check against R-tree candidates for the current
 * outer geometry and yields a new pair.
 *
 * **Usage:**
 * @code
 *   SpatialJoinIterator it(outer, inner, 1000.0);
 *   while (it.advance()) {
 *       const SpatialJoinPair& p = it.current();
 *       // process p ...
 *   }
 * @endcode
 *
 * **Thread safety:** not thread-safe; external synchronisation required.
 *
 * **Memory:** O(n_inner) for the R-tree index + O(candidates) for the current
 * candidate batch; no full result materialisation.
 */
class SpatialJoinIterator {
public:
    /**
     * @brief Construct a lazy spatial-join iterator.
     *
     * Builds the R-tree index on @p inner immediately; actual pair
     * generation is deferred to calls to `advance()`.
     *
     * @param outer        Left collection: vector of (key, geometry) pairs.
     * @param inner        Right collection: vector of (key, geometry) pairs.
     * @param threshold_m  Maximum distance in metres (must be > 0).
     * @param config       Optional configuration (e.g. max_pairs limit).
     */
    SpatialJoinIterator(
        const std::vector<std::pair<std::string, GeometryInfo>>& outer,
        const std::vector<std::pair<std::string, GeometryInfo>>& inner,
        double threshold_m,
        const SpatialJoinConfig& config = SpatialJoinConfig{});

    ~SpatialJoinIterator();

    // Non-copyable, movable
    SpatialJoinIterator(const SpatialJoinIterator&) = delete;
    SpatialJoinIterator& operator=(const SpatialJoinIterator&) = delete;
    SpatialJoinIterator(SpatialJoinIterator&&) noexcept;
    SpatialJoinIterator& operator=(SpatialJoinIterator&&) noexcept;

    /**
     * @brief Advance the iterator to the next matching pair.
     *
     * @return true  if a new pair is available (accessible via `current()`).
     * @return false if the iteration is exhausted or the max_pairs limit was reached.
     */
    bool advance();

    /**
     * @brief Return the current pair.
     *
     * Valid only when the most recent call to `advance()` returned true.
     */
    const SpatialJoinPair& current() const;

    /**
     * @brief Return true once `advance()` has returned false.
     */
    bool done() const;

    // Legacy compatibility alias.
    bool exhausted() const { return done(); }

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
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
 *        See geo/geo_math.h for the canonical implementation.
 * @return Distance in metres.
 */
double haversineDistanceM(double lon1, double lat1,
                          double lon2, double lat2) noexcept;

} // namespace geo
} // namespace themis
