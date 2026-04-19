#pragma once

/**
 * @file rtree_cursor.h
 * @brief Pull-based R-tree cursor API for range and k-NN spatial traversal.
 *
 * Implements the planned `IRTreeCursor` and `IGeoIndex` interfaces from
 * FUTURE_ENHANCEMENTS.md §"R-tree Cursor API".
 *
 * Design constraints (per FUTURE_ENHANCEMENTS.md):
 *  - `IRTreeCursor` is pull-based and single-threaded per instance.
 *  - Parallel traversal uses multiple cursors opened independently.
 *  - Cursor invalidated if the underlying index is mutated; `next()` returns
 *    `CursorStatus::STALE` thereafter.
 *  - `openRangeCursor(bbox)` — iterate all entries whose MBR overlaps bbox.
 *  - `openKNNCursor(point, k)` — iterate the k nearest entries by MBR centroid.
 *  - `estimatedResultCount()` for query planning.
 *
 * Target: v2.5.0
 */

#include "utils/geo/ewkb.h"

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace themis {
namespace geo {

// ---------------------------------------------------------------------------
// Cursor status
// ---------------------------------------------------------------------------

/**
 * @brief Return codes for `IRTreeCursor::next()`.
 */
enum class CursorStatus {
    OK,    ///< Entry written; caller may call `next()` again.
    END,   ///< No more entries; cursor is exhausted.
    STALE, ///< Underlying index was mutated; cursor result is undefined.
};

// ---------------------------------------------------------------------------
// GeoIndexEntry
// ---------------------------------------------------------------------------

/**
 * @brief A single entry returned by an R-tree cursor.
 *
 * For k-NN cursors, `distance_m` holds the Haversine distance in metres
 * from the query point to the MBR centroid.  For range cursors it is 0.0.
 */
struct GeoIndexEntry {
    std::string  key;        ///< User-supplied key of the indexed geometry.
    GeometryInfo geom;       ///< Geometry (as stored in the index).
    double       distance_m; ///< Centroid-to-centroid distance (k-NN only).
};

// ---------------------------------------------------------------------------
// IRTreeCursor
// ---------------------------------------------------------------------------

/**
 * @brief Abstract pull-based cursor for R-tree traversal.
 *
 * Callers obtain an `IRTreeCursor` from `IGeoIndex::openRangeCursor()` or
 * `IGeoIndex::openKNNCursor()` and repeatedly call `next()` until it returns
 * `CursorStatus::END`.
 *
 * A cursor is single-threaded; sharing across threads requires external
 * synchronisation.  If the owning index is mutated while the cursor is open
 * subsequent calls to `next()` return `CursorStatus::STALE`.
 */
class IRTreeCursor {
public:
    virtual ~IRTreeCursor() = default;

    /**
     * @brief Advance the cursor and write the next entry.
     *
     * @param[out] entry  Filled with the next matching entry on `OK`.
     *                    Contents undefined on `END` or `STALE`.
     * @return `CursorStatus::OK`    — entry valid; cursor not yet exhausted.
     * @return `CursorStatus::END`   — cursor exhausted; `entry` unchanged.
     * @return `CursorStatus::STALE` — index was mutated; `entry` unchanged.
     */
    [[nodiscard]] virtual CursorStatus next(GeoIndexEntry& entry) = 0;

    /**
     * @brief Return the estimated number of results this cursor will produce.
     *
     * The estimate may be an over- or under-count.  For range cursors it equals
     * the number of candidates pre-filtered by the R-tree MBR check.  For k-NN
     * cursors it returns the requested `k` (capped by index size).
     *
     * Useful for query planning to decide between index scan and full scan.
     */
    [[nodiscard]] virtual std::size_t estimatedResultCount() const noexcept = 0;
};

// ---------------------------------------------------------------------------
// IGeoIndex
// ---------------------------------------------------------------------------

/**
 * @brief Abstract spatial index interface.
 *
 * Provides cursor-based access to range and k-NN queries.
 * Concrete implementations: `GeoRTreeIndex` (wraps `GeoRTree`).
 *
 * The index is not thread-safe; callers must synchronise externally.
 */
class IGeoIndex {
public:
    virtual ~IGeoIndex() = default;

    /**
     * @brief Open a range cursor over all entries whose MBR overlaps @p bbox.
     *
     * @param bbox  Query bounding box.
     * @return      A new cursor positioned before the first result.
     */
    [[nodiscard]] virtual std::unique_ptr<IRTreeCursor> openRangeCursor(
            const MBR& bbox) = 0;

    /**
     * @brief Open a k-nearest-neighbour cursor.
     *
     * Results are ordered ascending by Haversine distance from the centroid
     * of @p query_point to the centroid of each candidate's MBR.
     *
     * @param query_point  The reference point (WGS-84 lon/lat degrees).
     * @param k            Maximum number of results to return.
     * @return             A new cursor positioned before the first result.
     */
    [[nodiscard]] virtual std::unique_ptr<IRTreeCursor> openKNNCursor(
            const Coordinate& query_point, std::size_t k) = 0;

    /// @return The number of entries in the index.
    [[nodiscard]] virtual std::size_t size() const noexcept = 0;

    /**
     * @brief Insert a geometry entry.
     *
     * This invalidates all open cursors; subsequent calls to `next()` on
     * previously opened cursors return `CursorStatus::STALE`.
     */
    virtual void insert(const std::string& key, const GeometryInfo& geom) = 0;

    /**
     * @brief Bulk-load the index from a collection of (key, geometry) pairs.
     *
     * Replaces all existing content.  Invalidates all open cursors.
     */
    virtual void bulkLoad(
        const std::vector<std::pair<std::string, GeometryInfo>>& entries) = 0;

    /// Remove all entries.  Invalidates all open cursors.
    virtual void clear() = 0;
};

// ---------------------------------------------------------------------------
// GeoRTreeIndex — concrete implementation wrapping GeoRTree
// ---------------------------------------------------------------------------

/**
 * @brief Concrete `IGeoIndex` backed by `GeoRTree`.
 *
 * Maintains a monotonically increasing `version_` counter.  Each cursor
 * records the version at open time; if `version_` changes before `next()` is
 * called the cursor returns `CursorStatus::STALE`.
 */
class GeoRTreeIndex final : public IGeoIndex {
public:
    GeoRTreeIndex();
    ~GeoRTreeIndex() override;

    // Non-copyable, movable
    GeoRTreeIndex(const GeoRTreeIndex&)            = delete;
    GeoRTreeIndex& operator=(const GeoRTreeIndex&) = delete;
    GeoRTreeIndex(GeoRTreeIndex&&) noexcept;
    GeoRTreeIndex& operator=(GeoRTreeIndex&&) noexcept;

    [[nodiscard]] std::unique_ptr<IRTreeCursor> openRangeCursor(
            const MBR& bbox) override;

    [[nodiscard]] std::unique_ptr<IRTreeCursor> openKNNCursor(
            const Coordinate& query_point, std::size_t k) override;

    [[nodiscard]] std::size_t size() const noexcept override;

    void insert(const std::string& key, const GeometryInfo& geom) override;

    void bulkLoad(
        const std::vector<std::pair<std::string, GeometryInfo>>& entries) override;

    void clear() override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace geo
} // namespace themis
