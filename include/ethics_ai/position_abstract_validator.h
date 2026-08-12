/**
 * @file position_abstract_validator.h
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
#include <stdexcept>
#include <string>
#include <vector>

namespace themis {
namespace plugins {
namespace ethics {

/**
 * @brief Error thrown when a DiscourseRoundOutput fails schema validation.
 */
struct PositionAbstractSchemaError : public std::runtime_error {
    std::string school_id;
    int round_number{0};
    explicit PositionAbstractSchemaError(
        const std::string& school, int round, const std::string& reason)
        : std::runtime_error(
              "PositionAbstractSchemaError [" + school + " R" +
              std::to_string(round) + "]: " + reason)
        , school_id(school)
        , round_number(round)
    {}
};

/**
 * @brief Configuration for position-abstract schema enforcement.
 */
struct PositionAbstractConfig {
    bool require_verdict{true};            ///< verdict must be one of the 4 allowed values
    bool require_position_abstract{true};  ///< position_abstract must be non-empty
    bool require_core_thesis_ids{true};    ///< core_thesis_ids must be non-empty
    int  max_abstract_tokens{100};         ///< Hard cap on position_abstract length in tokens
    int  max_core_thesis_ids{3};           ///< Maximum number of core_thesis_ids
};

/**
 * @brief Validates and enforces the Position-Abstract-Schema on DiscourseRoundOutput.
 *
 * Implements §12.2.3 (Khattab et al. DSPy TypedPredictor equivalent).
 * After each discourse round, the output must conform to the schema before
 * being stored or passed to subsequent rounds.
 *
 * All methods are const and thread-safe (no mutable state).
 */
class PositionAbstractValidator {
public:
    explicit PositionAbstractValidator(
        PositionAbstractConfig config = PositionAbstractConfig{});

    /**
     * @brief Validate a DiscourseRoundOutput against the position-abstract schema.
     *
     * Sets `output.schema_valid = true` on success.
     *
     * @param output  The round output to validate (non-const: sets schema_valid).
     * @throws PositionAbstractSchemaError if validation fails and strict=true.
     * @return true if valid, false if invalid (when strict=false).
     */
    bool validate(DiscourseRoundOutput& output, bool strict = true) const;

    /**
     * @brief Validate a batch of outputs; throws on first failure.
     *
     * @param outputs Vector of outputs from one discourse round (all schools).
     * @throws PositionAbstractSchemaError on first invalid output.
     */
    void validateBatch(std::vector<DiscourseRoundOutput>& outputs) const;

    /**
     * @brief Auto-repair a DiscourseRoundOutput: fill missing fields with defaults.
     *
     * Applied when an LLM fails to produce a valid position_abstract or verdict.
     * Does NOT override existing valid fields. Logs a WARN for each repaired field.
     *
     * @param output The output to repair in-place.
     * @return true if repair was applied, false if output was already valid.
     */
    bool autoRepair(DiscourseRoundOutput& output) const;

    /**
     * @brief Generate the schema injection instruction for the LLM system prompt.
     *
     * Returns a ≤ 150-token instruction that tells the LLM to produce
     * a position_abstract matching the configured schema.
     */
    std::string buildSchemaInstruction() const;

    const PositionAbstractConfig& config() const noexcept { return config_; }

private:
    PositionAbstractConfig config_;

    static bool        isValidVerdict(const std::string& v) noexcept;
    static int         countTokens(const std::string& text) noexcept;
    static std::string extractVerdictFromContent(const std::string& content);
    static std::string buildDefaultAbstract(const DiscourseRoundOutput& output);
};

} // namespace ethics
} // namespace plugins
} // namespace themis
