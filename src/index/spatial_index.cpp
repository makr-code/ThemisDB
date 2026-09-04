/**
 * @file spatial_index.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=9, M=17, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "index/spatial_index.h"
#include "index/connection_guard.h"  // Phase 3 A-6: Connection leak prevention
#include <stdexcept>
#include "utils/logger.h"
#include "utils/geometric_distances.h"
#include <cmath>
#include <algorithm>
#include <cstdio>
#include <nlohmann/json.hpp>

namespace themis {
namespace index {

using json = nlohmann::json;

// Constants
constexpr double Z_BUCKET_SIZE = 10.0;  // 10 meter buckets for elevation

// ===== Morton Encoder Implementation =====

// Interleave bits for 2D Morton code
uint64_t MortonEncoder::interleaveBits2D(uint32_t x, uint32_t y) {
    uint64_t result = 0;
    for (int i = 0; i < 32; ++i) {
        result |= ((x & (1ULL << i)) << i) | ((y & (1ULL << i)) << (i + 1));
    }
    return result;
}

// Interleave bits for 3D Morton code (21 bits each)
uint64_t MortonEncoder::interleaveBits3D(uint32_t x, uint32_t y, uint32_t z) {
    uint64_t result = 0;
    for (int i = 0; i < 21; ++i) {
        result |= ((x & (1ULL << i)) << (2 * i)) |
                  ((y & (1ULL << i)) << (2 * i + 1)) |
                  ((z & (1ULL << i)) << (2 * i + 2));
    }
    return result;
}

// Normalize coordinate to [0, 2^32-1]
uint32_t MortonEncoder::normalizeCoord(double coord, double min_val, double max_val) {
    if (max_val <= min_val) return 0;
    
    double normalized = (coord - min_val) / (max_val - min_val);
    normalized = std::clamp(normalized, 0.0, 1.0);
    
    return static_cast<uint32_t>(normalized * 0xFFFFFFFFULL);
}

// Encode 2D to Morton code
uint64_t MortonEncoder::encode2D(double x, double y, const geo::MBR& bounds) {
    uint32_t nx = normalizeCoord(x, bounds.minx, bounds.maxx);
    uint32_t ny = normalizeCoord(y, bounds.miny, bounds.maxy);
    return interleaveBits2D(nx, ny);
}

// Encode 3D to Morton code
uint64_t MortonEncoder::encode3D(double x, double y, double z, const geo::MBR& bounds) {
    // Use 21 bits per dimension for 3D
    uint32_t nx = normalizeCoord(x, bounds.minx, bounds.maxx) >> 11;
    uint32_t ny = normalizeCoord(y, bounds.miny, bounds.maxy) >> 11;
    
    double z_min = bounds.z_min.value_or(0.0);
    double z_max = bounds.z_max.value_or(1000.0);
    uint32_t nz = normalizeCoord(z, z_min, z_max) >> 11;
    
    return interleaveBits3D(nx, ny, nz);
}

// Decode 2D Morton code
std::pair<double, double> MortonEncoder::decode2D(uint64_t code, const geo::MBR& bounds) {
    uint32_t x = 0, y = 0;
    for (int i = 0; i < 32; ++i) {
        x |= ((code >> (2 * i)) & 1) << i;
        y |= ((code >> (2 * i + 1)) & 1) << i;
    }
    
    double dx = static_cast<double>(x) / 0xFFFFFFFFULL;
    double dy = static_cast<double>(y) / 0xFFFFFFFFULL;
    
    double real_x = bounds.minx + dx * (bounds.maxx - bounds.minx);
    double real_y = bounds.miny + dy * (bounds.maxy - bounds.miny);
    
    return {real_x, real_y};
}

// Get Morton ranges for MBR query using quadtree-style range decomposition.
//
// The 2D space is encoded as a 64-bit Morton (Z-order) code where x occupies
// even-numbered bits and y occupies odd-numbered bits.  A power-of-2-aligned
// quadtree cell at depth d forms a *contiguous* Morton range because all points
// in the cell share the same upper 2d prefix bits — the lower 2*(32-d) bits
// vary freely, so the range is [prefix<<free_bits, prefix<<free_bits | mask].
//
// The algorithm uses an explicit stack of quadtree nodes.  For each node:
//   • No overlap with query bbox  → skip
//   • Fully contained in query    → emit range (no false positives from this node)
//   • Partially overlapping       → subdivide into 4 children (or emit as superset
//                                   if the output budget is exhausted)
//
// The calling layer already applies a geometric post-filter, so emitting superset
// ranges (false positives) is correct — it only affects query performance, not
// correctness.  False negatives would be a bug; the algorithm never skips an
// overlapping node.
std::vector<std::pair<uint64_t, uint64_t>> MortonEncoder::getRanges(
    const geo::MBR& query_bbox,
    const geo::MBR& total_bounds,
    int max_ranges)
{
    if (max_ranges <= 0) max_ranges = 8;

    // Clip query to the declared total bounds and normalize to [0, 2^32-1].
    uint32_t qx_lo = normalizeCoord(
        std::max(query_bbox.minx, total_bounds.minx), total_bounds.minx, total_bounds.maxx);
    uint32_t qx_hi = normalizeCoord(
        std::min(query_bbox.maxx, total_bounds.maxx), total_bounds.minx, total_bounds.maxx);
    uint32_t qy_lo = normalizeCoord(
        std::max(query_bbox.miny, total_bounds.miny), total_bounds.miny, total_bounds.maxy);
    uint32_t qy_hi = normalizeCoord(
        std::min(query_bbox.maxy, total_bounds.maxy), total_bounds.miny, total_bounds.maxy);

    if (qx_lo > qx_hi || qy_lo > qy_hi) {
        THEMIS_DEBUG("MortonEncoder::getRanges - clipped query bbox is empty after normalization");
        return {};
    }

    // Quadtree node: x0/y0 are the inclusive low corners; `bits` is the number
    // of free bits per dimension (32 = root covering [0, 2^32-1], 0 = leaf).
    struct QNode { uint32_t x0, y0; int bits; };

    std::vector<std::pair<uint64_t, uint64_t>> ranges;
    std::vector<QNode> stack = {{0u, 0u, 32}};  // root covers the full space

    while (!stack.empty()) {
        QNode n = stack.back();
        stack.pop_back();

        // Inclusive high corner of this node.
        uint32_t x1 = (n.bits < 32) ? n.x0 + (1u << n.bits) - 1u : 0xFFFFFFFFu;
        uint32_t y1 = (n.bits < 32) ? n.y0 + (1u << n.bits) - 1u : 0xFFFFFFFFu;

        // Skip nodes that do not overlap the query bbox.
        if (x1 < qx_lo || n.x0 > qx_hi || y1 < qy_lo || n.y0 > qy_hi) continue;

        // Morton range for this power-of-2-aligned quadtree node.
        // lo = code of (x0, y0) with all free bits set to 0.
        // hi = lo | mask, where mask fills all 2*bits free low bits with 1.
        uint64_t lo = interleaveBits2D(n.x0, n.y0);
        uint64_t hi;
        if (n.bits == 0) {
            hi = lo;
        } else {
            const uint64_t free_bits = 2u * static_cast<uint64_t>(n.bits);
            const uint64_t mask = (free_bits < 64u) ? (1ULL << free_bits) - 1u : ~0ULL;
            hi = lo | mask;
        }

        bool fully_inside = (n.x0 >= qx_lo && x1 <= qx_hi &&
                             n.y0 >= qy_lo && y1 <= qy_hi);
        bool budget_full  = (static_cast<int>(ranges.size()) >= max_ranges - 1);

        if (fully_inside || n.bits == 0 || budget_full) {
            // Emit this range.  Merge with the previous entry when adjacent to
            // reduce the output size.
            if (!ranges.empty() && ranges.back().second + 1u == lo) {
                ranges.back().second = hi;
            } else {
                ranges.emplace_back(lo, hi);
            }
        } else {
            // Subdivide into four children (SW, SE, NW, NE).
            // Push in reverse processing order so SW is popped/processed first,
            // keeping output ranges sorted in ascending Morton-code order.
            const int cb   = n.bits - 1;
            const uint32_t half = (cb < 32) ? (1u << cb) : 0x80000000u;
            stack.push_back({n.x0 + half, n.y0 + half, cb});  // NE
            stack.push_back({n.x0,        n.y0 + half, cb});  // NW
            stack.push_back({n.x0 + half, n.y0,        cb});  // SE
            stack.push_back({n.x0,        n.y0,        cb});  // SW
        }
    }

    if (ranges.empty()) {
        // Safety fallback: return a single superset range covering the query.
        ranges.emplace_back(interleaveBits2D(qx_lo, qy_lo),
                            interleaveBits2D(qx_hi, qy_hi));
    }

    return ranges;
}

// ===== SpatialIndexManager Implementation =====

SpatialIndexManager::SpatialIndexManager(RocksDBWrapper& db)
    : db_(db) {}

// Key prefixes
std::string SpatialIndexManager::getSpatialKeyPrefix(std::string_view table) const {
    return std::string("spatial:") + std::string(table) + ":";
}

std::string SpatialIndexManager::getZRangeKeyPrefix(std::string_view table) const {
    return std::string("zrange:") + std::string(table) + ":";
}

std::string SpatialIndexManager::getConfigKey(std::string_view table) const {
    return std::string("spatial_config:") + std::string(table);
}

std::string SpatialIndexManager::makeSpatialKey(std::string_view table, uint64_t morton_code) const {
    char buf[32];
    snprintf(buf, sizeof(buf), "%016llx", static_cast<unsigned long long>(morton_code));
    return getSpatialKeyPrefix(table) + buf;
}

std::string SpatialIndexManager::makeZRangeKey(std::string_view table, int z_bucket) const {
    char buf[16];
    snprintf(buf, sizeof(buf), "%08d", z_bucket);
    return getZRangeKeyPrefix(table) + buf;
}

// Helper: Make per-PK sidecar key
// Format: spatial:<table>::pk:<morton_code>:<pk>
// This allows individual PK updates without rewriting entire bucket JSON
std::string SpatialIndexManager::makeSpatialPerPKKey(
    std::string_view table,
    uint64_t morton_code,
    std::string_view pk
) const {
    char buf[32];
    snprintf(buf, sizeof(buf), "%016llx", static_cast<unsigned long long>(morton_code));
    return getSpatialKeyPrefix(table) + "pk:" + buf + ":" + std::string(pk);
}

// ── R-tree helpers ────────────────────────────────────────────────────────

/// Convert an MBR to a minimal Polygon GeometryInfo for GeoRTree indexing.
/// The polygon's computeMBR() returns the same MBR, ensuring stable insert/remove.
geo::GeometryInfo SpatialIndexManager::mbrToGeometryInfo(const geo::MBR& mbr) {
    geo::GeometryInfo g(geo::GeometryType::Polygon);
    g.rings.push_back({
        {mbr.minx, mbr.miny}, {mbr.maxx, mbr.miny},
        {mbr.maxx, mbr.maxy}, {mbr.minx, mbr.maxy},
        {mbr.minx, mbr.miny}  // close ring
    });
    return g;
}

/// Lazily build the in-memory R-tree for `table` from per-PK RocksDB keys.
/// No-op if the R-tree has already been initialised for this table.
/// Falls back gracefully when per-PK keys are absent (e.g. very old data);
/// in that case the table's R-tree remains empty and searchIntersects uses
/// the Morton code scan instead.
void SpatialIndexManager::ensureRTree(std::string_view table) const {
    std::string table_str(table);

    // Fast path: already built — check with a shared (read) lock.
    {
        // LOCK: Tier 1 (Global R-tree protection, read-only) — Phase 3 A-5
        std::shared_lock<std::shared_mutex> rlock(rtree_mutex_);
        if (rtree_built_.count(table_str)) return;
    }

    // Slow path: acquire exclusive write lock and build.
    // LOCK: Tier 1 (Global R-tree protection) — Phase 3 A-5
    std::unique_lock<std::shared_mutex> lock(rtree_mutex_);
    // Double-check after acquiring write lock to avoid redundant work.
    if (rtree_built_.count(table_str)) return;

    // Mark as built before populating to block concurrent callers.
    rtree_built_.insert(table_str);

    // Scan all per-PK spatial keys for this table.
    // Key format: spatial:<table>:pk:<16hex>:<primary_key>
    // Value format: JSON {"mbr":{"minx":...,"miny":...,"maxx":...,"maxy":...}}
    const std::string pk_prefix = getSpatialKeyPrefix(table) + "pk:";
    constexpr std::size_t kMortonChars = 16; // 64-bit Morton code: 16 hex characters
    const std::size_t pk_strip = pk_prefix.size() + kMortonChars + 1; // +1 for ':'

    std::vector<std::pair<std::string, geo::GeometryInfo>> bulk_entries;
    auto& cache = mbr_cache_[table_str];

    db_.scanRange(pk_prefix, pk_prefix + "~",
        [&](std::string_view k, std::string_view v) {
            if (k.size() <= pk_strip) {
                THEMIS_WARN("SpatialIndexManager::ensureRTree: malformed per-PK key "
                            "for table='{}' (len={})", table_str, k.size());
                return true;
            }
            std::string pk(k.substr(pk_strip));
            try {
                auto j = json::parse(std::string(v));
                const auto& mbr_j = j.at("mbr");
                geo::MBR mbr;
                mbr.minx = mbr_j.at("minx").get<double>();
                mbr.miny = mbr_j.at("miny").get<double>();
                mbr.maxx = mbr_j.at("maxx").get<double>();
                mbr.maxy = mbr_j.at("maxy").get<double>();
                cache[pk] = mbr;
                bulk_entries.emplace_back(pk, mbrToGeometryInfo(mbr));
            } catch (const std::exception& ex) {
                THEMIS_WARN("SpatialIndexManager::ensureRTree: failed to parse "
                            "sidecar for pk='{}' in table='{}': {}",
                            pk, table_str, ex.what());
            } catch (...) {
                THEMIS_WARN("SpatialIndexManager::ensureRTree: unknown error "
                            "parsing sidecar for pk='{}' in table='{}'",
                            pk, table_str);
            }
            return true;
        });

    if (!bulk_entries.empty()) {
        rtrees_[table_str].bulkLoad(bulk_entries);
        THEMIS_INFO("SpatialIndexManager: R-tree built for table='{}': "
                    "entries={}, geo_index_bytes_allocated={}",
                    table_str, rtrees_[table_str].size(),
                    rtrees_[table_str].memoryBytes());
    }
}

// Config persistence
std::optional<RTreeConfig> SpatialIndexManager::getConfig(std::string_view table) const {
    auto value = db_.get(getConfigKey(table));
    if (!value) return std::nullopt;
    
    try {
        std::string s(reinterpret_cast<const char*>(value->data()), value->size());
        auto j = json::parse(s);
        RTreeConfig config;
        config.max_entries_per_node = j.value("max_entries", 16);
        config.use_3d = j.value("use_3d", false);
        
        if (j.contains("total_bounds")) {
            try {
                const auto& b = j.at("total_bounds");
                config.total_bounds.minx = b.value("minx", -180.0);
                config.total_bounds.miny = b.value("miny", -90.0);
                config.total_bounds.maxx = b.value("maxx", 180.0);
                config.total_bounds.maxy = b.value("maxy", 90.0);
            } catch (...) {
                // Use defaults if parsing fails
                config.total_bounds.minx = -180.0;
                config.total_bounds.miny = -90.0;
                config.total_bounds.maxx = 180.0;
                config.total_bounds.maxy = 90.0;
            }
        }
        
        return config;
    } catch (...) {
        return std::nullopt;
    }
}

SpatialIndexManager::Status SpatialIndexManager::saveConfig(std::string_view table, const RTreeConfig& config) {
    json j;
    j["max_entries"] = config.max_entries_per_node;
    j["use_3d"] = config.use_3d;
    j["total_bounds"] = {
        {"minx", config.total_bounds.minx},
        {"miny", config.total_bounds.miny},
        {"maxx", config.total_bounds.maxx},
        {"maxy", config.total_bounds.maxy}
    };
    
    const auto dump = j.dump();
    std::vector<uint8_t> bytes(dump.begin(), dump.end());
    return db_.put(getConfigKey(table), bytes) ? Status::OK() : Status::Error("failed to save config");
}

// Create spatial index
SpatialIndexManager::Status SpatialIndexManager::createSpatialIndex(
    std::string_view table,
    [[maybe_unused]] std::string_view geometry_column,
    const RTreeConfig& config
) {
    // unused parameter
    // Save config
    RTreeConfig cfg = config;
    if (cfg.total_bounds.minx == 0.0 && cfg.total_bounds.maxx == 0.0) {
        // Default: global lat/lon bounds
        cfg.total_bounds = geo::MBR(-180.0, -90.0, 180.0, 90.0);
    }

    // Invalidate any stale in-memory R-tree state for this table so that a
    // subsequent ensureRTree() rebuilds cleanly from the new (empty) index.
    std::string table_str(table);
    {
        // LOCK: Tier 1 (Global R-tree protection) — Phase 3 A-5
        std::unique_lock<std::shared_mutex> lock(rtree_mutex_);
        rtrees_.erase(table_str);
        mbr_cache_.erase(table_str);
        rtree_built_.erase(table_str);
    }

    return saveConfig(table, cfg);
}

// Drop spatial index
SpatialIndexManager::Status SpatialIndexManager::dropSpatialIndex(std::string_view table) {
    // Delete config
    db_.del(getConfigKey(table));
    
    // Delete all spatial keys (prefix scan + delete)
    std::string prefix = getSpatialKeyPrefix(table);
    db_.scanRange(prefix, prefix + "~", [this](std::string_view key, std::string_view /*value*/){
        db_.del(key);
        return true;
    });

    // Clear in-memory R-tree state so stale entries are not returned if the
    // index is recreated later.
    std::string table_str(table);
    {
        // LOCK: Tier 1 (Global R-tree protection) — Phase 3 A-5
        std::unique_lock<std::shared_mutex> lock(rtree_mutex_);
        rtrees_.erase(table_str);
        mbr_cache_.erase(table_str);
        rtree_built_.erase(table_str);
    }

    return Status::OK();
}

bool SpatialIndexManager::hasSpatialIndex(std::string_view table) const {
    return getConfig(table).has_value();
}

// Parse sidecar list
std::vector<SpatialIndexManager::SidecarEntry> SpatialIndexManager::parseSidecarList(
    const std::string& value
) const {
    std::vector<SidecarEntry> result;
    
    try {
        auto j = json::parse(value);
        for (const auto& item : j) {
            SidecarEntry entry;
            entry.primary_key = item.value("pk", std::string());
            
            if (item.contains("mbr") && item["mbr"].is_object()) {
                const auto& mbr = item["mbr"];
                entry.sidecar.mbr.minx = mbr.value("minx", 0.0);
                entry.sidecar.mbr.miny = mbr.value("miny", 0.0);
                entry.sidecar.mbr.maxx = mbr.value("maxx", 0.0);
                entry.sidecar.mbr.maxy = mbr.value("maxy", 0.0);
            }
            
            if (item.contains("z_min")) {
                entry.sidecar.z_min = item.value("z_min", 0.0);
                entry.sidecar.z_max = item.value("z_max", 0.0);
            }
            
            result.push_back(entry);
        }
    } catch (...) {
        // Parsing error
    }
    
    return result;
}

std::string SpatialIndexManager::serializeSidecarList(
    const std::vector<SidecarEntry>& entries
) const {
    json j = json::array();
    
    for (const auto& entry : entries) {
        json item;
        item["pk"] = entry.primary_key;
        item["mbr"] = {
            {"minx", entry.sidecar.mbr.minx},
            {"miny", entry.sidecar.mbr.miny},
            {"maxx", entry.sidecar.mbr.maxx},
            {"maxy", entry.sidecar.mbr.maxy}
        };
        
        if (entry.sidecar.z_min != 0.0 || entry.sidecar.z_max != 0.0) {
            item["z_min"] = entry.sidecar.z_min;
            item["z_max"] = entry.sidecar.z_max;
        }
        
        j.push_back(item);
    }
    
    return j.dump();
}

// Bulk-load
SpatialIndexManager::Status SpatialIndexManager::bulkLoad(
    std::string_view table,
    const std::vector<std::pair<std::string, geo::GeoSidecar>>& entries
) {
    auto config = getConfig(table);
    if (!config) {
        return Status::Error("Spatial index not found for table: " + std::string(table));
    }

    std::string table_str(table);

    // Purge all spatial keys (legacy Morton buckets + per-PK keys) so that
    // empty bulk-load fully clears query-visible state and restart rebuilds
    // cannot resurrect stale entries.
    {
        // LOCK: Tier 1 (Global R-tree protection) — Phase 3 A-5
        std::unique_lock<std::shared_mutex> lock(rtree_mutex_);
        rtrees_[table_str].clear();
        mbr_cache_[table_str].clear();
        rtree_built_.erase(table_str);
    }
    const std::string spatial_prefix = getSpatialKeyPrefix(table);
    db_.scanRange(spatial_prefix, spatial_prefix + "~",
        [this](std::string_view key, std::string_view /*value*/) {
            db_.del(key);
            return true;
        });

    // Build per-PK RocksDB entries and collect in-memory data.
    // RocksDB writes and local cache/rtree construction happen outside the
    // mutex to avoid holding the write lock during I/O.
    std::unordered_map<std::string, geo::MBR> local_cache;
    local_cache.reserve(entries.size());
    std::vector<std::pair<std::string, geo::GeometryInfo>> rtree_entries;
    rtree_entries.reserve(entries.size());

    for (const auto& [pk, sidecar] : entries) {
        // Persist per-PK sidecar key so lazy rebuild after restart can recover
        // the full collection without requiring another bulk load call.
        const uint64_t morton = MortonEncoder::encode2D(
            sidecar.centroid.x, sidecar.centroid.y, config->total_bounds);
        const std::string pk_key = makeSpatialPerPKKey(table, morton, pk);

        json pk_sidecar;
        pk_sidecar["mbr"] = {
            {"minx", sidecar.mbr.minx},
            {"miny", sidecar.mbr.miny},
            {"maxx", sidecar.mbr.maxx},
            {"maxy", sidecar.mbr.maxy}
        };
        if (sidecar.z_min != 0.0 || sidecar.z_max != 0.0) {
            pk_sidecar["z_min"] = sidecar.z_min;
            pk_sidecar["z_max"] = sidecar.z_max;
        }
        const auto dump = pk_sidecar.dump();
        const std::vector<uint8_t> bytes(dump.begin(), dump.end());
        db_.put(pk_key, bytes);  // Best effort; query path rebuilds from per-PK scan

        local_cache[pk] = sidecar.mbr;
        rtree_entries.emplace_back(pk, mbrToGeometryInfo(sidecar.mbr));

        metrics_.insert_count++;
    }

    // Atomically swap in the new in-memory state under the write lock.
    {
        // LOCK: Tier 1 (Global R-tree protection) — Phase 3 A-5
        std::unique_lock<std::shared_mutex> lock(rtree_mutex_);
        mbr_cache_[table_str] = std::move(local_cache);
        rtrees_[table_str].clear();
        if (!rtree_entries.empty()) {
            // STR bulk-load the R-tree (3–5× faster than incremental insert).
            rtrees_[table_str].bulkLoad(rtree_entries);
        }
        rtree_built_.insert(table_str);
    }

    {
        // LOCK: Tier 1 (Global R-tree protection, read-only) — Phase 3 A-5
        std::shared_lock<std::shared_mutex> rlock(rtree_mutex_);
        THEMIS_INFO("SpatialIndexManager::bulkLoad: table='{}', entries={}, "
                    "geo_index_bytes_allocated={}",
                    table_str, entries.size(),
                    rtrees_[table_str].memoryBytes());
    }

    return Status::OK();
}

// Insert
SpatialIndexManager::Status SpatialIndexManager::insert(
    std::string_view table,
    std::string_view primary_key,
    const geo::GeoSidecar& sidecar
) {
    // G5: Track insert metrics
    metrics_.insert_count++;
    
    auto config = getConfig(table);
    if (!config) {
        return Status::Error("Spatial index not found for table: " + std::string(table));
    }
    
    // Compute Morton code for centroid
    uint64_t morton = MortonEncoder::encode2D(
        sidecar.centroid.x,
        sidecar.centroid.y,
        config->total_bounds
    );
    
    std::string key = makeSpatialKey(table, morton);
    
    // Get existing entries for this Morton bucket
    auto value = db_.get(key);
    std::vector<SidecarEntry> entries;
    
    if (value) {
        std::string s(reinterpret_cast<const char*>(value->data()), value->size());
        entries = parseSidecarList(s);
    }
    
    // Add new entry
    SidecarEntry new_entry;
    new_entry.primary_key = std::string(primary_key);
    new_entry.sidecar = sidecar;
    entries.push_back(new_entry);
    
    // Save back to bucket (for backward compatibility)
    const auto dump = serializeSidecarList(entries);
    std::vector<uint8_t> bytes(dump.begin(), dump.end());
    if (!db_.put(key, bytes)) {
        return Status::Error("failed to insert");
    }
    
    // Storage improvement: Also write per-PK key
    // This allows updating/deleting individual PKs without rewriting entire bucket
    // Format: spatial:<table>::pk:<morton>:<pk> -> sidecar JSON
    std::string pk_key = makeSpatialPerPKKey(table, morton, primary_key);
    json pk_sidecar;
    pk_sidecar["mbr"] = {
        {"minx", sidecar.mbr.minx},
        {"miny", sidecar.mbr.miny},
        {"maxx", sidecar.mbr.maxx},
        {"maxy", sidecar.mbr.maxy}
    };
    if (sidecar.z_min != 0.0 || sidecar.z_max != 0.0) {
        pk_sidecar["z_min"] = sidecar.z_min;
        pk_sidecar["z_max"] = sidecar.z_max;
    }
    const auto pk_dump = pk_sidecar.dump();
    std::vector<uint8_t> pk_bytes(pk_dump.begin(), pk_dump.end());
    db_.put(pk_key, pk_bytes); // Best effort, don't fail on error

    // Update in-memory R-tree and MBR cache.
    std::string table_str(table);
    std::string pk_str(primary_key);
    {
        // LOCK: Tier 1 (Global R-tree protection) — Phase 3 A-5
        std::unique_lock<std::shared_mutex> lock(rtree_mutex_);
        mbr_cache_[table_str][pk_str] = sidecar.mbr;
        rtrees_[table_str].insert(pk_str, mbrToGeometryInfo(sidecar.mbr));
        // Mark the table's R-tree as built so ensureRTree won't overwrite our
        // incrementally maintained index on the first query.
        rtree_built_.insert(table_str);
    }

    return Status::OK();
}

// Insert with WriteBatch (atomic)
SpatialIndexManager::Status SpatialIndexManager::insertBatch(
    RocksDBWrapper::WriteBatchWrapper& batch,
    std::string_view table,
    std::string_view primary_key,
    const geo::GeoSidecar& sidecar
) {
    // G5: Track insert metrics
    metrics_.insert_count++;
    
    auto config = getConfig(table);
    if (!config) {
        return Status::Error("Spatial index not found for table: " + std::string(table));
    }
    
    // Compute Morton code for centroid
    uint64_t morton = MortonEncoder::encode2D(
        sidecar.centroid.x,
        sidecar.centroid.y,
        config->total_bounds
    );
    
    // Write per-PK key to WriteBatch. This allows updating/deleting
    // individual PKs without rewriting an entire bucket and avoids
    // concurrent write conflicts on a shared bucket value.
    // NOTE: We intentionally avoid a read-modify-write on the shared Morton
    // bucket key here, because concurrent inserts into the same bucket could
    // otherwise race and lose updates (last-writer-wins). Instead, we rely
    // on per-primary-key spatial index entries, which are independent keys
    // and can be safely written concurrently.
    std::string pk_key = makeSpatialPerPKKey(table, morton, primary_key);
    json pk_sidecar;
    pk_sidecar["mbr"] = {
        {"minx", sidecar.mbr.minx},
        {"miny", sidecar.mbr.miny},
        {"maxx", sidecar.mbr.maxx},
        {"maxy", sidecar.mbr.maxy}
    };
    if (sidecar.z_min != 0.0 || sidecar.z_max != 0.0) {
        pk_sidecar["z_min"] = sidecar.z_min;
        pk_sidecar["z_max"] = sidecar.z_max;
    }
    const auto pk_dump = pk_sidecar.dump();
    std::vector<uint8_t> pk_bytes(pk_dump.begin(), pk_dump.end());
    batch.put(pk_key, pk_bytes);

    // Update in-memory R-tree and MBR cache (R-tree is always in-memory;
    // updating it here keeps it consistent with the pending batch write).
    std::string table_str(table);
    std::string pk_str(primary_key);
    {
        // LOCK: Tier 1 (Global R-tree protection) — Phase 3 A-5
        std::unique_lock<std::shared_mutex> lock(rtree_mutex_);
        mbr_cache_[table_str][pk_str] = sidecar.mbr;
        rtrees_[table_str].insert(pk_str, mbrToGeometryInfo(sidecar.mbr));
        rtree_built_.insert(table_str);
    }

    return Status::OK();
}

// Remove with WriteBatch (atomic)
SpatialIndexManager::Status SpatialIndexManager::removeBatch(
    RocksDBWrapper::WriteBatchWrapper& batch,
    std::string_view table,
    std::string_view primary_key,
    const geo::GeoSidecar& sidecar
) {
    // G5: Track remove metrics
    metrics_.remove_count++;
    
    auto config = getConfig(table);
    if (!config) return Status::OK();
    
    uint64_t morton = MortonEncoder::encode2D(
        sidecar.centroid.x,
        sidecar.centroid.y,
        config->total_bounds
    );
    
    // Delete per-PK key from WriteBatch
    std::string pk_key = makeSpatialPerPKKey(table, morton, primary_key);
    batch.del(pk_key);

    // Backward compatibility: update legacy Morton bucket key as well.
    // This prevents stale candidates when search falls back to bucket scans.
    std::string bucket_key = makeSpatialKey(table, morton);
    if (auto bucket_value = db_.get(bucket_key)) {
        std::string s(reinterpret_cast<const char*>(bucket_value->data()), bucket_value->size());
        auto entries = parseSidecarList(s);
        entries.erase(
            std::remove_if(entries.begin(), entries.end(),
                [&](const SidecarEntry& e) { return e.primary_key == primary_key; }),
            entries.end());

        if (entries.empty()) {
            batch.del(bucket_key);
        } else {
            const auto dump = serializeSidecarList(entries);
            std::vector<uint8_t> bytes(dump.begin(), dump.end());
            batch.put(bucket_key, bytes);
        }
    }

    // Update in-memory R-tree and MBR cache.
    std::string table_str(table);
    std::string pk_str(primary_key);
    {
        // LOCK: Tier 1 (Global R-tree protection) — Phase 3 A-5
        std::unique_lock<std::shared_mutex> lock(rtree_mutex_);
        auto& cache = mbr_cache_[table_str];
        auto it = cache.find(pk_str);
        const geo::MBR mbr_to_remove = (it != cache.end()) ? it->second : sidecar.mbr;
        const bool removed = rtrees_[table_str].remove(pk_str, mbrToGeometryInfo(mbr_to_remove));
        if (it != cache.end()) {
            cache.erase(it);
        }
        if (!removed) {
            // Keep in-memory R-tree consistent even when direct remove misses
            // (e.g. geometry representation mismatch). Rebuild from cache.
            std::vector<std::pair<std::string, geo::GeometryInfo>> bulk_entries;
            bulk_entries.reserve(cache.size());
            for (const auto& [cached_pk, cached_mbr] : cache) {
                bulk_entries.emplace_back(cached_pk, mbrToGeometryInfo(cached_mbr));
            }
            rtrees_[table_str].clear();
            if (!bulk_entries.empty()) {
                rtrees_[table_str].bulkLoad(bulk_entries);
            }
        }
    }

    // Note: per-PK keys are the primary index storage. We still update the
    // legacy bucket key for backward compatibility and fallback query paths.

    return Status::OK();
}

// Remove
SpatialIndexManager::Status SpatialIndexManager::remove(
    std::string_view table,
    std::string_view primary_key,
    const geo::GeoSidecar& sidecar
) {
    // G5: Track remove metrics
    metrics_.remove_count++;
    
    auto config = getConfig(table);
    if (!config) return Status::OK();
    
    uint64_t morton = MortonEncoder::encode2D(
        sidecar.centroid.x,
        sidecar.centroid.y,
        config->total_bounds
    );
    
    std::string key = makeSpatialKey(table, morton);
    auto value = db_.get(key);
    
    if (!value) return Status::OK();
    
    std::string s(reinterpret_cast<const char*>(value->data()), value->size());
    auto entries = parseSidecarList(s);
    
    // Remove matching entry
    entries.erase(
        std::remove_if(entries.begin(), entries.end(),
            [&](const SidecarEntry& e) { return e.primary_key == primary_key; }),
        entries.end()
    );
    
    // Storage improvement: Also delete per-PK key
    std::string pk_key = makeSpatialPerPKKey(table, morton, primary_key);
    db_.del(pk_key); // Best effort

    // Update in-memory R-tree and MBR cache.
    std::string table_str(table);
    std::string pk_str(primary_key);
    {
        // LOCK: Tier 1 (Global R-tree protection) — Phase 3 A-5
        std::unique_lock<std::shared_mutex> lock(rtree_mutex_);
        auto& cache = mbr_cache_[table_str];
        auto it = cache.find(pk_str);
        const geo::MBR mbr_to_remove = (it != cache.end()) ? it->second : sidecar.mbr;
        const bool removed = rtrees_[table_str].remove(pk_str, mbrToGeometryInfo(mbr_to_remove));
        if (it != cache.end()) {
            cache.erase(it);
        }
        if (!removed) {
            // Keep in-memory R-tree consistent even when direct remove misses
            // (e.g. geometry representation mismatch). Rebuild from cache.
            std::vector<std::pair<std::string, geo::GeometryInfo>> bulk_entries;
            bulk_entries.reserve(cache.size());
            for (const auto& [cached_pk, cached_mbr] : cache) {
                bulk_entries.emplace_back(cached_pk, mbrToGeometryInfo(cached_mbr));
            }
            rtrees_[table_str].clear();
            if (!bulk_entries.empty()) {
                rtrees_[table_str].bulkLoad(bulk_entries);
            }
        }
    }

    if (entries.empty()) {
        return db_.del(key) ? Status::OK() : Status::Error("failed to remove");
    } else {
        const auto dump = serializeSidecarList(entries);
        std::vector<uint8_t> bytes(dump.begin(), dump.end());
        return db_.put(key, bytes) ? Status::OK() : Status::Error("failed to update bucket");
    }
}

// Update
SpatialIndexManager::Status SpatialIndexManager::update(
    std::string_view table,
    std::string_view primary_key,
    const geo::GeoSidecar& old_sidecar,
    const geo::GeoSidecar& new_sidecar
) {
    // G5: Track update metrics
    metrics_.update_count++;
    
    auto status = remove(table, primary_key, old_sidecar);
    if (!status) return status;
    
    return insert(table, primary_key, new_sidecar);
}

// Haversine distance
double SpatialIndexManager::haversineDistance(double lat1, double lon1, double lat2, double lon2) const {
    return themis::geo::haversine_m(lat1, lon1, lat2, lon2);
}

// Euclidean 3D distance
double SpatialIndexManager::euclidean3DDistance(
    double x1, double y1, double z1,
    double x2, double y2, double z2) const {
    double dx = x2 - x1;
    double dy = y2 - y1;
    double dz = z2 - z1;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

// Z-bucket (10 m buckets; negative elevations get negative bucket IDs)
int SpatialIndexManager::getZBucket(double z) const {
    return static_cast<int>(std::floor(z / Z_BUCKET_SIZE));
}

// Search intersects
std::vector<SpatialResult> SpatialIndexManager::searchIntersects(
    std::string_view table,
    const geo::MBR& query_bbox
) const {
    // Phase 3 A-6: Connection safety verified
    // DB queries use RocksDB's iterator-based scan (RAII-safe).
    // No manual connection management needed.
    // Exceptions automatically trigger cleanup via db_ destructor.
    
    // G5: Track query metrics
    metrics_.query_count++;

    auto config = getConfig(table);
    if (!config) {
        THEMIS_WARN("SpatialIndexManager::searchIntersects - missing config for table {}", std::string(table));
        return {};
    }

    // ── Fast path: use in-memory R-tree when available ───────────────────
    // ensureRTree() builds the R-tree lazily from per-PK RocksDB keys on the
    // first call; subsequent calls are no-ops (O(1) set-lookup).
    ensureRTree(table);

    std::string table_str(table);

    // Snapshot candidate keys and their MBRs under a shared lock, then
    // release before any I/O (db_ / exact_backend_) to avoid lock inversion.
    std::vector<std::string> candidate_keys;
    std::unordered_map<std::string, geo::MBR> candidate_mbrs;
    bool rtree_populated = false;
    {
        // LOCK: Tier 1 (Global R-tree protection, read-only) — Phase 3 A-5
        std::shared_lock<std::shared_mutex> slock(rtree_mutex_);
        const auto& rtree = rtrees_[table_str];
        if (rtree.size() > 0) {
            rtree_populated = true;
            candidate_keys = rtree.intersects(query_bbox);
            const auto& cache = mbr_cache_[table_str];
            for (const auto& pk : candidate_keys) {
                auto it = cache.find(pk);
                if (it != cache.end()) {
                    candidate_mbrs[pk] = it->second;
                }
            }
        }
    }

    if (rtree_populated) {
        // R-tree path: O(log n + k) MBR pre-filter, where k = number of hits.
        std::vector<SpatialResult> results;

        size_t mbr_candidates_this_query = static_cast<size_t>(candidate_keys.size());
        size_t exact_checks_this_query = 0;
        size_t exact_passed_this_query = 0;

        for (const auto& pk : candidate_keys) {
            // Look up the stored MBR from the snapshot.
            geo::MBR entry_mbr;
            auto mbr_it = candidate_mbrs.find(pk);
            if (mbr_it != candidate_mbrs.end()) {
                entry_mbr = mbr_it->second;
            }

            // Phase 2: Exact geometry check (if backend available).
            bool exact_match = true;
            if (exact_backend_) {
                exact_checks_this_query++;
                try {
                    std::string entity_key = "entity:" + table_str + ":" + pk;
                    auto blob = db_.get(entity_key);
                    if (blob) {
                        std::string blob_str(reinterpret_cast<const char*>(blob->data()),
                                             blob->size());
                        try {
                            auto j = json::parse(blob_str);
                            if (j.contains("geometry") && j["geometry"].is_object()) {
                                std::string geojson = j["geometry"].dump();
                                auto entity_geom = geo::EWKBParser::parseGeoJSON(geojson);
                                geo::GeometryInfo query_geom(geo::GeometryType::Polygon);
                                query_geom.coords = {
                                    geo::Coordinate(query_bbox.minx, query_bbox.miny),
                                    geo::Coordinate(query_bbox.maxx, query_bbox.miny),
                                    geo::Coordinate(query_bbox.maxx, query_bbox.maxy),
                                    geo::Coordinate(query_bbox.minx, query_bbox.maxy),
                                    geo::Coordinate(query_bbox.minx, query_bbox.miny)
                                };
                                exact_match = exact_backend_->exactIntersects(entity_geom, query_geom);
                                if (exact_match) exact_passed_this_query++;
                            }
                        } catch (...) { exact_match = true; }
                    }
                } catch (...) { exact_match = true; }
            }

            if (exact_match) {
                SpatialResult result;
                result.primary_key = pk;
                result.mbr = entry_mbr;
                results.push_back(result);
            }
        }

        // G5: Update global metrics
        metrics_.mbr_candidate_count += mbr_candidates_this_query;
        metrics_.exact_check_count += exact_checks_this_query;
        metrics_.exact_check_passed += exact_passed_this_query;
        metrics_.exact_check_failed += (exact_checks_this_query - exact_passed_this_query);

        return results;
    }

    // ── Fallback path: Morton code range scan ────────────────────────────
    // Used when the R-tree is empty (no data inserted in this session and no
    // per-PK keys found during lazy rebuild, e.g. very old bucket-only data).
    auto ranges = MortonEncoder::getRanges(query_bbox, config->total_bounds);

    std::vector<SpatialResult> results;
    size_t mbr_candidates_this_query = 0;
    size_t exact_checks_this_query = 0;
    size_t exact_passed_this_query = 0;

    for (const auto& [min_code, max_code] : ranges) {
        std::string start_key = makeSpatialKey(table, min_code);
        std::string end_key = makeSpatialKey(table, max_code);

        std::vector<std::pair<std::string,std::string>> kvs;
        db_.scanRange(start_key, end_key, [&kvs](std::string_view k, std::string_view v){
            kvs.emplace_back(std::string(k), std::string(v));
            return true;
        });

        for (const auto& [key, value] : kvs) {
            auto entries = parseSidecarList(value);

            for (const auto& entry : entries) {
                if (!entry.sidecar.mbr.intersects(query_bbox)) continue;

                mbr_candidates_this_query++;

                bool exact_match = true;
                if (exact_backend_) {
                    exact_checks_this_query++;
                    try {
                        std::string entity_key = "entity:" + std::string(table) + ":" + entry.primary_key;
                        auto blob = db_.get(entity_key);
                        if (blob) {
                            std::string blob_str(reinterpret_cast<const char*>(blob->data()), blob->size());
                            try {
                                auto j = json::parse(blob_str);
                                if (j.contains("geometry") && j["geometry"].is_object()) {
                                    std::string geojson = j["geometry"].dump();
                                    auto entity_geom = geo::EWKBParser::parseGeoJSON(geojson);
                                    geo::GeometryInfo query_geom(geo::GeometryType::Polygon);
                                    query_geom.coords = {
                                        geo::Coordinate(query_bbox.minx, query_bbox.miny),
                                        geo::Coordinate(query_bbox.maxx, query_bbox.miny),
                                        geo::Coordinate(query_bbox.maxx, query_bbox.maxy),
                                        geo::Coordinate(query_bbox.minx, query_bbox.maxy),
                                        geo::Coordinate(query_bbox.minx, query_bbox.miny)
                                    };
                                    exact_match = exact_backend_->exactIntersects(entity_geom, query_geom);
                                    if (exact_match) exact_passed_this_query++;
                                }
                            } catch (...) { exact_match = true; }
                        }
                    } catch (...) { exact_match = true; }
                }

                if (exact_match) {
                    SpatialResult result;
                    result.primary_key = entry.primary_key;
                    result.mbr = entry.sidecar.mbr;
                    result.z_min = entry.sidecar.z_min != 0.0
                        ? std::optional<double>(entry.sidecar.z_min) : std::nullopt;
                    result.z_max = entry.sidecar.z_max != 0.0
                        ? std::optional<double>(entry.sidecar.z_max) : std::nullopt;
                    results.push_back(result);
                }
            }
        }
    }

    // G5: Update global metrics
    metrics_.mbr_candidate_count += mbr_candidates_this_query;
    metrics_.exact_check_count += exact_checks_this_query;
    metrics_.exact_check_passed += exact_passed_this_query;
    metrics_.exact_check_failed += (exact_checks_this_query - exact_passed_this_query);

    return results;
}

// Search within
std::vector<SpatialResult> SpatialIndexManager::searchWithin(
    std::string_view table,
    const geo::MBR& query_bbox,
    double z_min,
    double z_max
) const {
    auto candidates = searchIntersects(table, query_bbox);
    
    // Filter: entity MBR must be fully within query MBR
    std::vector<SpatialResult> results;
    
    for (const auto& cand : candidates) {
        bool within = (cand.mbr.minx >= query_bbox.minx &&
                       cand.mbr.maxx <= query_bbox.maxx &&
                       cand.mbr.miny >= query_bbox.miny &&
                       cand.mbr.maxy <= query_bbox.maxy);
        
        // Check Z-range if specified
        if (within && z_min > -DBL_MAX && cand.z_min.has_value()) {
            within = (cand.z_min.value() >= z_min && cand.z_max.value() <= z_max);
        }
        
        if (within) {
            results.push_back(cand);
        }
    }
    
    return results;
}

// Search contains point
std::vector<SpatialResult> SpatialIndexManager::searchContains(
    std::string_view table,
    double x,
    double y,
    [[maybe_unused]] std::optional<double> z
) const {
    // unused parameter

    auto config = getConfig(table);
    if (!config) {
        THEMIS_WARN("SpatialIndexManager::searchWithin - missing config for table {}", std::string(table));
        return {};
    }

    // ── Fast path: use the R-tree's point-containment query directly ─────
    // GeoRTree::contains(x, y) issues a zero-area bounding-box query and
    // verifies MBR containment inside the tree, which is more precise than
    // the tiny-bbox workaround and avoids a redundant filter pass.
    ensureRTree(table);

    std::string table_str(table);

    // Snapshot candidate keys and their MBRs under a shared lock.
    std::vector<std::string> candidate_keys;
    std::unordered_map<std::string, geo::MBR> candidate_mbrs;
    bool rtree_populated = false;
    {
        // LOCK: Tier 1 (Global R-tree protection, read-only) — Phase 3 A-5
        std::shared_lock<std::shared_mutex> slock(rtree_mutex_);
        const auto& rtree = rtrees_[table_str];
        if (rtree.size() > 0) {
            rtree_populated = true;
            candidate_keys = rtree.contains(x, y);
            const auto& cache = mbr_cache_[table_str];
            for (const auto& pk : candidate_keys) {
                auto it = cache.find(pk);
                if (it != cache.end()) {
                    candidate_mbrs[pk] = it->second;
                }
            }
        }
    }

    if (rtree_populated) {
        std::vector<SpatialResult> results;
        results.reserve(candidate_keys.size());
        for (const auto& pk : candidate_keys) {
            SpatialResult result;
            result.primary_key = pk;
            auto it = candidate_mbrs.find(pk);
            if (it != candidate_mbrs.end()) result.mbr = it->second;
            results.push_back(std::move(result));
        }
        return results;
    }

    // ── Fallback: tiny-bbox approach (legacy Morton-bucket data, no R-tree) ─
    geo::MBR point_bbox(x - 0.0001, y - 0.0001, x + 0.0001, y + 0.0001);
    auto candidates = searchIntersects(table, point_bbox);

    std::vector<SpatialResult> results;
    for (const auto& cand : candidates) {
        if (cand.mbr.contains(x, y)) {
            results.push_back(cand);
        }
    }
    return results;
}

// Search nearby (distance-based)
std::vector<SpatialResult> SpatialIndexManager::searchNearby(
    std::string_view table,
    double x,
    double y,
    double max_distance_meters,
    [[maybe_unused]] std::optional<double> z,
    size_t limit
) const {
    // unused parameter
    // Expand to bbox (approximate)
    double degrees_delta = max_distance_meters / 111320.0;  // Rough approximation
    geo::MBR query_bbox(x - degrees_delta, y - degrees_delta, x + degrees_delta, y + degrees_delta);
    
    auto candidates = searchIntersects(table, query_bbox);
    
    // Compute exact distances
    std::vector<SpatialResult> results;
    
    for (auto& cand : candidates) {
        double dist = haversineDistance(y, x, cand.mbr.center().y, cand.mbr.center().x);
        
        if (dist <= max_distance_meters) {
            cand.distance = dist;
            results.push_back(cand);
        }
    }
    
    // Sort by distance
    std::sort(results.begin(), results.end(),
        [](const SpatialResult& a, const SpatialResult& b) {
            return a.distance < b.distance;
        });
    
    // Limit results
    if (results.size() > limit) {
        results.resize(limit);
    }
    
    return results;
}

// K-Nearest Neighbors search using the in-memory R-tree.
// Expands an initial search window exponentially until k candidates are found
// or the window covers the full table bounds.
// Note: the optional `z` parameter is reserved for future 3D distance
// filtering; it is currently unused and 2D Haversine distance is used.
std::vector<SpatialResult> SpatialIndexManager::searchKNN(
    std::string_view table,
    double x,
    double y,
    size_t k,
    [[maybe_unused]] std::optional<double> z
) const {
    // unused parameter — reserved for future 3D distance filtering
    if (k == 0) {
        THEMIS_DEBUG("SpatialIndexManager::searchKNN - k==0, returning empty result");
        return {};
    }

    auto config = getConfig(table);
    if (!config) {
        THEMIS_WARN("SpatialIndexManager::searchKNN - missing config for table {}", std::string(table));
        return {};
    }

    // Ensure the R-tree is built before we start querying.
    ensureRTree(table);

    const geo::MBR& bounds = config->total_bounds;

    // Initial search radius (~1 km at the equator: 1000 m / 111 320 m·deg⁻¹ ≈ 0.009°).
    // The radius doubles each iteration until k candidates are found or the
    // full table bounds are covered.  20 doublings allow expansion from 1 km to
    // ~1 000 000 km, which exceeds Earth's circumference and guarantees termination.
    static constexpr double kInitialRadiusDeg = 0.009;
    static constexpr int    kMaxExpansionIter = 20;

    double radius = kInitialRadiusDeg;
    std::vector<SpatialResult> candidates;

    // Double the search window until we have k candidates or exceed world bounds.
    for (size_t iter = 0; iter < kMaxExpansionIter && candidates.size() < k; ++iter) {
        geo::MBR bbox(x - radius, y - radius, x + radius, y + radius);
        // Clamp to table bounds
        bbox.minx = std::max(bbox.minx, bounds.minx);
        bbox.miny = std::max(bbox.miny, bounds.miny);
        bbox.maxx = std::min(bbox.maxx, bounds.maxx);
        bbox.maxy = std::min(bbox.maxy, bounds.maxy);

        candidates = searchIntersects(table, bbox);

        // Stop expanding if the window covers the full bounds.
        if (bbox.minx <= bounds.minx && bbox.maxx >= bounds.maxx &&
            bbox.miny <= bounds.miny && bbox.maxy >= bounds.maxy) {
            break;
        }
        radius *= 2.0;
    }

    // Compute distances and sort ascending.
    for (auto& cand : candidates) {
        cand.distance = haversineDistance(y, x, cand.mbr.center().y, cand.mbr.center().x);
    }
    std::sort(candidates.begin(), candidates.end(),
              [](const SpatialResult& a, const SpatialResult& b) {
                  return a.distance < b.distance;
              });

    if (candidates.size() > k) {
        candidates.resize(k);
    }
    return candidates;
}

// Z-range search: return all entities whose elevation overlaps [z_min, z_max].
// Scans all per-PK spatial keys for the table and filters by Z range.
std::vector<SpatialResult> SpatialIndexManager::searchZRange(
    std::string_view table,
    double z_min,
    double z_max
) const {
    if (!getConfig(table)) return {};

    // Ensure the R-tree (and MBR cache with Z data) is populated.
    ensureRTree(table);

    const std::string pk_prefix = getSpatialKeyPrefix(table) + "pk:";

    std::vector<SpatialResult> results;
    std::string table_str(table);

    db_.scanRange(pk_prefix, pk_prefix + "~",
        [&](std::string_view k, std::string_view value) {
            constexpr std::size_t kMortonChars = 16;
            const std::size_t pk_strip = pk_prefix.size() + kMortonChars + 1; // +1 for ':'
            // Validate key length and the expected ':' separator between morton code and PK.
            if (k.size() <= pk_strip) return true;
            if (k[pk_prefix.size() + kMortonChars] != ':') return true;
            std::string pk(k.substr(pk_strip));
            try {
                auto j = json::parse(std::string(value));
                // Only process entries that carry Z data.
                if (!j.contains("z_min") || !j.contains("z_max")) return true;
                double e_min = j["z_min"].get<double>();
                double e_max = j["z_max"].get<double>();
                // Skip entries whose Z range does not overlap [z_min, z_max].
                if (e_max < z_min || e_min > z_max) return true;

                SpatialResult result;
                result.primary_key = pk;
                result.z_min = e_min;
                result.z_max = e_max;
                const auto& mbr_j = j.at("mbr");
                result.mbr.minx = mbr_j.at("minx").get<double>();
                result.mbr.miny = mbr_j.at("miny").get<double>();
                result.mbr.maxx = mbr_j.at("maxx").get<double>();
                result.mbr.maxy = mbr_j.at("maxy").get<double>();
                results.push_back(std::move(result));
            } catch (...) {}
            return true;
        });

    return results;
}

// Combined spatial + Z-range filter.
// After the R-tree pre-filter, fetches Z metadata from per-PK RocksDB keys
// for each spatial candidate and applies the Z range check.
//
// Conservative-inclusion contract: if an entity was stored without Z metadata
// (z_min / z_max absent from the per-PK RocksDB value), it is included in the
// result regardless of the requested Z range.  This avoids false negatives for
// legacy data that pre-dates Z storage and is safe because the caller receives
// a superset of the true answer.  Applications that require exact Z exclusion
// must store z_min / z_max in the GeoSidecar at insert time.
std::vector<SpatialResult> SpatialIndexManager::searchIntersectsWithZ(
    std::string_view table,
    const geo::MBR& query_bbox,
    double z_min,
    double z_max
) const {
    auto candidates = searchIntersects(table, query_bbox);

    const std::string pk_prefix = getSpatialKeyPrefix(table) + "pk:";
    auto config = getConfig(table);
    const geo::MBR bounds = config ? config->total_bounds
                                   : geo::MBR(-180.0, -90.0, 180.0, 90.0);

    std::vector<SpatialResult> results;
    results.reserve(candidates.size());
    for (auto& cand : candidates) {
        // If Z values were already populated (Morton-scan path), use them directly.
        if (cand.z_min.has_value()) {
            if (cand.z_max.value() >= z_min && cand.z_min.value() <= z_max) {
                results.push_back(std::move(cand));
            }
            continue;
        }

        // R-tree path: look up Z data from the per-PK RocksDB key.
        uint64_t morton = MortonEncoder::encode2D(
            cand.mbr.center().x, cand.mbr.center().y, bounds);
        char buf[32];
        snprintf(buf, sizeof(buf), "%016llx", static_cast<unsigned long long>(morton));
        std::string pk_key = pk_prefix + buf + ":" + cand.primary_key;

        auto blob = db_.get(pk_key);
        if (!blob) {
            // No Z data: include conservatively.
            results.push_back(std::move(cand));
            continue;
        }
        try {
            std::string blob_str(reinterpret_cast<const char*>(blob->data()), blob->size());
            auto j = json::parse(blob_str);
            if (!j.contains("z_min") || !j.contains("z_max")) {
                results.push_back(std::move(cand));
                continue;
            }
            double e_min = j["z_min"].get<double>();
            double e_max = j["z_max"].get<double>();
            if (e_max >= z_min && e_min <= z_max) {
                cand.z_min = e_min;
                cand.z_max = e_max;
                results.push_back(std::move(cand));
            }
        } catch (...) {
            // Parse error: include conservatively.
            results.push_back(std::move(cand));
        }
    }
    return results;
}

// Get index stats
SpatialIndexManager::IndexStats SpatialIndexManager::getStats(std::string_view table) const {
    IndexStats stats;
    
    auto config = getConfig(table);
    if (!config) return stats;
    
    stats.total_bounds = config->total_bounds;
    
    // Scan all spatial keys
    std::string prefix = getSpatialKeyPrefix(table);
    size_t buckets = 0;
    double total_area = 0.0;
    db_.scanRange(prefix, prefix + "~", [this, &buckets, &total_area, &stats](std::string_view /*k*/, std::string_view v){
        ++buckets;
        auto entries = parseSidecarList(std::string(v));
        stats.entry_count += entries.size();
        for (const auto& entry : entries) {
            total_area += entry.sidecar.mbr.area();
        }
        return true;
    });
    stats.morton_buckets = buckets;
    
    if (stats.entry_count > 0) {
        stats.avg_area = total_area / stats.entry_count;
    }
    
    return stats;
}

}  // namespace index
}  // namespace themis


