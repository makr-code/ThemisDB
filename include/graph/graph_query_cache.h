/**
 * @file graph_query_cache.h
 * @brief Multi-tier LRU graph query result cache (P3-02).
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
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

class GraphQueryCache {
public:
    using ResultSet = std::vector<std::string>;

    // -----------------------------------------------------------------------
    // Configuration
    // -----------------------------------------------------------------------

    struct Config {
        size_t l1_capacity{64};

        size_t l2_capacity{512};

        std::chrono::milliseconds ttl{0};

        double default_cost{1.0};
    };

    // -----------------------------------------------------------------------
    // Observability
    // -----------------------------------------------------------------------

    struct Stats {
        size_t hits{0};
        size_t misses{0};
        size_t l1_hits{0};
        size_t l2_promotions{0};
        size_t evictions{0};
        size_t l1_size{0};
        size_t l2_size{0};

        [[nodiscard]] double hitRatio() const noexcept {
            const size_t total = hits + misses;
            return total == 0 ? 0.0 : static_cast<double>(hits) / static_cast<double>(total);
        }
    };

    // -----------------------------------------------------------------------
    // Construction / destruction
    // -----------------------------------------------------------------------

    /**
     * @brief Graph Query Cache.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit GraphQueryCache(Config config);

    GraphQueryCache(const GraphQueryCache&) = delete;
    GraphQueryCache& operator=(const GraphQueryCache&) = delete;
    GraphQueryCache(GraphQueryCache&&) = delete;
    GraphQueryCache& operator=(GraphQueryCache&&) = delete;

    ~GraphQueryCache() = default;

    // -----------------------------------------------------------------------
    // Core cache operations
    // -----------------------------------------------------------------------

    void put(const std::string& key, ResultSet result, double cost_hint = 0.0);

    [[nodiscard]] std::optional<ResultSet> get(const std::string& key);

    /**
     * @brief Invalidate.
     * @param[in] key Input parameter.
     */
    void invalidate(const std::string& key);

    /**
     * @brief Clear.
     */
    void clear();

    // -----------------------------------------------------------------------
    // Observability
    // -----------------------------------------------------------------------

    [[nodiscard]] Stats getStats() const;

    /**
     * @brief Reset Stats.
     */
    void resetStats();

    // -----------------------------------------------------------------------
    // Configuration inspection
    // -----------------------------------------------------------------------

    [[nodiscard]] size_t l1Capacity() const noexcept { return config_.l1_capacity; }

    [[nodiscard]] size_t l2Capacity() const noexcept { return config_.l2_capacity; }

    [[nodiscard]] std::chrono::milliseconds ttl() const noexcept { return config_.ttl; }

private:
    // -----------------------------------------------------------------------
    // Internal entry type
    // -----------------------------------------------------------------------

    struct Entry {
        ResultSet    result{};
        double       cost{1.0};
        std::chrono::steady_clock::time_point inserted_at{};
    };

    // LRU list node: the key stored in the ordering list.
    using LruList = std::list<std::string>;

    // -----------------------------------------------------------------------
    // L1 tier: pure LRU
    // -----------------------------------------------------------------------

    struct L1Tier {
        std::unordered_map<std::string,
                           std::pair<Entry, LruList::iterator>> map{};
        LruList lru{};   ///< Front = MRU, back = LRU victim.
    };

    // -----------------------------------------------------------------------
    // L2 tier: cost-weighted LRU
    // -----------------------------------------------------------------------

    struct L2Entry {
        Entry        entry{};
        LruList::iterator lru_it{};
    };

    struct L2Tier {
        std::unordered_map<std::string, L2Entry> map{};
        LruList lru{};   ///< Used for recency ordering; eviction score combines cost + recency.
    };

    // -----------------------------------------------------------------------
    // Helpers (called with mutex_ held)
    // -----------------------------------------------------------------------

    [[nodiscard]] bool isExpired(const Entry& e) const noexcept;

    /**
     * @brief Evict L1 To L2.
     */
    void evictL1ToL2();

    /**
     * @brief Evict L2.
     */
    void evictL2();

    [[nodiscard]] std::string selectL2Victim() const;

    /**
     * @brief Remove From L1.
     * @param[in] key Input parameter.
     */
    void removeFromL1(const std::string& key);

    /**
     * @brief Remove From L2.
     * @param[in] key Input parameter.
     */
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
