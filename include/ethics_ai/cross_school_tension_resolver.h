/**
 * @file cross_school_tension_resolver.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

struct SchoolTension {
    std::string own_thesis_id;           ///< Thesis ID in own school
    std::string opposing_school_id;      ///< Opposing school ID
    std::string opposing_thesis_id;      ///< Thesis ID in opposing school
    std::string tension_type;            ///< E.g. "categorical_vs_aggregate"
    float       rebuttal_cite_weight{0.5f}; ///< [0.0–1.0]; ≥ 0.6 → full injection
};

struct InjectionDecision {
    std::string school_id;
    std::string argument_content;   ///< Full content (empty when inject_full=false)
    std::string headline;           ///< Always populated: "[school:thesis_name]"
    bool        inject_full{false}; ///< true when rebuttal_cite_weight >= threshold
    float       relevance_score{0.0f};
};

class CrossSchoolTensionResolver {
public:
    CrossSchoolTensionResolver() = default;

    std::vector<InjectionDecision> resolveOpponentInjections(
        const std::string&                  own_school_id,
        const std::vector<std::string>&     opponent_school_ids,
        const std::vector<EthicalArgument>& opponent_round_args,
        const std::vector<SchoolTension>&   tensions,
        float                               full_injection_threshold = 0.6f,
        int                                 max_full_injections = 2) const;

    /**
     * @brief Load Tensions.
     * @param[in] profile Input parameter.
     * @return Return value.
     */
    std::vector<SchoolTension> loadTensions(
        const PhilosophyProfile& profile) const;
};

} // namespace ethics
} // namespace plugins
} // namespace themis
