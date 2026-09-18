/**
 * @file raster.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

struct RasterGrid {
    double min_lon{0.0};   ///< Western edge in degrees (WGS84).
    double min_lat{0.0};   ///< Southern edge in degrees (WGS84).
    double max_lon{0.0};   ///< Eastern edge in degrees (WGS84).
    double max_lat{0.0};   ///< Northern edge in degrees (WGS84).

    std::size_t width{0};  ///< Number of columns (longitude axis).
    std::size_t height{0}; ///< Number of rows    (latitude  axis).

    double cell_size_x{0.0};
    double cell_size_y{0.0};

    std::vector<float> data;

    float no_data_value{};

    RasterGrid() noexcept;

    RasterGrid(double min_lon_, double min_lat_,
               double max_lon_, double max_lat_,
               std::size_t width_, std::size_t height_,
               float fill = std::numeric_limits<float>::quiet_NaN());

    /**
     * @brief Is No Data.
     * @param[in] v Input parameter.
     * @return True when the operation succeeds.
     * @note Exception safety: noexcept.
     */
    bool isNoData(float v) const noexcept;

    /**
     * @brief Empty.
     * @return True when the operation succeeds.
     * @note Exception safety: noexcept.
     */
    bool empty() const noexcept;

    /**
     * @brief At.
     * @param[in] col Input parameter.
     * @param[in] row Input parameter.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    float at(std::size_t col, std::size_t row) const noexcept;

    /**
     * @brief Set.
     * @param[in] col Input parameter.
     * @param[in] row Input parameter.
     * @param[in] value Input parameter.
     * @note Exception safety: noexcept.
     */
    void set(std::size_t col, std::size_t row, float value) noexcept;
};

struct RasterSampleResult {
    float value{};       ///< Interpolated (or nearest-neighbour) value.
    bool  valid{false};  ///< False when the query point is outside the grid or
};

/**
 * @brief Sample At.
 * @param[in] grid Input parameter.
 * @param[in] lon Input parameter.
 * @param[in] lat Input parameter.
 * @return Return value.
 * @note Exception safety: noexcept.
 */
RasterSampleResult sampleAt(const RasterGrid& grid,
                             double lon, double lat) noexcept;

/**
 * @brief Query BBox.
 * @param[in] grid Input parameter.
 * @param[in] bbox Input parameter.
 * @return Return value.
 */
RasterGrid queryBBox(const RasterGrid& grid, const MBR& bbox);

// ---------------------------------------------------------------------------
// Heatmap generation
// ---------------------------------------------------------------------------

struct HeatmapConfig {
    double bandwidth_m{500.0}; ///< Gaussian kernel bandwidth in metres (σ).
    std::size_t width{100};    ///< Output grid columns.
    std::size_t height{100};   ///< Output grid rows.

    bool normalize{false};
};

RasterGrid generateHeatmap(const std::vector<Coordinate>& points,
                            const MBR& bbox,
                            const HeatmapConfig& config = HeatmapConfig{});

} // namespace geo
} // namespace themis
