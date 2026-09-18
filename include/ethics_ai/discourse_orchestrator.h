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
     * @brief Set LLMInference Fn.
     * @param[in] fn Input parameter.
     */
    void setLLMInferenceFn(LLMInferenceFn fn);

    /**
     * @brief Set School Timeout Ms.
     * @param[in] timeout_ms Input parameter.
     * @note Exception safety: noexcept.
     */
    void setSchoolTimeoutMs(int timeout_ms) noexcept;

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
