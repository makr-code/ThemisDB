#pragma once

/**
 * @file speculative_decoder.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include <algorithm>
#include <cstdint>
#include <mutex>
#include <random>
#include <string>
#include <vector>

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
    virtual ~SpeculativeDecoder() = default;
    // ── Configuration ────────────────────────────────────────────────

    struct Config {
        /// Number of draft tokens proposed per speculative step (K).
        size_t k = 4;

        /// Optional hard floor: always accept when p(t)/q(t) >= this value,
        /// even below the stochastic threshold.  0.0 = pure stochastic criterion.
        float min_acceptance_threshold = 0.0f;

        /// RNG seed for reproducibility in tests.  0 = use random seed.
        uint64_t rng_seed = 0;

        /**
         * @brief Remote draft shard identifier for cross-shard speculative decoding.
         *
         * When non-empty this field holds the shard address used to contact a
         * lightweight draft model running on a remote ThemisDB shard (e.g.
         * "shard-a:model:mistral-7b-q4").  The InferenceEngineEnhanced is
         * expected to use RemoteExecutor to forward draft-token requests to
         * that shard.  Falls back to the local draft model when the field is
         * empty or the remote shard is unavailable.
         *
         * Format: "<shard_id>:model:<model_id>"   (colon-separated)
         */
        std::string remote_draft_shard_id;
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

    /// Read-only access to the active configuration.
    const Config& getConfig() const noexcept { return config_; }

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
    std::mt19937 rng_ = {};

    // Single mutex protecting all mutable state (rng_ and stats_).
    // Taken for the entirety of verify() so that concurrent callers
    // sharing one instance do not race on the RNG or the counters.
    mutable std::mutex verify_mutex_;
    Statistics stats_;
};

} // namespace llm
} // namespace themis

