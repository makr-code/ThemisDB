/**
 * @file prior_round_compressor.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=5; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=1, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "ethics_ai/ethics_ai_types.h"
#include <functional>
#include <string>
#include <vector>

namespace themis {
namespace plugins {
namespace ethics {

/**
 * @brief Compression mode for prior discourse rounds.
 *
 * Three modes oriented at the RECOMP taxonomy (§12.1.2):
 *  - PRINCIPLE_CITATIONS_ONLY: Extractive — thesis_id + verdict only (~75 % reduction, ΔDC ≤ −0.05)
 *  - STRUCTURED_SUMMARY: Abstractive — LLM-generated summary (~60 % reduction, ΔDC ≤ −0.08)
 *  - HEADLINE: Ultra-sparse — only "[thesis_id: name]" tokens (~80 % reduction, ΔDC ≤ −0.15)
 */
enum class CompressionMode {
    PRINCIPLE_CITATIONS_ONLY,   ///< Extractive: thesis_id + verdict only (LLMLingua-style)
    STRUCTURED_SUMMARY,         ///< Abstractive: generative summary via small LLM (RECOMP)
    HEADLINE                    ///< Ultra-sparse: only "[thesis_id: name]" tokens
};

/**
 * @brief Configuration for PriorRoundCompressor.
 */
struct CompressionConfig {
    CompressionMode mode{CompressionMode::PRINCIPLE_CITATIONS_ONLY};
    int             trigger_round{3};           ///< Only compress rounds >= trigger_round
    int             max_tokens_per_round{300};  ///< Max output tokens per school/round
    bool            keep_thesis_id_anchors{true};
    bool            keep_verdict{true};
    std::string     coherence_anchor_field{"thesis_ids"};
};

/**
 * @brief Result of a compression operation.
 */
struct CompressionResult {
    std::string compressed_text;
    int         original_tokens{0};
    int         compressed_tokens{0};
    float       compression_ratio{0.0f};       ///< 0.0–1.0; lower = more compressed
    float       estimated_dc_loss{0.0f};       ///< ΔDC estimate (positive = information loss)
    bool        coherence_anchors_intact{true};
};

/**
 * @brief Compresses prior discourse rounds to fit within LLM context windows.
 *
 * Implements §12.1.2 of the Context-Window-Budget-Strategie. Three compression
 * modes map to the RECOMP taxonomy (extractive / abstractive / ultra-sparse).
 *
 * All methods are const and thread-safe (no mutable state).
 */
class PriorRoundCompressor {
public:
    PriorRoundCompressor() = default;

    /**
     * @brief Compress arguments from a single prior round.
     *
     * @param round_arguments Arguments produced in the round to compress.
     * @param config          Compression configuration.
     * @param current_round   Current discourse round number (1-based).
     * @return CompressionResult containing the compressed text and metrics.
     */
    CompressionResult compressPriorRound(
        const std::vector<EthicalArgument>& round_arguments,
        const CompressionConfig& config,
        int current_round) const;

    /**
     * @brief Build the full prior context for injection into the next round.
     *
     * Applies hierarchical compression: older rounds are compressed more
     * aggressively than recent rounds.
     *
     * @param all_rounds      Arguments from all previous rounds (index 0 = R1).
     * @param config          Base compression configuration.
     * @param current_round   Current discourse round number (1-based).
     * @param max_total_tokens Hard token budget for the entire prior context.
     * @return Assembled prior context string within the token budget.
     */
    std::string buildPriorContext(
        const std::vector<std::vector<EthicalArgument>>& all_rounds,
        const CompressionConfig& config,
        int current_round,
        int max_total_tokens) const;

    /**
     * @brief Estimate DC loss by computing token-level overlap.
     *
     * Implements the shared-token-overlap metric from §12.6.
     *
     * @param original_arg   Full original argument text.
     * @param compressed_arg Compressed argument text.
     * @return Estimated ΔDC (0.0 = no loss, 1.0 = total loss).
     */
    float measureDcLoss(
        const std::string& original_arg,
        const std::string& compressed_arg) const;

    /**
     * @brief Callback type for an external LLM abstractive summariser.
     *
     * Receives the full ethical argument and the maximum token budget.  Must
     * return a non-empty abstractive summary string that fits within the
     * budget, or an empty string to fall back to the built-in extractive path.
     *
     * Thread safety: fn must be callable from multiple threads.
     */
    using LlmSummaryFn = std::function<std::string(
        const EthicalArgument& arg, int max_tokens)>;

    /**
     * @brief Inject a real LLM abstractive summariser for STRUCTURED_SUMMARY mode.
     *
     * When set, `compressStructuredSummary()` delegates to @p fn instead of
     * the built-in TF-weighted extractive fallback.  An empty return from fn
     * (e.g. model timeout) transparently falls back to extractive selection.
     * Pass `nullptr` to revert to the extractive path.
     *
     * Roadmap ref: src/ethics_ai/FUTURE_ENHANCEMENTS.md §PriorRoundCompressor LLM (§12.2.1)
     */
    void setLlmSummaryFn(LlmSummaryFn fn);

    /// Approximate token count: chars / 4 (GPT-style BPE approximation).
    static int countTokens(const std::string& text) noexcept;

private:

    /// Extract principle citations (thesis_id references) from argument content.
    static std::vector<std::string> extractPrincipleCitations(
        const std::string& content);

    /// Extract verdict from argument content (looks for PROHIBIT/PERMIT/CONDITIONAL/ABSTAIN).
    static std::string extractVerdict(const std::string& content);

    /// Apply PRINCIPLE_CITATIONS_ONLY compression to a single argument.
    CompressionResult compressPrincipleCitationsOnly(
        const EthicalArgument& arg,
        const CompressionConfig& config) const;

    /// Apply HEADLINE compression to a single argument.
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
    CompressionResult compressStructuredSummary(
        const EthicalArgument& arg,
        const CompressionConfig& config) const;

    /// Injected LLM abstractive summariser (null → extractive fallback).
    LlmSummaryFn llm_summary_fn_;
};

} // namespace ethics
} // namespace plugins
} // namespace themis
