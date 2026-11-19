/**
 * @file discourse_orchestrator.h
 * @brief Layered Discourse Model (LDM) parallel execution engine — Ebene-1/2.
 *
 * @details The DiscourseOrchestrator drives the three-layer discourse pipeline
 *   defined in `src/ethics_ai/FUTURE_ENHANCEMENTS.md`:
 *
 *   - **Ebene-1** (O(N)): All N schools score the dilemma simultaneously with
 *     equal initial weight w₀ = 1/N.  Implemented via `std::async` futures;
 *     per-school LLM timeout triggers a fail-closed ABSTAIN verdict.
 *   - **Ebene-2** (O(K²·R)): Cluster-based inter-school discourse.  Produces
 *     one `ClusterPosition` per cluster and one `EpisodicMemoryEntry` per
 *     inter-cluster structural tension axis.
 *   - **Mirror schools**: Non-western schools run in parallel to Ebene-2
 *     (1 LLM inference step each) and contribute to `MetaVerdict::minority_dissent`.
 *
 * ## Thread safety
 * - `runEbene1()` and `runEbene2()` are **reentrant-safe** from a single caller
 *   thread; they spawn internal `std::async` tasks and collect results before
 *   returning.  Concurrent calls from **different** threads are not supported.
 * - `setLLMInferenceFn()` must be called before the first `runEbene1()` call.
 *
 * @note Production delta: `setLLMInferenceFn({})` activates the deterministic
 *   stub path (see STUB/SIMULATION NOTE in discourse_orchestrator.cpp).
 *
 * @since LDM-2 (Target: Q1 2027)
 */

#pragma once

#include "ethics_ai/ethics_ai_types.h"
#include "ethics_ai/ethics_selection_router.h"
#include "ethics_ai/ethics_profile_registry.h"

#include <functional>
#include <future>
#include <string>
#include <utility>
#include <vector>

namespace themis {
namespace plugins {
namespace ethics {

class DiscourseOrchestrator {
public:
    using LLMInferenceFn = std::function<
        DiscourseRoundOutput(const std::string& school_id,
                             const std::string& dilemma_text)>;

    /**
     * @brief Discourse Orchestrator.
     * @param[in,out] registry Input/output parameter.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit DiscourseOrchestrator(IEthicsProfileRegistry* registry,
                                   const RouterConfig&     config);

    ~DiscourseOrchestrator();

    // Non-copyable; move-constructible.
    DiscourseOrchestrator(const DiscourseOrchestrator&)            = delete;
    DiscourseOrchestrator& operator=(const DiscourseOrchestrator&) = delete;
    DiscourseOrchestrator(DiscourseOrchestrator&&)                 noexcept = default;
    DiscourseOrchestrator& operator=(DiscourseOrchestrator&&)      noexcept = default;

    /**
     * @brief Sets the inference callback used for each school.
     * @param fn Callback invoked with the school ID and dilemma text; an empty callback enables the deterministic
     *        non-production fallback.
     * @note Call before starting execution; concurrent mutation and execution are unsupported.
     */
    void setLLMInferenceFn(LLMInferenceFn fn);

    /**
     * @brief Sets the per-school result wait threshold.
     * @param timeout_ms Positive timeout in milliseconds.
     * @throws std::invalid_argument if timeout_ms is not positive.
     * @note A timeout classifies that school's result as ABSTAIN but does not cancel its callback. Because callbacks
     *       are run with std::async, destruction of unfinished futures may delay return; this is not a hard deadline.
     */
    void setSchoolTimeoutMs(int timeout_ms);

    /**
     * @brief Executes initial scoring after validating the equal-weight plan contract.
     * @param plan Plan containing at least one unique, non-empty school ID and a finite initial weight equal to 1/N.
     * @param dilemma_text Dilemma text passed to each configured inference callback.
     * @param mirror_policy Mirror policy retained for API compatibility; Ebene-1 does not consume it.
     * @return One result per planned school, including ABSTAIN results for callback errors or timeout classification.
     * @throws std::invalid_argument if the plan is empty, has invalid school IDs, or violates the equal-weight contract.
     * @note The per-school wait threshold does not guarantee prompt function return when a callback ignores the
     *       timeout because callback cancellation is not supported.
     */
    [[nodiscard]] std::vector<DiscourseRoundOutput> runEbene1(
        const DiscourseOrchestratorPlan& plan,
        const std::string&               dilemma_text,
        const MirrorSchoolPolicy&        mirror_policy);

    [[nodiscard]] std::pair<std::vector<ClusterPosition>,
                            std::vector<EpisodicMemoryEntry>>
    runEbene2(const DiscourseOrchestratorPlan&       plan,
              const std::vector<DiscourseRoundOutput>& ebene1_results);

    [[nodiscard]] std::vector<DiscourseRoundOutput> runMirrorSchools(
        const MirrorSchoolPolicy& mirror_policy,
        const std::string&        dilemma_text,
        const std::string&        domain);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace ethics
} // namespace plugins
} // namespace themis
