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

/**
 * @brief Parallel Ebene-1/2 orchestrator for the Layered Discourse Model.
 *
 * Constructed once per discourse session.  Inject a real LLM inference
 * function via `setLLMInferenceFn()` for production use; omit it (or pass
 * an empty function) to activate the deterministic stub path used in tests.
 *
 * ## Ownership
 * - The `IEthicsProfileRegistry*` pointer is **non-owning**; the caller must
 *   ensure the registry outlives this object.
 * - The `RouterConfig` is copied at construction time.
 *
 * @since LDM-2 (Target: Q1 2027)
 */
class DiscourseOrchestrator {
public:
    /**
     * @brief LLM inference function signature.
     *
     * Called once per school during Ebene-1.  The function receives the
     * combined dilemma context and the school identifier; returns a
     * `DiscourseRoundOutput` (only `ldm_verdict`, `position_abstract`,
     * `core_thesis_ids`, `timed_out` are consumed by the orchestrator).
     *
     * @param school_id     School identifier.
     * @param dilemma_text  Full dilemma context.
     * @return              Partially-filled `DiscourseRoundOutput`.
     *
     * @note Must be thread-safe (called from `std::async` worker tasks).
     */
    using LLMInferenceFn = std::function<
        DiscourseRoundOutput(const std::string& school_id,
                             const std::string& dilemma_text)>;

    /**
     * @brief Construct the orchestrator.
     *
     * @param registry  Non-owning pointer to the profile registry.
     *                  Must remain valid for the lifetime of this object.
     * @param config    Router configuration (copied).
     */
    explicit DiscourseOrchestrator(IEthicsProfileRegistry* registry,
                                   const RouterConfig&     config);

    ~DiscourseOrchestrator();

    // Non-copyable; move-constructible.
    DiscourseOrchestrator(const DiscourseOrchestrator&)            = delete;
    DiscourseOrchestrator& operator=(const DiscourseOrchestrator&) = delete;
    DiscourseOrchestrator(DiscourseOrchestrator&&)                 = default;
    DiscourseOrchestrator& operator=(DiscourseOrchestrator&&)      = default;

    /**
     * @brief Inject a real LLM inference function.
     *
     * Pass an empty `std::function` (the default) to activate the
     * deterministic stub path.  Must be called before the first
     * `runEbene1()` invocation.
     *
     * @param fn  LLM inference function; empty → stub mode.
     */
    void setLLMInferenceFn(LLMInferenceFn fn);

    /**
     * @brief Set per-school LLM timeout (Ebene-1 fail-closed contract).
     *
     * Schools whose LLM call exceeds @p timeout_ms milliseconds receive
     * `timed_out=true` and `ldm_verdict=ABSTAIN`.
     *
     * @param timeout_ms  Timeout in milliseconds.  Default: 1000 ms.
     */
    void setSchoolTimeoutMs(int timeout_ms) noexcept;

    /**
     * @brief Run Ebene-1: parallel equal-weight initial scoring.
     *
     * Launches one `std::async` task per school in
     * `plan.ebene1_school_ids`.  Each task calls the injected LLM inference
     * function (or the stub).  Results are collected synchronously before
     * returning.
     *
     * Contract:
     * - `result[i].initial_weight == plan.initial_weight` for all i.
     * - Schools that time out: `timed_out=true`, `ldm_verdict=ABSTAIN`.
     * - All N school entries are always present in the returned vector.
     *
     * @param plan         Discourse plan from `planDiscourse()`.
     * @param dilemma_text Full dilemma text forwarded to each LLM call.
     * @param mirror_policy Mirror-school activation policy (used to identify
     *                      which schools participate as mirrors in Ebene-2).
     * @return             One `DiscourseRoundOutput` per school in plan order.
     *
     * @throws std::invalid_argument if plan.ebene1_school_ids is empty.
     */
    [[nodiscard]] std::vector<DiscourseRoundOutput> runEbene1(
        const DiscourseOrchestratorPlan& plan,
        const std::string&               dilemma_text,
        const MirrorSchoolPolicy&        mirror_policy);

    /**
     * @brief Run Ebene-2: cluster-based inter-school discourse.
     *
     * Produces one `ClusterPosition` per cluster in `plan.cluster_map`
     * (populated for LAYERED_FULL) and one `EpisodicMemoryEntry` per
     * inter-cluster tension axis in `plan.tension_axes`.
     *
     * Only non-ABSTAIN schools participate in cluster consolidation.
     * Empty clusters (0 non-ABSTAIN schools) are silently skipped.
     *
     * @param plan          Discourse plan (must be LAYERED_FULL for cluster_map).
     * @param ebene1_results Ebene-1 output from `runEbene1()`.
     * @return              Pair of {cluster_positions, episodic_entries}.
     */
    [[nodiscard]] std::pair<std::vector<ClusterPosition>,
                            std::vector<EpisodicMemoryEntry>>
    runEbene2(const DiscourseOrchestratorPlan&       plan,
              const std::vector<DiscourseRoundOutput>& ebene1_results);

    /**
     * @brief Run mirror schools in parallel (lightweight, 1 inference step each).
     *
     * Activates when `mirror_policy.isActiveFor(domain)` is true.  Each
     * mirror school produces a `position_abstract` (≤ 100 tokens) and a
     * `strongest_tension` string.  Results are always persisted in
     * `MetaVerdict::minority_dissent`.
     *
     * Runs in parallel to Ebene-2 (caller's responsibility to launch both
     * concurrently if desired).
     *
     * @param mirror_policy  Policy controlling which schools to activate.
     * @param dilemma_text   Dilemma context forwarded to each LLM call.
     * @param domain         Dilemma domain key (e.g. "bioethics").
     * @return               One `DiscourseRoundOutput` per mirror school.
     */
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
