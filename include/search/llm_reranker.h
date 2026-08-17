/**
 * @file llm_reranker.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "search/learning_to_rank.h"

#include <functional>
#include <string>
#include <vector>

namespace themis {

/**
 * @brief A single candidate document for LLM re-ranking.
 */
struct LlmRerankCandidate {
    std::string document_id;      ///< Primary key of the document
    std::string content;          ///< Text snippet shown to the LLM for evaluation
    double initial_score = 0.0;   ///< Upstream retrieval score (e.g. hybrid_score)
};

/**
 * @brief A document after LLM-based re-ranking.
 */
struct LlmRerankResult {
    std::string document_id;    ///< Primary key
    double llm_score = 0.0;     ///< LLM-assigned relevance score, normalised to [0, 1]
    double initial_score = 0.0; ///< Original retrieval score preserved for reference
    double final_score = 0.0;   ///< Blended score: llm_weight * llm_score + (1-llm_weight) * initial_score
    bool llm_scored = false;    ///< Whether the LLM actually provided a score
};

/**
 * @brief Configurable re-ranker driven by an injected LLM backend.
 *
 * LlmReranker implements the "LLM feedback loop" phase of the search pipeline:
 *
 * 1. **Prompt**: batches candidate snippets with the original query into a
 *    structured prompt that instructs the LLM to rate each document 0–10.
 * 2. **Score**: parses the per-document scores from the LLM response, normalises
 *    them to [0, 1], and blends them with the initial retrieval score.
 * 3. **Rerank**: returns candidates sorted by the blended final_score descending.
 * 4. **Feedback loop**: `toClickEvents()` converts the LLM relevance judgments
 *    into `ClickEvent` objects that can be fed directly into
 *    `LearningToRank::recordClick()` and `LearningToRank::train()`, closing
 *    the feedback loop and allowing the LTR model to learn from LLM-assessed
 *    relevance without requiring real user clicks.
 *
 * ### Minimal usage
 * ```cpp
 * LlmReranker::LlmBackend llm = [](const std::string& prompt) {
 *     return my_model.generate(prompt);
 * };
 *
 * LlmReranker::Config cfg;
 * cfg.llm_weight = 0.7;
 * LlmReranker reranker(cfg, llm);
 *
 * std::vector<LlmRerankCandidate> candidates = buildCandidates(hs_results);
 * auto reranked = reranker.rerank("fast db insert", candidates);
 *
 * // Optionally feed back into LTR
 * auto clicks = LlmReranker::toClickEvents("fast db insert", reranked);
 * for (const auto& ev : clicks) ltr.recordClick(ev);
 * ltr.train();
 * ```
 *
 * ### Fallback behaviour
 * When the backend is absent or throws, `rerank()` either returns the
 * original order (when `Config::fallback_to_original == true`) or returns an
 * empty vector (when false). `rerank()` never propagates exceptions.
 *
 * @note Thread Safety: Not thread-safe. Use one instance per thread or protect
 *   shared access with a mutex.
 * @note Exception Safety: `rerank()` is noexcept at runtime; all backend errors
 *   are caught and logged. The constructor throws `std::invalid_argument` on
 *   invalid configuration.
 */
class LlmReranker {
public:
    /**
     * @brief Callable type for the LLM backend (same as LlmQueryRewriter).
     *
     * Receives a fully-formed prompt and returns the raw LLM text response.
     * May throw; `rerank()` catches all exceptions from it.
     */
    using LlmBackend = std::function<std::string(const std::string& prompt)>;

    struct Config {
        /// Maximum number of candidates to include in a single prompt batch.
        size_t batch_size = 5;

        /// Blending weight for the LLM score.
        /// final = llm_weight * llm_score + (1 - llm_weight) * initial_score
        double llm_weight = 0.7;

        /// Maximum characters of each document snippet included in the prompt.
        size_t max_snippet_length = 200;

        /// When true, return candidates in original order if the LLM fails.
        bool fallback_to_original = true;

        /// Recommended maximum tokens for the LLM response (hint in prompt).
        int max_tokens = 256;

        /// Recommended sampling temperature for the LLM backend.
        /// Passed as a hint in the prompt; actual enforcement is backend-specific.
        /// Set to 0.0 (default) to omit the temperature hint from the prompt entirely.
        /// Typical values: 0.0 (deterministic/no hint), 0.5 (balanced), 1.0 (creative).
        float temperature = 0.0f;

        /// Documents with final_score below this threshold are omitted from
        /// the output. Set to 0.0 to return all candidates.
        double min_score_threshold = 0.0;
        static Config defaults() { return {}; }
    };

    /**
     * @param config   Reranker configuration.
     * @param backend  LLM backend callable; may be nullptr / empty.
     * @throws std::invalid_argument on invalid config values.
     */
    explicit LlmReranker(const Config& config = Config::defaults(),
                         LlmBackend backend = nullptr);

    LlmReranker(const LlmReranker&) = delete;
    LlmReranker& operator=(const LlmReranker&) = delete;
    LlmReranker(LlmReranker&&) noexcept = default;
    LlmReranker& operator=(LlmReranker&&) noexcept = default;

    /**
     * @brief Replace the LLM backend at runtime (e.g. after model load).
     * @param backend  New backend; pass nullptr / empty to disable LLM.
     */
    void setBackend(LlmBackend backend);

    /**
     * @brief Re-rank candidates using LLM relevance feedback.
     *
     * Splits candidates into batches of `Config::batch_size`, submits each
     * batch to the LLM, parses per-document scores (0–10 integers), normalises
     * to [0, 1], blends with `initial_score`, and sorts descending by
     * `final_score`.
     *
     * Never throws; backend errors trigger the fallback path.
     *
     * @param query       The original user query shown to the LLM.
     * @param candidates  Candidate results with content snippets populated.
     * @return Re-ranked candidates; may be filtered by `min_score_threshold`.
     */
    std::vector<LlmRerankResult> rerank(
        const std::string& query,
        const std::vector<LlmRerankCandidate>& candidates
    ) const;

    /**
     * @brief Convert LLM re-rank results into ClickEvents for LTR training.
     *
     * Results whose `llm_score >= relevance_threshold` are treated as relevant
     * ("clicked") at their rank position. This closes the LLM feedback loop:
     * ```cpp
     * auto clicks = LlmReranker::toClickEvents(query, reranked, 0.5);
     * for (const auto& ev : clicks) ltr.recordClick(ev);
     * ltr.train();
     * ```
     *
     * @param query                Query string associated with the events.
     * @param results              Re-ranked results from `rerank()`.
     * @param relevance_threshold  Minimum llm_score to treat as relevant [0, 1].
     * @return ClickEvents suitable for `LearningToRank::recordClick()`.
     */
    static std::vector<ClickEvent> toClickEvents(
        const std::string& query,
        const std::vector<LlmRerankResult>& results,
        double relevance_threshold = 0.5
    );

    const Config& getConfig() const { return config_; }

private:
    Config config_;
    LlmBackend backend_;

    /// Build a prompt that asks the LLM to rate each candidate 0–10.
    std::string buildPrompt(
        const std::string& query,
        const std::vector<LlmRerankCandidate>& batch
    ) const;

    /// Parse one integer score per line from the LLM response.
    /// Returns a vector of `count` scores in [0, 1]; missing scores default to 0.
    std::vector<double> parseScores(
        const std::string& llm_output,
        size_t count
    ) const;
};

} // namespace themis
