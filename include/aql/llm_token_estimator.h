/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            llm_token_estimator.h                              ║
  Version:         0.0.1                                              ║
  Last Modified:   2026-03-13                                         ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <cstddef>
#include <string>
#include <memory>

namespace themis {
namespace aql {

/**
 * @brief Abstract interface for estimating the token count of a text string.
 *
 * Implementations can range from simple character-division heuristics to
 * full BPE tokenizer wrappers.  The interface is intentionally minimal so
 * that it can be injected wherever a token-budget check is required.
 */
class TokenEstimator {
public:
    virtual ~TokenEstimator() = default;

    /**
     * @brief Estimate the number of LLM tokens in @p text.
     * @param text  UTF-8 input string.
     * @return Estimated token count (≥ 0).
     */
    virtual std::size_t estimate(const std::string& text) const = 0;
};

/**
 * @brief Simple character-division token estimator.
 *
 * Divides the character count by a configurable ratio (default 4), which
 * approximates the BPE token density for English ASCII text.  This matches
 * the legacy behaviour of `CHARS_PER_TOKEN = 4` used elsewhere in the AQL
 * module and preserves backward compatibility.
 */
class CharDivisionEstimator : public TokenEstimator {
public:
    /**
     * @brief Construct with a custom characters-per-token ratio.
     * @param chars_per_token  Number of characters per estimated token.
     *                         Must be ≥ 1.  Defaults to 4.
     */
    explicit CharDivisionEstimator(std::size_t chars_per_token = 4)
        : chars_per_token_(chars_per_token > 0 ? chars_per_token : 4)
    {}

    std::size_t estimate(const std::string& text) const override {
        if (text.empty()) return 0;
        return (text.size() + chars_per_token_ - 1) / chars_per_token_;
    }

private:
    std::size_t chars_per_token_;
};

} // namespace aql
} // namespace themis
