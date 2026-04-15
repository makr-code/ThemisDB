/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            hkdf_cache.h                                       ║
  Version:         0.0.45                                             ║
  Last Modified:   2026-04-15 07:10:30                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     114                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 15a0bb6700  2026-03-09  feat(utils): add BloomFilter, ConsistentHashRing, RateLim... ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// Thread-local HKDF LRU cache with TTL eviction and sharded mutexes
#pragma once

#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace themis {
namespace utils {

// ---------------------------------------------------------------------------
// Configuration (defined outside HKDFCache to avoid nested-class default-
// initializer ordering issues on some GCC versions)
// ---------------------------------------------------------------------------
struct HKDFCacheConfig {
    size_t max_entries = 1000;
    std::chrono::seconds ttl{300}; ///< 5 min default; 0 = no expiry
};

// ---------------------------------------------------------------------------
// Stats
// ---------------------------------------------------------------------------
struct HKDFCacheStats {
    size_t hits      = 0;
    size_t misses    = 0;
    size_t evictions = 0;
};

class HKDFCache {
public:
    // Expose config/stats types via aliases so call sites can use
    // HKDFCache::Config and HKDFCache::Stats as the spec requires.
    using Config = HKDFCacheConfig;
    using Stats  = HKDFCacheStats;

    // ---------------------------------------------------------------------------
    // Construction
    // ---------------------------------------------------------------------------

    /// Default: Config{} — 1 000 entries, 5-minute TTL
    explicit HKDFCache(Config cfg = Config{});
    ~HKDFCache();

    /// Return a thread-local instance (preferred by callers in code/tests)
    static HKDFCache& threadLocal();

    // ---------------------------------------------------------------------------
    // Core API
    // ---------------------------------------------------------------------------

    /// Derive with cache: ikm, salt, info, output_length
    std::vector<uint8_t> derive_cached(const std::vector<uint8_t>& ikm,
                                       const std::vector<uint8_t>& salt,
                                       const std::string& info,
                                       size_t output_length);

    /// Clear all entries across all shards
    void clear();

    /// Configure capacity (applied per-shard; total ≈ cap)
    void setCapacity(size_t cap);

    // ---------------------------------------------------------------------------
    // Extended API
    // ---------------------------------------------------------------------------

    /**
     * @brief Purge all entries whose cache key was derived from a specific IKM.
     *
     * @param ikm_hash  SHA-256 hex string of the IKM bytes.  Only entries
     *                  whose raw key bytes start with the matching IKM are
     *                  removed, allowing selective invalidation when a root
     *                  key is rotated.
     */
    void purge_by_ikm_hash(const std::string& ikm_hash);

    /// Snapshot of cumulative hit/miss/eviction counters.
    Stats stats() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace utils
} // namespace themis
