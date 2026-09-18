/**
 * @file prior_round_compressor.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "ethics_ai/ethics_ai_types.h"
#include <functional>
#include <string>
#include <vector>
#include <mutex>

namespace themis {
namespace plugins {
namespace ethics {

enum class CompressionMode {
    PRINCIPLE_CITATIONS_ONLY,   ///< Extractive: thesis_id + verdict only (LLMLingua-style)
    STRUCTURED_SUMMARY,         ///< Abstractive: generative summary via small LLM (RECOMP)
    HEADLINE                    ///< Ultra-sparse: only "[thesis_id: name]" tokens
};

struct CompressionConfig {
    CompressionMode mode{CompressionMode::PRINCIPLE_CITATIONS_ONLY};
    int             trigger_round{3};           ///< Only compress rounds >= trigger_round
    int             max_tokens_per_round{300};  ///< Max output tokens per school/round
    bool            keep_thesis_id_anchors{true};
    bool            keep_verdict{true};
    std::string     coherence_anchor_field{"thesis_ids"};
};

struct CompressionResult {
    std::string compressed_text;
    int         original_tokens{0};
    int         compressed_tokens{0};
    float       compression_ratio{0.0f};       ///< 0.0–1.0; lower = more compressed
    float       estimated_dc_loss{0.0f};       ///< ΔDC estimate (positive = information loss)
    bool        coherence_anchors_intact{true};
};

class PriorRoundCompressor {
public:
    PriorRoundCompressor() = default;

    /**
     * @brief Compress Prior Round.
     * @param[in] round_arguments Input parameter.
     * @param[in] config Input parameter.
     * @param[in] current_round Input parameter.
     * @return Return value.
     */
    CompressionResult compressPriorRound(
        const std::vector<EthicalArgument>& round_arguments,
        const CompressionConfig& config,
        int current_round) const;

    /**
     * @brief Build Prior Context.
     * @param[in] all_rounds Input parameter.
     * @param[in] config Input parameter.
     * @param[in] current_round Input parameter.
     * @param[in] max_total_tokens Input parameter.
     * @return Return value.
     */
    std::string buildPriorContext(
        const std::vector<std::vector<EthicalArgument>>& all_rounds,
        const CompressionConfig& config,
        int current_round,
        int max_total_tokens) const;

    /**
     * @brief Measure Dc Loss.
     * @param[in] original_arg Input parameter.
     * @param[in] compressed_arg Input parameter.
     * @return Return value.
     */
    float measureDcLoss(
        const std::string& original_arg,
        const std::string& compressed_arg) const;

    using LlmSummaryFn = std::function<std::string(
        const EthicalArgument& arg, int max_tokens)>;

    /**
     * @brief Set Llm Summary Fn.
     * @param[in] fn Input parameter.
     */
    void setLlmSummaryFn(LlmSummaryFn fn);

    /**
     * @brief Count Tokens.
     * @param[in] text Input parameter.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    static int countTokens(const std::string& text) noexcept;

private:

    /**
     * @brief Extract Principle Citations.
     * @param[in] content Input parameter.
     * @return Return value.
     */
    static std::vector<std::string> extractPrincipleCitations(
        const std::string& content);

    /**
     * @brief Extract Verdict.
     * @param[in] content Input parameter.
     * @return Return value.
     */
    static std::string extractVerdict(const std::string& content);

    /**
     * @brief Compress Principle Citations Only.
     * @param[in] arg Input parameter.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    CompressionResult compressPrincipleCitationsOnly(
        const EthicalArgument& arg,
        const CompressionConfig& config) const;

    /**
     * @brief Compress Headline.
     * @param[in] arg Input parameter.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    CompressionResult compressHeadline(
        const EthicalArgument& arg,
        const CompressionConfig& config) const;

    // STUB/SIMULATION NOTE:
    // Purpose: STRUCTURED_SUMMARY mode requires a small LLM call. Until the
    //          LLM backend integration (§1, Target Q3 2026) is complete, this
    //          falls back to TF-weighted extractive sentence selection.
    // Activation: Always when no LlmSummaryFn is injected via setLlmSummaryFn().
    // Production Delta: Real impl sends the argument to a "small" model tier
    //                   (§12.2.1 Cascade) and returns its abstractive summary.
    // Removal Plan: Replace with real LLM dispatch when IArgumentGenerator
    //               is integrated with the Cascade Router (§12.2.1 Q3 2026).
    /**
     * @brief Compress Structured Summary.
     * @param[in] arg Input parameter.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    CompressionResult compressStructuredSummary(
        const EthicalArgument& arg,
        const CompressionConfig& config) const;

    LlmSummaryFn llm_summary_fn_;
    
    mutable std::mutex llm_fn_mutex_;
};

} // namespace ethics
} // namespace plugins
} // namespace themis
