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

/**
 * @brief Assembles the Ebene-3 MetaVerdict from Ebene-1/2 results.
 *
 * Inject a `LegalGrounding` via `setLegalGrounding()` before calling
 * `buildMetaVerdict()`.  When the Legal-DB is unavailable, pass a
 * `LegalGrounding` with `grounding_available = false`; no exception is thrown.
 *
 * @since LDM-4 (Target: Q2 2027)
 */
class MetaVerdictBuilder {
public:
    MetaVerdictBuilder()  = default;
    ~MetaVerdictBuilder() = default;

    // Non-copyable; move-constructible.
    MetaVerdictBuilder(const MetaVerdictBuilder&)            = delete;
    MetaVerdictBuilder& operator=(const MetaVerdictBuilder&) = delete;
    MetaVerdictBuilder(MetaVerdictBuilder&&)                 = default;
    MetaVerdictBuilder& operator=(MetaVerdictBuilder&&)      = default;

    /**
     * @brief Set the legal-DB grounding to attach to every MetaVerdict.
     *
     * Pass `LegalGrounding{.grounding_available = false}` when the Legal-DB
     * is offline.  The resulting MetaVerdict will have
     * `legal_grounding.grounding_available = false` — this is observable but
     * does not prevent MetaVerdict assembly.
     *
     * @param grounding  Legal-DB citation data (or unavailability flag).
     */
    void setLegalGrounding(LegalGrounding grounding) noexcept;

    /**
     * @brief Build a MetaVerdict from Ebene-1/2 results.
     *
     * @param ebene1_results    All N school outputs from Ebene-1.
     * @param cluster_positions Per-cluster consolidated positions from Ebene-2
     *                          (may be empty for LAYERED_FAST).
     * @param legal_grounding   Legal-DB citation; `grounding_available=false`
     *                          when Legal-DB is offline (no exception thrown).
     * @param mode              Active discourse mode.
     * @param mirror_dissent    Mirror-school outputs (always included in
     *                          MetaVerdict::minority_dissent for audit).
     * @return                  Assembled MetaVerdict.
     *
     * @note `participating_schools` in the returned MetaVerdict always
     *       contains ALL N schools (incl. ABSTAIN) for EU AI Act Art. 13.
     */
    [[nodiscard]] MetaVerdict buildMetaVerdict(
        const std::vector<DiscourseRoundOutput>& ebene1_results,
        const std::vector<ClusterPosition>&      cluster_positions,
        const LegalGrounding&                    legal_grounding,
        DiscourseMode                            mode,
        const std::vector<DiscourseRoundOutput>& mirror_dissent) const;

private:
    LegalGrounding grounding_;

    /**
     * @brief Map a school_id to its cultural region.
     *
     * @param school_id  School identifier.
     * @return           Cultural region string, e.g. "Western-European".
     */
    [[nodiscard]] static std::string culturalRegion(
        const std::string& school_id) noexcept;
};

} // namespace ethics
} // namespace plugins
} // namespace themis
