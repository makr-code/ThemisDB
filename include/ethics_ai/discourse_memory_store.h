/**
 * @file discourse_memory_store.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/**
 * @brief Configuration for the discourse memory store.
 */
struct DiscourseMemoryConfig {
    int   max_episodes_per_school{5};   ///< Ring buffer size per school (MemGPT §4.1)
    int   max_tokens_per_episode{50};   ///< Hard cap on compressed_position tokens
    float min_dc_score_to_store{0.0f};  ///< Only store episodes with DC >= this value
};

/**
 * @brief Episodic memory store for multi-school discourse rounds.
 *
 * Implements §12.2.4 Multi-Agent-Memory-Externalisierung:
 *  - Stores EpisodicMemoryEntry per school after each round (Reflexion [R2] §3).
 *  - Retrieves up to N episodes per school for context injection (MemGPT [R9] §4.1).
 *  - Auto-compresses position_abstract to max_tokens_per_episode (AutoCompressor [R15]).
 *
 * Token savings: ~1 600 tokens (full prior text) → ~150 tokens (3 episodes × 50 T) = −91 %.
 *
 * Thread-safe: all operations protected by internal mutex.
 */
class DiscourseMemoryStore {
public:
    explicit DiscourseMemoryStore(
        DiscourseMemoryConfig config = DiscourseMemoryConfig{});

    /**
     * @brief Store an episode from a completed discourse round.
     *
     * Builds an EpisodicMemoryEntry from the DiscourseRoundOutput:
     *  - compressed_position: truncated position_abstract (≤ max_tokens_per_episode)
     *  - dc_score: output.confidence (proxy for DC)
     *  - strongest_tension: output.primary_rebuttal_of
     *
     * If the ring buffer for this school is full, oldest entry is evicted.
     *
     * @param output  The round output to store as an episode.
     */
    void storeEpisode(const DiscourseRoundOutput& output);

    /**
     * @brief Store an episode with explicit fields (for testing).
     *
     * @param entry  The entry to store directly.
     */
    void storeEpisode(const EpisodicMemoryEntry& entry);

    /**
     * @brief Retrieve episodes for a school, newest-first.
     *
     * @param school_id    The school to retrieve episodes for.
     * @param max_episodes Maximum number of episodes to return (default: 3).
     * @return Up to max_episodes episodes, most recent first.
     */
    std::vector<EpisodicMemoryEntry> getEpisodesForSchool(
        const std::string& school_id,
        int max_episodes = 3) const;

    /**
     * @brief Build an injected context string from stored episodes.
     *
     * Concatenates episodes into a compact preamble for R3+ prompt injection.
     * Format per episode:
     *   "[{school} R{round}] DC={dc:.2f} {compressed_position}"
     *
     * Total tokens guaranteed ≤ max_episodes * max_tokens_per_episode.
     *
     * @param school_id    School whose episodes to inject.
     * @param max_episodes Maximum episodes to include (default: 3).
     * @return Assembled injection string.
     */
    std::string buildEpisodicContext(
        const std::string& school_id,
        int max_episodes = 3) const;

    /**
     * @brief Build episodic context for all schools in one pass.
     *
     * @param school_ids   Schools to include.
     * @param max_episodes Per-school episode count.
     * @return Map from school_id to its injection string.
     */
    std::map<std::string, std::string> buildAllEpisodicContexts(
        const std::vector<std::string>& school_ids,
        int max_episodes = 3) const;

    /**
     * @brief Clear all stored episodes (call between debate sessions).
     */
    void clear();

    /**
     * @brief Return the number of stored episodes for a school.
     */
    size_t episodeCount(const std::string& school_id) const;

    const DiscourseMemoryConfig& config() const noexcept { return config_; }

private:
    DiscourseMemoryConfig config_;
    mutable std::mutex    mutex_;
    // school_id → ring buffer (newest at back)
    std::map<std::string, std::vector<EpisodicMemoryEntry>> episodes_;

    static int         countTokens(const std::string& text) noexcept;
    static std::string compressPosition(
        const std::string& position_abstract,
        int max_tokens) noexcept;
};

} // namespace ethics
} // namespace plugins
} // namespace themis
