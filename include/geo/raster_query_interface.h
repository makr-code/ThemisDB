/*
 * ThemisDB | File: raster_query_interface.h | Version: 0.0.1 | Last Modified: 2026-05-20 17:13:04
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 94/100 | Lines: 263
 * Open Issues: TODOs=1, Stubs=3, Gaps=5, Unimpl=0, Mock=1, Sim=0, Debt=0
 * Gap Correlation: internal=5 | external_v3=n/a | delta=n/a | status=no_external_data
 * External Severity (v3): C=n/a, H=n/a, M=n/a
 * PR: #4483 feat(geo): Add 6 abstract interface headers for geo module extensio... (2026-04-09T06:15:00Z)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#pragma once

/**
 * @file raster_query_interface.h
 * @brief Compile-time optional raster tile and bounding-box query interface.
 *
 * Implements the planned `IRasterQueryInterface` from
 * FUTURE_ENHANCEMENTS.md §"Raster Data Query Interface".
 *
 * Compile-time guards:
 *  - The concrete implementation and `RasterStatus` values are always
 *    available, but the concrete implementations return
 *    `RasterStatus::NOT_SUPPORTED` when `THEMIS_ENABLE_RASTER` is not defined.
 *  - `#ifdef THEMIS_ENABLE_RASTER` guards only the full implementation in
 *    `raster_query_interface.cpp`; the header is always visible.
 *
 * Design constraints (per FUTURE_ENHANCEMENTS.md):
 *  - Tile size bounded by `RasterConfig::maxTileSizeBytes()`.
 *  - Raster queries return `RasterResult` with band data, resolution metadata,
 *    and CRS info.
 *  - No-op stub returns `RasterStatus::NOT_SUPPORTED` when
 *    `THEMIS_ENABLE_RASTER` is not defined.
 *
 * Target: v2.5.0
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

/**
 * @brief Configuration for `IRasterQueryInterface` implementations.
 *
 * Tile queries are bounded by `max_tile_size_bytes()` to prevent memory
 * exhaustion via oversized tile requests.
 */
class RasterConfig {
public:
    /// Default maximum tile size: 64 MiB.
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

/**
 * @brief Status codes returned by `IRasterQueryInterface` operations.
 */
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

/**
 * @brief Result of a raster query operation.
 *
 * On success (`status == OK`) the `grid` field contains the raster data.
 * The `crs_wkt` field holds the WKT representation of the coordinate reference
 * system (e.g. "EPSG:4326").
 */
struct RasterResult {
    RasterStatus status{RasterStatus::NOT_SUPPORTED};

    /// Raster data returned by the query.  Empty on error.
    RasterGrid grid;

    /// Well-known text identifier of the CRS (e.g. "EPSG:4326").
    std::string crs_wkt;

    /// Number of bands in the result (1 for elevation/density; >1 for RGB).
    std::size_t band_count{1};

    /// Resolution in degrees per pixel (longitude axis).
    double resolution_x{0.0};
    /// Resolution in degrees per pixel (latitude axis).
    double resolution_y{0.0};

    /// Human-readable error description (non-empty when status != OK).
    std::string error_message;

    /// @return true when the query succeeded.
    [[nodiscard]] bool ok() const noexcept { return status == RasterStatus::OK; }
};

// ---------------------------------------------------------------------------
// IRasterQueryInterface — abstract base
// ---------------------------------------------------------------------------

/**
 * @brief Abstract interface for raster tile and bbox queries.
 *
 * Implementations are compile-time optional:
 *  - `RasterGridQueryImpl` (raster_query_interface.cpp) — active when
 *    `THEMIS_ENABLE_RASTER` is defined at compile time.
 *  - `NoOpRasterQueryImpl` — always available; returns
 *    `RasterStatus::NOT_SUPPORTED` for every operation.
 *
 * @par Thread safety
 * Implementations must be thread-safe (read-only); multiple concurrent calls
 * to `queryTile()` / `queryBBox()` must not produce data races.
 */
class IRasterQueryInterface {
public:
    virtual ~IRasterQueryInterface() = default;

    /**
     * @brief Return the raster data for a single XYZ tile.
     *
     * @param tile    Tile address (zoom / x / y in Web Mercator).
     * @param config  Configuration controlling size limits.
     * @return        Result containing raster grid or error status.
     */
    [[nodiscard]] virtual RasterResult queryTile(
        const TileCoord& tile,
        const RasterConfig& config = RasterConfig{}) const = 0;

    /**
     * @brief Return the raster data covering a geographic bounding box.
     *
     * @param bbox        Query bounding box (WGS-84 degrees).
     * @param resolution  Output resolution in degrees-per-pixel (both axes).
     * @param config      Configuration controlling size limits.
     * @return            Result containing raster grid or error status.
     */
    [[nodiscard]] virtual RasterResult queryBBox(
        const MBR& bbox,
        double resolution,
        const RasterConfig& config = RasterConfig{}) const = 0;
};

// ---------------------------------------------------------------------------
// NoOpRasterQueryImpl — always-available no-op stub
// ---------------------------------------------------------------------------

/**
 * @brief No-op raster query implementation.
 *
 * Returns `RasterStatus::NOT_SUPPORTED` for every operation.
 * Active when `THEMIS_ENABLE_RASTER` is not defined.
 */
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

/**
 * @brief Raster query implementation backed by an in-memory `RasterGrid`.
 *
 * Provides `queryTile()` by converting the XYZ tile bounds to a WGS-84 bbox
 * and delegating to `queryBBox(bbox, tile_resolution)`.
 *
 * The grid must be injected at construction time.  Raster queries are
 * read-only; the implementation is thread-safe.
 *
 * This class is always declared; the implementation in
 * `raster_query_interface.cpp` is compiled unconditionally (the guard is
 * already satisfied by the existing `raster.h` / `raster.cpp`).
 */
class RasterGridQueryImpl final : public IRasterQueryInterface {
public:
    /**
     * @param grid      Source raster grid.
     * @param crs_wkt   WKT identifier for the CRS (default "EPSG:4326").
     * @param band_count Number of bands (default 1).
     */
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

/**
 * @brief Create the appropriate `IRasterQueryInterface` implementation.
 *
 * If @p grid is non-empty (width > 0 && height > 0) a `RasterGridQueryImpl`
 * wrapping @p grid is returned.  Otherwise a `NoOpRasterQueryImpl` is returned.
 *
 * This factory is the recommended way to create an implementation.
 */
[[nodiscard]] std::unique_ptr<IRasterQueryInterface> makeRasterQueryInterface(
    RasterGrid grid,
    const std::string& crs_wkt = "EPSG:4326");

} // namespace geo
} // namespace themis
