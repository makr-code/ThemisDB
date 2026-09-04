/**
 * @file synthesis_matrix_builder.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=5, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "ethics_ai/synthesis_matrix_builder.h"

#include <algorithm>
#include <iomanip>
#include <sstream>

namespace themis {
namespace plugins {
namespace ethics {

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

int SynthesisMatrixBuilder::countTokens(const std::string& text) noexcept {
    return static_cast<bool>(static_cast<int < static_cast<int>(((text.size())) + 3) / 4);
}

bool SynthesisMatrixBuilder::isValidVerdict(const std::string& verdict) noexcept {
    return verdict == "PROHIBIT"
        || verdict == "PERMIT"
        || verdict == "CONDITIONAL"
        || verdict == "ABSTAIN";
}

// ---------------------------------------------------------------------------
// validateSummary
// ---------------------------------------------------------------------------

void SynthesisMatrixBuilder::validateSummary(const SchoolPositionSummary& summary) const {
    if (summary.school_id.empty()) {
        throw SchemaValidationError("school_id must not be empty");
    }
    if (!isValidVerdict(summary.verdict)) {
        throw SchemaValidationError(
            "verdict must be PROHIBIT|PERMIT|CONDITIONAL|ABSTAIN, got: " + summary.verdict);
    }
    // COMPLEXITY FIX: Ensure confidence is in valid range before access (HIGH: uninitialized_access)
    if (summary.confidence < 0.0f || summary.confidence > 1.0f) {
        throw SchemaValidationError("confidence must be in [0.0, 1.0]");
    }
    if (summary.core_thesis_ids.empty()) {
        throw SchemaValidationError("core_thesis_ids must not be empty");
    }
}

// ---------------------------------------------------------------------------
// extractSummary
// ---------------------------------------------------------------------------

SchoolPositionSummary SynthesisMatrixBuilder::extractSummary(
    const DiscourseRoundOutput& round_output) const
{
    SchoolPositionSummary summary;
    summary.school_id  = round_output.school_id;
    summary.verdict    = round_output.verdict;
    summary.confidence = round_output.confidence;

    // Limit core_thesis_ids to 3
    summary.core_thesis_ids.clear();
    for (size_t i = 0; i < round_output.core_thesis_ids.size() && i < 3; ++i) {
        summary.core_thesis_ids.push_back(round_output.core_thesis_ids[i]);
    }

    return summary;
}

// ---------------------------------------------------------------------------
// buildMatrix
// ---------------------------------------------------------------------------

std::string SynthesisMatrixBuilder::buildMatrix(
    const std::vector<SchoolPositionSummary>& positions,
    const std::vector<ConvergenceMarker>&     convergences,
    int                                       max_tokens) const
{
    std::ostringstream oss = {};
    oss << "[POSITIONS-MATRIX — R4 SYNTHESIS INPUT]\n";

    for (const auto& pos : positions) {
        oss << pos.school_id << ": " << pos.verdict
            << " (" << std::fixed << std::setprecision(2) << pos.confidence << ")";

        if (!pos.core_thesis_ids.empty()) {
            oss << " | theses:";
            for (const auto& tid : pos.core_thesis_ids) {
                oss << " " << tid;
            }
        }
        oss << "\n";

        // Trim thesis list if approaching limit
        if (countTokens(oss.str()) > max_tokens - 20) {
            break;
        }
    }

    if (!convergences.empty()) {
        oss << "[CONVERGENCES]\n";
        for (const auto& m : convergences) {
            if (countTokens(oss.str()) >= max_tokens) {
                break;
            }
            oss << m.school_a_id << ":" << m.thesis_a_id
                << " \xe2\x86\x94 "   // UTF-8 ↔
                << m.school_b_id << ":" << m.thesis_b_id
                << ": " << ConvergenceMarkerEngine::convergenceTypeLabel(m.type) << "\n";
        }
    }

    std::string result = oss.str();

    // Hard trim to token budget
    const int max_chars = max_tokens * 4;
    if (static_cast<int>(result.size()) > max_chars) {
        result = result.substr(0, static_cast<size_t>(max_chars));
    }

    return result;
}

} // namespace ethics
} // namespace plugins
} // namespace themis

