/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            geo_rtree.cpp                                      ║
  Version:         0.0.16                                             ║
  Last Modified:   2026-04-15 07:11:55                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     251                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "geo/geo_rtree.h"
#include "utils/logger.h"

#include <algorithm>
#include <stdexcept>

// ── Boost.Geometry R-tree backend ─────────────────────────────────────────
#ifdef THEMIS_GEO_BOOST_BACKEND
#  if __has_include(<boost/geometry/index/rtree.hpp>)
#    include <boost/geometry/geometries/box.hpp>
#    include <boost/geometry/geometries/point_xy.hpp>
#    include <boost/geometry/index/rtree.hpp>
#    define THEMIS_RTREE_BOOST 1
#  else
#    define THEMIS_RTREE_BOOST 0
#  endif
#else
#  define THEMIS_RTREE_BOOST 0
#endif

namespace themis {
namespace geo {

// ──────────────────────────────────────────────────────────────────────────
// Pimpl implementation — two variants selected at compile time
// ──────────────────────────────────────────────────────────────────────────

#if THEMIS_RTREE_BOOST

namespace bg  = boost::geometry;
namespace bgi = boost::geometry::index;

using BgPoint = bg::model::d2::point_xy<double>;
using BgBox   = bg::model::box<BgPoint>;

// Each node stores (bounding-box, key-string)
using RtreeValue = std::pair<BgBox, std::string>;
using RtreeType  = bgi::rtree<RtreeValue, bgi::rstar<16>>;

/// Convert an MBR to a Boost.Geometry box
static BgBox toBox(const MBR& mbr) {
    return BgBox(BgPoint(mbr.minx, mbr.miny), BgPoint(mbr.maxx, mbr.maxy));
}

/// Build an MBR-based bounding box for a geometry (degrades to point for Point)
static BgBox geometryBox(const GeometryInfo& geom) {
    MBR mbr = geom.computeMBR();
    // For a 2D point, computeMBR returns minx==maxx, miny==maxy — that's fine.
    return toBox(mbr);
}

struct GeoRTree::Impl {
    RtreeType tree;

    void bulkLoad(const std::vector<std::pair<std::string, GeometryInfo>>& entries) {
        // Build value range for bulk constructor (STR packing)
        std::vector<RtreeValue> values;
        values.reserve(entries.size());
        for (const auto& [key, geom] : entries) {
            values.emplace_back(geometryBox(geom), key);
        }
        // Reconstruct with bulk-insert constructor for STR packing
        tree = RtreeType(values.begin(), values.end());
    }

    void insert(const std::string& key, const GeometryInfo& geom) {
        tree.insert(RtreeValue(geometryBox(geom), key));
    }

    bool remove(const std::string& key, const GeometryInfo& geom) {
        return tree.remove(RtreeValue(geometryBox(geom), key)) > 0;
    }

    void clear() { tree.clear(); }

    std::size_t size() const { return tree.size(); }

    std::vector<std::string> intersects(const MBR& query_bbox) const {
        BgBox qbox = toBox(query_bbox);
        std::vector<std::string> result;
        for (auto it = tree.qbegin(bgi::intersects(qbox)); it != tree.qend(); ++it) {
            result.push_back(it->second);
        }
        return result;
    }

    std::vector<std::string> contains(double x, double y) const {
        // A geometry "contains" a point when the point lies within its MBR.
        // We query with a degenerate box (point) and check containment.
        BgPoint qpt(x, y);
        BgBox   qbox(qpt, qpt);  // zero-area box at the query point
        std::vector<std::string> result;
        for (auto it = tree.qbegin(bgi::intersects(qbox)); it != tree.qend(); ++it) {
            // The candidate MBR intersects the point; now verify containment.
            const BgBox& b = it->first;
            if (b.min_corner().x() <= x && x <= b.max_corner().x() &&
                b.min_corner().y() <= y && y <= b.max_corner().y()) {
                result.push_back(it->second);
            }
        }
        return result;
    }

    // Conservative memory estimate: each node ≈ 2 doubles × 2 corners + key
    std::size_t memoryBytes() const {
        constexpr std::size_t kNodeOverhead = 4 * sizeof(double) + 32;  // box + key avg
        return size() * kNodeOverhead + sizeof(RtreeType);
    }
};

#else  // ── Linear-scan fallback (no Boost) ──────────────────────────────

struct Entry {
    MBR     mbr;
    std::string key;
};

struct GeoRTree::Impl {
    std::vector<Entry> entries;

    void bulkLoad(const std::vector<std::pair<std::string, GeometryInfo>>& in) {
        entries.clear();
        entries.reserve(in.size());
        for (const auto& [key, geom] : in) {
            entries.push_back({geom.computeMBR(), key});
        }
    }

    void insert(const std::string& key, const GeometryInfo& geom) {
        entries.push_back({geom.computeMBR(), key});
    }

    bool remove(const std::string& key, const GeometryInfo& geom) {
        MBR mbr = geom.computeMBR();
        for (auto it = entries.begin(); it != entries.end(); ++it) {
            if (it->key == key &&
                it->mbr.minx == mbr.minx && it->mbr.maxx == mbr.maxx &&
                it->mbr.miny == mbr.miny && it->mbr.maxy == mbr.maxy) {
                entries.erase(it);
                return true;
            }
        }
        return false;
    }

    void clear() { entries.clear(); }

    std::size_t size() const { return entries.size(); }

    std::vector<std::string> intersects(const MBR& query_bbox) const {
        std::vector<std::string> result;
        for (const auto& e : entries) {
            if (e.mbr.intersects(query_bbox)) {
                result.push_back(e.key);
            }
        }
        return result;
    }

    std::vector<std::string> contains(double x, double y) const {
        std::vector<std::string> result;
        for (const auto& e : entries) {
            if (e.mbr.contains(x, y)) {
                result.push_back(e.key);
            }
        }
        return result;
    }

    std::size_t memoryBytes() const {
        constexpr std::size_t kEntrySize = sizeof(Entry);
        return entries.capacity() * kEntrySize + sizeof(*this);
    }
};

#endif  // THEMIS_RTREE_BOOST

// ──────────────────────────────────────────────────────────────────────────
// GeoRTree public API
// ──────────────────────────────────────────────────────────────────────────

GeoRTree::GeoRTree()  : impl_(std::make_unique<Impl>()) {}
GeoRTree::~GeoRTree() = default;

GeoRTree::GeoRTree(GeoRTree&&) noexcept            = default;
GeoRTree& GeoRTree::operator=(GeoRTree&&) noexcept = default;

void GeoRTree::bulkLoad(const std::vector<std::pair<std::string, GeometryInfo>>& entries) {
    impl_->bulkLoad(entries);
    THEMIS_INFO("GeoRTree::bulkLoad completed: entries={}, geo_index_bytes_allocated={}",
                impl_->size(), impl_->memoryBytes());
}

void GeoRTree::insert(const std::string& key, const GeometryInfo& geom) {
    impl_->insert(key, geom);
    THEMIS_INFO("GeoRTree::insert: key={}, geo_index_bytes_allocated={}",
                key, impl_->memoryBytes());
}

bool GeoRTree::remove(const std::string& key, const GeometryInfo& geom) {
    bool removed = impl_->remove(key, geom);
    if (!removed) {
        THEMIS_WARN("GeoRTree::remove: key='{}' not found in index", key);
    }
    return removed;
}

void GeoRTree::clear() {
    impl_->clear();
}

std::vector<std::string> GeoRTree::intersects(const MBR& query_bbox) const {
    return impl_->intersects(query_bbox);
}

std::vector<std::string> GeoRTree::contains(double x, double y) const {
    return impl_->contains(x, y);
}

std::size_t GeoRTree::size() const {
    return impl_->size();
}

std::size_t GeoRTree::memoryBytes() const {
    return impl_->memoryBytes();
}

}  // namespace geo
}  // namespace themis
