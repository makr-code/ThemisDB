/**
 * @file tile_server.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=5, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "geo/tile_server.h"

#include <algorithm>
#include <cmath>
#include <sstream>

namespace themis {
namespace geo {

// ---------------------------------------------------------------------------
// Internal constants
// ---------------------------------------------------------------------------

static constexpr double kTilePi         = 3.14159265358979323846;
static constexpr double kMercatorMaxLat = 85.05112877980659; // atan(sinh(π)) in degrees
static constexpr uint32_t kMaxZoom      = 22;

/// Clamp value to [lo, hi].
static inline double clamp(double v, double lo, double hi) noexcept {
    return v < lo ? lo : (v > hi ? hi : v);
}

/// Replace all (non-overlapping) occurrences of `from` with `to` in `s`.
static std::string replaceAll(std::string s, const std::string &from, const std::string &to) {
    std::string::size_type pos = 0;
    while ((pos = s.find(from, pos)) != std::string::npos) {
        s.replace(pos, from.size(), to);
        pos += to.size();
    }
    return s;
}

// ---------------------------------------------------------------------------
// latLonToTile
// ---------------------------------------------------------------------------

TileCoord latLonToTile(double lon, double lat, uint32_t zoom) noexcept {
    if (zoom > kMaxZoom)
        zoom = kMaxZoom;

    // Clamp latitude to Mercator range to avoid infinite results at poles.
    lat = clamp(lat, -kMercatorMaxLat, kMercatorMaxLat);
    lon = clamp(lon, -180.0, 180.0);

    const double n = static_cast<double>(1u << zoom); // 2^zoom

    // Tile column: linear in longitude.
    const double x_f = (lon + 180.0) / 360.0 * n;

    // Tile row: inverse Mercator projection.
    // Note: std::log below is the natural logarithm (math), not a logging call.
    const double lat_r = lat * kTilePi / 180.0;
    const double y_f   = (1.0 - std::log(std::tan(lat_r) + 1.0 / std::cos(lat_r)) / kTilePi) / 2.0 * n;

    const uint32_t max_idx = (zoom == 0) ? 0u : static_cast<uint32_t>(n) - 1u;

    TileCoord tc;
    tc.zoom = zoom;
    tc.x    = static_cast<uint32_t>(clamp(std::floor(x_f), 0.0, static_cast<double>(max_idx)));
    tc.y    = static_cast<uint32_t>(clamp(std::floor(y_f), 0.0, static_cast<double>(max_idx)));
    return tc;
}

// ---------------------------------------------------------------------------
// tileXToLon / tileYToLat
// ---------------------------------------------------------------------------

double tileXToLon(uint32_t x, uint32_t zoom) noexcept {
    if (zoom > kMaxZoom)
        zoom = kMaxZoom;
    const double n = static_cast<double>(1u << zoom);
    return static_cast<double>(x) / n * 360.0 - 180.0;
}

double tileYToLat(uint32_t y, uint32_t zoom) noexcept {
    if (zoom > kMaxZoom)
        zoom = kMaxZoom;
    const double n   = static_cast<double>(1u << zoom);
    const double rad = std::atan(std::sinh(kTilePi * (1.0 - 2.0 * static_cast<double>(y) / n)));
    return rad * 180.0 / kTilePi;
}

// ---------------------------------------------------------------------------
// tileToBBox
// ---------------------------------------------------------------------------

MBR tileToBBox(const TileCoord &tile) noexcept {
    const double west  = tileXToLon(tile.x, tile.zoom);
    const double east  = tileXToLon(tile.x + 1, tile.zoom);
    const double north = tileYToLat(tile.y, tile.zoom);
    const double south = tileYToLat(tile.y + 1, tile.zoom);
    return MBR{west, south, east, north};
}

// ---------------------------------------------------------------------------
// formatTileUrl
// ---------------------------------------------------------------------------

std::string formatTileUrl(const std::string &url_template, const TileCoord &tile) {
    std::string url = url_template;
    url             = replaceAll(url, "{z}", std::to_string(tile.zoom));
    url             = replaceAll(url, "{x}", std::to_string(tile.x));
    url             = replaceAll(url, "{y}", std::to_string(tile.y));
    return url;
}

// ---------------------------------------------------------------------------
// tilesForBBox
// ---------------------------------------------------------------------------

std::vector<TileCoord> tilesForBBox(const MBR &bbox, uint32_t zoom) {
    if (zoom > kMaxZoom)
        zoom = kMaxZoom;

    // Clamp to world bounds.
    const double west  = clamp(bbox.minx, -180.0, 180.0);
    const double east  = clamp(bbox.maxx, -180.0, 180.0);
    const double south = clamp(bbox.miny, -kMercatorMaxLat, kMercatorMaxLat);
    const double north = clamp(bbox.maxy, -kMercatorMaxLat, kMercatorMaxLat);

    if (west > east || south > north) {
        return {};
    }

    const TileCoord nw = latLonToTile(west, north, zoom);
    const TileCoord se = latLonToTile(east, south, zoom);

    const uint32_t x0 = nw.x;
    const uint32_t x1 = se.x;
    const uint32_t y0 = nw.y;
    const uint32_t y1 = se.y;

    std::vector<TileCoord> result;
    result.reserve(static_cast<std::size_t>(x1 - x0 + 1) * static_cast<std::size_t>(y1 - y0 + 1));

    for (uint32_t y = y0; y <= y1; ++y) {
        for (uint32_t x = x0; x <= x1; ++x) {
            result.push_back(TileCoord{x, y, zoom});
        }
    }
    return result;
}

// ---------------------------------------------------------------------------
// encodeVectorTile
// ---------------------------------------------------------------------------

namespace {

/// Project a WGS84 coordinate into tile-local pixel space.
/// @param lon       Longitude in degrees.
/// @param lat       Latitude  in degrees.
/// @param bbox      Tile bounding box.
/// @param extent    Tile resolution (pixels).
/// @param px_out    Output pixel x (column, [0, extent]).
/// @param py_out    Output pixel y (row,    [0, extent]).
static void projectToTilePixels(double lon, double lat, const MBR &bbox, uint32_t extent, double &px_out,
                                double &py_out) noexcept {
    const double tile_w = bbox.maxx - bbox.minx;
    const double tile_h = bbox.maxy - bbox.miny;
    const double ext    = static_cast<double>(extent);

    // x increases left → right (west → east).
    px_out = (tile_w > 0.0) ? ((lon - bbox.minx) / tile_w * ext) : 0.0;

    // y increases top → bottom (north → south) in tile coordinates.
    py_out = (tile_h > 0.0) ? ((bbox.maxy - lat) / tile_h * ext) : 0.0;
}

/// Clip a coordinate to [0, extent].
static inline double clipExtent(double v, double extent) noexcept {
    return clamp(v, 0.0, extent);
}

/// Return true when (lon, lat) is inside or on the boundary of `bbox`.
static inline bool inBBox(double lon, double lat, const MBR &bbox) noexcept {
    return lon >= bbox.minx && lon <= bbox.maxx && lat >= bbox.miny && lat <= bbox.maxy;
}

} // anonymous namespace

VectorTileResult encodeVectorTile(const TileCoord &tile, const std::vector<GeometryInfo> &geometries,
                                  uint32_t tile_extent) {
    VectorTileResult result;
    result.tile = tile;

    const MBR bbox   = tileToBBox(tile);
    const double ext = static_cast<double>(tile_extent);

    for (const auto &geom : geometries) {
        VectorTileFeature feat;

        if (geom.type == GeometryType::Point) {
            if (geom.coords.empty()) {
                continue;
            }
            const auto &c = geom.coords.front();

            // Skip points outside the tile bounding box.
            if (!inBBox(c.x, c.y, bbox)) {
                continue;
            }

            double px{}, py{};
            projectToTilePixels(c.x, c.y, bbox, tile_extent, px, py);

            GeometryInfo projected(GeometryType::Point);
            projected.coords.emplace_back(clipExtent(px, ext), clipExtent(py, ext));
            feat.geometry = std::move(projected);

        } else if (geom.type == GeometryType::LineString) {
            if (geom.coords.empty()) {
                continue;
            }

            GeometryInfo projected(GeometryType::LineString);
            for (const auto &c : geom.coords) {
                double px{}, py{};
                projectToTilePixels(c.x, c.y, bbox, tile_extent, px, py);
                projected.coords.emplace_back(clipExtent(px, ext), clipExtent(py, ext));
            }
            if (projected.coords.empty()) {
                continue;
            }
            feat.geometry = std::move(projected);

        } else if (geom.type == GeometryType::Polygon) {
            if (geom.coords.empty()) {
                continue;
            }

            GeometryInfo projected(GeometryType::Polygon);
            for (const auto &c : geom.coords) {
                double px{}, py{};
                projectToTilePixels(c.x, c.y, bbox, tile_extent, px, py);
                projected.coords.emplace_back(clipExtent(px, ext), clipExtent(py, ext));
            }
            if (projected.coords.empty()) {
                continue;
            }
            feat.geometry = std::move(projected);

        } else {
            // Unsupported geometry type — skip.
            continue;
        }

        result.features.push_back(std::move(feat));
    }

    return result;
}

} // namespace geo
} // namespace themis
