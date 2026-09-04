/**
 * @file tournament_mode_selector.h
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
#include "ethics_ai/cross_school_tension_resolver.h"
#include <map>
#include <string>
#include <vector>

namespace themis {
namespace plugins {
namespace ethics {

/**
 * @brief Opponent injection mode for a discourse round.
 */
enum class OpponentInjectionMode {
    FULL,           ///< All opponents injected with full argument text
    TOURNAMENT,     ///< Primary opponent full; secondary opponents as headlines
    HEADLINE_ONLY   ///< All opponents injected as headlines only
};

/**
 * @brief Configuration for tournament mode opponent selection.
 */
struct TournamentConfig {
    int         primary_opponent_count{1};             ///< Number of fully-injected opponents
    std::string selection_criterion{"rebuttal_cite_weight"}; ///< "rebuttal_cite_weight" or "final_score"
    std::string secondary_injection{"headline"};       ///< "headline" or "none"
    OpponentInjectionMode mode{OpponentInjectionMode::TOURNAMENT};
};

/**
 * @brief Result of tournament selection for one school's opponents.
 */
struct TournamentSelectionResult {
    std::string              own_school_id;
    std::vector<std::string> primary_opponents;    ///< Fully-injected opponent school IDs
    std::vector<std::string> secondary_opponents;  ///< Headline-only opponent school IDs
    /// Assembled opponent context string for injection into the school's prompt.
    /// Primary opponents: full argument text. Secondary: "[school: headline_text]"
    std::string assembled_context = {};
    int         total_tokens_estimated{0};         ///< Estimated token count of assembled_context
};

/**
 * @brief Selects opponents for R3 SURREBUTTAL using tournament-style injection.
 *
 * Implements §12.2.2 Sequential Tournament Mode (Du et al. [R6] §5, Chan et al. [R7]).
 * Reduces R3 token pressure by −65 % vs full injection (4-school configuration).
 *
 * All methods are const and thread-safe (no mutable state).
 */
class TournamentModeSelector {
public:
    TournamentModeSelector() = default;

    /**
     * @brief Select and assemble opponent arguments for a given school.
     *
     * @param own_school_id         The school generating the SURREBUTTAL.
     * @param opponent_arguments    All opponent arguments from the prior round.
     * @param tensions              Tensions declared in own school's profile.
     * @param config                Tournament configuration.
     * @return TournamentSelectionResult with assembled context.
     */
    TournamentSelectionResult selectOpponents(
        const std::string&                  own_school_id,
        const std::vector<EthicalArgument>& opponent_arguments,
        const std::vector<SchoolTension>&   tensions,
        const TournamentConfig&             config = TournamentConfig{}) const;

    /**
     * @brief Build tournament context for all participating schools.
     *
     * Convenience wrapper that calls selectOpponents() for each school.
     *
     * @param all_round_arguments   All arguments from the prior round.
     * @param tensions_per_school   Map from school_id to its declared tensions.
     * @param config                Tournament configuration.
     * @return Map from school_id to its TournamentSelectionResult.
     */
    std::map<std::string, TournamentSelectionResult> buildTournamentContext(
        const std::vector<EthicalArgument>&                          all_round_arguments,
        const std::map<std::string, std::vector<SchoolTension>>&     tensions_per_school,
        const TournamentConfig&                                      config = TournamentConfig{}) const;

private:
    static int         countTokens(const std::string& text) noexcept;
    static std::string buildHeadline(const EthicalArgument& arg);
};

} // namespace ethics
} // namespace plugins
} // namespace themis
