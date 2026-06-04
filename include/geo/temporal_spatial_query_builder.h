/**
 * @file temporal_spatial_query_builder.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "geo/geo_json_geometry.h"
#include "geo/spatial_join_filter.h"
#include "temporal/temporal_types.h"
#include "temporal/system_versioned_table.h"
#include "utils/geo/ewkb.h"

#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace themis {
namespace geo {

// ---------------------------------------------------------------------------
// TimeWindowType
// ---------------------------------------------------------------------------

/**
 * @brief Supported temporal constraint types for a `BuiltTemporalSpatialQuery`.
 */
enum class TimeWindowType {
    /// Query as of a single point in time (T).
    POINT_IN_TIME,
    /// Query over a closed time interval [start, end].
    INTERVAL,
    /// Sliding window: [now − width_ms, now] evaluated at query execution time.
    SLIDING_WINDOW,
};

// ---------------------------------------------------------------------------
// BuiltTemporalSpatialQuery — immutable value type
// ---------------------------------------------------------------------------

/**
 * @brief Immutable result of `ITemporalSpatialQueryBuilder::build()`.
 *
 * Encapsulates all temporal and spatial constraints.  Provides an `execute()`
 * method that runs the query against a `SystemVersionedTable` and returns
 * matching (key, geometry) pairs.
 *
 * The query object is safe to share across threads once built.
 */
class BuiltTemporalSpatialQuery {
public:
    /// Spatial constraint type.
    enum class SpatialType { BBOX, PREDICATE };

    struct TemporalConstraint {
        TimeWindowType window_type;
        themisdb::temporal::Timestamp point_in_time{0};
        themisdb::temporal::Timestamp interval_start{0};
        themisdb::temporal::Timestamp interval_end{themisdb::temporal::kMaxTimestamp};
        int64_t sliding_window_ms{0};
    };

    struct SpatialConstraint {
        SpatialType type{SpatialType::BBOX};
        MBR         bbox;
        std::shared_ptr<ISpatialJoinFilter> predicate; ///< Used when type == PREDICATE
    };

    BuiltTemporalSpatialQuery(TemporalConstraint temporal,
                               SpatialConstraint  spatial,
                               std::string        geo_field)
        : temporal_(std::move(temporal))
        , spatial_(std::move(spatial))
        , geo_field_(std::move(geo_field)) {}

    // ---- Accessors (immutable after construction) ----

    [[nodiscard]] TimeWindowType windowType() const noexcept {
        return temporal_.window_type;
    }

    [[nodiscard]] const MBR& bbox() const noexcept { return spatial_.bbox; }

    [[nodiscard]] const std::string& geoField() const noexcept {
        return geo_field_;
    }

    /**
     * @brief Execute the query against @p table.
     *
     * Returns a vector of (key, geometry) pairs for entities that satisfy
     * both the temporal and spatial constraints.
     *
     * For `POINT_IN_TIME` and `INTERVAL` windows, `temporal_.point_in_time` /
     * `temporal_.interval_start .. interval_end` are used as-is.
     * For `SLIDING_WINDOW`, the window is computed as [now − width_ms, now].
     *
     * @param table  System-versioned table to query.
     * @return       Matching (key, geometry) pairs.
     */
    [[nodiscard]] std::vector<std::pair<std::string, GeometryInfo>> execute(
        const themisdb::temporal::SystemVersionedTable& table) const;

private:
    TemporalConstraint temporal_;
    SpatialConstraint  spatial_;
    std::string        geo_field_;
};

// ---------------------------------------------------------------------------
// ITemporalSpatialQueryBuilder — abstract builder interface
// ---------------------------------------------------------------------------

/**
 * @brief Abstract fluent builder for `BuiltTemporalSpatialQuery`.
 *
 * A concrete implementation is `TemporalSpatialQueryBuilder`.
 *
 * Builders are NOT thread-safe.  Each thread should use its own builder
 * instance.
 */
class ITemporalSpatialQueryBuilder {
public:
    virtual ~ITemporalSpatialQueryBuilder() = default;

    /**
     * @brief Set the spatial constraint to a bounding box.
     *
     * Replaces any previously set spatial constraint.
     */
    [[nodiscard]] virtual ITemporalSpatialQueryBuilder& withinBBox(const MBR& bbox) = 0;

    /**
     * @brief Set the spatial constraint to a composable filter predicate.
     *
     * Replaces any previously set spatial constraint.
     */
    virtual ITemporalSpatialQueryBuilder& withPredicate(
        std::shared_ptr<ISpatialJoinFilter> predicate) = 0;

    /**
     * @brief Set the temporal constraint to a closed interval [start, end].
     *
     * Sets window type to `TimeWindowType::INTERVAL`.
     * Replaces any previously set temporal constraint.
     */
    virtual ITemporalSpatialQueryBuilder& duringInterval(
        themisdb::temporal::Timestamp start,
        themisdb::temporal::Timestamp end) = 0;

    /**
     * @brief Set the temporal constraint to a single point in time.
     *
     * Sets window type to `TimeWindowType::POINT_IN_TIME`.
     * Replaces any previously set temporal constraint.
     */
    virtual ITemporalSpatialQueryBuilder& atTime(
        themisdb::temporal::Timestamp t) = 0;

    /**
     * @brief Set the temporal constraint to a sliding window of @p width_ms ms.
     *
     * Sets window type to `TimeWindowType::SLIDING_WINDOW`.
     * The window is evaluated as [now − width_ms, now] at `build()` time.
     * Replaces any previously set temporal constraint.
     *
     * @param width_ms  Window width in milliseconds (must be > 0).
     */
    [[nodiscard]] virtual ITemporalSpatialQueryBuilder& slidingWindow(int64_t width_ms) = 0;

    /**
     * @brief Set the name of the JSON field containing geometry data.
     *
     * Default is `TemporalSpatialQuery::kDefaultGeoField` ("location").
     */
    [[nodiscard]] virtual ITemporalSpatialQueryBuilder& withGeoField(const std::string& field) = 0;

    /**
     * @brief Build and return an immutable `BuiltTemporalSpatialQuery`.
     *
     * @throws std::logic_error when temporal or spatial constraints are missing.
     * @throws std::invalid_argument when constraints are mutually inconsistent.
     * @return Immutable query value.
     */
    [[nodiscard]] virtual BuiltTemporalSpatialQuery build() = 0;
};

// ---------------------------------------------------------------------------
// TemporalSpatialQueryBuilder — concrete implementation
// ---------------------------------------------------------------------------

/**
 * @brief Concrete fluent builder.
 *
 * Reset the builder with `reset()` to reuse for a second query.
 */
class TemporalSpatialQueryBuilder final : public ITemporalSpatialQueryBuilder {
public:
    TemporalSpatialQueryBuilder() = default;

    TemporalSpatialQueryBuilder& withinBBox(const MBR& bbox) override;
    TemporalSpatialQueryBuilder& withPredicate(
        std::shared_ptr<ISpatialJoinFilter> predicate) override;
    TemporalSpatialQueryBuilder& duringInterval(
        themisdb::temporal::Timestamp start,
        themisdb::temporal::Timestamp end) override;
    TemporalSpatialQueryBuilder& atTime(themisdb::temporal::Timestamp t) override;
    TemporalSpatialQueryBuilder& slidingWindow(int64_t width_ms) override;
    TemporalSpatialQueryBuilder& withGeoField(const std::string& field) override;

    [[nodiscard]] BuiltTemporalSpatialQuery build() override;

    /// Reset all constraints so the builder can be reused.
    TemporalSpatialQueryBuilder& reset();

private:
    std::optional<BuiltTemporalSpatialQuery::TemporalConstraint> temporal_;
    std::optional<BuiltTemporalSpatialQuery::SpatialConstraint>  spatial_;
    std::string geo_field_{"location"};
};

} // namespace geo
} // namespace themis
