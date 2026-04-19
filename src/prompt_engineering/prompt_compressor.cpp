/**
 * @file prompt_compressor.cpp
 * @brief Implementation of SimplePromptCompressor (Phase 6, v1.8.0).
 */

#include "prompt_engineering/prompt_compressor.h"

#include <algorithm>
#include <chrono>
#include <numeric>
#include <sstream>

namespace themis {
namespace prompt_engineering {

// ─────────────────────────────────────────────────────────────────────────────
// Helper functions
// ─────────────────────────────────────────────────────────────────────────────

std::vector<std::string> SimplePromptCompressor::splitParagraphs(
    const std::string& text) {

    std::vector<std::string> paragraphs;
    std::istringstream ss(text);
    std::string line;
    std::string current;

    while (std::getline(ss, line)) {
        if (line.empty() || line.find_first_not_of(" \t\r\n") == std::string::npos) {
            if (!current.empty()) {
                paragraphs.push_back(current);
                current.clear();
            }
        } else {
            if (!current.empty()) current += '\n';
            current += line;
        }
    }
    if (!current.empty()) paragraphs.push_back(current);
    return paragraphs;
}

std::vector<std::string> SimplePromptCompressor::splitWords(
    const std::string& text) {

    std::vector<std::string> words;
    std::istringstream ss(text);
    std::string word;
    while (ss >> word) words.push_back(word);
    return words;
}

std::string SimplePromptCompressor::joinWords(
    const std::vector<std::string>& words) {

    std::string result;
    for (size_t i = 0; i < words.size(); ++i) {
        if (i > 0) result += ' ';
        result += words[i];
    }
    return result;
}

// ─────────────────────────────────────────────────────────────────────────────
// Constructor
// ─────────────────────────────────────────────────────────────────────────────

SimplePromptCompressor::SimplePromptCompressor() {
    // Default GPT-2 approximation: 1 token ≈ 4 characters.
    token_estimator_ = [](const std::string& text) -> int {
        const int estimate = static_cast<int>(text.size()) / 4;
        return (estimate < 1 && !text.empty()) ? 1 : estimate;
    };

    // Default summary: placeholder string.
    // STUB/SIMULATION NOTE:
    // Purpose: Provide a deterministic, dependency-free fallback summary so
    //   the SUMMARY strategy is always functional without an LLM call.
    // Activation: Used whenever setSummaryFn() has not been called.
    // Production Delta: A real deployment should inject an LLM call via
    //   setSummaryFn() to produce an actual semantic summary.
    // Removal Plan: Replace or supplement once an LLM inference integration
    //   is wired into the prompt-engineering pipeline.
    summary_fn_ = [](const std::string& omitted_text,
                     const std::string& /*model_id*/) -> std::string {
        const int char_count = static_cast<int>(omitted_text.size());
        return "[…summary of approximately " +
               std::to_string(char_count / 4) +
               " omitted tokens…]";
    };
}

void SimplePromptCompressor::setTokenEstimator(TokenEstimatorFn fn) {
    if (fn) token_estimator_ = std::move(fn);
}

void SimplePromptCompressor::setSummaryFn(SummaryFn fn) {
    if (fn) summary_fn_ = std::move(fn);
}

// ─────────────────────────────────────────────────────────────────────────────
// Token estimation
// ─────────────────────────────────────────────────────────────────────────────

int SimplePromptCompressor::estimateTokenCount(const std::string& text) {
    return token_estimator_(text);
}

// ─────────────────────────────────────────────────────────────────────────────
// Strategy: TRUNCATE_HEAD
// ─────────────────────────────────────────────────────────────────────────────

std::string SimplePromptCompressor::truncateHead(const std::string& prompt,
                                                  int budget) const {
    const auto words = splitWords(prompt);
    if (words.empty()) return prompt;

    // Each word ≈ one token for the word-level trim pass; use the estimator
    // on the final string to be precise.
    // Approximation: average word length ≈ 5 chars → ~1.25 tokens/word.
    const int target_words =
        static_cast<int>(budget * 4.0 / 5.0);  // chars / avg_word_len

    if (static_cast<int>(words.size()) <= target_words) return prompt;

    const int skip = static_cast<int>(words.size()) - target_words;
    std::vector<std::string> kept(words.begin() + skip, words.end());
    return joinWords(kept);
}

// ─────────────────────────────────────────────────────────────────────────────
// Strategy: TRUNCATE_TAIL
// ─────────────────────────────────────────────────────────────────────────────

std::string SimplePromptCompressor::truncateTail(const std::string& prompt,
                                                  int budget) const {
    const auto words = splitWords(prompt);
    if (words.empty()) return prompt;

    const int target_words = static_cast<int>(budget * 4.0 / 5.0);
    if (static_cast<int>(words.size()) <= target_words) return prompt;

    std::vector<std::string> kept(words.begin(),
                                   words.begin() + target_words);
    return joinWords(kept);
}

// ─────────────────────────────────────────────────────────────────────────────
// Strategy: SELECTIVE_TRIM
// ─────────────────────────────────────────────────────────────────────────────

std::string SimplePromptCompressor::selectiveTrim(const std::string& prompt,
                                                    int budget,
                                                    bool preserve_system,
                                                    int  preserve_turns) const {
    auto paragraphs = splitParagraphs(prompt);
    if (paragraphs.empty()) return prompt;

    // Identify system-prompt block (first paragraph if preserve_system).
    const size_t sys_end   = preserve_system ? 1 : 0;
    const size_t tail_start =
        (static_cast<int>(paragraphs.size()) > preserve_turns)
        ? paragraphs.size() - static_cast<size_t>(preserve_turns)
        : 0;

    // Build result by starting with system + tail; fill in middle paragraphs
    // from the end until we exceed budget.
    std::vector<size_t> kept_indices;
    for (size_t i = 0; i < sys_end && i < paragraphs.size(); ++i)
        kept_indices.push_back(i);
    for (size_t i = std::max(sys_end, tail_start);
         i < paragraphs.size(); ++i)
        kept_indices.push_back(i);

    // Build the initial kept string
    std::string result;
    for (size_t idx : kept_indices) {
        if (!result.empty()) result += "\n\n";
        result += paragraphs[idx];
    }

    // Check if still over budget; if so, truncate tail
    if (token_estimator_(result) > budget) {
        result = truncateTail(result, budget);
    }
    return result;
}

// ─────────────────────────────────────────────────────────────────────────────
// Strategy: SUMMARY
// ─────────────────────────────────────────────────────────────────────────────

std::string SimplePromptCompressor::summarize(const std::string& prompt,
                                               int budget,
                                               bool preserve_system,
                                               int  preserve_turns,
                                               const std::string& model_id) const {
    auto paragraphs = splitParagraphs(prompt);
    if (paragraphs.empty()) return prompt;

    const size_t sys_end    = preserve_system ? 1 : 0;
    const size_t tail_start =
        (static_cast<int>(paragraphs.size()) > preserve_turns)
        ? paragraphs.size() - static_cast<size_t>(preserve_turns)
        : 0;

    // The "middle" is everything between sys_end and tail_start.
    std::string middle;
    for (size_t i = sys_end; i < tail_start; ++i) {
        if (!middle.empty()) middle += "\n\n";
        middle += paragraphs[i];
    }

    // Build result: system + summary placeholder + tail
    std::string result;
    for (size_t i = 0; i < sys_end && i < paragraphs.size(); ++i) {
        if (!result.empty()) result += "\n\n";
        result += paragraphs[i];
    }

    if (!middle.empty()) {
        if (!result.empty()) result += "\n\n";
        result += summary_fn_(middle, model_id);
    }

    for (size_t i = std::max(sys_end, tail_start);
         i < paragraphs.size(); ++i) {
        if (!result.empty()) result += "\n\n";
        result += paragraphs[i];
    }

    // If still over budget after summary, apply tail truncation.
    if (token_estimator_(result) > budget) {
        result = truncateTail(result, budget);
    }
    return result;
}

// ─────────────────────────────────────────────────────────────────────────────
// compress()
// ─────────────────────────────────────────────────────────────────────────────

CompressionResult SimplePromptCompressor::compress(
    const std::string&             prompt,
    const PromptCompressionConfig& config) {

    const auto t_start = std::chrono::steady_clock::now();

    CompressionResult result;
    result.strategy_used        = config.strategy;
    result.original_token_count = token_estimator_(prompt);

    // Nothing to do if already within budget.
    if (result.original_token_count <= config.target_token_budget) {
        result.compressed_prompt      = prompt;
        result.compressed_token_count = result.original_token_count;
        result.compression_ratio      = 0.0f;
        const auto t_end = std::chrono::steady_clock::now();
        result.compression_ms =
            std::chrono::duration<double, std::milli>(t_end - t_start).count();
        return result;
    }

    // Enforce max_compression_ratio: floor of tokens that may be dropped.
    const int max_drop =
        static_cast<int>(result.original_token_count * config.max_compression_ratio);
    const int effective_budget =
        std::max(1, result.original_token_count - max_drop);
    const int budget = std::max(config.target_token_budget, effective_budget);

    std::string compressed;
    switch (config.strategy) {
        case CompressionStrategy::TRUNCATE_HEAD:
            compressed = truncateHead(prompt, budget);
            break;
        case CompressionStrategy::TRUNCATE_TAIL:
            compressed = truncateTail(prompt, budget);
            break;
        case CompressionStrategy::SELECTIVE_TRIM:
        case CompressionStrategy::EMBEDDING_PRUNE:  // fallback
            compressed = selectiveTrim(prompt, budget,
                                       config.preserve_system_prompt,
                                       config.preserve_last_n_turns);
            break;
        case CompressionStrategy::SUMMARY:
            compressed = summarize(prompt, budget,
                                   config.preserve_system_prompt,
                                   config.preserve_last_n_turns,
                                   config.summary_model_id);
            break;
    }
    result.strategy_used = (config.strategy == CompressionStrategy::EMBEDDING_PRUNE)
                           ? CompressionStrategy::SELECTIVE_TRIM
                           : config.strategy;

    result.compressed_prompt      = compressed;
    result.compressed_token_count = token_estimator_(compressed);
    result.compression_ratio      =
        (result.original_token_count > 0)
        ? 1.0f - static_cast<float>(result.compressed_token_count) /
                     static_cast<float>(result.original_token_count)
        : 0.0f;
    result.compression_ratio = std::max(0.0f, result.compression_ratio);

    const auto t_end = std::chrono::steady_clock::now();
    result.compression_ms =
        std::chrono::duration<double, std::milli>(t_end - t_start).count();

    return result;
}

// ─────────────────────────────────────────────────────────────────────────────
// supportedStrategies()
// ─────────────────────────────────────────────────────────────────────────────

std::vector<CompressionStrategy> SimplePromptCompressor::supportedStrategies() const {
    return {
        CompressionStrategy::TRUNCATE_HEAD,
        CompressionStrategy::TRUNCATE_TAIL,
        CompressionStrategy::SELECTIVE_TRIM,
        CompressionStrategy::SUMMARY,
        CompressionStrategy::EMBEDDING_PRUNE,
    };
}

} // namespace prompt_engineering
} // namespace themis
