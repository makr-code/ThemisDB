#pragma once

/**
 * @file lookup_decoder.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include <cstddef>
#include <deque>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace themis {
namespace llm {

class LookupDecoder {
public:
    /**
     * @brief Lookup Decoder.
     * @return Return value.
     */
    virtual ~LookupDecoder() = default;
    // ── Hash helper (public for unit-testing) ────────────────────────

    struct VectorHash {
        size_t operator()(const std::vector<int>& v) const noexcept;
    };

    // ── Configuration ────────────────────────────────────────────────

    struct Config {
        size_t ngram_min = 2;
        size_t ngram_max = 4;
        size_t max_draft_tokens = 8;
        size_t max_index_entries = 32768;
    };

    // ── Cumulative statistics ─────────────────────────────────────────

    struct Stats {
        size_t total_probe_calls = 0;
        size_t total_hits = 0;
        size_t total_draft_tokens_proposed = 0;
        double hit_rate() const {
            if (total_probe_calls == 0) {
              return 0.0;
            }
            return static_cast<double>(total_hits) / total_probe_calls;
        }
    };

    // ── Lifecycle ────────────────────────────────────────────────────

    LookupDecoder();
    /**
     * @brief Lookup Decoder.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit LookupDecoder(const Config& config);

    /**
     * @brief ── Index construction ───────────────────────────────────────────
     * @param[in] tokens Input parameter.
     */

    void buildFromPrompt(const std::vector<int>& tokens);

    /**
     * @brief Update From Tokens.
     * @param[in] new_tokens Input parameter.
     */
    void updateFromTokens(const std::vector<int>& new_tokens);

    void loadStaticNgrams(
        const std::unordered_map<std::vector<int>,
                                 std::vector<int>,
                                 VectorHash>& ngrams);

    /**
     * @brief Clear.
     */
    void clear();

    // ── Draft proposal ───────────────────────────────────────────────

    std::vector<int> proposeDraftTokens(
        const std::vector<int>& context_tokens,
        size_t                  max_draft = 0
    ) const;

    /**
     * @brief ── Statistics ───────────────────────────────────────────────────
     * @return Return value.
     */

    Stats getStats() const;
    /**
     * @brief Reset Stats.
     */
    void  resetStats();

private:
    Config config_;

    // n-gram index: key = n-gram token IDs, value = continuation tokens.
    // Guarded by mutex_ so build/update/propose are thread-safe.
    using NGramMap = std::unordered_map<std::vector<int>, std::vector<int>, VectorHash>;
    NGramMap index_;
    mutable std::mutex mutex_;

    // Insertion-order tracking for eviction (oldest-first).
    std::vector<std::vector<int>> insertion_order_;

    mutable Stats stats_;

    // Internal helpers

    /**
     * @brief Insert Entry.
     * @param[in] key Input parameter.
     * @param[in] continuation Input parameter.
     */
    void insertEntry(std::vector<int> key, std::vector<int> continuation);

    /**
     * @brief Index Tokens.
     * @param[in] tokens Input parameter.
     */
    void indexTokens(const std::vector<int>& tokens);
};

} // namespace llm
} // namespace themis

