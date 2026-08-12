/**
 * @file graph_query_cache.h
 * @brief Multi-tier LRU graph query result cache (P3-02).
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=0
 * @note Status: Production Ready
 */


#pragma once

#include <cstddef>
#include <chrono>
#include <list>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace themis {
namespace graph {

/**
 * @brief Multi-tier LRU graph query result cache.
 *
 * Provides a two-tier caching strategy for graph query results:
 *
 * - **L1 (hot tier)**: Small, fast in-memory LRU cache for recently accessed
 *   results.  Default capacity: 64 entries.  Pure LRU eviction.
 *
 * - **L2 (warm tier)**: Larger capacity LRU cache with cost-weighted eviction.
 *   Results with a higher @p cost_hint (expensive to recompute) are retained
 *   longer than cheap results of similar age.  Default capacity: 512 entries.
 *   Entries are promoted from L2 to L1 on each cache hit.
 *
 * **Thread-safety**: all public methods are thread-safe via an internal mutex.
 *
 * **Cache entries** store the query key, a vector of result strings, the
 * associated cost hint, and the insertion timestamp.  TTL is optional: when
 * set to zero (default) entries never expire.
 *
 * **Eviction**:
 *   - L1 uses strict LRU (least recently used is evicted when full).
 *   - L2 uses weighted eviction: victim score = recency_weight / cost_hint.
 *     The entry with the lowest score is evicted first, favouring retention of
 *     high-cost, recently accessed results.
 *
 * @note The cache is intentionally header-declared with implementation in
 *       src/graph/graph_query_cache.cpp to keep the public API surface
 *       stable and the ABI boundary clean.
 */
class GraphQueryCache {
public:
    /// Opaque result type stored per cache entry.
    using ResultSet = std::vector<std::string>;

    // -----------------------------------------------------------------------
    // Configuration
    // -----------------------------------------------------------------------

    /**
     * @brief Construction-time configuration for the two-tier cache.
     */
    struct Config {
        /// Maximum number of entries in the L1 (hot) tier.  Must be > 0.
        size_t l1_capacity{64};

        /// Maximum number of entries in the L2 (warm) tier.  Must be > l1_capacity.
        size_t l2_capacity{512};

        /// Per-entry TTL.  Zero means entries never expire.
        std::chrono::milliseconds ttl{0};

        /// Default cost hint used when the caller does not supply one.
        /// Must be > 0.0.
        double default_cost{1.0};
    };

    // -----------------------------------------------------------------------
    // Observability
    // -----------------------------------------------------------------------

    /**
     * @brief Cumulative statistics for the cache instance.
     *
     * All counters are accumulated since construction and are never reset by
     * normal cache operations.  Call @ref resetStats to zero them explicitly.
     */
    struct Stats {
        /// Total number of successful cache lookups (L1 + L2 combined).
        size_t hits{0};
        /// Total number of cache misses.
        size_t misses{0};
        /// Number of hits served from the L1 (hot) tier.
        size_t l1_hits{0};
        /// Number of hits served from the L2 (warm) tier that were then
        /// promoted to L1.
        size_t l2_promotions{0};
        /// Total number of evictions across both tiers.
        size_t evictions{0};
        /// Current number of entries in the L1 tier.
        size_t l1_size{0};
        /// Current number of entries in the L2 tier.
        size_t l2_size{0};

        /// Cache hit ratio in [0.0, 1.0].  Returns 0.0 when no lookups have
        /// been performed yet.
        [[nodiscard]] double hitRatio() const noexcept {
            const size_t total = hits + misses;
            return total == 0 ? 0.0 : static_cast<double>(hits) / static_cast<double>(total);
        }
    };

    // -----------------------------------------------------------------------
    // Construction / destruction
    // -----------------------------------------------------------------------

    /**
     * @brief Construct a cache with the given configuration.
     *
     * @param config  Cache configuration.  l1_capacity must be > 0 and
     *                l2_capacity must be > l1_capacity.
     * @throws std::invalid_argument if the configuration violates the capacity
     *         invariants or default_cost is non-positive.
     */
    explicit GraphQueryCache(Config config = Config{});

    GraphQueryCache(const GraphQueryCache&) = delete;
    GraphQueryCache& operator=(const GraphQueryCache&) = delete;
    GraphQueryCache(GraphQueryCache&&) = delete;
    GraphQueryCache& operator=(GraphQueryCache&&) = delete;

    ~GraphQueryCache() = default;

    // -----------------------------------------------------------------------
    // Core cache operations
    // -----------------------------------------------------------------------

    /**
     * @brief Insert or update a result set in the cache.
     *
     * The entry is always placed in L1 first.  If L1 is full the current LRU
     * victim in L1 is demoted to L2 (unless L2 is also full, in which case
     * the lowest-scoring L2 entry is evicted first).
     *
     * @param key        Unique query key.  Must be non-empty.
     * @param result     Result set to cache.
     * @param cost_hint  Estimated cost to recompute this result; used by the
     *                   weighted eviction policy in L2.  Must be > 0.0; if
     *                   ≤ 0 the @ref Config::default_cost is used instead.
     */
    void put(const std::string& key, ResultSet result, double cost_hint = 0.0);

    /**
     * @brief Look up a result set by key.
     *
     * Checks L1 first; on L1 miss, checks L2 and promotes the entry to L1 on
     * hit.  Updates hit/miss statistics unconditionally.
     *
     * @param key  Query key to look up.
     * @return     The cached result set, or std::nullopt on miss or TTL expiry.
     */
    [[nodiscard]] std::optional<ResultSet> get(const std::string& key);

    /**
     * @brief Invalidate a specific cache entry across both tiers.
     *
     * No-op if the key is not present.
     *
     * @param key  Query key to invalidate.
     */
    void invalidate(const std::string& key);

    /**
     * @brief Evict all entries from both tiers and reset size counters.
     *
     * Cumulative statistics (@ref Stats) are NOT reset.  Call
     * @ref resetStats separately if needed.
     */
    void clear();

    // -----------------------------------------------------------------------
    // Observability
    // -----------------------------------------------------------------------

    /**
     * @brief Return a snapshot of the current statistics.
     *
     * The snapshot is consistent (taken under the internal mutex).
     */
    [[nodiscard]] Stats getStats() const;

    /**
     * @brief Reset all cumulative counters to zero.
     *
     * Cache contents are preserved.
     */
    void resetStats();

    // -----------------------------------------------------------------------
    // Configuration inspection
    // -----------------------------------------------------------------------

    /// Returns the L1 tier capacity.
    [[nodiscard]] size_t l1Capacity() const noexcept { return config_.l1_capacity; }

    /// Returns the L2 tier capacity.
    [[nodiscard]] size_t l2Capacity() const noexcept { return config_.l2_capacity; }

    /// Returns the configured TTL (zero = no expiry).
    [[nodiscard]] std::chrono::milliseconds ttl() const noexcept { return config_.ttl; }

private:
    // -----------------------------------------------------------------------
    // Internal entry type
    // -----------------------------------------------------------------------

    struct Entry {
        ResultSet    result;
        double       cost{1.0};
        std::chrono::steady_clock::time_point inserted_at;
    };

    // LRU list node: the key stored in the ordering list.
    using LruList = std::list<std::string>;

    // -----------------------------------------------------------------------
    // L1 tier: pure LRU
    // -----------------------------------------------------------------------

    struct L1Tier {
        std::unordered_map<std::string,
                           std::pair<Entry, LruList::iterator>> map;
        LruList lru;   ///< Front = MRU, back = LRU victim.
    };

    // -----------------------------------------------------------------------
    // L2 tier: cost-weighted LRU
    // -----------------------------------------------------------------------

    struct L2Entry {
        Entry        entry;
        LruList::iterator lru_it;
    };

    struct L2Tier {
        std::unordered_map<std::string, L2Entry> map;
        LruList lru;   ///< Used for recency ordering; eviction score combines cost + recency.
    };

    // -----------------------------------------------------------------------
    // Helpers (called with mutex_ held)
    // -----------------------------------------------------------------------

    /// Returns true when the entry has exceeded the configured TTL.
    [[nodiscard]] bool isExpired(const Entry& e) const noexcept;

    /// Evict the LRU entry from L1 and demote it to L2.  Evicts L2's
    /// lowest-score entry first if L2 is full.
    void evictL1ToL2();

    /// Evict the lowest-score entry from L2.
    void evictL2();

    /// Select the L2 eviction victim key.  Uses weighted score:
    ///   score = recency_weight / cost
    /// where recency_weight decays as the entry ages.
    [[nodiscard]] std::string selectL2Victim() const;

    /// Remove an entry from L1 without demoting it to L2.
    void removeFromL1(const std::string& key);

    /// Remove an entry from L2.
    void removeFromL2(const std::string& key);

    // -----------------------------------------------------------------------
    // State
    // -----------------------------------------------------------------------

    Config   config_;
    L1Tier   l1_;
    L2Tier   l2_;
    Stats    stats_;
    mutable std::mutex mutex_;
};

} // namespace graph
} // namespace themis
