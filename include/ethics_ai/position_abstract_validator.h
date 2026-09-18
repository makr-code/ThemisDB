/**
 * @file position_abstract_validator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

struct PositionAbstractSchemaError : public std::runtime_error {
    std::string school_id = {};
    int round_number{0};
    /**
     * @brief Position Abstract Schema Error.
     * @param[in] school Input parameter.
     * @param[in] round Input parameter.
     * @param[in] reason Input parameter.
     * @return Return value.
     */
    explicit PositionAbstractSchemaError(
        const std::string& school, int round, const std::string& reason)
        : std::runtime_error(
              "PositionAbstractSchemaError [" + school + " R" +
              std::to_string(round) + "]: " + reason)
        , school_id(school)
        , round_number(round)
    {}
};

struct PositionAbstractConfig {
    bool require_verdict{true};            ///< verdict must be one of the 4 allowed values
    bool require_position_abstract{true};  ///< position_abstract must be non-empty
    bool require_core_thesis_ids{true};    ///< core_thesis_ids must be non-empty
    int  max_abstract_tokens{100};         ///< Hard cap on position_abstract length in tokens
    int  max_core_thesis_ids{3};           ///< Maximum number of core_thesis_ids
};

class PositionAbstractValidator {
public:
    explicit PositionAbstractValidator(
        PositionAbstractConfig config = PositionAbstractConfig{});

    bool validate(DiscourseRoundOutput& output, bool strict = true) const;

    /**
     * @brief Validate Batch.
     * @param[in,out] outputs Input/output parameter.
     */
    void validateBatch(std::vector<DiscourseRoundOutput>& outputs) const;

    /**
     * @brief Auto Repair.
     * @param[in,out] output Input/output parameter.
     * @return True when the operation succeeds.
     */
    bool autoRepair(DiscourseRoundOutput& output) const;

    /**
     * @brief Build Schema Instruction.
     * @return Return value.
     */
    std::string buildSchemaInstruction() const;

    const PositionAbstractConfig& config() const noexcept { return config_; }

private:
    PositionAbstractConfig config_;

    /**
     * @brief Is Valid Verdict.
     * @param[in] v Input parameter.
     * @return True when the operation succeeds.
     * @note Exception safety: noexcept.
     */
    static bool        isValidVerdict(const std::string& v) noexcept;
    /**
     * @brief Count Tokens.
     * @param[in] text Input parameter.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    static int         countTokens(const std::string& text) noexcept;
    /**
     * @brief Extract Verdict From Content.
     * @param[in] content Input parameter.
     * @return Return value.
     */
    static std::string extractVerdictFromContent(const std::string& content);
    /**
     * @brief Build Default Abstract.
     * @param[in] output Input parameter.
     * @return Return value.
     */
    static std::string buildDefaultAbstract(const DiscourseRoundOutput& output);
};

} // namespace ethics
} // namespace plugins
} // namespace themis
