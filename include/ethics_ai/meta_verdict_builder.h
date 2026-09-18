/**
 * @file meta_verdict_builder.h
 * @brief Ebene-3 convergence-counting MetaVerdict assembler for the LDM.
 *
 * @details MetaVerdictBuilder combines Ebene-1 school verdicts, Ebene-2 cluster
 *   positions, legal-DB grounding, and mirror-school minority-dissent output into
 *   a single `MetaVerdict` that is EU AI Act Art. 13 compliant.
 *
 *   Convergence formula:
 *   ```
 *   convergence_score(v) = |{schools: Ebene-1 verdict == v}| / N_active
 *   ```
 *   where N_active = number of non-ABSTAIN schools in Ebene-1.
 *
 *   Edge cases:
 *   - N_active == 0 (all ABSTAIN) → DISSENT MetaVerdict; EthicsErrorCode::LDM_ALL_ABSTAINED.
 *   - Legal-DB unavailable → `legal_grounding.grounding_available = false`; no exception.
 *
 * ## Thread safety
 * `buildMetaVerdict()` is stateless (all inputs are const references); it is
 * safe to call from multiple threads simultaneously.  `setLegalGrounding()` must
 * be called before the first `buildMetaVerdict()` call (not thread-safe).
 *
 * @since LDM-4 (Target: Q2 2027)
 */

#pragma once

#include "ethics_ai/ethics_ai_types.h"

#include <string>
#include <vector>

namespace themis {
namespace plugins {
namespace ethics {

class MetaVerdictBuilder {
public:
    MetaVerdictBuilder()  = default;
    ~MetaVerdictBuilder() = default;

    // Non-copyable; move-constructible.
    MetaVerdictBuilder(const MetaVerdictBuilder&)            = delete;
    MetaVerdictBuilder& operator=(const MetaVerdictBuilder&) = delete;
    MetaVerdictBuilder(MetaVerdictBuilder&&)                 noexcept = default;
    MetaVerdictBuilder& operator=(MetaVerdictBuilder&&)      noexcept = default;

    /**
     * @brief Set Legal Grounding.
     * @param[in] grounding Input parameter.
     * @note Exception safety: noexcept.
     */
    void setLegalGrounding(LegalGrounding grounding) noexcept;

    [[nodiscard]] MetaVerdict buildMetaVerdict(
        const std::vector<DiscourseRoundOutput>& ebene1_results,
        const std::vector<ClusterPosition>&      cluster_positions,
        const LegalGrounding&                    legal_grounding,
        DiscourseMode                            mode,
        const std::vector<DiscourseRoundOutput>& mirror_dissent) const;

private:
    LegalGrounding grounding_;

    [[nodiscard]] static std::string culturalRegion(
        const std::string& school_id) noexcept;
};

} // namespace ethics
} // namespace plugins
} // namespace themis
