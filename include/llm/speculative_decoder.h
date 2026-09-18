#pragma once

/**
 * @file speculative_decoder.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
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

class SpeculativeDecoder {
public:
    /**
     * @brief Speculative Decoder.
     * @return Return value.
     */
    virtual ~SpeculativeDecoder() = default;
    // ── Configuration ────────────────────────────────────────────────

    struct Config {
        size_t k = 4;

        float min_acceptance_threshold = 0.0f;

        uint64_t rng_seed = 0;

        std::string remote_draft_shard_id;
    };

    // ── Result of one verify() call ──────────────────────────────────

    struct VerifyResult {
        std::vector<int> accepted_tokens;

        int bonus_token = -1;

        size_t num_accepted = 0;

        bool all_accepted = false;

        float acceptance_rate = 0.0f;
    };

    // ── Cumulative statistics ─────────────────────────────────────────

    struct Statistics {
        size_t total_draft_tokens = 0;
        size_t total_accepted_tokens = 0;
        size_t total_rejected_tokens = 0;
        double avg_acceptance_rate = 0.0;
        size_t total_steps = 0;
    };

    // ── Lifecycle ────────────────────────────────────────────────────

    SpeculativeDecoder();
    /**
     * @brief Speculative Decoder.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit SpeculativeDecoder(const Config& config);

    const Config& getConfig() const noexcept { return config_; }

    /**
     * @brief ── Core interface ───────────────────────────────────────────────
     * @param[in] draft_tokens Input parameter.
     * @param[in] draft_logits Input parameter.
     * @param[in] target_logits Input parameter.
     * @return Verification result.
     */

    VerifyResult verify(
        const std::vector<int>&                      draft_tokens,
        const std::vector<std::vector<float>>&       draft_logits,
        const std::vector<std::vector<float>>&       target_logits
    );

    /**
     * @brief ── Statistics ───────────────────────────────────────────────────
     * @return Access control statistics.
     */

    Statistics getStatistics() const;

    /**
     * @brief Reset Statistics.
     */
    void resetStatistics();

    /**
     * @brief ── Helpers (public for unit-testing) ────────────────────────────
     * @param[in] logits Input parameter.
     * @return Return value.
     */

    static std::vector<float> softmax(const std::vector<float>& logits);

    /**
     * @brief Adjusted Distribution.
     * @param[in] target_probs Input parameter.
     * @param[in] draft_probs Input parameter.
     * @return Return value.
     */
    static std::vector<float> adjustedDistribution(
        const std::vector<float>& target_probs,
        const std::vector<float>& draft_probs
    );

    /**
     * @brief Sample Token.
     * @param[in] probs Input parameter.
     * @param[in,out] rng Input/output parameter.
     * @return Return value.
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

