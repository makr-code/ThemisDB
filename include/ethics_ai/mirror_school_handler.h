/**
 * @file mirror_school_handler.h
 * @brief Mirror-School lightweight parallel inference handler (LDM-5).
 *
 * @details Non-western schools (islamische_ethik, konfuzianismus,
 *   buddhistische_ethik, juedische_bioethik) participate as structural
 *   self-reflection mirrors when `cross_cultural_sensitivity` is active.
 *
 *   Each mirror school executes exactly 1 LLM inference step and produces:
 *   - `position_abstract` (≤ 100 tokens)
 *   - `strongest_tension` string
 *
 *   Output is always persisted in `MetaVerdict::minority_dissent[]` — visible
 *   in the audit trail regardless of convergence_score.
 *
 * ## Thread safety
 * `runMirror()` launches internal `std::async` tasks and is safe to call
 * from a single caller thread.  Concurrent calls from different threads are
 * not supported.
 *
 * @note STUB/SIMULATION NOTE:
 *   Purpose: Mirror-school scoring requires a real LLM inference step per school.
 *   Activation: When no LLM provider is injected (default in tests / non-LDM builds).
 *   Production Delta: Returns deterministic position_abstract based on school_id.
 *     In production, a real ILLMPlugin call is made for each mirror school.
 *   Removal Plan: Inject real ILLMPlugin at call site when LDM-5 LLM wiring is
 *     complete (Target: Q2 2027).
 *
 * @since LDM-5 (Target: Q2 2027)
 */

#pragma once

#include "ethics_ai/ethics_ai_types.h"

#include <functional>
#include <string>
#include <vector>

namespace themis {
namespace plugins {
namespace ethics {

/**
 * @brief Lightweight mirror-school inference handler.
 *
 * Provides exactly 1 LLM inference step per mirror school.  Inject a real
 * inference function via `setLLMInferenceFn()` for production use; omit it
 * to activate the deterministic stub path.
 *
 * @since LDM-5 (Target: Q2 2027)
 */
class MirrorSchoolHandler {
public:
    /**
     * @brief LLM inference function signature (same as DiscourseOrchestrator).
     */
    using LLMInferenceFn = std::function<
        DiscourseRoundOutput(const std::string& school_id,
                             const std::string& dilemma_text)>;

    MirrorSchoolHandler()  = default;
    ~MirrorSchoolHandler() = default;

    // Non-copyable; move-constructible.
    MirrorSchoolHandler(const MirrorSchoolHandler&)            = delete;
    MirrorSchoolHandler& operator=(const MirrorSchoolHandler&) = delete;
    MirrorSchoolHandler(MirrorSchoolHandler&&)                 noexcept = default;
    MirrorSchoolHandler& operator=(MirrorSchoolHandler&&)      noexcept = default;

    /**
     * @brief Inject a real LLM inference function.
     *
     * Pass an empty `std::function` (default) to activate the stub path.
     *
     * @param fn  Real LLM inference function; empty → stub mode.
     */
    void setLLMInferenceFn(LLMInferenceFn fn);

    /**
     * @brief Set per-school LLM timeout.
     *
     * @param timeout_ms  Timeout in milliseconds.  Default: 1000 ms.
     */
    void setSchoolTimeoutMs(int timeout_ms) noexcept;

    /**
     * @brief Run lightweight mirror inference for the given schools.
     *
     * Each school executes 1 LLM inference step (position_abstract + strongest_tension).
     * Results are always returned regardless of convergence outcome, for audit-trail
     * inclusion in MetaVerdict::minority_dissent.
     *
     * @param mirror_school_ids  List of mirror school identifiers.
     * @param dilemma_text       Dilemma context text.
     * @param domain             Dilemma domain (e.g. "bioethics").
     * @return                   One DiscourseRoundOutput per mirror school.
     */
    [[nodiscard]] std::vector<DiscourseRoundOutput> runMirror(
        const std::vector<std::string>& mirror_school_ids,
        const std::string&              dilemma_text,
        const std::string&              domain);

private:
    LLMInferenceFn inference_fn_;
    int            timeout_ms_{1000};
};

} // namespace ethics
} // namespace plugins
} // namespace themis
