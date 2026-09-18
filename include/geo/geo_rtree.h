/**
 * @file geo_rtree.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.18
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

    void bulkLoad(const std::vector<std::pair<std::string, GeometryInfo>>& entries);

    /**
     * @brief Insert.
     * @param[in] key Input parameter.
     * @param[in] geom Input parameter.
     */
    void insert(const std::string& key, const GeometryInfo& geom);

    /**
     * @brief Remove.
     * @param[in] key Input parameter.
     * @param[in] geom Input parameter.
     * @return True when the operation succeeds.
     */
    bool remove(const std::string& key, const GeometryInfo& geom);

    /**
     * @brief Clear.
     */
    void clear();


    /**
     * @brief Intersects.
     * @param[in] query_bbox Input parameter.
     * @return Return value.
     */
    std::vector<std::string> intersects(const MBR& query_bbox) const;

    /**
     * @brief Contains.
     * @param[in] x Input parameter.
     * @param[in] y Input parameter.
     * @return Return value.
     */
    std::vector<std::string> contains(double x, double y) const;


    /**
     * @brief Size.
     * @return Return value.
     */
    std::size_t size() const;

    /**
     * @brief Memory Bytes.
     * @return Return value.
     */
    std::size_t memoryBytes() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace geo
}  // namespace themis
