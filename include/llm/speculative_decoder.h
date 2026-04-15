/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            speculative_decoder.h                              ║
  Version:         0.0.11                                             ║
  Last Modified:   2026-04-15 04:11:09                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     202                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 67965456c8  2026-03-22  Add constructors with default config for various classes ... ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 747406559a  2026-02-28  fix(llm): code audit — thread safety, seed truncation, re... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <vector>
#include <random>
#include <atomic>
#include <mutex>
#include <cstdint>

/**
 * @file speculative_decoder.h
 * @brief Speculative decoding acceptance/rejection loop for latency reduction.
 *
 * Implements the draft-model verification algorithm from:
 *   Leviathan et al., "Fast Inference from Transformers via Speculative Decoding",
 *   ICML 2023 (https://arxiv.org/abs/2211.17192).
 *
 * Algorithm:
 *   1. A small draft model proposes K candidate tokens with probabilities q(t|ctx).
 *   2. The target model evaluates positions 1…K+1 in a single forward pass,
 *      producing probabilities p(t|ctx).
 *   3. For each draft token t̃ᵢ (i = 1..K):
 *      - Draw r ~ Uniform(0,1).
 *      - If r ≤ p(t̃ᵢ)/q(t̃ᵢ): accept t̃ᵢ and advance context.
 *      - Otherwise: resample a correction token from the adjusted distribution
 *        p'(t) = normalize(max(0, p(t) − q(t))) and stop.
 *   4. If all K draft tokens were accepted: sample one additional token from p.
 *
 * Consumer: InferenceEngineEnhanced (draft-model path).
 */

namespace themis {
namespace llm {

/**
 * @brief Stateful speculative decoding verifier.
 *
 * Thread-safety: verify(), getStatistics(), and resetStatistics() are
 * all individually thread-safe — each acquires `verify_mutex_` before
 * touching any mutable state (RNG and statistics).  Multiple callers may
 * therefore share one SpeculativeDecoder instance safely.
 */
class SpeculativeDecoder {
public:
    // ── Configuration ────────────────────────────────────────────────

    struct Config {
        /// Number of draft tokens proposed per speculative step (K).
        size_t k = 4;

        /// Optional hard floor: always accept when p(t)/q(t) >= this value,
        /// even below the stochastic threshold.  0.0 = pure stochastic criterion.
        float min_acceptance_threshold = 0.0f;

        /// RNG seed for reproducibility in tests.  0 = use random seed.
        uint64_t rng_seed = 0;
    };

    // ── Result of one verify() call ──────────────────────────────────

    struct VerifyResult {
        /// The draft-token IDs that were accepted (subset of the input K tokens).
        std::vector<int> accepted_tokens;

        /// Correction token: resampled from the adjusted distribution when a
        /// draft token is rejected, or freshly sampled from the target distribution
        /// when all K draft tokens were accepted.  -1 if vocab size is 0.
        int bonus_token = -1;

        /// Number of accepted draft tokens (0..K).
        size_t num_accepted = 0;

        /// True when all K draft tokens were accepted.
        bool all_accepted = false;

        /// Fraction of draft tokens accepted this step (num_accepted / K).
        float acceptance_rate = 0.0f;
    };

    // ── Cumulative statistics ─────────────────────────────────────────

    struct Statistics {
        /// Total draft tokens presented to verify().
        size_t total_draft_tokens = 0;
        /// Total draft tokens accepted.
        size_t total_accepted_tokens = 0;
        /// Total draft tokens rejected.
        size_t total_rejected_tokens = 0;
        /// Running average acceptance rate across all steps (0..1).
        double avg_acceptance_rate = 0.0;
        /// Total verify() calls (steps).
        size_t total_steps = 0;
    };

    // ── Lifecycle ────────────────────────────────────────────────────

    SpeculativeDecoder();
    explicit SpeculativeDecoder(const Config& config);

    // ── Core interface ───────────────────────────────────────────────

    /**
     * @brief Verify K draft tokens against target-model logits.
     *
     * @param draft_tokens   K token IDs proposed by the draft model.
     * @param draft_logits   K × vocab_size raw logits from the draft model
     *                       (one row per proposed token position).
     * @param target_logits  (K+1) × vocab_size raw logits from the target model
     *                       (positions 1…K for verification, position K+1 for
     *                       the bonus token when all K are accepted).
     *
     * @return VerifyResult  Accepted tokens, correction/bonus token, and
     *                       per-step acceptance statistics.
     *
     * @pre draft_tokens.size() == draft_logits.size()
     * @pre target_logits.size() == draft_tokens.size() + 1
     * @pre All logit rows have the same vocab_size > 0.
     * @throws std::invalid_argument on precondition violations.
     */
    VerifyResult verify(
        const std::vector<int>&                      draft_tokens,
        const std::vector<std::vector<float>>&       draft_logits,
        const std::vector<std::vector<float>>&       target_logits
    );

    // ── Statistics ───────────────────────────────────────────────────

    /// Snapshot of cumulative statistics.  Thread-safe read.
    Statistics getStatistics() const;

    /// Reset all cumulative counters to zero.
    void resetStatistics();

    // ── Helpers (public for unit-testing) ────────────────────────────

    /**
     * @brief Compute softmax probabilities from raw logits.
     * @param logits  Raw logit vector (any finite float values).
     * @return        Probability vector summing to 1.0.
     */
    static std::vector<float> softmax(const std::vector<float>& logits);

    /**
     * @brief Compute the adjusted distribution p'(t) = normalize(max(0, p-q)).
     *
     * Used when a draft token is rejected to sample the correction token
     * without bias toward over-represented draft tokens.
     *
     * @param target_probs  Target model probabilities (sums to 1).
     * @param draft_probs   Draft model probabilities (sums to 1).
     * @return              Adjusted, renormalized distribution.
     */
    static std::vector<float> adjustedDistribution(
        const std::vector<float>& target_probs,
        const std::vector<float>& draft_probs
    );

    /**
     * @brief Sample a single token index from a probability distribution.
     * @param probs  Probability vector (must sum to > 0).
     * @param rng    Random number generator.
     * @return       Sampled token index.
     */
    static int sampleToken(const std::vector<float>& probs, std::mt19937& rng);

private:
    Config     config_;
    std::mt19937 rng_;

    // Single mutex protecting all mutable state (rng_ and stats_).
    // Taken for the entirety of verify() so that concurrent callers
    // sharing one instance do not race on the RNG or the counters.
    mutable std::mutex verify_mutex_;
    Statistics stats_;
};

} // namespace llm
} // namespace themis
