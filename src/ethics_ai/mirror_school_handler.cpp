/**
 * @file mirror_school_handler.cpp
 * @brief MirrorSchoolHandler — lightweight parallel mirror-school inference.
 *
 * @note PERMANENT FALLBACK NOTE:
 *   Purpose: Mirror-school scoring requires a real LLM inference step per school.
 *   Activation: When setLLMInferenceFn({}) is called or when no inference
 *     function has been set — this fallback is intentional and permanent for
 *     test / non-LDM builds.
 *   Production Delta: Returns deterministic CONDITIONAL position_abstract based
 *     on school_id.  In production, a real ILLMPlugin call is made for each
 *     mirror school in parallel by injecting a fn via setLLMInferenceFn().
 *   Wiring: Inject a real ILLMPlugin-backed function via setLLMInferenceFn() when
 *     LDM-5 LLM wiring is complete (Target: Q2 2027).
 *     The deterministic fallback remains permanently for test / non-LDM builds.
 */

#include "ethics_ai/mirror_school_handler.h"

#include <chrono>
#include <future>
#include <string>
#include <vector>

namespace themis {
namespace plugins {
namespace ethics {

// ============================================================================
// Stub mirror inference
// ============================================================================

namespace {

/// Deterministic stub mirror inference.
/// Each non-western school produces a brief position_abstract with a
/// culturally-representative framing.
DiscourseRoundOutput stubMirrorInference(const std::string& school_id,
                                         const std::string& dilemma_text)
{
    (void)dilemma_text;

    DiscourseRoundOutput out;
    out.school_id    = school_id;
    out.schema_valid = true;
    out.timed_out    = false;

    // Stub: deterministic CONDITIONAL for all mirror schools.
    // In production, the LLM generates a culturally-grounded abstract.
    out.ldm_verdict      = DiscourseVerdict::CONDITIONAL;
    out.verdict          = "CONDITIONAL";
    out.position_abstract =
        "Mirror-school stub [" + school_id +
        "]: conditional perspective; cultural context requires further deliberation.";
    // strongest_tension is stored in EpisodicMemoryEntry (LDM-3), not in
    // DiscourseRoundOutput.  Mirror output here is the position_abstract only.

    return out;
}

} // anonymous namespace

// ============================================================================
// MirrorSchoolHandler implementation
// ============================================================================

void MirrorSchoolHandler::setLLMInferenceFn(LLMInferenceFn fn)
{
    inference_fn_ = std::move(fn);
}

void MirrorSchoolHandler::setSchoolTimeoutMs([[maybe_unused]] int timeout_ms) noexcept
{
    timeout_ms_ = timeout_ms;
}

std::vector<DiscourseRoundOutput> MirrorSchoolHandler::runMirror(
    const std::vector<std::string>& mirror_school_ids,
    const std::string&              dilemma_text,
    const std::string&              domain)
{
    (void)domain;  // Domain is used by caller (MirrorSchoolPolicy::isActiveFor);
                   // forwarded for context but not consumed directly here.

    std::vector<DiscourseRoundOutput> results = {};

    if (mirror_school_ids.empty()) {
      return results;
    }

    const auto infer      = inference_fn_ ? inference_fn_ : stubMirrorInference;
    const int  timeout_ms = timeout_ms_;

    std::vector<std::future<DiscourseRoundOutput>> futures;
    futures.reserve(mirror_school_ids.size());

    // --- Launch one async task per mirror school ---
    for (const auto& sid : mirror_school_ids) {
        futures.push_back(std::async(std::launch::async,
                                     [&infer, sid, &dilemma_text]()
                                         -> DiscourseRoundOutput {
                                         return infer(sid, dilemma_text);
                                     }));
    }

    // --- Collect with per-school timeout (fail-closed: ABSTAIN on timeout) ---
    results.reserve(mirror_school_ids.size());
    for (std::size_t i = 0; i < mirror_school_ids.size(); ++i) {
        const std::string& sid    = mirror_school_ids[i];
        auto status = futures[i].wait_for(std::chrono::milliseconds(timeout_ms));

        if (status == std::future_status::ready) {
            try {
                auto out      = futures[i].get();
                out.school_id = sid;
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
            // LDM_LLM_TIMEOUT for mirror school — still included in minority_dissent.
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
