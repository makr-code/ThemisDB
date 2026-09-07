/**
 * @file convergence_marker_engine.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "ethics_ai/convergence_marker_engine.h"

#include <algorithm>
#include <map>
#include <sstream>

namespace themis {
namespace plugins {
namespace ethics {

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

int ConvergenceMarkerEngine::countTokens(const std::string& text) noexcept {
    return static_cast<int>((text.size() + 3U) / 4U);
}

// ---------------------------------------------------------------------------
// Static public method
// ---------------------------------------------------------------------------

std::string ConvergenceMarkerEngine::convergenceTypeLabel(ConvergenceType type) noexcept {
    switch (type) {
        case ConvergenceType::CO_PROHIBITIVE:         return "CO_PROHIBITIVE";
        case ConvergenceType::CO_PERMISSIVE:          return "CO_PERMISSIVE";
        case ConvergenceType::CONDITIONAL_CONVERGENT: return "CONDITIONAL_CONVERGENT";
        case ConvergenceType::IRREDUCIBLE_SPLIT:      return "IRREDUCIBLE_SPLIT";
        case ConvergenceType::PARTIAL_OVERLAP:        return "PARTIAL_OVERLAP";
        case ConvergenceType::UNKNOWN:                return "UNKNOWN";
    }
    return "UNKNOWN";
}

// ---------------------------------------------------------------------------
// detectConvergences
// ---------------------------------------------------------------------------

std::vector<ConvergenceMarker> ConvergenceMarkerEngine::detectConvergences(
    const std::vector<DiscourseRoundOutput>& round_outputs,
    const std::vector<SchoolTension>&        tensions) const
{
    std::vector<ConvergenceMarker> markers;

    if (static_cast<int>(round_outputs.size()) < 2) {
        return markers;
    }

    // Build a lookup: school_id → verdict
    std::map<std::string, std::string> school_verdict;
    std::map<std::string, std::string> school_thesis = {};

    for (const auto& out : round_outputs) {
        school_verdict[out.school_id] = out.verdict;
        school_thesis[out.school_id] =
            out.core_thesis_ids.empty() ? out.school_id : out.core_thesis_ids[0];
    }

    // Build a tension lookup for quick split detection:
    //   pair(school_a, school_b) → max rebuttal_cite_weight
    std::map<std::pair<std::string, std::string>, float> tension_weight;
    for (const auto& t : tensions) {
        auto key_ab = std::make_pair(t.own_thesis_id, t.opposing_school_id);
        auto key_ba = std::make_pair(t.opposing_school_id, t.own_thesis_id);
        tension_weight[key_ab] = std::max(tension_weight[key_ab], t.rebuttal_cite_weight);
        tension_weight[key_ba] = std::max(tension_weight[key_ba], t.rebuttal_cite_weight);
    }

    const float high_tension_threshold = 0.7f;

    // Compare all pairs
    for (size_t i = 0; i < round_outputs.size(); ++i) {
        for (size_t j = i + 1; j < round_outputs.size(); ++j) {
            const auto& a = round_outputs[i];
            const auto& b = round_outputs[j];

            const std::string& va = a.verdict;
            const std::string& vb = b.verdict;

            ConvergenceMarker marker;
            marker.school_a_id = a.school_id;
            marker.thesis_a_id = school_thesis[a.school_id];
            marker.school_b_id = b.school_id;
            marker.thesis_b_id = school_thesis[b.school_id];
            marker.confidence  = (a.confidence + b.confidence) * 0.5f;

            if (va == vb) {
                if (va == "PROHIBIT") {
                    marker.type = ConvergenceType::CO_PROHIBITIVE;
                } else if (va == "PERMIT") {
                    marker.type = ConvergenceType::CO_PERMISSIVE;
                } else if (va == "CONDITIONAL") {
                    marker.type = ConvergenceType::CONDITIONAL_CONVERGENT;
                    marker.condition = "Both conditional";
                } else {
                    marker.type = ConvergenceType::PARTIAL_OVERLAP;
                }
            } else {
                // Mixed verdicts — check tension weight
                const auto key = std::make_pair(a.school_id, b.school_id);
                const float w = tension_weight.count(key) ? tension_weight.at(key) : 0.0f;

                if (w >= high_tension_threshold) {
                    marker.type = ConvergenceType::IRREDUCIBLE_SPLIT;
                    marker.split_reason = va + "_vs_" + vb;
                } else {
                    marker.type = ConvergenceType::PARTIAL_OVERLAP;
                }
            }

            markers.push_back(std::move(marker));
        }
    }

    return markers;
}

// ---------------------------------------------------------------------------
// buildConvergencePreamble
// ---------------------------------------------------------------------------

std::string ConvergenceMarkerEngine::buildConvergencePreamble(
    const std::vector<ConvergenceMarker>& markers,
    int max_tokens) const
{
    std::ostringstream oss = {};
    oss << "[CONVERGENCE MATRIX — R4 SYNTHESIS]\n";

    // Convergent markers first
    for (const auto& m : markers) {
        if (m.type == ConvergenceType::IRREDUCIBLE_SPLIT) {
            continue;
        }
        oss << m.school_a_id << ":" << m.thesis_a_id
            << " \xe2\x86\x94 "  // UTF-8 ↔
            << m.school_b_id << ":" << m.thesis_b_id
            << ": " << convergenceTypeLabel(m.type);
        if (!m.condition.empty()) {
            oss << " (" << m.condition << ")";
        }
        oss << "\n";

        if (countTokens(oss.str()) >= max_tokens) {
            break;
        }
    }

    // Split section
    bool has_splits = false;
    for (const auto& m : markers) {
        if (m.type != ConvergenceType::IRREDUCIBLE_SPLIT) {
            continue;
        }
        if (!has_splits) {
            oss << "[PERSISTENT SPLITS]\n";
            has_splits = true;
        }
        oss << m.school_a_id << ":" << m.thesis_a_id
            << " \xe2\x86\x94 "
            << m.school_b_id << ":" << m.thesis_b_id
            << ": IRREDUCIBLE";
        if (!m.split_reason.empty()) {
            oss << " (" << m.split_reason << ")";
        }
        oss << "\n";

        if (countTokens(oss.str()) >= max_tokens) {
            break;
        }
    }

    std::string result = oss.str();

    // Hard trim to max_tokens
    const int max_chars = max_tokens * 4;
    if (static_cast<int>(result.size()) > max_chars) {
        result = result.substr(0, static_cast<size_t>(max_chars));
    }

    return result;
}

} // namespace ethics
} // namespace plugins
} // namespace themis

