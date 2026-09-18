#pragma once

/**
 * @file rtree_cursor.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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

enum class CursorStatus {
    OK,    ///< Entry written; caller may call `next()` again.
    END,   ///< No more entries; cursor is exhausted.
    STALE, ///< Underlying index was mutated; cursor result is undefined.
};

// ---------------------------------------------------------------------------
// GeoIndexEntry
// ---------------------------------------------------------------------------

struct GeoIndexEntry {
    std::string  key;        ///< User-supplied key of the indexed geometry.
    GeometryInfo geom;       ///< Geometry (as stored in the index).
    double       distance_m; ///< Centroid-to-centroid distance (k-NN only).
};

// ---------------------------------------------------------------------------
// IRTreeCursor
// ---------------------------------------------------------------------------

class IRTreeCursor {
public:
    /**
     * @brief IRTree Cursor.
     * @return Return value.
     */
    virtual ~IRTreeCursor() = default;

    [[nodiscard]] virtual CursorStatus next(GeoIndexEntry& entry) = 0;

    [[nodiscard]] virtual std::size_t estimatedResultCount() const noexcept = 0;
};

// ---------------------------------------------------------------------------
// IGeoIndex
// ---------------------------------------------------------------------------

class IGeoIndex {
public:
    /**
     * @brief IGeo Index.
     * @return Return value.
     */
    virtual ~IGeoIndex() = default;

    [[nodiscard]] virtual std::unique_ptr<IRTreeCursor> openRangeCursor(
            const MBR& bbox) = 0;

    [[nodiscard]] virtual std::unique_ptr<IRTreeCursor> openKNNCursor(
            const Coordinate& query_point, std::size_t k) = 0;

    [[nodiscard]] virtual std::size_t size() const noexcept = 0;

    /**
     * @brief Insert.
     * @param[in] key Input parameter.
     * @param[in] geom Input parameter.
     */
    virtual void insert(const std::string& key, const GeometryInfo& geom) = 0;

    virtual void bulkLoad(
        const std::vector<std::pair<std::string, GeometryInfo>>& entries) = 0;

    /**
     * @brief Clear.
     */
    virtual void clear() = 0;
};

// ---------------------------------------------------------------------------
// GeoRTreeIndex — concrete implementation wrapping GeoRTree
// ---------------------------------------------------------------------------

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
