/**
 * @file advanced_cache_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Copyright 2026 ThemisDB
// Licensed under MIT License

#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <list>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace themis {
namespace performance {

// ============================================================================
// AdvancedCacheManager — Issue #229 (v1.9.0)
// ============================================================================

/**
 * @brief Eviction policy for a cache partition.
 */
enum class EvictionPolicy {
    LRU,   ///< Least Recently Used
    LIRS,  ///< Low Inter-reference Recency Set (scan-resistant)
    ARC,   ///< Adaptive Replacement Cache
    TwoQ   ///< Two-Queue (2Q)
};

/**
 * @brief Compression algorithm for cold partitions.
 */
enum class CompressionAlgorithm {
    None,
    LZ4,
    Snappy,
    Zstd
};

/**
 * @brief Configuration for a single cache partition.
 */
struct CachePartition {
    std::string        name;
    size_t             size_mb           = 64;
    EvictionPolicy     policy            = EvictionPolicy::LRU;
    bool               enable_compression = false;
    CompressionAlgorithm compression     = CompressionAlgorithm::LZ4;
};

/**
 * @brief Top-level cache configuration.
 */
struct CacheConfig {
    size_t                     total_size_mb      = 256;
    std::vector<CachePartition> partitions;
    bool                       enable_bloom_filters = true;
    double                     bloom_filter_fp_rate = 0.01;
};

/**
 * @brief Per-partition hit/miss statistics.
 */
struct PartitionStats {
    size_t hits              = 0;
    size_t misses            = 0;
    double hit_rate          = 0.0;
    size_t entries           = 0;
    size_t bytes_used        = 0;
    double compression_ratio = 1.0;
};

/**
 * @brief Multi-level cache manager with partitioning and bloom-filter
 *        pre-screening (v1.9.0, Issue #229).
 *
 * Research basis: Multiple cache optimization papers.
 *
 * Features:
 *  - Cache Partitioning:        hot / cold / metadata regions with independent
 *                               eviction policies.
 *  - Cache-Oblivious Scan:      template helper that processes an iterator
 *                               range in small tiles to improve cache reuse.
 *  - Bloom Filter Pre-Screening: optional per-partition Bloom filter that
 *                               avoids polluting the partition for definite
 *                               misses.
 *  - Adaptive Eviction:         LRU (default) or LRU-2 approximation.
 *  - Cache Compression:         transparent value compression for cold
 *                               partitions (stubbed codec layer).
 *
 * Performance targets:
 *  - Hit rate:         +15–25 % vs single-partition cache
 *  - Memory efficiency: +30–50 % with compression on cold partition
 *
 * Thread safety: all public methods are thread-safe.
 */
class AdvancedCacheManager {
public:
    using CompressFn = std::function<std::string(const std::string&, CompressionAlgorithm)>;
    using DecompressFn = std::function<std::string(const std::string&, CompressionAlgorithm)>;

    // =========================================================================
    // Construction
    // =========================================================================

    AdvancedCacheManager();
    explicit AdvancedCacheManager(const CacheConfig& config);
    ~AdvancedCacheManager();

    AdvancedCacheManager(const AdvancedCacheManager&)            = delete;
    AdvancedCacheManager& operator=(const AdvancedCacheManager&) = delete;
    AdvancedCacheManager(AdvancedCacheManager&&)                 = default;
    AdvancedCacheManager& operator=(AdvancedCacheManager&&)      = default;

    // =========================================================================
    // Partition management
    // =========================================================================

    /**
     * @brief (Re)create partitions from a CacheConfig.
     *
     * Discards all existing entries and bloom-filter state.
     */
    void create_partitions(const CacheConfig& config);

    /**
     * @brief Return the names of all registered partitions.
     */
    std::vector<std::string> partition_names() const;

    // =========================================================================
    // Core get / put API
    // =========================================================================

    /**
     * @brief Look up key in the named partition.
     *
     * @return The cached value, or std::nullopt on a miss.
     */
    std::optional<std::string> get(const std::string& key,
                                   const std::string& partition);

    /**
     * @brief Insert or update key in the named partition.
     *
     * Evicts the least-recently-used entry when the partition is full.
     * When bloom-filter pre-screening is enabled, the key is added to the
     * partition's Bloom filter.
     */
    void put(const std::string& key,
             const std::string& value,
             const std::string& partition);

    /**
     * @brief Remove key from the named partition.
     *
     * @return true if the key was present and removed.
     */
    bool evict(const std::string& key, const std::string& partition);

    /**
     * @brief Check whether key is in the named partition without modifying
     *        LRU state.
     */
    bool contains(const std::string& key, const std::string& partition) const;

    // =========================================================================
    // Cache-oblivious scan helper
    // =========================================================================

    /**
     * @brief Apply func to each element in [begin, end) in cache-friendly
     *        tiles of tile_size elements.
     *
     * Improves spatial locality for large sequential scans by processing
     * kTileSize = 64 elements per tile.
     */
    template <typename Iterator, typename Func>
    static void cache_oblivious_scan(Iterator begin, Iterator end, Func func,
                                     size_t tile_size = 64) {
        for (auto it = begin; it != end; ) {
            auto tile_end = it;
            for (size_t i = 0; i < tile_size && tile_end != end; ++i, ++tile_end) {}
            for (auto t = it; t != tile_end; ++t) {
                func(*t);
            }
            it = tile_end;
        }
    }

    // =========================================================================
    // Statistics
    // =========================================================================

    /** @brief Return per-partition statistics. */
    PartitionStats get_partition_stats(const std::string& partition) const;

    /** @brief Reset all statistics counters without flushing cached data. */
    void reset_stats();

    /** @brief Flush all entries from a single partition. */
    void flush_partition(const std::string& partition);

    /** @brief Flush all partitions. */
    void flush_all();

    // =========================================================================
    // Configuration accessor
    // =========================================================================

    const CacheConfig& config() const noexcept { return config_; }

    static void setCompressFn(CompressFn fn);
    static void setDecompressFn(DecompressFn fn);

private:
    // ── Bloom filter (simple bit-array, k=3 hash functions) ──────────────────
    struct BloomFilter {
        static constexpr size_t kBits = 1 << 16;  // 8 KB
        uint64_t bits[kBits / 64]{};

        void insert(const std::string& key) noexcept;
        bool maybe_contains(const std::string& key) const noexcept;
        void clear() noexcept;

    private:
        static uint64_t hash(const std::string& key, uint64_t seed) noexcept;
    };

    // ── LRU partition entry ───────────────────────────────────────────────────
    struct Entry {
        std::string key;
        std::string value;  ///< raw (or compressed when partition.enable_compression)
    };

    struct PartitionState {
        CachePartition           cfg;
        size_t                   capacity = 0;  ///< max entries derived from size_mb
        std::list<Entry>         lru_list;       ///< front = most recent
        std::unordered_map<std::string, std::list<Entry>::iterator> index;
        BloomFilter              bloom;
        PartitionStats           stats;
        mutable std::mutex       mtx;
    };

    CacheConfig config_;
    std::vector<std::unique_ptr<PartitionState>> partitions_;

    PartitionState* find_partition(const std::string& name) const noexcept;

    static size_t entries_for_mb(size_t mb) noexcept;
    static std::string compress(const std::string& val,
                                CompressionAlgorithm algo);
    static std::string decompress(const std::string& val,
                                  CompressionAlgorithm algo);
};

}  // namespace performance
}  // namespace themis
