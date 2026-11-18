/**
 * @file spatial_join.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "query/request_context.h"
#include "utils/geo/ewkb.h"

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "geo/geo_math.h"

namespace themis {
namespace geo {

struct SpatialJoinPair {
    std::string key_a;   ///< Key from the outer (left) collection.
    std::string key_b;   ///< Key from the inner (right) collection.
    double distance_m;   ///< Geodesic (Haversine) distance between the two geometries in metres.
};

struct SpatialJoinConfig {
    std::size_t max_pairs = 1'000'000;
};

class SpatialJoinIterator {
public:
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
     * @brief Advance an iterator within the validated range.
     * @return None.
     */
    bool advance();

    /**
     * @brief Advance while cooperatively observing request cancellation/deadline.
     * @param request Request stop state to poll; must remain valid for this call.
     * @param stop_reason Output reset to None on entry and set if the request stops.
     * @return True when a pair is yielded; false on exhaustion or a stop.
     * @note Check stop_reason to distinguish cancellation/deadline from normal
     *       exhaustion. R-tree construction, candidate lookup, and one geometry
     *       distance calculation remain synchronous and cannot be interrupted.
     */
    [[nodiscard]] bool advance(
        const query::RequestContext& request,
        query::StopReason& stop_reason);

    /**
     * @brief Current.
     * @return Return value.
     */
    const SpatialJoinPair& current() const;

    /**
     * @brief Done.
     * @return True when the operation succeeds.
     */
    bool done() const;

    // Legacy compatibility alias.
    bool exhausted() const { return done(); }

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

std::vector<SpatialJoinPair> spatialJoin(
    const std::vector<std::pair<std::string, GeometryInfo>>& outer,
    const std::vector<std::pair<std::string, GeometryInfo>>& inner,
    double threshold_m,
    const SpatialJoinConfig& config = SpatialJoinConfig{});

} // namespace geo
} // namespace themis
