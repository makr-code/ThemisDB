/**
 * @file raster.h
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
#include <limits>
#include <vector>

namespace themis {
namespace geo {

/**
 * @brief A 2D grid of scalar values (e.g. elevation in metres, heat density)
 *        covering a geographic bounding box.
 *
 * Values are stored in row-major order: index = row * width + col, where
 * row 0 corresponds to `min_lat` and col 0 corresponds to `min_lon`.
 *
 * Invalid / no-data cells are indicated by the sentinel value `no_data_value`
 * (default: quiet NaN).  Use `isNoData(v)` to test for it.
 */
struct RasterGrid {
    double min_lon{0.0};   ///< Western edge in degrees (WGS84).
    double min_lat{0.0};   ///< Southern edge in degrees (WGS84).
    double max_lon{0.0};   ///< Eastern edge in degrees (WGS84).
    double max_lat{0.0};   ///< Northern edge in degrees (WGS84).

    std::size_t width{0};  ///< Number of columns (longitude axis).
    std::size_t height{0}; ///< Number of rows    (latitude  axis).

    /// Cell size in degrees along the longitude axis.
    double cell_size_x{0.0};
    /// Cell size in degrees along the latitude axis.
    double cell_size_y{0.0};

    /// Scalar payload in row-major order; size == width * height.
    std::vector<float> data;

    /// Sentinel value for missing / invalid cells (default: NaN).
    float no_data_value{};

    RasterGrid() noexcept;

    /**
     * @brief Construct and pre-allocate a grid.
     *
     * @param min_lon_  Western bound (degrees).
     * @param min_lat_  Southern bound (degrees).
     * @param max_lon_  Eastern bound (degrees).
     * @param max_lat_  Northern bound (degrees).
     * @param width_    Number of columns.
     * @param height_   Number of rows.
     * @param fill      Initial fill value (default: no_data sentinel).
     */
    RasterGrid(double min_lon_, double min_lat_,
               double max_lon_, double max_lat_,
               std::size_t width_, std::size_t height_,
               float fill = std::numeric_limits<float>::quiet_NaN());

    /// True when `v` equals the no_data sentinel for this grid.
    bool isNoData(float v) const noexcept;

    /// True when the grid is empty (width == 0 || height == 0 || data empty).
    bool empty() const noexcept;

    /// Return the value at grid position (col, row).  Bounds-checked; returns
    /// no_data_value when out of range.
    float at(std::size_t col, std::size_t row) const noexcept;

    /// Set the value at grid position (col, row).  No-op when out of range.
    void set(std::size_t col, std::size_t row, float value) noexcept;
};

/**
 * @brief Result of a single raster sample operation.
 */
struct RasterSampleResult {
    float value{};       ///< Interpolated (or nearest-neighbour) value.
    bool  valid{false};  ///< False when the query point is outside the grid or
                         ///< all contributing cells are no-data.
};

/**
 * @brief Sample the raster at an arbitrary geographic point using bilinear
 *        interpolation.
 *
 * The four nearest grid cells surrounding (lon, lat) are blended
 * proportionally to their sub-pixel distances.  If any contributing cell
 * holds the no-data sentinel the contribution from that cell is excluded
 * and the remaining valid cells are re-normalised.  The result is invalid
 * when all four cells are no-data or the point lies outside the grid bounds.
 *
 * @param grid  Source raster.
 * @param lon   Longitude of the query point (degrees, WGS84).
 * @param lat   Latitude  of the query point (degrees, WGS84).
 * @return      Interpolated value and validity flag.
 */
RasterSampleResult sampleAt(const RasterGrid& grid,
                             double lon, double lat) noexcept;

/**
 * @brief Extract the sub-raster covering `bbox`.
 *
 * The returned grid spans exactly the cells whose cell centres fall inside
 * (or on the border of) `bbox`.  Cell values are copied without
 * interpolation.  Returns an empty RasterGrid when `bbox` does not overlap
 * the source grid.
 *
 * @param grid  Source raster.
 * @param bbox  Query bounding box (WGS84 degrees).
 * @return      Sub-raster covering the intersection.
 */
RasterGrid queryBBox(const RasterGrid& grid, const MBR& bbox);

// ---------------------------------------------------------------------------
// Heatmap generation
// ---------------------------------------------------------------------------

/**
 * @brief Configuration for heatmap generation from a point cloud.
 */
struct HeatmapConfig {
    double bandwidth_m{500.0}; ///< Gaussian kernel bandwidth in metres (σ).
    std::size_t width{100};    ///< Output grid columns.
    std::size_t height{100};   ///< Output grid rows.

    /// When true the output values are normalised to [0, 1].
    bool normalize{false};
};

/**
 * @brief Generate a heatmap RasterGrid from a set of geographic points using
 *        a Gaussian kernel density estimator.
 *
 * Each point contributes to nearby cells via a Gaussian kernel whose σ is
 * `config.bandwidth_m` (converted to degrees at the grid's centre latitude
 * for efficient computation).  Cells outside the bounding box or with no
 * contributing points receive a density of 0.
 *
 * @param points  Input point cloud as (lon, lat) Coordinate pairs.
 * @param bbox    Geographic extent of the output grid.
 * @param config  Heatmap parameters (bandwidth, resolution, normalisation).
 * @return        Density RasterGrid; empty on invalid input.
 */
RasterGrid generateHeatmap(const std::vector<Coordinate>& points,
                            const MBR& bbox,
                            const HeatmapConfig& config = HeatmapConfig{});

} // namespace geo
} // namespace themis
