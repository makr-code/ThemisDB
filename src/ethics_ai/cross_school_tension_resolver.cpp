/**
 * @file cross_school_tension_resolver.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "ethics_ai/cross_school_tension_resolver.h"

#include <algorithm>
#include <sstream>
#include <stdexcept>
#include "utils/logger.h"

namespace themis {
namespace plugins {
namespace ethics {

std::vector<SchoolTension> CrossSchoolTensionResolver::loadTensions(
    const PhilosophyProfile& profile) const
{
    std::vector<SchoolTension> tensions;

    // Parse metadata entries of the form:
    //   key:   "tension:<opposing_school>:<own_thesis>:<opposing_thesis>"
    //   value: "<rebuttal_cite_weight>" (float as string) [:<tension_type>] (optional)
    // Stored in profile.philosophical_positioning (cross-school key-value map)
    for (const auto& kv : profile.philosophical_positioning) {
        const std::string& key = kv.first;
        if (key.rfind("tension:", 0) != 0) {
            continue;
        }

        // Split key by ':'
        std::vector<std::string> parts;
        std::istringstream ss(key);
        std::string token = {};
        while (std::getline(ss, token, ':')) {
            parts.push_back(token);
        }

        // Expected: ["tension", opposing_school, own_thesis, opposing_thesis]
        if (parts.size() < 4) {
            continue;
        }

        SchoolTension t;
        t.opposing_school_id  = parts[1];
        t.own_thesis_id       = parts[2];
        t.opposing_thesis_id  = parts[3];

        // Value may be "<weight>" or "<weight>:<tension_type>"
        const std::string& val = kv.second;
        std::istringstream val_ss(val);
        std::string weight_str = {};
        std::string ttype = {};
        std::getline(val_ss, weight_str, ':');
        std::getline(val_ss, ttype);

        try {
            t.rebuttal_cite_weight = std::stof(weight_str);
        } catch (const std::invalid_argument& ex) {
            THEMIS_WARN("Invalid tension weight format: {}", weight_str);
            t.rebuttal_cite_weight = 0.5f;
        } catch (const std::out_of_range& ex) {
            THEMIS_WARN("Tension weight out of range: {}", weight_str);
            t.rebuttal_cite_weight = 0.5f;
        }

        if (!ttype.empty()) {
            t.tension_type = ttype;
        }

        tensions.push_back(std::move(t));
    }

    return tensions;
}

std::vector<InjectionDecision> CrossSchoolTensionResolver::resolveOpponentInjections(
    const std::string&                  own_school_id,
    const std::vector<std::string>&     opponent_school_ids,
    const std::vector<EthicalArgument>& opponent_round_args,
    const std::vector<SchoolTension>&   tensions,
    float                               full_injection_threshold,
    int                                 max_full_injections) const
{
    (void)own_school_id;  // used for context; tensions are already filtered to own school

    std::vector<InjectionDecision> decisions;
    int full_injection_count = 0;

    for (const auto& opp_school : opponent_school_ids) {
        InjectionDecision decision;
        decision.school_id    = opp_school;
        decision.inject_full  = false;
        decision.relevance_score = 0.0f;

        // Find max rebuttal_cite_weight tension for this opponent
        float max_weight = 0.0f;
        std::string best_own_thesis = {};
        std::string best_opp_thesis = {};
        for (const auto& t : tensions) {
            if (t.opposing_school_id == opp_school && t.rebuttal_cite_weight > max_weight) {
                max_weight      = t.rebuttal_cite_weight;
                best_own_thesis = t.own_thesis_id;
                best_opp_thesis = t.opposing_thesis_id;
            }
        }
        decision.relevance_score = max_weight;

        // Build headline (always populated)
        const std::string thesis_label = best_opp_thesis.empty() ? opp_school : best_opp_thesis;
        decision.headline = "[" + opp_school + ":" + thesis_label + "]";

        // Find matching argument from opponent
        const EthicalArgument* matched_arg = nullptr;
        for (const auto& arg : opponent_round_args) {
            if (arg.philosophy_school == opp_school) {
                matched_arg = &arg;
                break;
            }
        }

        // Decide full injection
        if (max_weight >= full_injection_threshold
                && full_injection_count < max_full_injections
                && matched_arg != nullptr)
        {
            decision.inject_full       = true;
            decision.argument_content  = matched_arg->content;
            ++full_injection_count;
        }

        decisions.push_back(std::move(decision));
    }

    // Sort by relevance_score descending
    std::sort(decisions.begin(), decisions.end(),
        [](const InjectionDecision& a, const InjectionDecision& b) {
            return a.relevance_score > b.relevance_score;
        });

    return decisions;
}

} // namespace ethics
} // namespace plugins
} // namespace themis

