/**
 * @file spatial_index.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "utils/geo/ewkb.h"
#include "geo/spatial_backend.h"
#include "geo/geo_rtree.h"
#include "storage/rocksdb_wrapper.h"
#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include <optional>
#include <cfloat>
#include <atomic>
#include <mutex>
#include <shared_mutex>
#include <unordered_map>
#include <unordered_set>

namespace themis {
namespace index {

// Morton code encoder for Z-order spatial indexing
/** @brief Morton code encoder for Z-order spatial indexing. */
class MortonEncoder {
public:
    // Encode 2D coordinates to Morton code (Z-order curve)
    static uint64_t encode2D(double x, double y, const geo::MBR& bounds);
    
    // Encode 3D coordinates to Morton code
    static uint64_t encode3D(double x, double y, double z, const geo::MBR& bounds);
    
    // Decode Morton code to 2D coordinates
    static std::pair<double, double> decode2D(uint64_t code, const geo::MBR& bounds);
    
    // Get Morton code ranges for MBR query (multiple ranges for 2D query on 1D curve)
    static std::vector<std::pair<uint64_t, uint64_t>> getRanges(
        const geo::MBR& query_bbox,
        const geo::MBR& total_bounds,
        int max_ranges = 16
    );

private:
    // Interleave bits of two 32-bit integers
    static uint64_t interleaveBits2D(uint32_t x, uint32_t y);
    
    // Interleave bits of three 21-bit integers
    static uint64_t interleaveBits3D(uint32_t x, uint32_t y, uint32_t z);
    
    // Normalize coordinate to [0, 2^32-1] range
    static uint32_t normalizeCoord(double coord, double min_val, double max_val);
};

// R-Tree configuration
struct RTreeConfig {
    int max_entries_per_node = 16;    // M parameter
    int min_entries_per_node = 4;     // m parameter (typically M/4)
    bool use_3d = false;              // Enable Z-coordinate indexing
    geo::MBR total_bounds;            // Global bounds for normalization
};

// Spatial query result
struct SpatialResult {
    std::string primary_key;
    geo::MBR mbr;
    std::optional<double> z_min;
    std::optional<double> z_max;
    double distance = 0.0;  // For distance-based queries
};

// Spatial Index Manager (table-agnostic, works for all 5 models)
/** @brief Spatial Index Manager (table-agnostic, works for all 5 models). */
class SpatialIndexManager {
public:
    struct Status {
        bool ok = true;
        std::string message;
        static Status OK() { return {}; }
        static Status Error(std::string msg) { return Status{false, std::move(msg)}; }
        explicit operator bool() const { return ok; }
    };
    
    // Metrics for monitoring and benchmarking (G5)
    struct Metrics {
        std::atomic<uint64_t> query_count{0};           // Total searchIntersects() calls
        std::atomic<uint64_t> mbr_candidate_count{0};   // Total MBR candidates found
        std::atomic<uint64_t> exact_check_count{0};     // Exact geometry checks performed
        std::atomic<uint64_t> exact_check_passed{0};    // Exact checks that passed
        std::atomic<uint64_t> exact_check_failed{0};    // Exact checks that failed (false positives)
        std::atomic<uint64_t> insert_count{0};          // Sidecar inserts
        std::atomic<uint64_t> remove_count{0};          // Sidecar removes
        std::atomic<uint64_t> update_count{0};          // Sidecar updates
        
        // Reset all metrics
        void reset() {
            query_count = 0;
            mbr_candidate_count = 0;
            exact_check_count = 0;
            exact_check_passed = 0;
            exact_check_failed = 0;
            insert_count = 0;
            remove_count = 0;
            update_count = 0;
        }
    };

    explicit SpatialIndexManager(RocksDBWrapper& db);
    ~SpatialIndexManager() = default;
    
    // Set exact geometry backend (optional, for exact checks)
    void setExactBackend(geo::ISpatialComputeBackend* backend) { exact_backend_ = backend; }
    
    // Get metrics (G5)
    const Metrics& getMetrics() const { return metrics_; }
    Metrics& getMetrics() { return metrics_; }
    void resetMetrics() { metrics_.reset(); }
    
    // ===== Index Management =====
    
    /// Create spatial index for ANY table (relational, graph, vector, content, time-series)
    Status createSpatialIndex(
        std::string_view table,
        std::string_view geometry_column = "geometry",
        const RTreeConfig& config = {}
    );
    
    /// Drop spatial index
    Status dropSpatialIndex(std::string_view table);
    
    /// Check if table has spatial index
    bool hasSpatialIndex(std::string_view table) const;
    
    /// Get index statistics
    struct IndexStats {
        size_t entry_count = 0;
        geo::MBR total_bounds;
        double avg_area = 0.0;
        size_t morton_buckets = 0;
    };
    IndexStats getStats(std::string_view table) const;
    
    // ===== Bulk Operations =====

    /**
     * @brief Bulk-load a collection of (primary_key, sidecar) pairs into the
     *        spatial index for a given table.
     *
     * Uses STR (Sort-Tile-Recursive) packing via GeoRTree::bulkLoad(), which
     * is 3–5× faster than incremental insertion for read-heavy workloads.
     * Replaces any previously cached in-memory R-tree for the table.
     *
     * Memory usage is reported as a structured log entry with field
     * `geo_index_bytes_allocated` after the bulk load completes.
     *
     * @param table   Name of the table whose spatial index should be populated.
     * @param entries Vector of {primary_key, GeoSidecar} pairs to index.
     * @return Status::OK() on success or Status::Error() if the table has no
     *         spatial index registered.
     */
    Status bulkLoad(
        std::string_view table,
        const std::vector<std::pair<std::string, geo::GeoSidecar>>& entries
    );

    // ===== Insert/Update/Delete =====
    
    /// Insert entity into spatial index
    Status insert(
        std::string_view table,
        std::string_view primary_key,
        const geo::GeoSidecar& sidecar
    );
    
    /// Insert entity into spatial index using WriteBatch (atomic with entity write)
    /// This is used by GeoIndexHooks::onEntityPutAtomic for transactional updates
    Status insertBatch(
        RocksDBWrapper::WriteBatchWrapper& batch,
        std::string_view table,
        std::string_view primary_key,
        const geo::GeoSidecar& sidecar
    );
    
    /// Remove entity from spatial index using WriteBatch (atomic)
    Status removeBatch(
        RocksDBWrapper::WriteBatchWrapper& batch,
        std::string_view table,
        std::string_view primary_key,
        const geo::GeoSidecar& sidecar
    );
    
    /// Update entity location
    Status update(
        std::string_view table,
        std::string_view primary_key,
        const geo::GeoSidecar& old_sidecar,
        const geo::GeoSidecar& new_sidecar
    );
    
    /// Remove entity from spatial index
    Status remove(
        std::string_view table,
        std::string_view primary_key,
        const geo::GeoSidecar& sidecar
    );
    
    // ===== Query Operations (Model-Agnostic) =====
    
    /// Find all entities intersecting with query MBR
    std::vector<SpatialResult> searchIntersects(
        std::string_view table,
        const geo::MBR& query_bbox
    ) const;
    
    /// Find all entities within query MBR
    std::vector<SpatialResult> searchWithin(
        std::string_view table,
        const geo::MBR& query_bbox,
        double z_min = -DBL_MAX,
        double z_max = DBL_MAX
    ) const;
    
    /// Find all entities containing a point
    std::vector<SpatialResult> searchContains(
        std::string_view table,
        double x,
        double y,
        std::optional<double> z = std::nullopt
    ) const;
    
    /// Find all entities within distance from point (2D Haversine or 3D Euclidean)
    std::vector<SpatialResult> searchNearby(
        std::string_view table,
        double x,
        double y,
        double max_distance_meters,
        std::optional<double> z = std::nullopt,
        size_t limit = 100
    ) const;
    
    /// K-Nearest Neighbors search (spatial only, no exact geometry check)
    std::vector<SpatialResult> searchKNN(
        std::string_view table,
        double x,
        double y,
        size_t k,
        std::optional<double> z = std::nullopt
    ) const;
    
    // ===== 3D Z-Range Queries =====
    
    /// Find entities within Z-range (elevation filtering)
    std::vector<SpatialResult> searchZRange(
        std::string_view table,
        double z_min,
        double z_max
    ) const;
    
    /// Combine spatial + Z-range filter
    std::vector<SpatialResult> searchIntersectsWithZ(
        std::string_view table,
        const geo::MBR& query_bbox,
        double z_min,
        double z_max
    ) const;
    
private:
    RocksDBWrapper& db_;
    geo::ISpatialComputeBackend* exact_backend_ = nullptr; // Optional exact geometry backend
    mutable Metrics metrics_; // G5: Performance metrics

    // ── In-memory R-tree index (per table) ──────────────────────────────
    // Populated lazily on first spatial query; updated on every write.
    // Provides O(log n + k) MBR pre-filtering, replacing the O(n) Morton
    // code range scan for tables with > 10 000 entries.
    mutable std::unordered_map<std::string, geo::GeoRTree> rtrees_;
    // Per-table, per-PK MBR cache used to reconstruct SpatialResult after
    // R-tree query (avoids a per-PK RocksDB point lookup in the hot path).
    mutable std::unordered_map<std::string,
                               std::unordered_map<std::string, geo::MBR>> mbr_cache_;
    // Set of tables whose R-tree has been built (lazily or from writes).
    mutable std::unordered_set<std::string> rtree_built_;

    // ========================================================================
    // Thread Safety: Lock Hierarchy (Phase 3 A-5 Circular Lock Ordering)
    // ========================================================================
    //
    // LOCK HIERARCHY (prevents deadlocks via consistent acquisition order):
    //   Tier 1 (Global):    rtree_mutex_        ← Acquire FIRST
    //   Tier 2 (Partition): [reserved for future per-table locks]
    //   Tier 3 (Element):   [reserved for future per-entry locks]
    //
    // INVARIANT: All code paths must acquire locks in order Tier 1 → Tier 2 → Tier 3.
    //            Violating this order creates deadlock risk. ThreadSanitizer detects violations.
    //            See: https://github.com/google/sanitizers/wiki/ThreadSanitizerDeadlockDetector
    //
    // Mutex protecting rtrees_, mbr_cache_, and rtree_built_ for thread-safe
    // concurrent read (shared) and exclusive write (unique) access.
    mutable std::shared_mutex rtree_mutex_;  // Tier 1: Global R-tree lock

    // Lazily build the R-tree for `table` by scanning per-PK RocksDB keys.
    // No-op if already built.  Called automatically inside searchIntersects.
    void ensureRTree(std::string_view table) const;

    // Convert an MBR to a GeometryInfo (polygon) suitable for GeoRTree.
    static geo::GeometryInfo mbrToGeometryInfo(const geo::MBR& mbr);
    
    // RocksDB key prefixes
    std::string getSpatialKeyPrefix(std::string_view table) const;
    std::string getZRangeKeyPrefix(std::string_view table) const;
    std::string getConfigKey(std::string_view table) const;
    
    // Key construction
    std::string makeSpatialKey(std::string_view table, uint64_t morton_code) const;
    std::string makeZRangeKey(std::string_view table, int z_bucket) const;
    
    // Per-PK sidecar key (storage improvement)
    // Allows updating individual PKs without rewriting entire bucket JSON
    std::string makeSpatialPerPKKey(
        std::string_view table,
        uint64_t morton_code,
        std::string_view pk
    ) const;
    
    // Get/Set config
    std::optional<RTreeConfig> getConfig(std::string_view table) const;
    Status saveConfig(std::string_view table, const RTreeConfig& config);
    
    // Haversine distance (lat/lon in degrees)
    double haversineDistance(double lat1, double lon1, double lat2, double lon2) const;
    
    // Euclidean 3D distance
    double euclidean3DDistance(double x1, double y1, double z1, double x2, double y2, double z2) const;
    
    // Z-bucket for elevation indexing (10m buckets)
    int getZBucket(double z) const;
    
    // Parse sidecar entry from RocksDB value
    struct SidecarEntry {
        std::string primary_key;
        geo::GeoSidecar sidecar;
    };
    std::vector<SidecarEntry> parseSidecarList(const std::string& value) const;
    std::string serializeSidecarList(const std::vector<SidecarEntry>& entries) const;
};

}  // namespace index
}  // namespace themis
