/*
 * ThemisDB | File: prompt_compressor.h | Version: 0.0.1
 * Maturity: 🟢 PRODUCTION-READY | Score: 94/100
 * Gap Summary: total=7; TODO=1, Stub=5, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

/**
 * @file prompt_compressor.h
 * @brief Prompt compression and summarization for context-window reduction.
 *
 * ## Purpose
 *
 * `SimplePromptCompressor` reduces a prompt to fit within a target token
 * budget using one of several configurable strategies:
 *
 * | Strategy           | Behaviour                                                |
 * |--------------------|----------------------------------------------------------|
 * | `TRUNCATE_HEAD`    | Remove tokens from the **beginning** of the prompt.     |
 * | `TRUNCATE_TAIL`    | Remove tokens from the **end** of the prompt.           |
 * | `SELECTIVE_TRIM`   | Drop middle paragraphs while preserving the system      |
 * |                    | prompt prefix and the last N turns.                     |
 * | `SUMMARY`          | Replace the trimmed middle with a compact placeholder   |
 * |                    | `[…summary of <N> omitted tokens…]`.  In production,   |
 * |                    | a real LLM call can be injected via `setSummaryFn()`.   |
 * | `EMBEDDING_PRUNE`  | Falls back to `SELECTIVE_TRIM` (no embedding model in   |
 * |                    | this compilation unit; swap in a real retriever via     |
 * |                    | a custom `IPromptCompressor` if needed).                |
 *
 * ### Token estimation
 *
 * `estimateTokenCount()` uses the GPT-2 approximation: one token per four
 * characters.  This is a conservative lower bound; callers can override by
 * injecting a custom tokeniser function via `setTokenEstimator()`.
 *
 * ### Thread safety
 *
 * `compress()` and `estimateTokenCount()` are const-compatible after
 * construction.  `setSummaryFn()` and `setTokenEstimator()` must be called
 * before any concurrent use.
 *
 * ## Usage
 * ```cpp
 * SimplePromptCompressor compressor;
 * PromptCompressionConfig cfg;
 * cfg.strategy            = CompressionStrategy::SELECTIVE_TRIM;
 * cfg.target_token_budget = 1024;
 * cfg.preserve_last_n_turns = 2;
 *
 * auto result = compressor.compress(long_prompt, cfg);
 * // result.compression_ratio < 1.0 if compression occurred
 * ```
 *
 * Copyright (c) 2026 ThemisDB Project
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <functional>
#include <string>
#include <vector>

namespace themis {
namespace prompt_engineering {

// ── Strategy ──────────────────────────────────────────────────────────────────

/**
 * @brief Compression strategy applied by `IPromptCompressor::compress()`.
 */
enum class CompressionStrategy {
    TRUNCATE_HEAD,    ///< Remove leading tokens.
    TRUNCATE_TAIL,    ///< Remove trailing tokens.
    SELECTIVE_TRIM,   ///< Drop middle paragraphs; keep system prompt + last N turns.
    SUMMARY,          ///< Replace middle with a summary placeholder / real LLM call.
    EMBEDDING_PRUNE,  ///< Semantic relevance pruning (falls back to SELECTIVE_TRIM).
};

// ── Configuration ─────────────────────────────────────────────────────────────

/**
 * @brief Configuration for a single compression pass.
 */
struct PromptCompressionConfig {
    CompressionStrategy strategy              = CompressionStrategy::SELECTIVE_TRIM;
    int                 target_token_budget   = 2048; ///< Target tokens after compression.
    float               max_compression_ratio = 0.5f; ///< Max fraction of tokens to drop.
    bool                preserve_system_prompt = true; ///< Keep leading system-prompt block.
    int                 preserve_last_n_turns  = 3;    ///< Keep last N conversational turns.
    std::string         summary_model_id;              ///< Model ID hint for SUMMARY strategy.
};

// ── Result ────────────────────────────────────────────────────────────────────

/**
 * @brief Result of a single compression pass.
 */
struct CompressionResult {
    std::string         compressed_prompt;
    int                 original_token_count   = 0;
    int                 compressed_token_count = 0;
    float               compression_ratio      = 0.0f; ///< 0 = no compression, 1 = fully empty.
    CompressionStrategy strategy_used{CompressionStrategy::SELECTIVE_TRIM};
    double              compression_ms         = 0.0;
};

// ── Abstract interface ────────────────────────────────────────────────────────

/**
 * @brief Abstract interface for prompt compression and summarization.
 */
class IPromptCompressor {
public:
    virtual ~IPromptCompressor() = default;

    /**
     * @brief Compress @p prompt to fit within the configured token budget.
     *
     * If `original_token_count <= target_token_budget`, the prompt is
     * returned unchanged with `compression_ratio == 0.0f`.
     */
    [[nodiscard]] virtual CompressionResult compress(
        const std::string&           prompt,
        const PromptCompressionConfig& config) = 0;

    /**
     * @brief Estimate the token count of @p text.
     *
     * Default: GPT-2 approximation (chars / 4).  Implementations may use a
     * proper tokeniser.
     */
    [[nodiscard]] virtual int estimateTokenCount(const std::string& text) = 0;

    /// Returns the set of strategies this compressor supports.
    [[nodiscard]] virtual std::vector<CompressionStrategy> supportedStrategies() const = 0;
};

// ── Concrete implementation ───────────────────────────────────────────────────

/**
 * @brief Standard library–only implementation of `IPromptCompressor`.
 *
 * Dependency injection points:
 *  - `setTokenEstimator(fn)` — swap in a proper tokeniser.
 *  - `setSummaryFn(fn)`      — inject a real LLM summarisation call for the
 *                              `SUMMARY` strategy; the default writes a
 *                              placeholder `[…summary of N omitted tokens…]`.
 */
class SimplePromptCompressor final : public IPromptCompressor {
public:
    /// Tokeniser function type: maps text → token count.
    using TokenEstimatorFn = std::function<int(const std::string&)>;

    /// Summary function type: maps (omitted_text, model_id) → summary string.
    using SummaryFn = std::function<std::string(const std::string& omitted_text,
                                                 const std::string& model_id)>;

    SimplePromptCompressor();

    /// Replace the built-in GPT-2 token estimator.
    void setTokenEstimator(TokenEstimatorFn fn);

    /// Replace the built-in placeholder summary function.
    void setSummaryFn(SummaryFn fn);

    // IPromptCompressor interface
    CompressionResult compress(
        const std::string&           prompt,
        const PromptCompressionConfig& config) override;

    int estimateTokenCount(const std::string& text) override;

    std::vector<CompressionStrategy> supportedStrategies() const override;

private:
    // ── Strategy implementations ─────────────────────────────────────────────

    std::string truncateHead(const std::string& prompt, int budget) const;
    std::string truncateTail(const std::string& prompt, int budget) const;
    std::string selectiveTrim(const std::string& prompt,
                               int budget,
                               bool preserve_system,
                               int  preserve_turns) const;
    std::string summarize(const std::string& prompt,
                          int                budget,
                          bool               preserve_system,
                          int                preserve_turns,
                          const std::string& model_id) const;

    // ── Helpers ──────────────────────────────────────────────────────────────

    /// Split @p text into paragraphs separated by blank lines.
    static std::vector<std::string> splitParagraphs(const std::string& text);

    /// Split @p text into words (by whitespace).
    static std::vector<std::string> splitWords(const std::string& text);

    /// Join @p words back into a space-separated string.
    static std::string joinWords(const std::vector<std::string>& words);

    // ── State ─────────────────────────────────────────────────────────────────
    TokenEstimatorFn token_estimator_;
    SummaryFn        summary_fn_;
};

} // namespace prompt_engineering
} // namespace themis
