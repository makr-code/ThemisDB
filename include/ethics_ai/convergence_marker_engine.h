/**
 * @file convergence_marker_engine.h
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
#include <string>
#include <vector>

namespace themis {
namespace plugins {
namespace ethics {

enum class ConvergenceType {
    CO_PROHIBITIVE,         ///< Both schools prohibit the action
    CO_PERMISSIVE,          ///< Both schools permit the action
    CONDITIONAL_CONVERGENT, ///< Convergent under specific conditions
    IRREDUCIBLE_SPLIT,      ///< Persistent irreconcilable disagreement
    PARTIAL_OVERLAP,        ///< Partial agreement on sub-thesis level
    UNKNOWN                 ///< Insufficient data
};

struct ConvergenceMarker {
    std::string school_a_id;
    std::string thesis_a_id;
    std::string school_b_id;
    std::string thesis_b_id;
    ConvergenceType type{ConvergenceType::UNKNOWN};
    std::string condition;        ///< For CONDITIONAL_CONVERGENT: the condition
    std::string split_reason;     ///< For IRREDUCIBLE_SPLIT: the reason
    float       confidence{0.0f}; ///< [0.0–1.0]
};

class ConvergenceMarkerEngine {
public:
    ConvergenceMarkerEngine() = default;

    std::vector<ConvergenceMarker> detectConvergences(
        const std::vector<DiscourseRoundOutput>& round_outputs,
        const std::vector<SchoolTension>&        tensions = {}) const;

    std::string buildConvergencePreamble(
        const std::vector<ConvergenceMarker>& markers,
        int max_tokens = 250) const;

    /**
     * @brief Convergence Type Label.
     * @param[in] type Input parameter.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    static std::string convergenceTypeLabel(ConvergenceType type) noexcept;

private:
    /**
     * @brief Count Tokens.
     * @param[in] text Input parameter.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    static int countTokens(const std::string& text) noexcept;
};

} // namespace ethics
} // namespace plugins
} // namespace themis
