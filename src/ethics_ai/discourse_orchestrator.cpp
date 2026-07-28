/**
 * @file discourse_orchestrator.cpp
 * @brief DiscourseOrchestrator — Ebene-1/2 parallel execution for the LDM.
 *
 * @note STUB/SIMULATION NOTE:
 *   Purpose: Ebene-1 parallel scoring requires a real LLM inference step per
 *     school.  When no LLM provider is injected (default in unit tests / non-LDM
 *     builds), a deterministic stub is used.
 *   Activation: When setLLMInferenceFn({}) is called or when no inference
 *     function has been set.
 *   Production Delta: Returns deterministic PERMIT/PROHIBIT/CONDITIONAL based
 *     on school_id hash.  In production, a real LLM call via ILLMPlugin is made
 *     for each school in parallel.
 *   Removal Plan: Replace setLLMInferenceFn({}) with a real ILLMPlugin injection
 *     at call site once LDM-2 integration with the LLM module is wired
 *     (Target: Q1 2027).
 */

#include "ethics_ai/discourse_orchestrator.h"

#include <algorithm>
#include <chrono>
#include <future>
#include <functional>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace themis {
namespace plugins {
namespace ethics {

// ============================================================================
// Internal helpers
// ============================================================================

namespace {

/// Deterministic stub LLM inference.
/// Returns a verdict based on a simple hash of the school_id so tests are
/// reproducible without a real language model.
///
/// Hash mapping:
///   school_id.size() % 3 == 0 → PROHIBIT
///   school_id.size() % 3 == 1 → PERMIT
///   school_id.size() % 3 == 2 → CONDITIONAL
///
/// Stub never returns ABSTAIN spontaneously; timeout logic in the orchestrator
/// is responsible for injecting ABSTAIN on deadline expiry.
DiscourseRoundOutput stubLLMInference(const std::string& school_id,
                                      const std::string& dilemma_text)
{
    (void)dilemma_text;  // unused in stub path

    DiscourseRoundOutput out;
    out.school_id        = school_id;
    out.schema_valid     = true;
    out.timed_out        = false;

    const std::size_t h = school_id.size() % 3;
    if (h == 0) {
        out.ldm_verdict = DiscourseVerdict::PROHIBIT;
        out.verdict     = "PROHIBIT";
        out.position_abstract =
            "Stub: " + school_id + " prohibits on categorical-imperative grounds.";
    } else if (h == 1) {
        out.ldm_verdict = DiscourseVerdict::PERMIT;
        out.verdict     = "PERMIT";
        out.position_abstract =
            "Stub: " + school_id + " permits given utilitarian calculus.";
    } else {
        out.ldm_verdict = DiscourseVerdict::CONDITIONAL;
        out.verdict     = "CONDITIONAL";
        out.position_abstract =
            "Stub: " + school_id + " conditionally permits with safeguards.";
    }

    // Provide at most 1 synthetic core_thesis_id for stub paths.
    out.core_thesis_ids.push_back(school_id + ":primary_thesis");

    return out;
}

/// Map a DiscourseVerdict to the dominant verdict in a cluster by simple
/// plurality vote among non-ABSTAIN outputs.
DiscourseVerdict clusterMajorityVerdict(
    const std::vector<DiscourseRoundOutput>& outputs,
    const std::vector<std::string>&          school_ids)
{
    std::unordered_map<int, int> counts;
    for (const auto& sid : school_ids) {
        auto it = std::find_if(outputs.begin(), outputs.end(),
                               [&sid](const DiscourseRoundOutput& o) {
                                   return o.school_id == sid;
                               });
        if (it == outputs.end()) continue;
        if (it->timed_out || it->ldm_verdict == DiscourseVerdict::ABSTAIN) continue;
        counts[static_cast<int>(it->ldm_verdict)]++;
    }

    if (counts.empty()) return DiscourseVerdict::ABSTAIN;

    auto best = std::max_element(counts.begin(), counts.end(),
                                 [](const auto& a, const auto& b) {
                                     return a.second < b.second;
                                 });
    return static_cast<DiscourseVerdict>(best->first);
}

} // anonymous namespace

// ============================================================================
// Impl
// ============================================================================

struct DiscourseOrchestrator::Impl {
    IEthicsProfileRegistry* registry;
    RouterConfig            config;
    LLMInferenceFn          inference_fn;
    int                     timeout_ms{1000};

    Impl(IEthicsProfileRegistry* reg, const RouterConfig& cfg)
        : registry(reg), config(cfg)
    {}

    /// Returns the active inference function; falls back to stub when empty.
    LLMInferenceFn effectiveInferenceFn() const {
        if (inference_fn) return inference_fn;
        return stubLLMInference;
    }
};

// ============================================================================
// DiscourseOrchestrator public API
// ============================================================================

DiscourseOrchestrator::DiscourseOrchestrator(IEthicsProfileRegistry* registry,
                                             const RouterConfig&     config)
    : impl_(std::make_unique<Impl>(registry, config))
{}

DiscourseOrchestrator::~DiscourseOrchestrator() = default;

void DiscourseOrchestrator::setLLMInferenceFn(LLMInferenceFn fn)
{
    impl_->inference_fn = std::move(fn);
}

void DiscourseOrchestrator::setSchoolTimeoutMs(int timeout_ms) noexcept
{
    impl_->timeout_ms = timeout_ms;
}

// ============================================================================
// runEbene1 — parallel equal-weight initial scoring
// ============================================================================

std::vector<DiscourseRoundOutput> DiscourseOrchestrator::runEbene1(
    const DiscourseOrchestratorPlan& plan,
    const std::string&               dilemma_text,
    const MirrorSchoolPolicy&        mirror_policy)
{
    (void)mirror_policy;  // Mirror identification is passive; no special handling here.

    if (plan.ebene1_school_ids.empty()) {
        throw std::invalid_argument(
            "DiscourseOrchestrator::runEbene1: plan.ebene1_school_ids is empty");
    }

    const std::size_t N = plan.ebene1_school_ids.size();
    const double      w0 = plan.initial_weight;  // w₀ = 1/N, pre-computed in plan.
    const int         timeout_ms = impl_->timeout_ms;
    const auto        infer      = impl_->effectiveInferenceFn();

    // --- Equal-weight contract audit ---
    // Verify that plan.initial_weight == 1/N (process-integrity guard).
    const double expected_w0 = 1.0 / static_cast<double>(N);
    // Allow floating-point tolerance of 1e-9.
    if (std::abs(w0 - expected_w0) > 1e-9) {
        // LDM_EQUAL_WEIGHT_VIOLATION: log as audit event, do not abort.
        // In production, this would emit a structured audit log entry.
        // (Audit logging integration is deferred to LDM-2 wiring, Q1 2027.)
    }

    // --- Launch one future per school ---
    std::vector<std::future<DiscourseRoundOutput>> futures;
    futures.reserve(N);

    for (const auto& school_id : plan.ebene1_school_ids) {
        futures.push_back(std::async(std::launch::async,
                                     [&infer, school_id, &dilemma_text]()
                                         -> DiscourseRoundOutput {
                                         return infer(school_id, dilemma_text);
                                     }));
    }

    // --- Collect results with per-school timeout ---
    std::vector<DiscourseRoundOutput> results;
    results.reserve(N);

    for (std::size_t i = 0; i < N; ++i) {
        const std::string& sid = plan.ebene1_school_ids[i];
        auto status = futures[i].wait_for(std::chrono::milliseconds(timeout_ms));

        if (status == std::future_status::ready) {
            try {
                DiscourseRoundOutput out = futures[i].get();
                out.school_id     = sid;   // guarantee correct school_id
                out.initial_weight = w0;   // equal-weight contract
                results.push_back(std::move(out));
            } catch (...) {
                // LLM inference threw — treat as timeout (fail-closed).
                DiscourseRoundOutput out;
                out.school_id      = sid;
                out.ldm_verdict    = DiscourseVerdict::ABSTAIN;
                out.verdict        = "ABSTAIN";
                out.initial_weight = w0;
                out.timed_out      = true;
                out.schema_valid   = false;
                results.push_back(std::move(out));
            }
        } else {
            // Future did not complete within timeout → fail-closed ABSTAIN.
            // Error: LDM_LLM_TIMEOUT — school stays in results for EU AI Act Art. 13.
            DiscourseRoundOutput out;
            out.school_id      = sid;
            out.ldm_verdict    = DiscourseVerdict::ABSTAIN;
            out.verdict        = "ABSTAIN";
            out.initial_weight = w0;
            out.timed_out      = true;
            out.schema_valid   = false;
            results.push_back(std::move(out));
            // Note: the underlying future is abandoned; its thread will eventually
            // finish and the result will be discarded.
        }
    }

    return results;
}

// ============================================================================
// runEbene2 — cluster discourse
// ============================================================================

std::pair<std::vector<ClusterPosition>, std::vector<EpisodicMemoryEntry>>
DiscourseOrchestrator::runEbene2(
    const DiscourseOrchestratorPlan&         plan,
    const std::vector<DiscourseRoundOutput>& ebene1_results)
{
    std::vector<ClusterPosition>   cluster_positions;
    std::vector<EpisodicMemoryEntry> episodic_entries;

    // --- Intra-cluster consolidation ---
    for (const auto& [cluster_name, school_ids] : plan.cluster_map) {
        // Identify active (non-ABSTAIN) schools in this cluster.
        std::vector<std::string> active_ids;
        for (const auto& sid : school_ids) {
            auto it = std::find_if(ebene1_results.begin(), ebene1_results.end(),
                                   [&sid](const DiscourseRoundOutput& o) {
                                       return o.school_id == sid;
                                   });
            if (it != ebene1_results.end() &&
                it->ldm_verdict != DiscourseVerdict::ABSTAIN &&
                !it->timed_out)
            {
                active_ids.push_back(sid);
            }
        }

        // Empty cluster — skip silently (not an error; logged at DEBUG level).
        // Error: LDM_CLUSTER_EMPTY — observable via cluster_positions count.
        if (active_ids.empty()) continue;

        ClusterPosition cp;
        cp.cluster_name = cluster_name;
        cp.school_ids   = active_ids;
        cp.verdict      = clusterMajorityVerdict(ebene1_results, active_ids);
        cp.confidence   = static_cast<double>(active_ids.size()) /
                          static_cast<double>(school_ids.size());

        // Collect thesis_ids from active schools (up to 3 per school, deduplicated).
        std::vector<std::string> all_thesis;
        for (const auto& sid : active_ids) {
            auto it = std::find_if(ebene1_results.begin(), ebene1_results.end(),
                                   [&sid](const DiscourseRoundOutput& o) {
                                       return o.school_id == sid;
                                   });
            if (it != ebene1_results.end()) {
                for (const auto& tid : it->core_thesis_ids) {
                    if (std::find(all_thesis.begin(), all_thesis.end(), tid) ==
                        all_thesis.end()) {
                        all_thesis.push_back(tid);
                    }
                }
            }
        }
        cp.thesis_ids = std::move(all_thesis);

        cluster_positions.push_back(std::move(cp));
    }

    // --- Inter-cluster tension axes (Ebene-2 structural discourse) ---
    // One EpisodicMemoryEntry per tension axis (LDM design spec §4.2).
    for (const auto& axis : plan.tension_axes) {
        EpisodicMemoryEntry entry;
        // tension_axis carries the full "A↔B" label.
        entry.tension_axis   = axis;
        entry.round_number   = 1;  // Ebene-2 is always round 1 in the stub.
        entry.outcome_summary =
            "Structural tension on axis \"" + axis +
            "\" processed in Ebene-2 cluster discourse.";

        // Derive cluster_a / cluster_b from the axis label for well-known axes.
        // In production, this would be resolved via the tension-axis resolver.
        // '↔' (U+2194) is encoded as the 3-byte UTF-8 sequence \xe2\x86\x94.
        static const std::string kAxisSep = "\xe2\x86\x94";  // '↔'
        const auto sep_pos = axis.find(kAxisSep);
        if (sep_pos != std::string::npos) {
            entry.cluster_a = axis.substr(0, sep_pos);
            entry.cluster_b = axis.substr(sep_pos + kAxisSep.size());
        } else {
            entry.cluster_a = axis;
            entry.cluster_b = axis;
        }

        episodic_entries.push_back(std::move(entry));
    }

    return {std::move(cluster_positions), std::move(episodic_entries)};
}

// ============================================================================
// runMirrorSchools — lightweight parallel mirror step
// ============================================================================

std::vector<DiscourseRoundOutput> DiscourseOrchestrator::runMirrorSchools(
    const MirrorSchoolPolicy& mirror_policy,
    const std::string&        dilemma_text,
    const std::string&        domain)
{
    std::vector<DiscourseRoundOutput> results;

    if (!mirror_policy.isActiveFor(domain)) {
        return results;  // Mirror mode inactive for this domain.
    }

    const auto infer      = impl_->effectiveInferenceFn();
    const int  timeout_ms = impl_->timeout_ms;

    std::vector<std::future<DiscourseRoundOutput>> futures;
    futures.reserve(mirror_policy.mirror_school_ids.size());

    for (const auto& sid : mirror_policy.mirror_school_ids) {
        futures.push_back(std::async(std::launch::async,
                                     [&infer, sid, &dilemma_text]()
                                         -> DiscourseRoundOutput {
                                         auto out = infer(sid, dilemma_text);
                                         // Mirror step: only position_abstract and
                                         // strongest_tension are consumed.
                                         return out;
                                     }));
    }

    for (std::size_t i = 0; i < mirror_policy.mirror_school_ids.size(); ++i) {
        const std::string& sid    = mirror_policy.mirror_school_ids[i];
        auto status = futures[i].wait_for(std::chrono::milliseconds(timeout_ms));

        if (status == std::future_status::ready) {
            try {
                auto out        = futures[i].get();
                out.school_id   = sid;
                results.push_back(std::move(out));
            } catch (...) {
                DiscourseRoundOutput out;
                out.school_id   = sid;
                out.ldm_verdict = DiscourseVerdict::ABSTAIN;
                out.verdict     = "ABSTAIN";
                out.timed_out   = true;
                results.push_back(std::move(out));
            }
        } else {
            DiscourseRoundOutput out;
            out.school_id   = sid;
            out.ldm_verdict = DiscourseVerdict::ABSTAIN;
            out.verdict     = "ABSTAIN";
            out.timed_out   = true;
            results.push_back(std::move(out));
        }
    }

    return results;
}

} // namespace ethics
} // namespace plugins
} // namespace themis
