/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            temporal_spatial_query_builder.cpp                 ║
  Version:         1.0.0                                              ║
  Last Modified:   2026-04-09                                         ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "geo/temporal_spatial_query_builder.h"

#include <optional>
#include <stdexcept>

namespace themis {
namespace geo {

// ── Default builder implementation ───────────────────────────────────────

class DefaultTemporalSpatialQueryBuilder final
    : public ITemporalSpatialQueryBuilder {
public:
    DefaultTemporalSpatialQueryBuilder() = default;

    ITemporalSpatialQueryBuilder& withinBBox(const MBR& bbox) override {
        bbox_ = bbox;
        bbox_set_ = true;
        return *this;
    }

    ITemporalSpatialQueryBuilder& atPointInTime(int64_t ts_ms) override {
        window_type_ = TimeWindowType::POINT_IN_TIME;
        start_ms_    = ts_ms;
        end_or_width_ms_ = ts_ms;
        temporal_set_ = true;
        return *this;
    }

    ITemporalSpatialQueryBuilder& duringInterval(int64_t start_ms,
                                                  int64_t end_ms) override {
        window_type_      = TimeWindowType::INTERVAL;
        start_ms_         = start_ms;
        end_or_width_ms_  = end_ms;
        temporal_set_     = true;
        return *this;
    }

    ITemporalSpatialQueryBuilder& withSlidingWindow(int64_t width_ms) override {
        window_type_      = TimeWindowType::SLIDING_WINDOW;
        start_ms_         = 0;
        end_or_width_ms_  = width_ms;
        temporal_set_     = true;
        return *this;
    }

    ITemporalSpatialQueryBuilder& withPredicate(
        const std::string& predicate) override {
        predicate_ = predicate;
        return *this;
    }

    ITemporalSpatialQueryBuilder& withGeoField(
        const std::string& geo_field) override {
        geo_field_ = geo_field;
        return *this;
    }

    TemporalSpatialQuery build() const override {
        if (!bbox_set_) {
            throw TemporalSpatialQueryException(
                TemporalSpatialQueryError::MISSING_BBOX,
                "withinBBox() must be called before build()");
        }
        if (!temporal_set_) {
            throw TemporalSpatialQueryException(
                TemporalSpatialQueryError::MISSING_TEMPORAL,
                "A temporal constraint (atPointInTime / duringInterval / "
                "withSlidingWindow) must be set before build()");
        }

        if (window_type_ == TimeWindowType::INTERVAL &&
            end_or_width_ms_ < start_ms_) {
            throw TemporalSpatialQueryException(
                TemporalSpatialQueryError::INVALID_INTERVAL,
                "Interval end_ms must be >= start_ms");
        }

        if (window_type_ == TimeWindowType::SLIDING_WINDOW &&
            end_or_width_ms_ <= 0) {
            throw TemporalSpatialQueryException(
                TemporalSpatialQueryError::ZERO_SLIDING_WINDOW,
                "Sliding window width must be > 0");
        }

        TemporalSpatialQuery q;
        q.bbox             = bbox_;
        q.window_type      = window_type_;
        q.start_ms         = start_ms_;
        q.end_or_width_ms  = end_or_width_ms_;
        q.predicate        = predicate_;
        q.geo_field        = geo_field_;
        return q;
    }

private:
    MBR      bbox_{};
    bool     bbox_set_{false};
    bool     temporal_set_{false};

    TimeWindowType window_type_{TimeWindowType::POINT_IN_TIME};
    int64_t  start_ms_{0};
    int64_t  end_or_width_ms_{0};

    std::optional<std::string> predicate_;
    std::string geo_field_{"location"};
};

// ── Factory ───────────────────────────────────────────────────────────────

std::unique_ptr<ITemporalSpatialQueryBuilder>
ITemporalSpatialQueryBuilder::create() {
    return std::make_unique<DefaultTemporalSpatialQueryBuilder>();
}

} // namespace geo
} // namespace themis
