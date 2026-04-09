/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            temporal_spatial_query_builder.h                   ║
  Version:         1.0.0                                              ║
  Last Modified:   2026-04-09                                         ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "utils/geo/ewkb.h"
#include "geo/geojson_geometry.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>

namespace themis {
namespace geo {

// ── Time-window types ─────────────────────────────────────────────────────

/// How the temporal constraint is interpreted by a TemporalSpatialQuery.
enum class TimeWindowType {
    /// Single point in time: only records valid at exactly T are matched.
    POINT_IN_TIME,
    /// Closed interval [start_ms, end_ms]: records that were alive at any
    /// point within the interval are matched.
    INTERVAL,
    /// Sliding window: the interval is [now - width_ms, now], re-evaluated
    /// at query execution time.
    SLIDING_WINDOW,
};

// ── Immutable query value type ────────────────────────────────────────────

/**
 * @brief Immutable temporal-spatial query produced by ITemporalSpatialQueryBuilder.
 *
 * Once constructed, the query object is immutable.  It can be passed to an
 * execution engine or cached.
 */
struct TemporalSpatialQuery {
    /// Spatial constraint: axis-aligned bounding box (WGS84).
    MBR bbox;

    /// Temporal window type.
    TimeWindowType window_type{TimeWindowType::POINT_IN_TIME};

    /// For POINT_IN_TIME and INTERVAL: start of the time range (ms since epoch).
    int64_t start_ms{0};

    /// For INTERVAL: end of the time range (ms since epoch; must be ≥ start_ms).
    /// For POINT_IN_TIME: same as start_ms.
    /// For SLIDING_WINDOW: the sliding window width in milliseconds.
    int64_t end_or_width_ms{0};

    /// Optional additional spatial predicate name (e.g. "INTERSECTS", "CONTAINS").
    std::optional<std::string> predicate;

    /// Name of the document field containing the GeoJSON geometry string.
    std::string geo_field{"location"};
};

// ── Error ─────────────────────────────────────────────────────────────────

/// Error codes thrown by ITemporalSpatialQueryBuilder::build().
enum class TemporalSpatialQueryError {
    MISSING_BBOX,            ///< No spatial constraint was specified.
    MISSING_TEMPORAL,        ///< No temporal constraint was specified.
    INVALID_INTERVAL,        ///< end_ms < start_ms for INTERVAL window.
    ZERO_SLIDING_WINDOW,     ///< Sliding window width is zero or negative.
    UNBOUNDED_WINDOW,        ///< Unbounded time window without explicit opt-in.
};

/// Exception thrown by ITemporalSpatialQueryBuilder::build() on validation failure.
class TemporalSpatialQueryException : public std::invalid_argument {
public:
    explicit TemporalSpatialQueryException(TemporalSpatialQueryError code,
                                           const std::string& msg)
        : std::invalid_argument(msg), code_(code) {}

    TemporalSpatialQueryError errorCode() const noexcept { return code_; }

private:
    TemporalSpatialQueryError code_;
};

// ── Builder interface ─────────────────────────────────────────────────────

/**
 * @brief Fluent builder for TemporalSpatialQuery value types.
 *
 * The builder validates that both a spatial constraint and a temporal
 * constraint are set before `build()` succeeds.  The returned query object
 * is immutable.
 *
 * **Usage (POINT_IN_TIME):**
 * @code
 *   auto q = ITemporalSpatialQueryBuilder::create()
 *       ->withinBBox(MBR{10.0, 50.0, 15.0, 55.0})
 *       ->atPointInTime(as_of_ms)
 *       ->withPredicate("CONTAINS")
 *       ->build();
 * @endcode
 *
 * **Usage (INTERVAL):**
 * @code
 *   auto q = ITemporalSpatialQueryBuilder::create()
 *       ->withinBBox(bbox)
 *       ->duringInterval(start_ms, end_ms)
 *       ->build();
 * @endcode
 *
 * **Usage (SLIDING_WINDOW):**
 * @code
 *   auto q = ITemporalSpatialQueryBuilder::create()
 *       ->withinBBox(bbox)
 *       ->withSlidingWindow(3600000LL)   // last 1 hour
 *       ->build();
 * @endcode
 *
 * **Unbounded windows**: queries without a temporal constraint throw
 * TemporalSpatialQueryException(MISSING_TEMPORAL).
 */
class ITemporalSpatialQueryBuilder {
public:
    virtual ~ITemporalSpatialQueryBuilder() = default;

    // ── Spatial constraint ─────────────────────────────────────────────

    /**
     * @brief Set the spatial bounding-box filter (WGS84).
     *
     * @param bbox  Axis-aligned bounding box.  Must be non-degenerate.
     * @return *this for method chaining.
     */
    virtual ITemporalSpatialQueryBuilder& withinBBox(const MBR& bbox) = 0;

    // ── Temporal constraints ───────────────────────────────────────────

    /**
     * @brief Set a single point-in-time constraint.
     *
     * @param timestamp_ms  Milliseconds since epoch (Unix time).
     * @return *this for method chaining.
     */
    virtual ITemporalSpatialQueryBuilder& atPointInTime(int64_t timestamp_ms) = 0;

    /**
     * @brief Set a closed time-interval constraint [start_ms, end_ms].
     *
     * @param start_ms  Interval start (ms since epoch).
     * @param end_ms    Interval end   (ms since epoch; must be ≥ start_ms).
     * @return *this for method chaining.
     */
    virtual ITemporalSpatialQueryBuilder& duringInterval(int64_t start_ms,
                                                          int64_t end_ms) = 0;

    /**
     * @brief Set a sliding window constraint of @p width_ms milliseconds.
     *
     * At query execution time the window is [now - width_ms, now].
     *
     * @param width_ms  Window width in milliseconds (must be > 0).
     * @return *this for method chaining.
     */
    virtual ITemporalSpatialQueryBuilder& withSlidingWindow(
        int64_t width_ms) = 0;

    // ── Optional filters ───────────────────────────────────────────────

    /**
     * @brief Set an optional spatial predicate name.
     *
     * @param predicate  OGC predicate name: "INTERSECTS", "CONTAINS", etc.
     * @return *this for method chaining.
     */
    virtual ITemporalSpatialQueryBuilder& withPredicate(
        const std::string& predicate) = 0;

    /**
     * @brief Override the document field containing the GeoJSON geometry.
     *
     * @param geo_field  JSON field name (default: "location").
     * @return *this for method chaining.
     */
    virtual ITemporalSpatialQueryBuilder& withGeoField(
        const std::string& geo_field) = 0;

    // ── Build ──────────────────────────────────────────────────────────

    /**
     * @brief Validate constraints and produce an immutable query object.
     *
     * @return Immutable TemporalSpatialQuery value type.
     * @throws TemporalSpatialQueryException if any constraint is missing or
     *         invalid (MISSING_BBOX, MISSING_TEMPORAL, INVALID_INTERVAL, etc.).
     */
    virtual TemporalSpatialQuery build() const = 0;

    // ── Factory ────────────────────────────────────────────────────────

    /// Create a new, empty builder instance.
    static std::unique_ptr<ITemporalSpatialQueryBuilder> create();
};

} // namespace geo
} // namespace themis
