/**
 * @file columnar_cache.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

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
namespace storage {

// ============================================================================
// SegmentKey
// ============================================================================

/**
 * @brief Compound key identifying one column segment.
 *
 * A segment covers rows `[segment_id * segment_rows, (segment_id+1) * segment_rows)`.
 */
struct SegmentKey {
    std::string table_name;   ///< Logical table or collection name.
    std::string column_name;  ///< Column name within the table.
    uint64_t    segment_id;   ///< Zero-based segment index.

    bool operator==(const SegmentKey& o) const noexcept {
        return table_name == o.table_name &&
               column_name == o.column_name &&
               segment_id  == o.segment_id;
    }
};

} // namespace storage
} // namespace themis

namespace std {
template<>
struct hash<themis::storage::SegmentKey> {
    size_t operator()(const themis::storage::SegmentKey& k) const noexcept {
        size_t h = std::hash<std::string>{}(k.table_name);
        h ^= std::hash<std::string>{}(k.column_name) + 0x9e3779b9u + (h << 6) + (h >> 2);
        h ^= std::hash<uint64_t>{}(k.segment_id)    + 0x9e3779b9u + (h << 6) + (h >> 2);
        return h;
    }
};
} // namespace std

namespace themis {
namespace storage {

// ============================================================================
// ColumnSegment
// ============================================================================

/**
 * @brief Typed value held inside a `ColumnSegment`.
 *
 * Only the active union member for the segment's `dtype` is valid.
 */
enum class SegmentDType : uint8_t {
    Int64,
    Double,
    String,
    Bool,
};

/**
 * @brief A contiguous typed column buffer for one segment of a table column.
 *
 * The buffers are plain `std::vector` objects and can be handed to
 * `analytics::Column::appendXxx()` without copying.
 *
 * `null_bitmap[i]` is `true` when row `i` within this segment is NULL.
 */
struct ColumnSegment {
    SegmentKey  key;
    SegmentDType dtype = SegmentDType::Int64;

    // Exactly one of the following is populated (matching `dtype`).
    std::vector<int64_t>     int64_data;
    std::vector<double>      double_data;
    std::vector<std::string> string_data;
    std::vector<bool>        bool_data;

    std::vector<bool> null_bitmap; ///< true = null; same length as data vector.

    uint64_t row_count = 0; ///< Number of rows in this segment.

    /// Estimated memory footprint in bytes.
    size_t byteSize() const noexcept;
};

// ============================================================================
// PinGuard
// ============================================================================

/**
 * @brief RAII pin guard returned by `ColumnarCache::get()`.
 *
 * While a `PinGuard` is alive the associated segment is guaranteed not to be
 * evicted.  Destroying (or releasing) the guard decrements the pin count.
 *
 * ### Example
 * @code
 * auto guard = cache.get(key);
 * if (guard) {
 *     process(guard.segment());
 * }   // pin released here
 * @endcode
 */
class ColumnarCache; // forward

/** @brief Raii guard for pin. */
class PinGuard {
public:
    PinGuard() noexcept = default;
    ~PinGuard() noexcept;

    PinGuard(PinGuard&&) noexcept;
    PinGuard& operator=(PinGuard&&) noexcept;

    PinGuard(const PinGuard&)            = delete;
    PinGuard& operator=(const PinGuard&) = delete;

    /** @brief Returns true when a valid segment is pinned. */
    explicit operator bool() const noexcept { return segment_ != nullptr; }

    /** @brief Access the pinned segment. UB if !*this. */
    const ColumnSegment& segment() const noexcept { return *segment_; }

    /** @brief Manually release the pin. After release operator bool() is false. */
    void release() noexcept;

private:
    friend class ColumnarCache;

    PinGuard(ColumnarCache* cache,
             SegmentKey     key,
             const ColumnSegment* seg) noexcept;

    ColumnarCache*       cache_   = nullptr;
    SegmentKey           key_;
    const ColumnSegment* segment_ = nullptr;
};

// ============================================================================
// ColumnarCache
// ============================================================================

/**
 * @brief In-memory LRU columnar segment cache.
 */
class ColumnarCache {
public:
    // -----------------------------------------------------------------------
    // Configuration
    // -----------------------------------------------------------------------

    struct Config {
        /// Maximum total bytes allowed in the cache (eviction threshold).
        size_t max_bytes = 256ull * 1024 * 1024; // 256 MB default

        /// Optional eviction callback invoked (under no lock) when a segment
        /// is evicted.  Useful for write-back policies or metrics.
        std::function<void(const SegmentKey&)> on_evict;

        Config() = default;
    };

    // -----------------------------------------------------------------------
    // Construction
    // -----------------------------------------------------------------------

    explicit ColumnarCache(Config config);

    /** @brief Construct with default configuration. */
    ColumnarCache() : ColumnarCache(Config{}) {}
    ~ColumnarCache() = default;

    ColumnarCache(const ColumnarCache&)            = delete;
    ColumnarCache& operator=(const ColumnarCache&) = delete;

    // -----------------------------------------------------------------------
    // Cache operations
    // -----------------------------------------------------------------------

    /**
     * @brief Insert or replace a segment.
     *
     * If inserting the segment would exceed `max_bytes`, unpinned segments
     * are evicted in LRU order until enough space is available.  If
     * insufficient unpinned space exists the segment is still inserted (the
     * cache temporarily exceeds `max_bytes` until pins are released).
     *
     * @param segment  The segment to insert.  Ownership is taken by copy.
     */
    void put(ColumnSegment segment);

    /**
     * @brief Retrieve and pin a segment.
     *
     * @param key   The segment key.
     * @return      A `PinGuard` wrapping the segment, or an empty guard if
     *              the key is not present.
     *
     * On a cache hit the segment is promoted to the MRU position.
     */
    PinGuard get(const SegmentKey& key);

    /**
     * @brief Returns `true` if the key is present in the cache.
     *
     * Does **not** update the LRU order or pin count.
     */
    bool contains(const SegmentKey& key) const noexcept;

    /**
     * @brief Explicitly evict a segment by key.
     *
     * Silently ignored if the segment is pinned or not present.
     * @return `true` if the segment was evicted, `false` otherwise.
     */
    bool evict(const SegmentKey& key);

    /**
     * @brief Evict all unpinned segments.
     */
    void clear();

    // -----------------------------------------------------------------------
    // Stats / introspection
    // -----------------------------------------------------------------------

    /** Total segments currently in the cache (pinned + unpinned). */
    size_t size() const noexcept;

    /** Number of pinned segments. */
    size_t pinnedCount() const noexcept;

    /** Total bytes currently used by cached segments. */
    size_t bytesUsed() const noexcept;

    /** Maximum bytes allowed by configuration. */
    size_t maxBytes() const noexcept { return cfg_.max_bytes; }

    /** Cache hit count since construction. */
    uint64_t hitCount() const noexcept;

    /** Cache miss count since construction. */
    uint64_t missCount() const noexcept;

private:
    friend class PinGuard;

    void decrementPin(const SegmentKey& key) noexcept;

    // Evict unpinned LRU segments until `bytes_used_ <= cfg_.max_bytes`.
    // Must be called under mu_.
    void evictLRU();

    // -----------------------------------------------------------------------
    // Internal entry
    // -----------------------------------------------------------------------

    struct Entry {
        ColumnSegment segment;
        int           pin_count = 0;
    };

    // LRU list: front = MRU, back = LRU.
    using LruList = std::list<SegmentKey>;

    mutable std::mutex mu_;

    Config cfg_;

    /// The actual cached entries.
    std::unordered_map<SegmentKey, Entry> store_;

    /// LRU ordering; each key appears at most once.
    LruList lru_list_;

    /// Fast lookup: key → iterator in lru_list_.
    std::unordered_map<SegmentKey, LruList::iterator> lru_map_;

    size_t   bytes_used_{0};
    uint64_t hit_count_{0};
    uint64_t miss_count_{0};
};

} // namespace storage
} // namespace themis
