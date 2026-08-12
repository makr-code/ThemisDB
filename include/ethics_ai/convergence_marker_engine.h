/**
 * @file convergence_marker_engine.h
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
#include <string>
#include <vector>

namespace themis {
namespace plugins {
namespace ethics {

/**
 * @brief Convergence type between two school-thesis pairs.
 */
enum class ConvergenceType {
    CO_PROHIBITIVE,         ///< Both schools prohibit the action
    CO_PERMISSIVE,          ///< Both schools permit the action
    CONDITIONAL_CONVERGENT, ///< Convergent under specific conditions
    IRREDUCIBLE_SPLIT,      ///< Persistent irreconcilable disagreement
    PARTIAL_OVERLAP,        ///< Partial agreement on sub-thesis level
    UNKNOWN                 ///< Insufficient data
};

/**
 * @brief A convergence/divergence marker between two school positions.
 */
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

/**
 * @brief Builds compact convergence preambles for R4 SYNTHESIS context.
 *
 * Implements §12.1.4: the 4×4 convergence matrix replaces ~3 600 tokens of
 * full argument text with ~200 tokens of structured convergence data,
 * equivalent to Structured State Representation (Du et al. [R6]).
 *
 * All methods are const and thread-safe.
 */
class ConvergenceMarkerEngine {
public:
    ConvergenceMarkerEngine() = default;

    /**
     * @brief Detect convergence markers from discourse round outputs.
     *
     * Infers convergence by comparing verdict fields across schools. Schools
     * with the same verdict → CO_PROHIBITIVE or CO_PERMISSIVE.
     * Mixed verdicts → PARTIAL_OVERLAP or IRREDUCIBLE_SPLIT based on
     * cross-school tension weights.
     *
     * @param round_outputs  Latest round outputs for all participating schools.
     * @param tensions       Known tensions between schools (for split detection).
     * @return Vector of ConvergenceMarker (may be empty if data is insufficient).
     */
    std::vector<ConvergenceMarker> detectConvergences(
        const std::vector<DiscourseRoundOutput>& round_outputs,
        const std::vector<SchoolTension>&        tensions = {}) const;

    /**
     * @brief Build a compact convergence preamble text for injection into R4.
     *
     * Format matches the example in §12.1.4:
     *   [CONVERGENCE MATRIX — R4 SYNTHESIS]
     *   school_a:thesis_a ↔ school_b:thesis_b: CO_PROHIBITIVE (do_not_push)
     *   ...
     *   [PERSISTENT SPLITS]
     *   school_a:thesis_a ↔ school_b:thesis_b: IRREDUCIBLE (reason)
     *
     * @param markers     Convergence markers from detectConvergences().
     * @param max_tokens  Hard token limit for the preamble (default 250).
     * @return Formatted preamble string.
     */
    std::string buildConvergencePreamble(
        const std::vector<ConvergenceMarker>& markers,
        int max_tokens = 250) const;

    /**
     * @brief Convert ConvergenceType enum to human-readable label.
     */
    static std::string convergenceTypeLabel(ConvergenceType type) noexcept;

private:
    static int countTokens(const std::string& text) noexcept;
};

} // namespace ethics
} // namespace plugins
} // namespace themis
