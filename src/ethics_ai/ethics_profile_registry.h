/**
 * @file ethics_profile_registry.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

class EthicsProfileRegistry final : public IEthicsProfileRegistry {
public:
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

    /**
     * @brief Lru Put.
     * @param[in] id Input parameter.
     * @param[in] profile Input parameter.
     */
    void lruPut(const std::string& id, const PhilosophyProfile& profile);
    /**
     * @brief Lru Get.
     * @param[in] id Input parameter.
     * @return Pointer to the result.
     */
    const PhilosophyProfile* lruGet(const std::string& id); // nullptr = miss
    /**
     * @brief Lru Evict.
     */
    void lruEvict(); // remove LRU entry

    /**
     * @brief ── Internal ─────────────────────────────────────────────────────────────
     * @param[in] filepath Input parameter.
     * @return Return value.
     */
    static EthicsProfileMeta scanHeader(const std::string& filepath);

    mutable std::mutex mutex_;

    std::map<std::string, EthicsProfileMeta> index_;

    LruList lru_list_;
    LruMap  lru_map_;
    size_t  lru_capacity_;

    PhilosophyLoader loader_;
};

} // namespace ethics
} // namespace plugins
} // namespace themis
