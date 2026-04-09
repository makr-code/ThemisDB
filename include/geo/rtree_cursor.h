/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            rtree_cursor.h                                     ║
  Version:         1.0.0                                              ║
  Last Modified:   2026-04-09                                         ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "utils/geo/ewkb.h"

#include <cstddef>
#include <string>

namespace themis {
namespace geo {

/// Entry returned when iterating through an R-tree cursor.
/// Full definition lives in geo_index.h; forward-declared here to avoid
/// a circular include dependency (geo_index.h includes rtree_cursor.h).
struct GeoIndexEntry {
    std::string key;
    GeometryInfo geom;
};

/// Status values returned by IRTreeCursor::next().
enum class CursorStatus {
    /// A new entry was written into the output parameter.
    OK,
    /// The cursor is exhausted; no more entries.
    END,
    /// The underlying index was mutated while the cursor was open.
    /// The cursor must be discarded and re-opened.
    STALE,
};

/**
 * @brief Pull-based, single-threaded cursor over a spatial index result set.
 *
 * A cursor is obtained via IGeoIndex::openRangeCursor() or
 * IGeoIndex::openKNNCursor().  It is single-use and non-copyable.
 *
 * **Usage:**
 * @code
 *   GeoIndexEntry entry;
 *   auto cursor = index.openRangeCursor(bbox);
 *   while (cursor->next(entry) == CursorStatus::OK) {
 *       // process entry ...
 *   }
 * @endcode
 *
 * **Thread safety**: not thread-safe; one cursor per thread.
 *
 * **Staleness**: if the underlying index is mutated (insert / remove / clear)
 * while a cursor is open, subsequent calls to `next()` return
 * CursorStatus::STALE.  The cursor must then be discarded and a new one
 * opened to resume iteration.
 */
class IRTreeCursor {
public:
    virtual ~IRTreeCursor() = default;

    // Non-copyable
    IRTreeCursor(const IRTreeCursor&) = delete;
    IRTreeCursor& operator=(const IRTreeCursor&) = delete;

    /**
     * @brief Advance the cursor and write the next entry into @p out.
     *
     * @param out  Output parameter; filled with the next (key, geom) entry
     *             when CursorStatus::OK is returned.
     * @return CursorStatus::OK    — entry written, call again for more.
     * @return CursorStatus::END   — iteration complete; @p out is unchanged.
     * @return CursorStatus::STALE — index was mutated; cursor is invalid.
     */
    virtual CursorStatus next(GeoIndexEntry& out) = 0;

    /**
     * @brief Return an estimate of the number of results that will be yielded.
     *
     * This is a hint for query planning; the actual count may differ.
     * Returns 0 when no estimate is available.
     */
    virtual std::size_t estimatedResultCount() const noexcept = 0;

    /**
     * @brief Return true once the cursor is exhausted or stale.
     */
    virtual bool done() const noexcept = 0;

protected:
    IRTreeCursor() = default;
};

} // namespace geo
} // namespace themis
