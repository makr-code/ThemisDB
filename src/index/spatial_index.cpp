/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            spatial_index.cpp                                  ║
  Version:         0.0.32                                             ║
  Last Modified:   2026-02-23 03:58:09                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     783                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "index/spatial_index.h"
#include "utils/logger.h"
#include <cmath>
#include <algorithm>
#include <cstdio>
#include <nlohmann/json.hpp>

namespace themis {
namespace index {

using json = nlohmann::json;

// Constants
constexpr double EARTH_RADIUS_METERS = 6371000.0;
constexpr double PI_CONST = 3.14159265358979323846;
constexpr double DEG_TO_RAD = PI_CONST / 180.0;
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

// Get Morton ranges for MBR query (simplified: just return min/max range)
std::vector<std::pair<uint64_t, uint64_t>> MortonEncoder::getRanges(
    const geo::MBR& query_bbox,
    const geo::MBR& total_bounds,
    int max_ranges
) {
    (void)max_ranges; // unused parameter
    // Simplified implementation: compute min/max Morton codes
    uint64_t min_code = encode2D(query_bbox.minx, query_bbox.miny, total_bounds);
    uint64_t max_code = encode2D(query_bbox.maxx, query_bbox.maxy, total_bounds);
    
    // For accurate query, we'd need to decompose into multiple ranges
    // For MVP, use single range (may include false positives)
    return {{min_code, max_code}};
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
    if (rtree_built_.count(table_str)) return;

    // Mark as built immediately to avoid re-entry in recursive / concurrent paths.
    rtree_built_.insert(table_str);

    // Scan all per-PK spatial keys for this table.
    // Key format: spatial:<table>:pk:<16hex>:<primary_key>
    // Value format: JSON {"mbr":{"minx":...,"miny":...,"maxx":...,"maxy":...}}
    const std::string pk_prefix = getSpatialKeyPrefix(table) + "pk:";
    constexpr std::size_t kMortonChars = 16; // 64-bit hex without prefix
    const std::size_t pk_strip = pk_prefix.size() + kMortonChars + 1; // +1 for ':'

    std::vector<std::pair<std::string, geo::GeometryInfo>> bulk_entries;
    auto& cache = mbr_cache_[table_str];

    db_.scanRange(pk_prefix, pk_prefix + "~",
        [&](std::string_view k, std::string_view v) {
            if (k.size() <= pk_strip) return true; // malformed key, skip
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
            } catch (...) {
                // Malformed entry — skip; do not abort the scan.
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
    std::string_view geometry_column,
    const RTreeConfig& config
) {
    (void)geometry_column; // unused parameter
    // Save config
    RTreeConfig cfg = config;
    if (cfg.total_bounds.minx == 0.0 && cfg.total_bounds.maxx == 0.0) {
        // Default: global lat/lon bounds
        cfg.total_bounds = geo::MBR(-180.0, -90.0, 180.0, 90.0);
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
    mbr_cache_[table_str][pk_str] = sidecar.mbr;
    rtrees_[table_str].insert(pk_str, mbrToGeometryInfo(sidecar.mbr));
    // Mark the table's R-tree as built so ensureRTree won't overwrite our
    // incrementally maintained index on the first query.
    rtree_built_.insert(table_str);

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
    mbr_cache_[table_str][pk_str] = sidecar.mbr;
    rtrees_[table_str].insert(pk_str, mbrToGeometryInfo(sidecar.mbr));
    rtree_built_.insert(table_str);

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

    // Update in-memory R-tree and MBR cache.
    std::string table_str(table);
    std::string pk_str(primary_key);
    auto& cache = mbr_cache_[table_str];
    auto it = cache.find(pk_str);
    if (it != cache.end()) {
        rtrees_[table_str].remove(pk_str, mbrToGeometryInfo(it->second));
        cache.erase(it);
    }

    // Note: We rely on per-PK keys for spatial queries to avoid bucket-level
    // read-modify-write conflicts. The bucket key is kept for backward compatibility
    // but is not updated here to prevent concurrent write conflicts.

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
    auto& cache = mbr_cache_[table_str];
    auto it = cache.find(pk_str);
    if (it != cache.end()) {
        rtrees_[table_str].remove(pk_str, mbrToGeometryInfo(it->second));
        cache.erase(it);
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
    double dlat = (lat2 - lat1) * DEG_TO_RAD;
    double dlon = (lon2 - lon1) * DEG_TO_RAD;
    
    double a = std::sin(dlat / 2) * std::sin(dlat / 2) +
               std::cos(lat1 * DEG_TO_RAD) * std::cos(lat2 * DEG_TO_RAD) *
               std::sin(dlon / 2) * std::sin(dlon / 2);
    
    double c = 2 * std::atan2(std::sqrt(a), std::sqrt(1 - a));
    
    return EARTH_RADIUS_METERS * c;
}

// Search intersects
std::vector<SpatialResult> SpatialIndexManager::searchIntersects(
    std::string_view table,
    const geo::MBR& query_bbox
) const {
    // G5: Track query metrics
    metrics_.query_count++;

    auto config = getConfig(table);
    if (!config) return {};

    // ── Fast path: use in-memory R-tree when available ───────────────────
    // ensureRTree() builds the R-tree lazily from per-PK RocksDB keys on the
    // first call; subsequent calls are no-ops (O(1) set-lookup).
    ensureRTree(table);

    std::string table_str(table);
    const auto& rtree = rtrees_[table_str];

    if (rtree.size() > 0) {
        // R-tree path: O(log n + k) MBR pre-filter, where k = number of hits.
        auto candidate_keys = rtree.intersects(query_bbox);

        std::vector<SpatialResult> results;
        const auto& cache = mbr_cache_[table_str];

        size_t mbr_candidates_this_query = static_cast<size_t>(candidate_keys.size());
        size_t exact_checks_this_query = 0;
        size_t exact_passed_this_query = 0;

        for (const auto& pk : candidate_keys) {
            // Look up the stored MBR from the in-memory cache.
            auto cache_it = cache.find(pk);
            geo::MBR entry_mbr;
            if (cache_it != cache.end()) {
                entry_mbr = cache_it->second;
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
    std::optional<double> z
) const {
    (void)z; // unused parameter
    // Create small query box around point
    geo::MBR point_bbox(x - 0.0001, y - 0.0001, x + 0.0001, y + 0.0001);
    
    auto candidates = searchIntersects(table, point_bbox);
    
    // Filter: MBR must contain point
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
    std::optional<double> z,
    size_t limit
) const {
    (void)z; // unused parameter
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
