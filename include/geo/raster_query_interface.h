#pragma once

/**
 * @file raster_query_interface.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 94/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "geo/raster.h"
#include "geo/tile_server.h"
#include "utils/geo/ewkb.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace themis {
namespace geo {

// ---------------------------------------------------------------------------
// RasterConfig
// ---------------------------------------------------------------------------

class RasterConfig {
public:
    static constexpr std::size_t kDefaultMaxTileSizeBytes = 64ULL * 1024 * 1024;

    explicit RasterConfig(std::size_t max_tile_size_bytes = kDefaultMaxTileSizeBytes)
        : max_tile_size_bytes_(max_tile_size_bytes) {}

    [[nodiscard]] std::size_t maxTileSizeBytes() const noexcept {
        return max_tile_size_bytes_;
    }

private:
    std::size_t max_tile_size_bytes_;
};

// ---------------------------------------------------------------------------
// RasterStatus
// ---------------------------------------------------------------------------

enum class RasterStatus {
    OK,             ///< Operation succeeded.
    NOT_SUPPORTED,  ///< THEMIS_ENABLE_RASTER not defined; raster disabled.
    TILE_TOO_LARGE, ///< Requested tile exceeds `RasterConfig::maxTileSizeBytes()`.
    INVALID_KEY,    ///< Tile key is out of range (zoom > 22 or x/y out of bounds).
    BACKEND_ERROR,  ///< Unspecified backend error (see `RasterResult::error_message`).
    INVALID_BBOX,   ///< Bounding box is empty, inverted, or out of WGS-84 range.
};

// ---------------------------------------------------------------------------
// RasterResult
// ---------------------------------------------------------------------------

struct RasterResult {
    RasterStatus status{RasterStatus::NOT_SUPPORTED};

    RasterGrid grid;

    std::string crs_wkt;

    std::size_t band_count{1};

    double resolution_x{0.0};
    double resolution_y{0.0};

    std::string error_message;

    [[nodiscard]] bool ok() const noexcept { return status == RasterStatus::OK; }
};

// ---------------------------------------------------------------------------
// IRasterQueryInterface — abstract base
// ---------------------------------------------------------------------------

class IRasterQueryInterface {
public:
    /**
     * @brief IRaster Query Interface.
     * @return Return value.
     */
    virtual ~IRasterQueryInterface() = default;

    [[nodiscard]] virtual RasterResult queryTile(
        const TileCoord& tile,
        const RasterConfig& config = RasterConfig{}) const = 0;

    [[nodiscard]] virtual RasterResult queryBBox(
        const MBR& bbox,
        double resolution,
        const RasterConfig& config = RasterConfig{}) const = 0;
};

// ---------------------------------------------------------------------------
// NoOpRasterQueryImpl — always-available no-op stub
// ---------------------------------------------------------------------------

class NoOpRasterQueryImpl final : public IRasterQueryInterface {
public:
    [[nodiscard]] RasterResult queryTile(
        [[maybe_unused]] const TileCoord& tile,
        [[maybe_unused]] const RasterConfig& config = RasterConfig{}) const override {
        RasterResult r;
        r.status        = RasterStatus::NOT_SUPPORTED;
        r.error_message = "Raster support not compiled in (THEMIS_ENABLE_RASTER not set)";
        return r;
    }

    [[nodiscard]] RasterResult queryBBox(
        [[maybe_unused]] const MBR& bbox,
        [[maybe_unused]] double resolution,
        [[maybe_unused]] const RasterConfig& config = RasterConfig{}) const override {
        RasterResult r;
        r.status        = RasterStatus::NOT_SUPPORTED;
        r.error_message = "Raster support not compiled in (THEMIS_ENABLE_RASTER not set)";
        return r;
    }
};

// ---------------------------------------------------------------------------
// RasterGridQueryImpl — full implementation (THEMIS_ENABLE_RASTER guard)
// ---------------------------------------------------------------------------

class RasterGridQueryImpl final : public IRasterQueryInterface {
public:
    explicit RasterGridQueryImpl(RasterGrid grid,
                                  std::string crs_wkt = "EPSG:4326",
                                  std::size_t band_count = 1);

    [[nodiscard]] RasterResult queryTile(
        const TileCoord& tile,
        const RasterConfig& config = RasterConfig{}) const override;

    [[nodiscard]] RasterResult queryBBox(
        const MBR& bbox,
        double resolution,
        const RasterConfig& config = RasterConfig{}) const override;

private:
    RasterGrid  grid_;
    std::string crs_wkt_;
    std::size_t band_count_;
};

// ---------------------------------------------------------------------------
// Factory
// ---------------------------------------------------------------------------

[[nodiscard]] std::unique_ptr<IRasterQueryInterface> makeRasterQueryInterface(
    RasterGrid grid,
    const std::string& crs_wkt = "EPSG:4326");

} // namespace geo
} // namespace themis
