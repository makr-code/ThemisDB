/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            geo_index.h                                        ║
  Version:         1.0.0                                              ║
  Last Modified:   2026-04-09                                         ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "utils/geo/ewkb.h"
#include "geo/rtree_cursor.h"

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace themis {
namespace geo {

/// Entry returned by index queries: an associated key and its geometry.
/// Defined in rtree_cursor.h; included transitively via geo_index.h.

/**
 * @brief Abstract spatial index interface.
 *
 * Concrete implementations may use R-tree, flat (linear) scan, or a
 * GPU-backed structure.  All operations are purely spatial; access control
 * is enforced at the collection layer before any index is consulted.
 *
 * **Thread safety**: implementations are NOT required to be thread-safe.
 * External synchronisation is the caller's responsibility.
 */
class IGeoIndex {
public:
    virtual ~IGeoIndex() = default;

    // ── Build ──────────────────────────────────────────────────────────

    /**
     * @brief Insert a geometry with an associated key into the index.
     *
     * @param key  Unique identifier for the geometry.
     * @param geom Geometry to index.
     */
    virtual void insert(const std::string& key, const GeometryInfo& geom) = 0;

    /**
     * @brief Remove a geometry from the index.
     *
     * @param key  The key used during insertion.
     * @param geom The geometry (MBR must match the indexed geometry).
     * @return true if the entry was found and removed.
     */
    virtual bool remove(const std::string& key, const GeometryInfo& geom) = 0;

    /**
     * @brief Remove all entries from the index.
     */
    virtual void clear() = 0;

    // ── Query ──────────────────────────────────────────────────────────

    /**
     * @brief Return keys of all entries whose MBR intersects @p bbox.
     *
     * @param bbox  Axis-aligned bounding box filter.
     * @return Unordered vector of matching keys.
     */
    virtual std::vector<std::string> intersects(const MBR& bbox) const = 0;

    /**
     * @brief Return keys of all entries whose MBR contains point (x, y).
     *
     * @param x  Longitude (WGS84 degrees).
     * @param y  Latitude  (WGS84 degrees).
     * @return Unordered vector of matching keys.
     */
    virtual std::vector<std::string> contains(double x, double y) const = 0;

    /**
     * @brief Open a pull-based cursor for range queries over @p bbox.
     *
     * The cursor yields one matching entry at a time without materialising
     * the full result set.  Invalidated if the index is mutated while the
     * cursor is open; subsequent `next()` calls return CursorStatus::STALE.
     *
     * @param bbox  Axis-aligned bounding box filter.
     * @return Unique pointer to a cursor; never null.
     */
    virtual std::unique_ptr<IRTreeCursor> openRangeCursor(const MBR& bbox) const = 0;

    /**
     * @brief Open a pull-based cursor for k-nearest-neighbour queries.
     *
     * @param centre  Query point (lon, lat in WGS84).
     * @param k       Number of nearest neighbours to return.
     * @return Unique pointer to a cursor; never null.
     */
    virtual std::unique_ptr<IRTreeCursor> openKNNCursor(
        const GeometryInfo& centre, std::size_t k) const = 0;

    // ── Metadata ───────────────────────────────────────────────────────

    /// Return the number of entries currently in the index.
    virtual std::size_t size() const = 0;

    /// Return a conservative estimate of heap memory used by the index (bytes).
    virtual std::size_t memoryBytes() const = 0;
};

} // namespace geo
} // namespace themis
