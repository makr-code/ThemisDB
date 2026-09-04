/**
 * @file raster.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "geo/raster.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace themis {
namespace geo {

// ---------------------------------------------------------------------------
// Internal constants
// ---------------------------------------------------------------------------

static constexpr double kRasterPi           = 3.14159265358979323846;
static constexpr double kRasterEarthRadiusM = 6371000.0; // mean Earth radius (m)

/// Convert metres to degrees of latitude (constant everywhere).
static double metresToDegreesLat(double m) noexcept {
    return m / (kRasterEarthRadiusM * kRasterPi / 180.0);
}

/// Convert metres to degrees of longitude at a given latitude.
static double metresToDegreesLon(double m, double lat_deg) noexcept {
    const double cos_lat = std::cos(lat_deg * kRasterPi / 180.0);
    if (cos_lat < 1e-10) {
        return 0.0;
    }
    return m / (kRasterEarthRadiusM * kRasterPi / 180.0 * cos_lat);
}

// ---------------------------------------------------------------------------
// RasterGrid implementation
// ---------------------------------------------------------------------------

RasterGrid::RasterGrid() noexcept : no_data_value(std::numeric_limits<float>::quiet_NaN()) {}

RasterGrid::RasterGrid(double min_lon_, double min_lat_, double max_lon_, double max_lat_, std::size_t width_,
                       std::size_t height_, float fill)
    : min_lon(min_lon_), min_lat(min_lat_), max_lon(max_lon_), max_lat(max_lat_), width(width_), height(height_),
      no_data_value(std::numeric_limits<float>::quiet_NaN()) {
    if (width_ > 0 && height_ > 0) {
        cell_size_x = (max_lon_ - min_lon_) / static_cast<double>(width_);
        cell_size_y = (max_lat_ - min_lat_) / static_cast<double>(height_);
        data.assign(width_ * height_, fill);
        no_data_value = std::numeric_limits<float>::quiet_NaN();
    }
}

bool RasterGrid::isNoData(float v) const noexcept {
    // NaN != NaN by IEEE 754, so special-case NaN sentinel
    if (std::isnan(no_data_value)) {
        return std::isnan(v);
    }
    // Use relative epsilon comparison for finite no-data sentinels to avoid
    // false misses caused by float rounding near the sentinel value.
    // abs_diff <= epsilon * scale  handles large sentinels (e.g. -9999.0f);
    // the + min() term guards against the near-zero edge case.
    const float abs_diff = std::abs(v - no_data_value);
    const float scale    = std::abs(no_data_value);
    return abs_diff <= std::numeric_limits<float>::epsilon() * scale
                           + std::numeric_limits<float>::min();
}

bool RasterGrid::empty() const noexcept {
    return width == 0 || height == 0 || data.empty();
}

float RasterGrid::at(std::size_t col, std::size_t row) const noexcept {
    if (col >= width || row >= height) {
        return no_data_value;
    }
    return data[row * width + col];
}

void RasterGrid::set(std::size_t col, std::size_t row, float value) noexcept {
    if (col >= width || row >= height) {
        return;
    }
    data[row * width + col] = value;
}

// ---------------------------------------------------------------------------
// sampleAt — bilinear interpolation
// ---------------------------------------------------------------------------

RasterSampleResult sampleAt(const RasterGrid &grid, double lon, double lat) noexcept {
    if (grid.empty() || grid.cell_size_x <= 0.0 || grid.cell_size_y <= 0.0) {
        return {};
    }

    // Map (lon, lat) to fractional grid coordinates.
    // Cell centres are at: lon_c(col) = min_lon + (col + 0.5) * cell_size_x
    // Solving for col: fcol = (lon - min_lon) / cell_size_x - 0.5
    const double fcol = (lon - grid.min_lon) / grid.cell_size_x - 0.5;
    const double frow = (lat - grid.min_lat) / grid.cell_size_y - 0.5;

    // Require the point to be within the grid extent.
    if (fcol < -0.5 || fcol > static_cast<double>(grid.width) - 0.5 || frow < -0.5
        || frow > static_cast<double>(grid.height) - 0.5) {
        return {};
    }

    // Four surrounding cell indices (clamped to valid range).
    const auto clamp_col = [&]([[maybe_unused]] long c) -> std::size_t {
        return static_cast<std::size_t>(std::max(0L, std::min(c, static_cast<long>(grid.width) - 1L)));
    };
    const auto clamp_row = [&]([[maybe_unused]] long r) -> std::size_t {
        return static_cast<std::size_t>(std::max(0L, std::min(r, static_cast<long>(grid.height) - 1L)));
    };

    const long c0 = static_cast<long>(std::floor(fcol));
    const long r0 = static_cast<long>(std::floor(frow));

    const std::size_t col0 = clamp_col(c0);
    const std::size_t col1 = clamp_col(c0 + 1);
    const std::size_t row0 = clamp_row(r0);
    const std::size_t row1 = clamp_row(r0 + 1);

    // Fractional offsets within the cell [0, 1].
    const double tx = fcol - std::floor(fcol);
    const double ty = frow - std::floor(frow);

    const float v00 = grid.at(col0, row0);
    const float v10 = grid.at(col1, row0);
    const float v01 = grid.at(col0, row1);
    const float v11 = grid.at(col1, row1);

    // Bilinear blend with no-data exclusion: collect valid weights.
    struct WV {
        double w;
        float v;
    };
    WV samples[4] = {
        {(1.0 - tx) * (1.0 - ty), v00},
        {tx * (1.0 - ty), v10},
        {(1.0 - tx) * ty, v01},
        {tx * ty, v11},
    };

    double sum_w  = 0.0;
    double sum_wv = 0.0;
    for (const auto &s : samples) {
        if (!grid.isNoData(s.v)) {
            sum_w += s.w;
            sum_wv += s.w * static_cast<double>(s.v);
        }
    }

    if (sum_w < 1e-15) {
        return {}; // all contributing cells are no-data
    }

    return {static_cast<float>(sum_wv / sum_w), true};
}

// ---------------------------------------------------------------------------
// queryBBox — sub-raster extraction
// ---------------------------------------------------------------------------

RasterGrid queryBBox(const RasterGrid &grid, const MBR &bbox) {
    if (grid.empty() || grid.cell_size_x <= 0.0 || grid.cell_size_y <= 0.0) {
        return {};
    }

    // Find the column range whose cell centres lie inside [bbox.minx, bbox.maxx].
    // Cell centre for column c: lon_c = min_lon + (c + 0.5) * cell_size_x
    // Solving: c_min = ceil((bbox.minx - min_lon) / cell_size_x - 0.5)
    //          c_max = floor((bbox.maxx - min_lon) / cell_size_x - 0.5)
    const auto to_col = [&]([[maybe_unused]] double lon) -> double { return (lon - grid.min_lon) / grid.cell_size_x - 0.5; };
    const auto to_row = [&]([[maybe_unused]] double lat) -> double { return (lat - grid.min_lat) / grid.cell_size_y - 0.5; };

    long c_min = static_cast<long>(std::ceil(to_col(bbox.minx)));
    long c_max = static_cast<long>(std::floor(to_col(bbox.maxx)));
    long r_min = static_cast<long>(std::ceil(to_row(bbox.miny)));
    long r_max = static_cast<long>(std::floor(to_row(bbox.maxy)));

    // Clamp to valid index range.
    c_min = std::max(c_min, 0L);
    c_max = std::min(c_max, static_cast<long>(grid.width) - 1L);
    r_min = std::max(r_min, 0L);
    r_max = std::min(r_max, static_cast<long>(grid.height) - 1L);

    if (c_min > c_max || r_min > r_max) {
        return {};
    }

    const std::size_t out_w = static_cast<std::size_t>(c_max - c_min + 1);
    const std::size_t out_h = static_cast<std::size_t>(r_max - r_min + 1);

    // Geographic bounds of the output grid (cell-centre edges).
    const double out_min_lon
        = grid.min_lon + (static_cast<double>(c_min) + 0.5) * grid.cell_size_x - 0.5 * grid.cell_size_x;
    const double out_min_lat
        = grid.min_lat + (static_cast<double>(r_min) + 0.5) * grid.cell_size_y - 0.5 * grid.cell_size_y;
    const double out_max_lon = out_min_lon + static_cast<double>(out_w) * grid.cell_size_x;
    const double out_max_lat = out_min_lat + static_cast<double>(out_h) * grid.cell_size_y;

    RasterGrid out(out_min_lon, out_min_lat, out_max_lon, out_max_lat, out_w, out_h);
    out.no_data_value = grid.no_data_value;

    for (std::size_t r = 0; r < out_h; ++r) {
        for (std::size_t c = 0; c < out_w; ++c) {
            out.set(c, r,
                    grid.at(static_cast<std::size_t>(c_min + static_cast<long>(c)),
                            static_cast<std::size_t>(r_min + static_cast<long>(r))));
        }
    }
    return out;
}

// ---------------------------------------------------------------------------
// generateHeatmap — Gaussian kernel density estimation
// ---------------------------------------------------------------------------

RasterGrid generateHeatmap(const std::vector<Coordinate> &points, const MBR &bbox, const HeatmapConfig &config) {
    if (points.empty() || config.width == 0 || config.height == 0) {
        return {};
    }
    if (bbox.maxx <= bbox.minx || bbox.maxy <= bbox.miny) {
        return {};
    }
    if (config.bandwidth_m <= 0.0) {
        return {};
    }

    RasterGrid out(bbox.minx, bbox.miny, bbox.maxx, bbox.maxy, config.width, config.height, 0.0f);
    // Use 0 (no density) as the default, not NaN.
    out.no_data_value = std::numeric_limits<float>::quiet_NaN();

    // Convert bandwidth from metres to degrees at the grid's centre latitude.
    const double centre_lat = (bbox.miny + bbox.maxy) * 0.5;
    const double sigma_lon  = metresToDegreesLon(config.bandwidth_m, centre_lat);
    const double sigma_lat  = metresToDegreesLat(config.bandwidth_m);

    // Precompute the kernel radius in grid cells (3σ cutoff).
    const double radius_lon = 3.0 * sigma_lon;
    const double radius_lat = 3.0 * sigma_lat;
    const long rc           = static_cast<long>(std::ceil(radius_lon / out.cell_size_x)) + 1;
    const long rr           = static_cast<long>(std::ceil(radius_lat / out.cell_size_y)) + 1;

    const double inv2_sl2 = 1.0 / (2.0 * sigma_lon * sigma_lon);
    const double inv2_sb2 = 1.0 / (2.0 * sigma_lat * sigma_lat);

    for (const auto &pt : points) {
        // Skip points outside the bounding box.
        if (pt.x < bbox.minx || pt.x > bbox.maxx || pt.y < bbox.miny || pt.y > bbox.maxy) {
            continue;
        }

        // Fractional grid coordinates of this point (cell-centre origin).
        const double fcol = (pt.x - bbox.minx) / out.cell_size_x - 0.5;
        const double frow = (pt.y - bbox.miny) / out.cell_size_y - 0.5;

        const long pcol = static_cast<long>(std::round(fcol));
        const long prow = static_cast<long>(std::round(frow));

        const long col_lo = std::max(0L, pcol - rc);
        const long col_hi = std::min(static_cast<long>(config.width) - 1L, pcol + rc);
        const long row_lo = std::max(0L, prow - rr);
        const long row_hi = std::min(static_cast<long>(config.height) - 1L, prow + rr);

        for (long r = row_lo; r <= row_hi; ++r) {
            // Latitude of this cell's centre.
            const double cell_lat = bbox.miny + (static_cast<double>(r) + 0.5) * out.cell_size_y;
            const double dlat     = cell_lat - pt.y;
            const double dlat2    = dlat * dlat * inv2_sb2;

            for (long c = col_lo; c <= col_hi; ++c) {
                const double cell_lon = bbox.minx + (static_cast<double>(c) + 0.5) * out.cell_size_x;
                const double dlon     = cell_lon - pt.x;
                const double exponent = dlon * dlon * inv2_sl2 + dlat2;
                const float contrib   = static_cast<float>(std::exp(-exponent));

                const std::size_t sc = static_cast<std::size_t>(c);
                const std::size_t sr = static_cast<std::size_t>(r);
                out.set(sc, sr, out.at(sc, sr) + contrib);
            }
        }
    }

    if (config.normalize) {
        float max_val = 0.0f;
        for (float v : out.data) {
            if (!std::isnan(v) && v > max_val) {
                max_val = v;
            }
        }
        if (max_val > 0.0f) {
            for (float &v : out.data) {
                if (!std::isnan(v)) {
                    v /= max_val;
                }
            }
        }
    }

    return out;
}

} // namespace geo
} // namespace themis
