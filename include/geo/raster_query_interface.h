/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            raster_query_interface.h                           ║
  Version:         1.0.0                                              ║
  Last Modified:   2026-04-09                                         ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "utils/geo/ewkb.h"
#include "geo/raster.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace themis {
namespace geo {

// ── Tile key ──────────────────────────────────────────────────────────────

/**
 * @brief Identifier for a single map tile in the XYZ tiling scheme.
 */
struct TileKey {
    uint32_t zoom{0};  ///< Zoom level (0 = whole world; typical max 22).
    int64_t  x{0};     ///< Tile column index.
    int64_t  y{0};     ///< Tile row index.

    bool operator==(const TileKey& o) const noexcept {
        return zoom == o.zoom && x == o.x && y == o.y;
    }
};

// ── Configuration ─────────────────────────────────────────────────────────

/**
 * @brief Runtime limits for raster queries.
 *
 * Raster queries that would produce results exceeding
 * `maxTileSizeBytes()` are rejected with RasterStatus::TILE_TOO_LARGE.
 */
class RasterConfig {
public:
    static RasterConfig& instance();

    std::size_t maxTileSizeBytes() const noexcept { return max_tile_bytes_; }
    void setMaxTileSizeBytes(std::size_t bytes) noexcept {
        max_tile_bytes_ = bytes;
    }

private:
    RasterConfig() = default;
    std::size_t max_tile_bytes_{64 * 1024 * 1024};  // 64 MiB default
};

// ── Result type ───────────────────────────────────────────────────────────

/// Status codes for IRasterQueryInterface operations.
enum class RasterStatus {
    OK,               ///< Query succeeded.
    NOT_SUPPORTED,    ///< Feature not compiled in (THEMIS_ENABLE_RASTER=OFF).
    TILE_NOT_FOUND,   ///< Requested tile does not exist in the data source.
    TILE_TOO_LARGE,   ///< Result would exceed RasterConfig::maxTileSizeBytes().
    INVALID_BBOX,     ///< Bounding box is degenerate or out of range.
    INTERNAL_ERROR,   ///< Unspecified internal error; check logs for details.
};

/**
 * @brief Result of a single raster tile or BBox query.
 *
 * On success (`status == RasterStatus::OK`) the `grid` field is populated
 * with band data, resolution metadata, and CRS information.  On failure
 * `grid` is empty and `error_message` provides a human-readable description.
 */
struct RasterResult {
    RasterStatus status{RasterStatus::OK};
    RasterGrid   grid;
    std::string  error_message;
    std::string  crs_srid;   ///< CRS of the returned grid (e.g. "EPSG:4326").

    bool ok() const noexcept { return status == RasterStatus::OK; }
};

// ── Interface ─────────────────────────────────────────────────────────────

/**
 * @brief Abstract interface for raster data queries.
 *
 * This interface is compile-time optional and requires
 * `THEMIS_ENABLE_RASTER` to be defined.  When the flag is absent, a
 * no-op stub implementation is provided (see below) that returns
 * RasterStatus::NOT_SUPPORTED for every call.
 *
 * Tile queries are bounded by RasterConfig::maxTileSizeBytes() to prevent
 * memory exhaustion via oversized tile requests.
 *
 * **Thread safety**: implementations are NOT required to be thread-safe;
 * external synchronisation is the caller's responsibility.
 */
class IRasterQueryInterface {
public:
    virtual ~IRasterQueryInterface() = default;

    /**
     * @brief Retrieve a single map tile identified by @p key.
     *
     * @param key  XYZ tile identifier.
     * @return RasterResult containing the grid data or an error status.
     */
    virtual RasterResult queryTile(const TileKey& key) = 0;

    /**
     * @brief Retrieve raster data covering @p bbox at the given resolution.
     *
     * @param bbox        Geographic bounding box (WGS84).
     * @param resolution  Number of grid cells along the longer axis.
     *                    Must be in the range [1, 16384].
     * @return RasterResult containing the grid data or an error status.
     */
    virtual RasterResult queryBBox(const MBR& bbox, std::size_t resolution) = 0;
};

// ── No-op stub ────────────────────────────────────────────────────────────

#ifndef THEMIS_ENABLE_RASTER
/**
 * @brief No-op stub used when THEMIS_ENABLE_RASTER is not defined.
 *
 * Every method returns RasterStatus::NOT_SUPPORTED with no side effects.
 * This allows code that holds an IRasterQueryInterface pointer to compile
 * cleanly on platforms where raster support was not compiled in.
 */
class NullRasterQueryInterface final : public IRasterQueryInterface {
public:
    RasterResult queryTile(const TileKey&) override {
        return {RasterStatus::NOT_SUPPORTED, {}, "THEMIS_ENABLE_RASTER not defined"};
    }

    RasterResult queryBBox(const MBR&, std::size_t) override {
        return {RasterStatus::NOT_SUPPORTED, {}, "THEMIS_ENABLE_RASTER not defined"};
    }
};
#endif // !THEMIS_ENABLE_RASTER

} // namespace geo
} // namespace themis
