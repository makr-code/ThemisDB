/**
 * @file cross_school_tension_resolver.h
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
#include <string>
#include <vector>

namespace themis {
namespace plugins {
namespace ethics {

/**
 * @brief Tension between two philosophy schools (from YAML cross_school_tensions).
 */
struct SchoolTension {
    std::string own_thesis_id;           ///< Thesis ID in own school
    std::string opposing_school_id;      ///< Opposing school ID
    std::string opposing_thesis_id;      ///< Thesis ID in opposing school
    std::string tension_type;            ///< E.g. "categorical_vs_aggregate"
    float       rebuttal_cite_weight{0.5f}; ///< [0.0–1.0]; ≥ 0.6 → full injection
};

/**
 * @brief Decision for injecting an opponent's argument into the current context.
 */
struct InjectionDecision {
    std::string school_id;
    std::string argument_content;   ///< Full content (empty when inject_full=false)
    std::string headline;           ///< Always populated: "[school:thesis_name]"
    bool        inject_full{false}; ///< true when rebuttal_cite_weight >= threshold
    float       relevance_score{0.0f};
};

/**
 * @brief Resolves which opponent arguments to inject based on school tensions.
 *
 * Implements §12.1.3 (FILCO-style selective injection). Only opponent arguments
 * with `rebuttal_cite_weight >= full_injection_threshold` are embedded in full;
 * the rest appear as 15-token headlines.
 *
 * All methods are const and thread-safe (no mutable state).
 */
class CrossSchoolTensionResolver {
public:
    CrossSchoolTensionResolver() = default;

    /**
     * @brief Determine injection decisions for all opponent schools.
     *
     * @param own_school_id             The school generating the argument.
     * @param opponent_school_ids       All participating opponent schools.
     * @param opponent_round_args       Arguments produced by opponents last round.
     * @param tensions                  Tension declarations from own school's profile.
     * @param full_injection_threshold  Weight threshold for full injection (default 0.6).
     * @param max_full_injections       Maximum number of fully-injected opponents (default 2).
     * @return Vector of InjectionDecision (one per opponent school).
     */
    std::vector<InjectionDecision> resolveOpponentInjections(
        const std::string&                  own_school_id,
        const std::vector<std::string>&     opponent_school_ids,
        const std::vector<EthicalArgument>& opponent_round_args,
        const std::vector<SchoolTension>&   tensions,
        float                               full_injection_threshold = 0.6f,
        int                                 max_full_injections = 2) const;

    /**
     * @brief Load tensions from a PhilosophyProfile's metadata map.
     *
     * Expects metadata keys like "tension:opposing_school_id:own_thesis:opposing_thesis"
     * with value = rebuttal_cite_weight as string. Also accepts direct
     * cross_school_tensions entries if stored in profile metadata.
     *
     * @param profile The philosophy profile to extract tensions from.
     * @return Vector of SchoolTension entries.
     */
    std::vector<SchoolTension> loadTensions(
        const PhilosophyProfile& profile) const;
};

} // namespace ethics
} // namespace plugins
} // namespace themis
