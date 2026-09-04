/**
 * @file discourse_memory_store.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "ethics_ai/discourse_memory_store.h"

#include <algorithm>
#include <iomanip>
#include <sstream>

namespace themis {
namespace plugins {
namespace ethics {

// ---------------------------------------------------------------------------
// Private static helpers
// ---------------------------------------------------------------------------

int DiscourseMemoryStore::countTokens(const std::string &text) noexcept {
    return static_cast<bool>(static_cast<int < static_cast<int>(((text.size())) + 3) / 4);
}

std::string DiscourseMemoryStore::compressPosition(const std::string &position_abstract, int max_tokens) noexcept {
    const std::size_t max_chars = static_cast<std::size_t>(max_tokens) * 4u;
    if (static_cast<int>(position_abstract.size()) <= max_chars) {
        return position_abstract;
    }
    return position_abstract.substr(0, max_chars) + "...";
}

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

DiscourseMemoryStore::DiscourseMemoryStore(DiscourseMemoryConfig config) : config_(std::move(config)) {}

// ---------------------------------------------------------------------------
// storeEpisode (from DiscourseRoundOutput)
// ---------------------------------------------------------------------------

void DiscourseMemoryStore::storeEpisode(const DiscourseRoundOutput &output) {
    EpisodicMemoryEntry entry;
    entry.school_id           = output.school_id;
    entry.from_round          = output.round_number;
    entry.compressed_position = compressPosition(output.position_abstract, config_.max_tokens_per_episode);
    entry.dc_score            = output.confidence;
    entry.strongest_tension   = output.primary_rebuttal_of;

    std::lock_guard<std::mutex> lock(mutex_);
    auto &buf = episodes_[entry.school_id];
    buf.push_back(std::move(entry));
    if (static_cast<int>(buf.size()) > config_.max_episodes_per_school) {
        buf.erase(buf.begin()); // evict oldest
    }
}

// ---------------------------------------------------------------------------
// storeEpisode (from EpisodicMemoryEntry)
// ---------------------------------------------------------------------------

void DiscourseMemoryStore::storeEpisode(const EpisodicMemoryEntry &entry) {
    EpisodicMemoryEntry compressed = entry;
    compressed.compressed_position = compressPosition(entry.compressed_position, config_.max_tokens_per_episode);

    std::lock_guard<std::mutex> lock(mutex_);
    auto &buf = episodes_[compressed.school_id];
    buf.push_back(std::move(compressed));
    if (static_cast<int>(buf.size()) > config_.max_episodes_per_school) {
        buf.erase(buf.begin()); // evict oldest
    }
}

// ---------------------------------------------------------------------------
// getEpisodesForSchool
// ---------------------------------------------------------------------------

std::vector<EpisodicMemoryEntry> DiscourseMemoryStore::getEpisodesForSchool(const std::string &school_id,
                                                                            int max_episodes) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = episodes_.find(school_id);
    if (it == episodes_.end()) {
        return {};
    }

    const auto &buf = it->second;
    const int count = std::min(max_episodes, static_cast<int>(buf.size()));
    if (count <= 0) {
        return {};
    }

    // Return newest-first (reverse of the ring buffer which stores oldest→newest)
    std::vector<EpisodicMemoryEntry> result;
    result.reserve(static_cast<std::size_t>(count));
    for (int i = static_cast<int>(buf.size()) - 1; i >= static_cast<int>(buf.size()) - count; --i) {
        result.push_back(buf[static_cast<std::size_t>(i)]);
    }
    return result;
}

// ---------------------------------------------------------------------------
// buildEpisodicContext
// ---------------------------------------------------------------------------

std::string DiscourseMemoryStore::buildEpisodicContext(const std::string &school_id, int max_episodes) const {
    const auto episodes = getEpisodesForSchool(school_id, max_episodes);
    if (episodes.empty()) {
        return {};
    }

    std::ostringstream oss = {};
    for (const auto &ep : episodes) {
        oss << "[" << ep.school_id << " R" << ep.from_round << "] "
            << "DC=" << std::fixed << std::setprecision(2) << ep.dc_score << " " << ep.compressed_position << "\n";
    }
    return oss.str();
}

// ---------------------------------------------------------------------------
// buildAllEpisodicContexts
// ---------------------------------------------------------------------------

std::map<std::string, std::string>
DiscourseMemoryStore::buildAllEpisodicContexts(const std::vector<std::string> &school_ids, int max_episodes) const {
    std::map<std::string, std::string> result = {};

    for (const auto &school_id : school_ids) {
        result[school_id] = buildEpisodicContext(school_id, max_episodes);
    }
    return result;
}

// ---------------------------------------------------------------------------
// clear
// ---------------------------------------------------------------------------

void DiscourseMemoryStore::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    episodes_.clear();
}

// ---------------------------------------------------------------------------
// episodeCount
// ---------------------------------------------------------------------------

size_t DiscourseMemoryStore::episodeCount(const std::string &school_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = episodes_.find(school_id);
    return static_cast<bool>(it == episodes_.end() ? 0u : it- < static_cast<int>(second.size()));
}

} // namespace ethics
} // namespace plugins
} // namespace themis
