/**
 * @file tile_server.h
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

#include <cstdint>
#include <string>
#include <vector>

namespace themis {
namespace geo {

// ---------------------------------------------------------------------------
// Tile coordinate system (Web Mercator / XYZ / EPSG:3857)
// ---------------------------------------------------------------------------

/**
 * @brief An XYZ tile address in the slippy-map / Web Mercator tile scheme.
 *
 * Zoom level 0 is a single 256×256 tile covering the whole world.
 * At zoom level z, the world is divided into 2^z × 2^z tiles.
 * Tile origin (x=0, y=0) is at the top-left (NW) corner of the map.
 */
struct TileCoord {
    uint32_t x{0};     ///< Column index (0 … 2^zoom − 1, west → east).
    uint32_t y{0};     ///< Row index    (0 … 2^zoom − 1, north → south).
    uint32_t zoom{0};  ///< Zoom level (0 … 22).
};

/**
 * @brief Configuration for a tile server / tile layer.
 *
 * URL templates follow the OpenStreetMap / Leaflet convention:
 * `{z}` is replaced by the zoom level, `{x}` by the tile column,
 * `{y}` by the tile row (top-left origin).
 *
 * Example (OpenStreetMap):
 * @code
 *   TileLayerConfig cfg;
 *   cfg.url_template = "https://tile.openstreetmap.org/{z}/{x}/{y}.png";
 *   cfg.min_zoom = 0;
 *   cfg.max_zoom = 19;
 *   cfg.attribution = "© OpenStreetMap contributors";
 * @endcode
 */
struct TileLayerConfig {
    /// URL template with `{z}`, `{x}`, `{y}` placeholders.
    std::string url_template;

    /// Human-readable attribution string (displayed in the map UI).
    std::string attribution;

    /// Minimum supported zoom level (inclusive, default 0).
    uint32_t min_zoom{0};

    /// Maximum supported zoom level (inclusive, default 19).
    uint32_t max_zoom{19};

    /// Tile size in pixels (default 256; some servers use 512).
    uint32_t tile_size{256};
};

/**
 * @brief A single feature encoded for inclusion in a vector tile.
 *
 * This is a lightweight representation suitable for AQL result rendering
 * without a full MVT (Mapbox Vector Tile) protobuf dependency.
 */
struct VectorTileFeature {
    /// Geometry clipped to the tile extent in tile-local pixel coordinates
    /// (origin top-left; extents [0, tile_extent)).
    GeometryInfo geometry;

    /// Optional JSON-encoded property bag for the feature.
    std::string properties_json;
};

/**
 * @brief Result of encoding a set of geometries into a single tile.
 */
struct VectorTileResult {
    TileCoord tile;                       ///< Tile address.
    std::vector<VectorTileFeature> features; ///< Clipped and encoded features.
};

// ---------------------------------------------------------------------------
// Coordinate ↔ tile conversion
// ---------------------------------------------------------------------------

/**
 * @brief Convert a WGS84 longitude/latitude to the containing tile at the
 *        given zoom level.
 *
 * Uses the standard Web Mercator (EPSG:3857) tile projection.
 * Latitude is clamped to the Mercator range (≈ ±85.051129°) before
 * conversion.
 *
 * @param lon   Longitude in decimal degrees (WGS84, -180 … 180).
 * @param lat   Latitude  in decimal degrees (WGS84, ≈ -85.05 … 85.05).
 * @param zoom  Zoom level (0 … 22).
 * @return      Tile coordinate containing the point.
 */
TileCoord latLonToTile(double lon, double lat, uint32_t zoom) noexcept;

/**
 * @brief Compute the WGS84 bounding box (MBR) of a tile.
 *
 * Returns the geographic extent (min/max longitude and latitude) of the
 * tile identified by (x, y, zoom).  The MBR can be used directly as a
 * spatial filter in geo queries.
 *
 * @param tile  Tile address.
 * @return      WGS84 bounding box: { minx=west, miny=south,
 *              maxx=east, maxy=north } in degrees.
 */
MBR tileToBBox(const TileCoord& tile) noexcept;

/**
 * @brief Convert a tile x index to the western longitude boundary.
 *
 * @param x     Tile column (0 … 2^zoom − 1).
 * @param zoom  Zoom level.
 * @return      Longitude of the western edge in degrees.
 */
double tileXToLon(uint32_t x, uint32_t zoom) noexcept;

/**
 * @brief Convert a tile y index to the northern latitude boundary.
 *
 * Uses the inverse Mercator projection.
 *
 * @param y     Tile row (0 … 2^zoom − 1).
 * @param zoom  Zoom level.
 * @return      Latitude of the northern edge in degrees.
 */
double tileYToLat(uint32_t y, uint32_t zoom) noexcept;

// ---------------------------------------------------------------------------
// URL templating
// ---------------------------------------------------------------------------

/**
 * @brief Format a tile URL from a template.
 *
 * Replaces `{z}`, `{x}`, and `{y}` in `url_template` with the
 * corresponding values from `tile`.  The replacement is case-sensitive and
 * non-recursive.
 *
 * @param url_template  URL pattern with `{z}`, `{x}`, `{y}` placeholders.
 * @param tile          Tile address.
 * @return              Formatted URL string.
 */
std::string formatTileUrl(const std::string& url_template,
                          const TileCoord& tile);

// ---------------------------------------------------------------------------
// Spatial query helpers
// ---------------------------------------------------------------------------

/**
 * @brief List all tiles at `zoom` that intersect the given bounding box.
 *
 * Returns the minimal set of `TileCoord` values whose geographic extents
 * overlap with `bbox` at the requested zoom level.  Useful for pre-fetching
 * tiles to cover a spatial query result.
 *
 * @param bbox  Geographic query region (WGS84 degrees).
 * @param zoom  Zoom level.
 * @return      Vector of intersecting tiles (may be empty when bbox is
 *              outside [-180, -85.05, 180, 85.05]).
 */
std::vector<TileCoord> tilesForBBox(const MBR& bbox, uint32_t zoom);

// ---------------------------------------------------------------------------
// Vector tile encoding
// ---------------------------------------------------------------------------

/**
 * @brief Clip and encode a list of geometries into a single vector tile.
 *
 * Each geometry is clipped to the WGS84 bounding box of `tile` and
 * projected into tile-local pixel coordinates (origin top-left, extent
 * [0, tile_extent)).  The `tile_extent` parameter controls the coordinate
 * precision of the output (default 4096, matching the MVT spec default).
 *
 * Only `Point`, `LineString`, and `Polygon` geometries are currently
 * supported; unsupported types are silently skipped.
 *
 * @param tile         Target tile address.
 * @param geometries   Input geometries (WGS84 degrees).
 * @param tile_extent  Tile pixel resolution (default 4096).
 * @return             VectorTileResult with clipped features.
 */
VectorTileResult encodeVectorTile(
    const TileCoord& tile,
    const std::vector<GeometryInfo>& geometries,
    uint32_t tile_extent = 4096);

} // namespace geo
} // namespace themis
