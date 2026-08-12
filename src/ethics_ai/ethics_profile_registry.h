/**
 * @file ethics_profile_registry.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "ethics_ai/ethics_profile_registry.h"
#include "philosophy_loader.h"

#include <list>
#include <mutex>
#include <unordered_map>
#include <map>

namespace themis {
namespace plugins {
namespace ethics {

/**
 * @brief Concrete implementation of IEthicsProfileRegistry.
 *
 * Design:
 *  - A lightweight `EthicsProfileMeta` index (always in RAM, ~500 B/profile)
 *    is built by scanning YAML header fields only (no full profile parse).
 *  - Full `PhilosophyProfile` objects are loaded on first access and cached
 *    in a bounded LRU cache (default capacity: 20 warm profiles).
 *  - Thread-safety: a single `std::mutex` guards both the index and cache.
 *
 * @note When yaml-cpp is not available the index scan falls back to a
 *       filename-only mode (school_id from filename, all other meta empty).
 */
class EthicsProfileRegistry final : public IEthicsProfileRegistry {
public:
    /**
     * @param lru_capacity  Maximum number of full profiles kept warm in
     *                      the LRU cache.  Must be ≥ 1.
     */
    explicit EthicsProfileRegistry(size_t lru_capacity = 20);
    ~EthicsProfileRegistry() override = default;

    // IEthicsProfileRegistry
    std::vector<EthicsProfileMeta> queryIndex(
        const EthicsIndexQuery& query) const override;

    std::variant<PhilosophyProfile, Status> getProfile(
        const std::string& school_id) override;

    std::variant<size_t, Status> rebuildIndex(
        const std::string& directory) override;

    size_t indexSize() const override;
    bool   hasProfile(const std::string& school_id) const override;

private:
    // ── LRU cache helpers ─────────────────────────────────────────────────────
    // Cache entry list: front = most-recently used
    using LruList = std::list<std::pair<std::string, PhilosophyProfile>>;
    using LruMap  = std::unordered_map<std::string, LruList::iterator>;

    void lruPut(const std::string& id, const PhilosophyProfile& profile);
    const PhilosophyProfile* lruGet(const std::string& id); // nullptr = miss
    void lruEvict(); // remove LRU entry

    // ── Internal ─────────────────────────────────────────────────────────────
    /// Parse only header fields from a YAML file into EthicsProfileMeta.
    /// Returns an empty meta (school_id derived from filename) on error.
    static EthicsProfileMeta scanHeader(const std::string& filepath);

    mutable std::mutex mutex_;

    /// Metadata index: school_id → meta
    std::map<std::string, EthicsProfileMeta> index_;

    /// LRU cache
    LruList lru_list_;
    LruMap  lru_map_;
    size_t  lru_capacity_;

    /// Reused loader for full-profile parsing
    PhilosophyLoader loader_;
};

} // namespace ethics
} // namespace plugins
} // namespace themis
