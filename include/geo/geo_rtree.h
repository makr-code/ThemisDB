/**
 * @file geo_rtree.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.18
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "utils/geo/ewkb.h"
#include <string>
#include <vector>
#include <memory>
#include <cstddef>

namespace themis {
namespace geo {

/**
 * @brief In-memory R-tree spatial index for sub-linear `intersects` and
 *        `contains` queries on collections of `GeometryInfo` objects.
 *
 * When compiled with `THEMIS_GEO_BOOST_BACKEND` and Boost.Geometry headers
 * present, the implementation uses `boost::geometry::index::rtree` with the
 * `rstar<16>` splitting strategy.  Without Boost the class falls back to a
 * plain MBR list with an O(n) linear scan — semantically identical but slower.
 *
 * **Thread safety**: not thread-safe; callers must synchronize externally.
 *
 * **Lifecycle**:
 * 1. Build the index once via `bulkLoad()` (preferred, uses STR packing) or
 *    incrementally via repeated `insert()` calls.
 * 2. Query with `intersects()` or `contains()`.
 * 3. Invalidate on write via `remove()` or `clear()`.
 *
 * **Memory reporting**: `memoryBytes()` returns a conservative estimate of
 * heap usage; the value is also emitted as a structured log entry with the
 * field `geo_index_bytes_allocated` on every `bulkLoad()` / `insert()` call
 * that changes the index size.
 */
class GeoRTree {
public:
    GeoRTree();
    ~GeoRTree();

    // Non-copyable, movable
    GeoRTree(const GeoRTree&) = delete;
    GeoRTree& operator=(const GeoRTree&) = delete;
    GeoRTree(GeoRTree&&) noexcept;
    GeoRTree& operator=(GeoRTree&&) noexcept;

    // ── Build ──────────────────────────────────────────────────────────────

    /**
     * @brief Bulk-load the index from a collection of (key, geometry) pairs.
     *
     * Uses STR (Sort-Tile-Recursive) packing via Boost's bulk-insert
     * constructor, which is 3–5× faster than incremental insertion for
     * read-heavy workloads.  Replaces any existing index content.
     *
     * @param entries  Vector of {string key, GeometryInfo geom} pairs.
     */
    void bulkLoad(const std::vector<std::pair<std::string, GeometryInfo>>& entries);

    /**
     * @brief Insert a single geometry with an associated string key.
     *
     * Incremental insertion is O(log n) with the R*-tree variant.
     * Prefer `bulkLoad()` when inserting many geometries at once.
     */
    void insert(const std::string& key, const GeometryInfo& geom);

    /**
     * @brief Remove a geometry from the index.
     *
     * @param key   The key originally used during `insert()` / `bulkLoad()`.
     * @param geom  The geometry (MBR must match what was indexed).
     * @return true if the entry was found and removed, false otherwise.
     */
    bool remove(const std::string& key, const GeometryInfo& geom);

    /**
     * @brief Remove all entries from the index.
     */
    void clear();

    // ── Query ──────────────────────────────────────────────────────────────

    /**
     * @brief Return the keys of all entries whose MBR intersects `query_bbox`.
     *
     * @param query_bbox  The axis-aligned bounding box to test against.
     * @return Unordered vector of matching keys.
     */
    std::vector<std::string> intersects(const MBR& query_bbox) const;

    /**
     * @brief Return the keys of all entries whose MBR contains the point (x, y).
     *
     * @param x  Longitude (WGS84).
     * @param y  Latitude (WGS84).
     * @return Unordered vector of matching keys.
     */
    std::vector<std::string> contains(double x, double y) const;

    // ── Metadata ───────────────────────────────────────────────────────────

    /**
     * @brief Return the number of entries currently in the index.
     */
    std::size_t size() const;

    /**
     * @brief Return a conservative estimate of heap memory used by the index
     *        in bytes.
     *
     * The value is also logged as a structured entry with field
     * `geo_index_bytes_allocated` for operator observability.
     */
    std::size_t memoryBytes() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace geo
}  // namespace themis
