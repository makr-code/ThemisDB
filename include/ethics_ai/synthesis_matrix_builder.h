/**
 * @file synthesis_matrix_builder.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

struct SchoolPositionSummary {
    std::string school_id;
    std::string verdict;                        ///< "PROHIBIT"|"PERMIT"|"CONDITIONAL"|"ABSTAIN"
    float       confidence{0.0f};
    std::vector<std::string> core_thesis_ids;  ///< ≤ 3 thesis_ids
};

struct SchemaValidationError : public std::runtime_error {
    /**
     * @brief Schema Validation Error.
     * @param[in] msg Input parameter.
     * @return Return value.
     */
    explicit SchemaValidationError(const std::string& msg)
        : std::runtime_error("SchemaValidationError: " + msg) {}
};

class SynthesisMatrixBuilder {
public:
    SynthesisMatrixBuilder() = default;

    std::string buildMatrix(
        const std::vector<SchoolPositionSummary>& positions,
        const std::vector<ConvergenceMarker>&     convergences = {},
        int                                       max_tokens = 300) const;

    /**
     * @brief Extract Summary.
     * @param[in] round_output Input parameter.
     * @return Return value.
     */
    SchoolPositionSummary extractSummary(
        const DiscourseRoundOutput& round_output) const;

    /**
     * @brief Validate Summary.
     * @param[in] summary Input parameter.
     */
    void validateSummary(const SchoolPositionSummary& summary) const;

private:
    /**
     * @brief Count Tokens.
     * @param[in] text Input parameter.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    static int countTokens(const std::string& text) noexcept;
    /**
     * @brief Is Valid Verdict.
     * @param[in] verdict Input parameter.
     * @return True when the operation succeeds.
     * @note Exception safety: noexcept.
     */
    static bool isValidVerdict(const std::string& verdict) noexcept;
};

} // namespace ethics
} // namespace plugins
} // namespace themis
