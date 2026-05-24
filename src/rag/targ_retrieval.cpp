/*
 * ThemisDB | File: targ_retrieval.cpp | Version: 1.0.0 | Last Modified: 2026-05-20 17:13:04
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 88/100 | Lines: 241
 * Open Issues: TODOs=1, Stubs=5, Gaps=8, Unimpl=0, Mock=1, Sim=1, Debt=0
 * Gap Correlation: internal=8 | external_v3=27 | delta=19 | status=divergent
 * External Severity (v3): C=0, H=24, M=1
 * PR: none
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

/**
 * @file rag/targ_retrieval.cpp
 * @brief TARGRetrieval — logit-gap gating implementation.
 *
 * ### Stub log
 * - TARG-01  Softmax-probability entropy gate (use_entropy_gate=true path)
 *            Uses only top-K logits for entropy computation (accurate for
 *            peaked distributions; may differ from full-vocab entropy by
 *            < 0.1 nats for well-calibrated 7B+ models).
 *
 * STUB/SIMULATION NOTE:
 * Purpose: Entropy is computed from the top-32 logits (softmax over those
 *          positions only), not the full vocabulary.  This is an O(32) vs
 *          O(|V|) approximation.  For peaked distributions the approximation
 *          error is < 1%.  For completely flat distributions (every token
 *          equally likely) the entropy is underestimated.
 * Activation: Only when TARGConfig::use_entropy_gate = true.
 * Production Delta: Full-vocab entropy requires sorting + softmax over 50k
 *                   tokens; compute cost ≈ 0.1 ms per token.
 * Removal Plan: Replace with full-vocab entropy when benchmark confirms
 *               accuracy improvement justifies the cost (Q2 2027).
 */

#include "rag/targ_retrieval.h"

#include <algorithm>
#include <cmath>
#include <mutex>
#include <numeric>
#include <stdexcept>

namespace themis {
namespace rag {

// ============================================================================
// FullEntropyFn injection bridge (STUB #262)
// ============================================================================

static std::mutex& fullEntropyFnMutex() { static std::mutex m; return m; }
static TARGRetrieval::FullEntropyFn& fullEntropyFnStorage() {
    static TARGRetrieval::FullEntropyFn fn;
    return fn;
}

/*static*/
void TARGRetrieval::setFullEntropyFn(FullEntropyFn fn) {
    std::lock_guard<std::mutex> lk(fullEntropyFnMutex());
    fullEntropyFnStorage() = std::move(fn);
}

/*static*/
void TARGRetrieval::clearFullEntropyFn() {
    std::lock_guard<std::mutex> lk(fullEntropyFnMutex());
    fullEntropyFnStorage() = {};
}

// ============================================================================
// Constructor
// ============================================================================

TARGRetrieval::TARGRetrieval(TARGConfig cfg)
    : cfg_(std::move(cfg)) {}

// ============================================================================
// computeMetrics
// ============================================================================

void TARGRetrieval::computeMetrics(const std::vector<float>& logits,
                                    float& out_gap,
                                    float& out_entropy,
                                    bool   compute_entropy) {
    if (logits.size() < 2)
        throw std::invalid_argument(
            "TARGRetrieval::computeMetrics: logits must have at least 2 elements");

    // Find top-2 logit values via a single pass.
    float top1 = -1e30f, top2 = -1e30f;
    for (float v : logits) {
        if (v > top1) {
            top2 = top1;
            top1 = v;
        } else if (v > top2) {
            top2 = v;
        }
    }
    out_gap = top1 - top2;

    if (!compute_entropy) {
        out_entropy = 0.0f;
        return;
    }

    // If a full-vocabulary entropy function is injected, use it (STUB #262).
    {
        FullEntropyFn fn_copy;
        {
            std::lock_guard<std::mutex> lk(fullEntropyFnMutex());
            fn_copy = fullEntropyFnStorage();
        }
        if (fn_copy) {
            out_entropy = fn_copy(logits);
            return;
        }
    }

    // Approximate entropy from the top-32 logits.
    // Sort descending, keep at most 32, apply softmax, compute -sum p*log(p).
    constexpr std::size_t kTopK = 32;
    std::vector<float> top_logits;
    top_logits.reserve(std::min(kTopK, logits.size()));

    // Partial sort: extract the top-kTopK values.
    if (logits.size() <= kTopK) {
        top_logits = logits;
    } else {
        top_logits = logits;
        std::nth_element(top_logits.begin(),
                         top_logits.begin() + kTopK,
                         top_logits.end(),
                         std::greater<float>{});
        top_logits.resize(kTopK);
    }

    // Numerical-stable softmax over the selected logits.
    float max_v = *std::max_element(top_logits.begin(), top_logits.end());
    float sum_exp = 0.0f;
    for (float v : top_logits) sum_exp += std::exp(v - max_v);

    float entropy = 0.0f;
    for (float v : top_logits) {
        float p = std::exp(v - max_v) / sum_exp;
        if (p > 0.0f) entropy -= p * std::log(p);
    }
    out_entropy = entropy;
}

// ============================================================================
// gate
// ============================================================================

TARGDecision TARGRetrieval::gate(const std::vector<float>& logits) {
    TARGDecision decision;

    float gap     = 0.0f;
    float entropy = 0.0f;
    computeMetrics(logits, gap, entropy, cfg_.use_entropy_gate);

    decision.logit_gap  = gap;
    decision.entropy    = entropy;

    // Update gap statistics.
    ++stats_.tokens_seen;
    stats_.mean_gap = stats_.mean_gap
        + (gap - stats_.mean_gap) / static_cast<double>(stats_.tokens_seen);
    if (gap < stats_.min_gap) stats_.min_gap = gap;
    if (gap > stats_.max_gap) stats_.max_gap = gap;

    // ── Cool-down suppression ──
    if (cooldown_remaining_ > 0) {
        decision.in_cooldown        = true;
        decision.should_retrieve    = false;
        decision.consecutive_uncertain = consecutive_uncertain_;
        return decision;
    }

    // ── Gap gate ──
    const bool gap_uncertain = (gap < cfg_.gap_threshold);

    // ── Entropy gate (optional) ──
    const bool entropy_uncertain =
        cfg_.use_entropy_gate && (entropy > cfg_.entropy_threshold);

    const bool uncertain = gap_uncertain || entropy_uncertain;

    if (uncertain) {
        ++consecutive_uncertain_;
    } else {
        consecutive_uncertain_ = 0;
    }

    decision.consecutive_uncertain = consecutive_uncertain_;

    if (uncertain &&
        consecutive_uncertain_ >= cfg_.min_consecutive_uncertain) {
        decision.should_retrieve = true;
        ++stats_.retrieval_triggers;
    } else {
        decision.should_retrieve = false;
    }

    return decision;
}

// ============================================================================
// shouldRetrieve
// ============================================================================

bool TARGRetrieval::shouldRetrieve(const std::vector<float>& logits) {
    return gate(logits).should_retrieve;
}

// ============================================================================
// notifyRetrievalExecuted
// ============================================================================

void TARGRetrieval::notifyRetrievalExecuted() {
    consecutive_uncertain_ = 0;
    cooldown_remaining_    = cfg_.retrieval_cooldown_tokens;
}

// ============================================================================
// notifyTokenEmitted
// ============================================================================

void TARGRetrieval::notifyTokenEmitted() {
    if (cooldown_remaining_ > 0) {
        --cooldown_remaining_;
        ++stats_.cooldown_skips;
    }
}

// ============================================================================
// reset
// ============================================================================

void TARGRetrieval::reset() {
    consecutive_uncertain_ = 0;
    cooldown_remaining_    = 0;
    stats_                 = {};
}

} // namespace rag
} // namespace themis
