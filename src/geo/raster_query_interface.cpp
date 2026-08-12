/**
 * @file raster_query_interface.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors
//

#include "geo/raster_query_interface.h"

#include <cmath>
#include <cstddef>
#include <stdexcept>

namespace themis {
namespace geo {

// ---------------------------------------------------------------------------
// Tile → WGS-84 bbox conversion
// ---------------------------------------------------------------------------

namespace {

/// Convert a slippy-map tile (zoom/x/y) to WGS-84 lon/lat bounds.
/// Reference: https://wiki.openstreetmap.org/wiki/Slippy_map_tilenames
MBR tileToWGS84(const TileCoord &tile) {
    constexpr double kPi     = 3.14159265358979323846;
    const double n           = static_cast<double>(1u << tile.zoom);
    const double lon_min     = static_cast<double>(tile.x) / n * 360.0 - 180.0;
    const double lon_max     = static_cast<double>(tile.x + 1) / n * 360.0 - 180.0;
    const double lat_min_rad = std::atan(std::sinh(kPi * (1.0 - 2.0 * static_cast<double>(tile.y + 1) / n)));
    const double lat_max_rad = std::atan(std::sinh(kPi * (1.0 - 2.0 * static_cast<double>(tile.y) / n)));
    return MBR{lon_min, lat_min_rad * 180.0 / kPi, lon_max, lat_max_rad * 180.0 / kPi};
}

bool isValidBBox(const MBR &bbox) noexcept {
    return bbox.minx < bbox.maxx && bbox.miny < bbox.maxy && bbox.minx >= -180.0 && bbox.maxx <= 180.0
           && bbox.miny >= -90.0 && bbox.maxy <= 90.0;
}

bool isValidTile(const TileCoord &tile) noexcept {
    if (tile.zoom > 22) {
        return false;
    }
    const uint32_t max_xy = 1u << tile.zoom;
    return tile.x < max_xy && tile.y < max_xy;
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// RasterGridQueryImpl
// ---------------------------------------------------------------------------

RasterGridQueryImpl::RasterGridQueryImpl(RasterGrid grid, std::string crs_wkt, std::size_t band_count)
    : grid_(std::move(grid)), crs_wkt_(std::move(crs_wkt)), band_count_(band_count) {}

RasterResult RasterGridQueryImpl::queryBBox(const MBR &bbox, double resolution, const RasterConfig &config) const {
    RasterResult result;

    if (!isValidBBox(bbox)) {
        result.status        = RasterStatus::INVALID_BBOX;
        result.error_message = "queryBBox: bounding box is invalid or out of WGS-84 range";
        return result;
    }

    if (resolution <= 0.0) {
        result.status        = RasterStatus::INVALID_BBOX;
        result.error_message = "queryBBox: resolution must be > 0";
        return result;
    }

    // Estimate the resulting grid size and check against the size limit.
    const double lon_span             = bbox.maxx - bbox.minx;
    const double lat_span             = bbox.maxy - bbox.miny;
    const auto width                  = static_cast<std::size_t>(std::ceil(lon_span / resolution));
    const auto height                 = static_cast<std::size_t>(std::ceil(lat_span / resolution));
    const std::size_t estimated_bytes = width * height * sizeof(float) * band_count_;

    if (estimated_bytes > config.maxTileSizeBytes()) {
        result.status        = RasterStatus::TILE_TOO_LARGE;
        result.error_message = "queryBBox: estimated tile size " + std::to_string(estimated_bytes)
                               + " bytes exceeds limit " + std::to_string(config.maxTileSizeBytes());
        return result;
    }

    // Delegate to the existing queryBBox free function in raster.h
    result.grid         = geo::queryBBox(grid_, bbox);
    result.status       = RasterStatus::OK;
    result.crs_wkt      = crs_wkt_;
    result.band_count   = band_count_;
    result.resolution_x = resolution;
    result.resolution_y = resolution;
    return result;
}

RasterResult RasterGridQueryImpl::queryTile(const TileCoord &tile, const RasterConfig &config) const {
    if (!isValidTile(tile)) {
        RasterResult r;
        r.status        = RasterStatus::INVALID_KEY;
        r.error_message = "queryTile: tile key is out of range";
        return r;
    }
    const MBR bbox = tileToWGS84(tile);

    // Tile resolution: width of the tile in degrees / 256 pixels
    const double tile_res = (bbox.maxx - bbox.minx) / 256.0;
    return queryBBox(bbox, tile_res, config);
}

// ---------------------------------------------------------------------------
// Factory
// ---------------------------------------------------------------------------

std::unique_ptr<IRasterQueryInterface> makeRasterQueryInterface(RasterGrid grid, const std::string &crs_wkt) {
    if (grid.empty()) {
        return std::make_unique<NoOpRasterQueryImpl>();
    }
    return std::make_unique<RasterGridQueryImpl>(std::move(grid), crs_wkt);
}

} // namespace geo
} // namespace themis
