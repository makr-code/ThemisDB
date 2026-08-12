/**
 * @file personalized_ranker.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "search/learning_to_rank.h"
#include <chrono>
#include <map>
#include <mutex>
#include <string>
#include <vector>

namespace themis {

/**
 * @brief Type of user interaction with a document.
 */
enum class InteractionType {
    VIEW,      ///< User viewed / opened the document (implicit weak signal)
    CLICK,     ///< User clicked on the document in a result list
    BOOKMARK,  ///< User bookmarked / saved the document (strong positive signal)
    LIKE,      ///< User explicitly liked / rated the document (strong positive signal)
    DISLIKE,   ///< User explicitly disliked the document (negative signal)
};

/**
 * @brief A single recorded interaction between a user and a document.
 */
struct UserInteraction {
    std::string user_id;       ///< Identifier of the user
    std::string document_id;   ///< Identifier of the document
    InteractionType type;      ///< Type of interaction
    std::chrono::system_clock::time_point timestamp; ///< When the interaction occurred
};

/**
 * @brief Personalized re-ranker that boosts or suppresses results based on
 *        per-user interaction history.
 *
 * PersonalizedRanker records user interactions (views, clicks, bookmarks,
 * likes, and dislikes) and uses them to compute a personalization score for
 * each (user, document) pair.  The score can then be blended into the
 * `RankingFeatures::click_count` or `popularity` of LTR candidates, or
 * used via `applyPersonalization()` to directly adjust `final_score`.
 *
 * ### Score model
 * Each interaction contributes a weighted, time-decayed signal:
 * ```
 * personalization_score += type_weight * exp(-decay_rate * age_days)
 * ```
 * where `age_days` is the age of the interaction in days and `type_weight`
 * is one of:
 *  - VIEW      →  0.2
 *  - CLICK     →  0.5
 *  - BOOKMARK  →  1.0
 *  - LIKE      →  1.0
 *  - DISLIKE   → -0.5
 *
 * The final score is clamped to [-1, 1].
 *
 * ### Usage
 * ```cpp
 * PersonalizedRanker::Config cfg;
 * cfg.decay_rate = 0.1;   // fast decay: interactions lose half weight ~7 days
 * PersonalizedRanker pr(cfg);
 *
 * // Record interactions as users browse
 * pr.recordInteraction({"alice", "doc_42", InteractionType::CLICK,
 *                        std::chrono::system_clock::now()});
 *
 * // Apply personalization to LTR candidates before or after re-ranking
 * auto ranked = ltr.rerank(candidates);
 * pr.applyPersonalization("alice", ranked);
 * ```
 *
 * @note Thread Safety: All public methods are thread-safe via a shared mutex.
 *
 * @note Memory: The interaction log per user is bounded by
 *   `Config::max_interactions_per_user`.  When the limit is reached the
 *   oldest interactions are evicted (circular buffer semantics).
 */
class PersonalizedRanker {
public:
    struct Config {
        /// Exponential decay rate per day (0 = no decay; higher = faster decay).
        /// Default 0.05 ≈ half-weight after ~14 days.
        double decay_rate = 0.05;

        /// Maximum interactions stored per user (oldest evicted when full).
        size_t max_interactions_per_user = 500;

        /// Weight added to RankedResult::final_score for each unit of
        /// personalization signal (scales the [-1,1] score).
        double boost_weight = 0.2;

        static Config defaults() { return {}; }
    };

    /**
     * @param config  Ranker configuration.
     * @throws std::invalid_argument on invalid config (decay_rate < 0,
     *         max_interactions_per_user == 0, boost_weight < 0).
     */
    explicit PersonalizedRanker(const Config& config = Config::defaults());

    // -----------------------------------------------------------------------
    // Interaction recording
    // -----------------------------------------------------------------------

    /**
     * @brief Record a user interaction for future personalization.
     *
     * If the per-user buffer is full, the oldest interaction is evicted.
     *
     * @param interaction  Interaction to record.
     */
    void recordInteraction(const UserInteraction& interaction);

    // -----------------------------------------------------------------------
    // Personalization
    // -----------------------------------------------------------------------

    /**
     * @brief Compute the personalization score for a (user, document) pair.
     *
     * Returns a value in [-1, 1]:
     *  - Positive: the user has shown positive interest in the document.
     *  - Negative: the user has shown negative interest (disliked).
     *  - Zero: no recorded interaction.
     *
     * @param user_id      The user to personalize for.
     * @param document_id  The document to score.
     * @param now          Reference time for decay calculation
     *                     (defaults to system clock now).
     * @return Personalization score in [-1, 1].
     */
    double computeScore(
        const std::string& user_id,
        const std::string& document_id,
        std::chrono::system_clock::time_point now =
            std::chrono::system_clock::now()) const;

    /**
     * @brief Apply personalization boosts to a list of ranked candidates.
     *
     * For each candidate, `final_score` is incremented by
     * `Config::boost_weight * personalizationScore(user_id, doc_id)`.
     * The list is then re-sorted by `final_score` descending.
     *
     * @param user_id     The user to personalize for.
     * @param candidates  In-out list of ranked results; final_score is updated.
     * @param now         Reference time for decay (defaults to now()).
     */
    void applyPersonalization(
        const std::string& user_id,
        std::vector<RankedResult>& candidates,
        std::chrono::system_clock::time_point now =
            std::chrono::system_clock::now()) const;

    // -----------------------------------------------------------------------
    // Inspection
    // -----------------------------------------------------------------------

    /**
     * @brief Return all recorded interactions for a user (most-recent first).
     */
    std::vector<UserInteraction> getUserInteractions(
        const std::string& user_id) const;

    /**
     * @brief Return the number of distinct users with interaction history.
     */
    size_t userCount() const;

    /**
     * @brief Remove all interaction history for a specific user.
     */
    void clearUser(const std::string& user_id);

    /**
     * @brief Remove all interaction history.
     */
    void clear();

    const Config& getConfig() const { return config_; }

private:
    Config config_;
    /// Per-user interaction lists (oldest-first ordering for eviction)
    std::map<std::string, std::vector<UserInteraction>> history_;
    mutable std::mutex mu_;

    static double typeWeight(InteractionType type);
    /// Compute personalization score without locking (caller must hold mu_).
    double computeScoreUnlocked(
        const std::string& user_id,
        const std::string& document_id,
        std::chrono::system_clock::time_point now) const;
};

} // namespace themis
