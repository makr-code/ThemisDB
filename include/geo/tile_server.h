/**
 * @file tile_server.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "utils/geo/ewkb.h"

#include <cstdint>
#include <string>
#include <vector>

namespace themis {
namespace geo {

// ---------------------------------------------------------------------------
// Tile coordinate system (Web Mercator / XYZ / EPSG:3857)
// ---------------------------------------------------------------------------

struct TileCoord {
    uint32_t x{0};     ///< Column index (0 … 2^zoom − 1, west → east).
    uint32_t y{0};     ///< Row index    (0 … 2^zoom − 1, north → south).
    uint32_t zoom{0};  ///< Zoom level (0 … 22).
};

struct TileLayerConfig {
    std::string url_template;

    std::string attribution;

    uint32_t min_zoom{0};

    uint32_t max_zoom{19};

    uint32_t tile_size{256};
};

struct VectorTileFeature {
    GeometryInfo geometry;

    std::string properties_json;
};

struct VectorTileResult {
    TileCoord tile;                       ///< Tile address.
    std::vector<VectorTileFeature> features; ///< Clipped and encoded features.
};

// ---------------------------------------------------------------------------
// Coordinate ↔ tile conversion
// ---------------------------------------------------------------------------

/**
 * @brief Lat Lon To Tile.
 * @param[in] lon Input parameter.
 * @param[in] lat Input parameter.
 * @param[in] zoom Input parameter.
 * @return Return value.
 * @note Exception safety: noexcept.
 */
TileCoord latLonToTile(double lon, double lat, uint32_t zoom) noexcept;

/**
 * @brief Tile To BBox.
 * @param[in] tile Input parameter.
 * @return Return value.
 * @note Exception safety: noexcept.
 */
MBR tileToBBox(const TileCoord& tile) noexcept;

/**
 * @brief Tile XTo Lon.
 * @param[in] x Input parameter.
 * @param[in] zoom Input parameter.
 * @return Return value.
 * @note Exception safety: noexcept.
 */
double tileXToLon(uint32_t x, uint32_t zoom) noexcept;

/**
 * @brief Tile YTo Lat.
 * @param[in] y Input parameter.
 * @param[in] zoom Input parameter.
 * @return Return value.
 * @note Exception safety: noexcept.
 */
double tileYToLat(uint32_t y, uint32_t zoom) noexcept;

// ---------------------------------------------------------------------------
// URL templating
// ---------------------------------------------------------------------------

/**
 * @brief Format Tile Url.
 * @param[in] url_template Input parameter.
 * @param[in] tile Input parameter.
 * @return Return value.
 */
std::string formatTileUrl(const std::string& url_template,
                          const TileCoord& tile);

// ---------------------------------------------------------------------------
// Spatial query helpers
// ---------------------------------------------------------------------------

/**
 * @brief Tiles For BBox.
 * @param[in] bbox Input parameter.
 * @param[in] zoom Input parameter.
 * @return Return value.
 */
std::vector<TileCoord> tilesForBBox(const MBR& bbox, uint32_t zoom);

// ---------------------------------------------------------------------------
// Vector tile encoding
// ---------------------------------------------------------------------------

VectorTileResult encodeVectorTile(
    const TileCoord& tile,
    const std::vector<GeometryInfo>& geometries,
    uint32_t tile_extent = 4096);

} // namespace geo
} // namespace themis
