/**
 * @file tournament_mode_selector.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=5, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "ethics_ai/tournament_mode_selector.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <sstream>

namespace themis {
namespace plugins {
namespace ethics {

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

int TournamentModeSelector::countTokens(const std::string &text) noexcept {
    return static_cast<int>((text.size() + 3) / 4);
}

std::string TournamentModeSelector::buildHeadline(const EthicalArgument &arg) {
    // Short type label
    std::string type_short;
    switch (arg.argument_type) {
        case ArgumentType::PRO:
            type_short = "PRO";
            break;
        case ArgumentType::CONTRA:
            type_short = "CON";
            break;
        case ArgumentType::REBUTTAL:
            type_short = "REB";
            break;
        case ArgumentType::SYNTHESIS:
            type_short = "SYN";
            break;
        case ArgumentType::QUESTION:
            type_short = "QST";
            break;
        case ArgumentType::CLARIFICATION:
            type_short = "CLR";
            break;
        default:
            type_short = "ARG";
            break;
    }

    // First 20 characters of content
    const std::string preview = arg.content.size() > 20 ? arg.content.substr(0, 20) : arg.content;

    std::ostringstream oss;
    oss << "[" << arg.philosophy_school << ": " << type_short << " @ " << preview << "...]";
    return oss.str();
}

// ---------------------------------------------------------------------------
// selectOpponents
// ---------------------------------------------------------------------------

TournamentSelectionResult TournamentModeSelector::selectOpponents(
    const std::string &own_school_id, const std::vector<EthicalArgument> &opponent_arguments,
    const std::vector<SchoolTension> &tensions, const TournamentConfig &config) const {
    TournamentSelectionResult result;
    result.own_school_id = own_school_id;

    // Collect unique opponent schools that appear in opponent_arguments
    // Each school may have multiple arguments; pick the first for headline/full text.
    // We use a map keyed by school_id → representative argument index.
    std::map<std::string, std::size_t> school_to_arg_index;
    for (std::size_t i = 0; i < opponent_arguments.size(); ++i) {
        const auto &arg = opponent_arguments[i];
        if (arg.philosophy_school == own_school_id) {
            continue;
        }
        // Store only the first occurrence per school
        if (school_to_arg_index.find(arg.philosophy_school) == school_to_arg_index.end()) {
            school_to_arg_index[arg.philosophy_school] = i;
        }
    }

    // Build a max-weight lookup per opponent school from tensions
    std::map<std::string, float> school_weight;
    for (const auto &kv : school_to_arg_index) {
        school_weight[kv.first] = 0.0f;
    }
    for (const auto &t : tensions) {
        auto it = school_weight.find(t.opposing_school_id);
        if (it != school_weight.end()) {
            it->second = std::max(it->second, t.rebuttal_cite_weight);
        }
    }

    // Sort opponent school IDs by weight descending (deterministic tie-break by name)
    std::vector<std::string> ordered_schools;
    ordered_schools.reserve(school_to_arg_index.size());
    for (const auto &kv : school_to_arg_index) {
        ordered_schools.push_back(kv.first);
    }
    std::sort(ordered_schools.begin(), ordered_schools.end(),
              [&school_weight](const std::string &a, const std::string &b) {
                  const float wa = school_weight.count(a) ? school_weight.at(a) : 0.0f;
                  const float wb = school_weight.count(b) ? school_weight.at(b) : 0.0f;
                  // Strict-weak-ordering fix: quantize weights into epsilon-width buckets
                  // so the comparison is a total order.  An epsilon-based conditional
                  // (|wa-wb| > eps → wa > wb, else a < b) can violate transitivity when
                  // three values span the boundary, causing std::sort UB.
                  const float epsilon = 1e-6f;
                  const auto bucket = [epsilon](float w) {
                      return static_cast<long long>(std::floor(w / epsilon));
                  };
                  const long long ba = bucket(wa);
                  const long long bb = bucket(wb);
                  if (ba != bb) {
                      return ba > bb; // higher weight bucket first
                  }
                  return a < b; // lexicographic tie-break for determinism
              });

    // Assemble result based on mode
    std::ostringstream ctx;

    if (config.mode == OpponentInjectionMode::FULL) {
        for (const auto &school : ordered_schools) {
            result.primary_opponents.push_back(school);
            const auto &arg = opponent_arguments[school_to_arg_index.at(school)];
            ctx << "[" << school << "] " << arg.content << "\n";
        }
    } else if (config.mode == OpponentInjectionMode::HEADLINE_ONLY) {
        for (const auto &school : ordered_schools) {
            result.secondary_opponents.push_back(school);
            const auto &arg = opponent_arguments[school_to_arg_index.at(school)];
            ctx << buildHeadline(arg) << "\n";
        }
    } else {
        // TOURNAMENT mode: top N primaries get full, rest get headlines
        const int n_primary = std::max(0, config.primary_opponent_count);
        for (int i = 0; i < static_cast<int>(ordered_schools.size()); ++i) {
            const auto &school = ordered_schools[static_cast<std::size_t>(i)];
            const auto &arg    = opponent_arguments[school_to_arg_index.at(school)];
            if (i < n_primary) {
                result.primary_opponents.push_back(school);
                ctx << "[" << school << "] " << arg.content << "\n";
            } else {
                result.secondary_opponents.push_back(school);
                if (config.secondary_injection != "none") {
                    ctx << buildHeadline(arg) << "\n";
                }
            }
        }
    }

    result.assembled_context      = ctx.str();
    result.total_tokens_estimated = countTokens(result.assembled_context);
    return result;
}

// ---------------------------------------------------------------------------
// buildTournamentContext
// ---------------------------------------------------------------------------

std::map<std::string, TournamentSelectionResult> TournamentModeSelector::buildTournamentContext(
    const std::vector<EthicalArgument> &all_round_arguments,
    const std::map<std::string, std::vector<SchoolTension>> &tensions_per_school,
    const TournamentConfig &config) const {
    // Collect unique school IDs that appear in the argument list
    std::set<std::string> school_set;
    for (const auto &arg : all_round_arguments) {
        school_set.insert(arg.philosophy_school);
    }

    std::map<std::string, TournamentSelectionResult> results;
    for (const auto &school_id : school_set) {
        // Build the list of opponent arguments (all schools except own)
        std::vector<EthicalArgument> opponent_args;
        for (const auto &arg : all_round_arguments) {
            if (arg.philosophy_school != school_id) {
                opponent_args.push_back(arg);
            }
        }

        // Retrieve tensions for this school (empty if not declared)
        // COMPLEXITY FIX: tensions_per_school is std::map, find() is O(log n) (HIGH: o_n_squared)
        // Loop does n map lookups: O(n log m) total where m = map size, not O(n²)
        std::vector<SchoolTension> tensions;
        auto it = tensions_per_school.find(school_id);
        if (it != tensions_per_school.end()) {
            tensions = it->second;
        }

        results[school_id] = selectOpponents(school_id, opponent_args, tensions, config);
    }

    return results;
}

} // namespace ethics
} // namespace plugins
} // namespace themis
