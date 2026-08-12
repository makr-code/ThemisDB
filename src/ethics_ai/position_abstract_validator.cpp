/**
 * @file position_abstract_validator.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "ethics_ai/position_abstract_validator.h"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace themis {
namespace plugins {
namespace ethics {

// ---------------------------------------------------------------------------
// Private static helpers
// ---------------------------------------------------------------------------

bool PositionAbstractValidator::isValidVerdict(const std::string &v) noexcept {
    return v == "PROHIBIT" || v == "PERMIT" || v == "CONDITIONAL" || v == "ABSTAIN";
}

int PositionAbstractValidator::countTokens(const std::string &text) noexcept {
    return static_cast<int>((text.size() + 3) / 4);
}

std::string PositionAbstractValidator::extractVerdictFromContent(const std::string &content) {
    // Uppercase copy for case-insensitive search
    std::string upper = content;
    std::transform(upper.begin(), upper.end(), upper.begin(),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });

    for (const auto &candidate : {"PROHIBIT", "CONDITIONAL", "PERMIT", "ABSTAIN"}) {
        if (upper.find(candidate) != std::string::npos) {
            return std::string(candidate);
        }
    }
    return "";
}

std::string PositionAbstractValidator::buildDefaultAbstract(const DiscourseRoundOutput &output) {
    std::string thesis_joined;
    for (std::size_t i = 0; i < output.core_thesis_ids.size(); ++i) {
        if (i > 0) {
            thesis_joined += ", ";
        }
        thesis_joined += output.core_thesis_ids[i];
    }
    if (thesis_joined.empty()) {
        thesis_joined = "none";
    }

    std::ostringstream oss;
    oss << "[" << output.school_id << "] Verdict: " << output.verdict << ". Core: " << thesis_joined << ".";
    return oss.str();
}

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

PositionAbstractValidator::PositionAbstractValidator(PositionAbstractConfig config) : config_(std::move(config)) {}

// ---------------------------------------------------------------------------
// validate
// ---------------------------------------------------------------------------

bool PositionAbstractValidator::validate(DiscourseRoundOutput &output, bool strict) const {
    // 1. Verdict check
    if (config_.require_verdict && !isValidVerdict(output.verdict)) {
        if (strict) {
            throw PositionAbstractSchemaError(output.school_id, output.round_number,
                                              "invalid verdict '" + output.verdict
                                                  + "' (must be PROHIBIT|PERMIT|CONDITIONAL|ABSTAIN)");
        }
        return false;
    }

    // 2. position_abstract non-empty check
    if (config_.require_position_abstract && output.position_abstract.empty()) {
        if (strict) {
            throw PositionAbstractSchemaError(output.school_id, output.round_number, "position_abstract is empty");
        }
        return false;
    }

    // 3. position_abstract token length check
    if (!output.position_abstract.empty() && countTokens(output.position_abstract) > config_.max_abstract_tokens) {
        if (strict) {
            throw PositionAbstractSchemaError(output.school_id, output.round_number,
                                              "position_abstract exceeds " + std::to_string(config_.max_abstract_tokens)
                                                  + " tokens");
        }
        return false;
    }

    // 4. core_thesis_ids non-empty check
    if (config_.require_core_thesis_ids && output.core_thesis_ids.empty()) {
        if (strict) {
            throw PositionAbstractSchemaError(output.school_id, output.round_number, "core_thesis_ids is empty");
        }
        return false;
    }

    // 5. Truncate core_thesis_ids silently if over the limit (not an error)
    if (static_cast<int>(output.core_thesis_ids.size()) > config_.max_core_thesis_ids) {
        output.core_thesis_ids.resize(static_cast<std::size_t>(config_.max_core_thesis_ids));
    }

    output.schema_valid = true;
    return true;
}

// ---------------------------------------------------------------------------
// validateBatch
// ---------------------------------------------------------------------------

void PositionAbstractValidator::validateBatch(std::vector<DiscourseRoundOutput> &outputs) const {
    for (auto &out : outputs) {
        validate(out, /*strict=*/true);
    }
}

// ---------------------------------------------------------------------------
// autoRepair
// ---------------------------------------------------------------------------

bool PositionAbstractValidator::autoRepair(DiscourseRoundOutput &output) const {
    bool repaired = false;

    // 1. Verdict repair
    if (!isValidVerdict(output.verdict)) {
        const std::string found = extractVerdictFromContent(output.content);
        output.verdict          = found.empty() ? "ABSTAIN" : found;
        repaired                = true;
    }

    // 2. position_abstract repair: generate default if empty
    if (output.position_abstract.empty()) {
        output.position_abstract = buildDefaultAbstract(output);
        repaired                 = true;
    }

    // 3. Truncate position_abstract if too long (rough char-based truncation)
    if (countTokens(output.position_abstract) > config_.max_abstract_tokens) {
        const std::size_t max_chars = static_cast<std::size_t>(config_.max_abstract_tokens) * 4u;
        if (output.position_abstract.size() > max_chars) {
            output.position_abstract = output.position_abstract.substr(0, max_chars);
        }
        repaired = true;
    }

    // Mark valid if it now passes (non-strict, so no throw)
    validate(output, /*strict=*/false);

    return repaired;
}

// ---------------------------------------------------------------------------
// buildSchemaInstruction
// ---------------------------------------------------------------------------

std::string PositionAbstractValidator::buildSchemaInstruction() const {
    return "After your argument, provide: "
           "VERDICT: [PROHIBIT|PERMIT|CONDITIONAL|ABSTAIN] | "
           "CORE_THESES: [thesis_id1, thesis_id2] | "
           "POSITION_ABSTRACT: [≤100 token summary of your position "
           "and main opponent rebuttal]";
}

} // namespace ethics
} // namespace plugins
} // namespace themis
