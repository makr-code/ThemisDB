/**
 * @file speculative_decoder.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 99/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "llm/speculative_decoder.h"

#include <stdexcept>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <sstream>

#include <spdlog/spdlog.h>

namespace themis {
namespace llm {

// ═══════════════════════════════════════════════════════════
// Constructor
// ═══════════════════════════════════════════════════════════

SpeculativeDecoder::SpeculativeDecoder()
    : SpeculativeDecoder(Config{}) {
}

SpeculativeDecoder::SpeculativeDecoder(const Config& config)
    : config_(config)
{
    if (config_.rng_seed == 0) {
        std::random_device rd = {};
        rng_.seed(rd());
    } else {
        // Use std::seed_seq to honour all 64 bits of the seed rather than
        // truncating to 32 bits with a direct seed() call.
        const uint32_t lo = static_cast<uint32_t>(config_.rng_seed & 0xFFFFFFFFu);
        const uint32_t hi = static_cast<uint32_t>(config_.rng_seed >> 32);
        std::seed_seq seq{lo, hi};
        rng_.seed(seq);
    }
    spdlog::debug("SpeculativeDecoder initialised: k={}, min_threshold={:.3f}",
                  config_.k, config_.min_acceptance_threshold);
}

// ═══════════════════════════════════════════════════════════
// Core verification (Leviathan et al., Algorithm 1)
// ═══════════════════════════════════════════════════════════

SpeculativeDecoder::VerifyResult SpeculativeDecoder::verify(
    const std::vector<int>&                      draft_tokens,
    const std::vector<std::vector<float>>&       draft_logits,
    const std::vector<std::vector<float>>&       target_logits
) {
    // ── Pre-condition checks ─────────────────────────────────────────
    if (draft_tokens.size() != draft_logits.size()) {
        throw std::invalid_argument(
            "draft_tokens.size() must equal draft_logits.size()");
    }
    if (target_logits.size() != draft_tokens.size() + 1) {
        std::ostringstream msg = {};
        msg << "target_logits.size() (" << target_logits.size()
            << ") must be draft_tokens.size() + 1 ("
            << (draft_tokens.size() + 1) << ")";
        throw std::invalid_argument(msg.str());
    }
    if (draft_tokens.empty()) {
        throw std::invalid_argument("draft_tokens must not be empty");
    }

    const size_t K = draft_tokens.size();

    // All logit rows must share the same vocab size.
    const size_t vocab_size = target_logits[0].size();
    if (vocab_size == 0) {
        throw std::invalid_argument("vocab_size must be > 0");
    }
    for (size_t i = 0; i < K; ++i) {
        if (draft_logits[i].size() != vocab_size) {
            throw std::invalid_argument("All logit rows must have the same vocab_size");
        }
        if (target_logits[i].size() != vocab_size) {
            throw std::invalid_argument("All logit rows must have the same vocab_size");
        }
    }
    if (target_logits[K].size() != vocab_size) {
        throw std::invalid_argument("All logit rows must have the same vocab_size");
    }

    // ── Acceptance / rejection loop ──────────────────────────────────
    // Acquire the lock covering both rng_ and stats_ for the entire loop.
    // Precondition checks above are intentionally outside the lock because
    // they only inspect the caller-supplied const arguments.
    std::lock_guard<std::mutex> lk(verify_mutex_);

    VerifyResult result;
    result.accepted_tokens.reserve(K);

    std::uniform_real_distribution<float> uniform(0.0f, 1.0f);

    size_t accepted_count = 0;
    bool   rejected = false;

    for (size_t i = 0; i < K; ++i) {
        const int    t_draft = draft_tokens[i];

        // Guard against out-of-range token ids.
        if (t_draft < 0 || static_cast<size_t>(t_draft) >= vocab_size) {
            spdlog::warn("SpeculativeDecoder: draft token {} out of vocab range {}, "
                         "rejecting", t_draft, vocab_size);
            // Treat as rejection: sample correction from target at this position.
            auto target_probs = softmax(target_logits[i]);
            auto draft_probs  = softmax(draft_logits[i]);
            auto adjusted     = adjustedDistribution(target_probs, draft_probs);
            result.bonus_token = sampleToken(adjusted, rng_);
            rejected = true;
            break;
        }

        auto target_probs = softmax(target_logits[i]);
        auto draft_probs  = softmax(draft_logits[i]);

        const float p_t = target_probs[static_cast<size_t>(t_draft)];
        const float q_t = draft_probs [static_cast<size_t>(t_draft)];

        // Acceptance probability: min(1, p/q).
        // If q == 0 the draft token has zero probability — reject immediately.
        float acceptance_prob = 0.0f;
        if (q_t > 0.0f) {
            acceptance_prob = std::min(1.0f, p_t / q_t);
        }
        // Apply optional hard-floor threshold.
        if (config_.min_acceptance_threshold > 0.0f) {
            acceptance_prob = std::max(acceptance_prob,
                                       config_.min_acceptance_threshold);
        }

        const float r = uniform(rng_);

        if (r <= acceptance_prob) {
            // Accept this draft token.
            result.accepted_tokens.push_back(t_draft);
            ++accepted_count;
        } else {
            // Reject: resample correction token from adjusted distribution.
            auto adjusted = adjustedDistribution(target_probs, draft_probs);
            result.bonus_token = sampleToken(adjusted, rng_);
            rejected = true;
            break;
        }
    }

    // If all K draft tokens were accepted, sample one bonus token from target.
    if (!rejected) {
        result.all_accepted = true;
        auto bonus_probs = softmax(target_logits[K]);
        result.bonus_token = sampleToken(bonus_probs, rng_);
    }

    result.num_accepted   = accepted_count;
    result.acceptance_rate =
        K > 0 ? static_cast<float>(accepted_count) / static_cast<float>(K) : 0.0f;

    spdlog::debug("SpeculativeDecoder::verify: accepted={}/{}, all_accepted={}, "
                  "bonus_token={}",
                  accepted_count, K, result.all_accepted, result.bonus_token);

    // ── Update cumulative statistics ─────────────────────────────────
    // (still holding verify_mutex_ from the top of the acceptance loop)
    stats_.total_draft_tokens    += K;
    stats_.total_accepted_tokens += accepted_count;
    stats_.total_rejected_tokens += (K - accepted_count);
    stats_.total_steps           += 1;

    // Exponential moving average of acceptance rate.
    if (stats_.total_steps == 1) {
        stats_.avg_acceptance_rate = result.acceptance_rate;
    } else {
        stats_.avg_acceptance_rate =
            0.95 * stats_.avg_acceptance_rate +
            0.05 * result.acceptance_rate;
    }

    return result;
}

// ═══════════════════════════════════════════════════════════
// Statistics
// ═══════════════════════════════════════════════════════════

SpeculativeDecoder::Statistics SpeculativeDecoder::getStatistics() const {
    std::lock_guard<std::mutex> lk(verify_mutex_);
    return stats_;
}

void SpeculativeDecoder::resetStatistics() {
    std::lock_guard<std::mutex> lk(verify_mutex_);
    stats_ = Statistics{};
}

// ═══════════════════════════════════════════════════════════
// Static helpers
// ═══════════════════════════════════════════════════════════

std::vector<float> SpeculativeDecoder::softmax(const std::vector<float>& logits) {
    if (logits.empty()) return {};

    // Numerically stable softmax: subtract max before exp.
    const float max_val = *std::max_element(logits.begin(), logits.end());

    std::vector<float> probs(logits.size());
    float sum = 0.0f;
    for (size_t i = 0; i < logits.size(); ++i) {
        probs[i] = std::exp(logits[i] - max_val);
        sum += probs[i];
    }
    if (sum > 0.0f) {
        for (float& p : probs) {
          p /= sum;
        }
    }
    return probs;
}

std::vector<float> SpeculativeDecoder::adjustedDistribution(
    const std::vector<float>& target_probs,
    const std::vector<float>& draft_probs
) {
    const size_t n = target_probs.size();
    std::vector<float> adjusted(n);

    float sum = 0.0f;
    for (size_t i = 0; i < n; ++i) {
        const float dp = (i < draft_probs.size()) ? draft_probs[i] : 0.0f;
        adjusted[i] = std::max(0.0f, target_probs[i] - dp);
        sum += adjusted[i];
    }

    // Renormalise.
    if (sum > 0.0f) {
        for (float& v : adjusted) {
          v /= sum;
        }
    } else {
        // Edge case: target and draft are identical distributions; fall back
        // to uniform to avoid a zero distribution.
        const float uniform_val = 1.0f / static_cast<float>(n);
        std::fill(adjusted.begin(), adjusted.end(), uniform_val);
    }

    return adjusted;
}

int SpeculativeDecoder::sampleToken(
    const std::vector<float>& probs,
    std::mt19937&              rng
) {
    if (probs.empty()) {
      return -1;
    }

    std::uniform_real_distribution<float> uniform(0.0f, 1.0f);
    const float r = uniform(rng);

    float cumulative = 0.0f;
    for (size_t i = 0; i < probs.size(); ++i) {
        cumulative += probs[i];
        if (r <= cumulative) {
            return static_cast<int>(i);
        }
    }
    // Fallback: return last token (handles floating-point rounding).
    return static_cast<int>(probs.size() - 1);
}

} // namespace llm
} // namespace themis

