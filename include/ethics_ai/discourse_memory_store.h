/**
 * @file discourse_memory_store.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "ethics_ai/ethics_ai_types.h"
#include <map>
#include <mutex>
#include <string>
#include <vector>

namespace themis {
namespace plugins {
namespace ethics {

struct DiscourseMemoryConfig {
    int   max_episodes_per_school{5};   ///< Ring buffer size per school (MemGPT §4.1)
    int   max_tokens_per_episode{50};   ///< Hard cap on compressed_position tokens
    float min_dc_score_to_store{0.0f};  ///< Only store episodes with DC >= this value
};

class DiscourseMemoryStore {
public:
    explicit DiscourseMemoryStore(
        DiscourseMemoryConfig config = DiscourseMemoryConfig{});

    /**
     * @brief Store Episode.
     * @param[in] output Input parameter.
     */
    void storeEpisode(const DiscourseRoundOutput& output);

    /**
     * @brief Store Episode.
     * @param[in] entry Input parameter.
     */
    void storeEpisode(const EpisodicMemoryEntry& entry);

    std::vector<EpisodicMemoryEntry> getEpisodesForSchool(
        const std::string& school_id,
        int max_episodes = 3) const;

    std::string buildEpisodicContext(
        const std::string& school_id,
        int max_episodes = 3) const;

    std::map<std::string, std::string> buildAllEpisodicContexts(
        const std::vector<std::string>& school_ids,
        int max_episodes = 3) const;

    /**
     * @brief Clear.
     */
    void clear();

    /**
     * @brief Episode Count.
     * @param[in] school_id Identifier of the school.
     * @return Return value.
     */
    size_t episodeCount(const std::string& school_id) const;

    const DiscourseMemoryConfig& config() const noexcept { return config_; }

private:
    DiscourseMemoryConfig config_;
    mutable std::mutex    mutex_;
    // school_id → ring buffer (newest at back)
    std::map<std::string, std::vector<EpisodicMemoryEntry>> episodes_;

    /**
     * @brief Count Tokens.
     * @param[in] text Input parameter.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    static int         countTokens(const std::string& text) noexcept;
    /**
     * @brief Compress Position.
     * @param[in] position_abstract Input parameter.
     * @param[in] max_tokens Input parameter.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    static std::string compressPosition(
        const std::string& position_abstract,
        int max_tokens) noexcept;
};

} // namespace ethics
} // namespace plugins
} // namespace themis
