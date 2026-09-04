/**
 * @file personalized_ranker.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "search/personalized_ranker.h"
#include "utils/logger.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace themis {

// ============================================================================
// Construction
// ============================================================================

PersonalizedRanker::PersonalizedRanker(const Config& config) : config_(config) {
    if (config_.decay_rate < 0.0) {
        throw std::invalid_argument("PersonalizedRanker: decay_rate must be >= 0");
    }
    if (config_.max_interactions_per_user == 0) {
        throw std::invalid_argument(
            "PersonalizedRanker: max_interactions_per_user must be > 0");
    }
    if (config_.boost_weight < 0.0) {
        throw std::invalid_argument("PersonalizedRanker: boost_weight must be >= 0");
    }
}

// ============================================================================
// Interaction recording
// ============================================================================

void PersonalizedRanker::recordInteraction(const UserInteraction& interaction) {
    std::lock_guard<std::mutex> lock(mu_);

    auto& user_history = history_[interaction.user_id];
    if (user_history.size() >= config_.max_interactions_per_user) {
        // Evict oldest interaction (front of the vector)
        user_history.erase(user_history.begin());
    }
    user_history.push_back(interaction);

    THEMIS_DEBUG("PersonalizedRanker: recorded {} interaction for user '{}' on doc '{}'",
                 static_cast<int>(interaction.type),
                 interaction.user_id,
                 interaction.document_id);
}

// ============================================================================
// Personalization scoring
// ============================================================================

double PersonalizedRanker::typeWeight(InteractionType type) {
    switch (type) {
        case InteractionType::VIEW:      return 0.2;
        case InteractionType::CLICK:     return 0.5;
        case InteractionType::BOOKMARK:  return 1.0;
        case InteractionType::LIKE:      return 1.0;
        case InteractionType::DISLIKE:   return -0.5;
    }
    return 0.0;
}

double PersonalizedRanker::computeScore(
    const std::string& user_id,
    const std::string& document_id,
    std::chrono::system_clock::time_point now) const {

    std::lock_guard<std::mutex> lock(mu_);
    return computeScoreUnlocked(user_id, document_id, now);
}

double PersonalizedRanker::computeScoreUnlocked(
    const std::string& user_id,
    const std::string& document_id,
    std::chrono::system_clock::time_point now) const {

    auto it = history_.find(user_id);
    if (it == history_.end()) {
      return 0.0;
    }

    double score = 0.0;
    for (const auto& interaction : it->second) {
        if (interaction.document_id != document_id) {
          continue;
        }

        double age_seconds = std::chrono::duration<double>(
            now - interaction.timestamp).count();
        double age_days = age_seconds / 86400.0;

        // Exponential decay: score *= exp(-decay_rate * age_days)
        double decay = (config_.decay_rate > 0.0)
            ? std::exp(-config_.decay_rate * age_days)
            : 1.0;

        score += typeWeight(interaction.type) * decay;
    }

    // Clamp to [-1, 1]
    return std::max(-1.0, std::min(1.0, score));
}

void PersonalizedRanker::applyPersonalization(
    const std::string& user_id,
    std::vector<RankedResult>& candidates,
    std::chrono::system_clock::time_point now) const {

    if (candidates.empty() || user_id.empty()) {
      return;
    }

    std::lock_guard<std::mutex> lock(mu_);
    for (auto& candidate : candidates) {
        double personal_score = computeScoreUnlocked(user_id, candidate.document_id, now);
        candidate.final_score += config_.boost_weight * personal_score;
    }

    std::sort(candidates.begin(), candidates.end(),
              [](const RankedResult& a, const RankedResult& b) {
                  return a.final_score > b.final_score;
              });

    THEMIS_DEBUG("PersonalizedRanker::applyPersonalization: user='{}', {} candidates",
                 user_id, candidates.size());
}

// ============================================================================
// Inspection
// ============================================================================

std::vector<UserInteraction> PersonalizedRanker::getUserInteractions(
    const std::string& user_id) const {

    std::lock_guard<std::mutex> lock(mu_);

    auto it = history_.find(user_id);
    if (it == history_.end()) return {};

    // Return most-recent first
    auto result = it->second;
    std::reverse(result.begin(), result.end());
    return result;
}

size_t PersonalizedRanker::userCount() const {
    std::lock_guard<std::mutex> lock(mu_);
    return history_.size();
}

void PersonalizedRanker::clearUser(const std::string& user_id) {
    std::lock_guard<std::mutex> lock(mu_);
    history_.erase(user_id);
}

void PersonalizedRanker::clear() {
    std::lock_guard<std::mutex> lock(mu_);
    history_.clear();
}

} // namespace themis

