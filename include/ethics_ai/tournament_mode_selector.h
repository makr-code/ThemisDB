/**
 * @file tournament_mode_selector.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

enum class OpponentInjectionMode {
    FULL,           ///< All opponents injected with full argument text
    TOURNAMENT,     ///< Primary opponent full; secondary opponents as headlines
    HEADLINE_ONLY   ///< All opponents injected as headlines only
};

struct TournamentConfig {
    int         primary_opponent_count{1};             ///< Number of fully-injected opponents
    std::string selection_criterion{"rebuttal_cite_weight"}; ///< "rebuttal_cite_weight" or "final_score"
    std::string secondary_injection{"headline"};       ///< "headline" or "none"
    OpponentInjectionMode mode{OpponentInjectionMode::TOURNAMENT};
};

struct TournamentSelectionResult {
    std::string              own_school_id;
    std::vector<std::string> primary_opponents;    ///< Fully-injected opponent school IDs
    std::vector<std::string> secondary_opponents;  ///< Headline-only opponent school IDs
    std::string assembled_context = {};
    int         total_tokens_estimated{0};         ///< Estimated token count of assembled_context
};

class TournamentModeSelector {
public:
    TournamentModeSelector() = default;

    TournamentSelectionResult selectOpponents(
        const std::string&                  own_school_id,
        const std::vector<EthicalArgument>& opponent_arguments,
        const std::vector<SchoolTension>&   tensions,
        const TournamentConfig&             config = TournamentConfig{}) const;

    std::map<std::string, TournamentSelectionResult> buildTournamentContext(
        const std::vector<EthicalArgument>&                          all_round_arguments,
        const std::map<std::string, std::vector<SchoolTension>>&     tensions_per_school,
        const TournamentConfig&                                      config = TournamentConfig{}) const;

private:
    /**
     * @brief Count Tokens.
     * @param[in] text Input parameter.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    static int         countTokens(const std::string& text) noexcept;
    /**
     * @brief Build Headline.
     * @param[in] arg Input parameter.
     * @return Return value.
     */
    static std::string buildHeadline(const EthicalArgument& arg);
};

} // namespace ethics
} // namespace plugins
} // namespace themis
