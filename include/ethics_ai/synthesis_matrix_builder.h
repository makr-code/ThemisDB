/**
 * @file synthesis_matrix_builder.h
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
#include "ethics_ai/convergence_marker_engine.h"
#include <stdexcept>
#include <string>
#include <vector>

namespace themis {
namespace plugins {
namespace ethics {

/**
 * @brief Compact position summary for a single school (for R4 SYNTHESIS).
 */
struct SchoolPositionSummary {
    std::string school_id;
    std::string verdict;                        ///< "PROHIBIT"|"PERMIT"|"CONDITIONAL"|"ABSTAIN"
    float       confidence{0.0f};
    std::vector<std::string> core_thesis_ids;  ///< ≤ 3 thesis_ids
};

/**
 * @brief Error thrown when a SchoolPositionSummary fails schema validation.
 */
struct SchemaValidationError : public std::runtime_error {
    explicit SchemaValidationError(const std::string& msg)
        : std::runtime_error("SchemaValidationError: " + msg) {}
};

/**
 * @brief Builds compact positions matrices for R4 SYNTHESIS context injection.
 *
 * Implements §12.2.5 (Du et al. [R6] Agent State Summary). Replaces ~3 600
 * tokens of full argument text with ~250 tokens of structured matrix data,
 * providing a further 93 % token reduction on the R4 prior-context side.
 *
 * All methods are const and thread-safe.
 */
class SynthesisMatrixBuilder {
public:
    SynthesisMatrixBuilder() = default;

    /**
     * @brief Build the positions matrix text for injection into R4 SYNTHESIS.
     *
     * @param positions    School position summaries (one per school).
     * @param convergences Convergence markers from ConvergenceMarkerEngine.
     * @param max_tokens   Hard token cap (default 300). Exceeding: WARN + truncate thesis_ids.
     * @return Formatted matrix string ≤ max_tokens.
     */
    std::string buildMatrix(
        const std::vector<SchoolPositionSummary>& positions,
        const std::vector<ConvergenceMarker>&     convergences = {},
        int                                       max_tokens = 300) const;

    /**
     * @brief Extract a SchoolPositionSummary from a DiscourseRoundOutput.
     *
     * Reads verdict, confidence, core_thesis_ids, and position_abstract.
     *
     * @param round_output The round output to extract from.
     * @return SchoolPositionSummary.
     */
    SchoolPositionSummary extractSummary(
        const DiscourseRoundOutput& round_output) const;

    /**
     * @brief Validate a SchoolPositionSummary — throws SchemaValidationError on violation.
     *
     * Validates: verdict is one of allowed values; confidence in [0,1];
     * core_thesis_ids not empty; school_id not empty.
     */
    void validateSummary(const SchoolPositionSummary& summary) const;

private:
    static int countTokens(const std::string& text) noexcept;
    static bool isValidVerdict(const std::string& verdict) noexcept;
};

} // namespace ethics
} // namespace plugins
} // namespace themis
