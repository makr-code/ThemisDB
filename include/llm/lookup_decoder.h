#pragma once

/**
 * @file lookup_decoder.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/**
 * @brief Draft-model-free speculative decoder using n-gram prompt lookup.
 *
 * Typical usage:
 * @code
 *   LookupDecoder decoder({.ngram_min = 2, .ngram_max = 4});
 *   decoder.buildFromPrompt(prompt_token_ids);
 *
 *   // Per decode step:
 *   auto drafts = decoder.proposeDraftTokens(context_tokens, 8);
 *   // Pass drafts to SpeculativeDecoder::verify() …
 *   decoder.updateFromTokens(accepted_tokens);
 * @endcode
 */
class LookupDecoder {
public:
    virtual ~LookupDecoder() = default;
    // ── Hash helper (public for unit-testing) ────────────────────────

    struct VectorHash {
        size_t operator()(const std::vector<int>& v) const noexcept;
    };

    // ── Configuration ────────────────────────────────────────────────

    struct Config {
        /// Minimum n-gram size for index probing.
        size_t ngram_min = 2;
        /// Maximum n-gram size for index probing (= max draft token count per step).
        size_t ngram_max = 4;
        /// Maximum number of draft tokens returned per proposeDraftTokens() call.
        size_t max_draft_tokens = 8;
        /// Maximum number of distinct n-gram keys stored in the index.
        /// Oldest entries are evicted when the limit is reached.
        size_t max_index_entries = 32768;
    };

    // ── Cumulative statistics ─────────────────────────────────────────

    struct Stats {
        /// Total proposeDraftTokens() calls.
        size_t total_probe_calls = 0;
        /// Calls that produced at least one draft token (hit).
        size_t total_hits = 0;
        /// Total draft tokens proposed across all hit steps.
        size_t total_draft_tokens_proposed = 0;
        /// Running hit rate (total_hits / total_probe_calls).
        double hit_rate() const {
            if (total_probe_calls == 0) {
              return 0.0;
            }
            return static_cast<double>(total_hits) / total_probe_calls;
        }
    };

    // ── Lifecycle ────────────────────────────────────────────────────

    LookupDecoder();
    explicit LookupDecoder(const Config& config);

    // ── Index construction ───────────────────────────────────────────

    /**
     * @brief Build (or rebuild) the n-gram index from a token sequence.
     *
     * Replaces the current context index.  Call this at the start of each
     * request with the prompt token IDs to seed the context-mode index.
     *
     * @param tokens  Token ID sequence (prompt).
     */
    void buildFromPrompt(const std::vector<int>& tokens);

    /**
     * @brief Incrementally update the index with newly generated tokens.
     *
     * Adds n-grams formed by the tail of the existing context combined with
     * @p new_tokens to support dynamic mode (cross-step speculation).
     *
     * @param new_tokens  Freshly decoded/accepted token IDs to ingest.
     */
    void updateFromTokens(const std::vector<int>& new_tokens);

    /**
     * @brief Load a pre-built static n-gram table.
     *
     * Merges @p ngrams into the existing index.  Useful for providing a
     * corpus-derived prior (e.g., from a domain-specific document set).
     *
     * Key:   n-gram token IDs (length between ngram_min and ngram_max).
     * Value: continuation token IDs (up to max_draft_tokens).
     *
     * @param ngrams  External n-gram → continuation map.
     */
    void loadStaticNgrams(
        const std::unordered_map<std::vector<int>,
                                 std::vector<int>,
                                 VectorHash>& ngrams);

    /**
     * @brief Clear the entire index (context, dynamic, and static entries).
     */
    void clear();

    // ── Draft proposal ───────────────────────────────────────────────

    /**
     * @brief Propose draft tokens by probing the n-gram index.
     *
     * Probes from longest matching suffix (@p ngram_max) down to
     * @p ngram_min.  Returns the continuation of the first hit, trimmed to
     * @p max_draft (further capped by Config::max_draft_tokens).
     *
     * @param context_tokens  Current token context (prompt + generated so far).
     * @param max_draft       Maximum number of draft tokens to return.
     * @return                Draft token IDs (empty if no n-gram matches).
     */
    std::vector<int> proposeDraftTokens(
        const std::vector<int>& context_tokens,
        size_t                  max_draft = 0
    ) const;

    // ── Statistics ───────────────────────────────────────────────────

    Stats getStats() const;
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

    /// Insert a single n-gram → continuation entry, evicting if over capacity.
    void insertEntry(std::vector<int> key, std::vector<int> continuation);

    /// Add all n-grams formed by a token window to the index.
    void indexTokens(const std::vector<int>& tokens);
};

} // namespace llm
} // namespace themis

