/**
 * @file llm_token_estimator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <cstddef>
#include <functional>
#include <string>

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
        if (text.empty()) {
          return 0;
        }
        return (text.size() + chars_per_token_ - 1) / chars_per_token_;
    }

private:
    std::size_t chars_per_token_;
};

/**
 * @brief Token estimator backed by an external tokenizer function.
 *
 * Wraps any callable that maps a string to a token count.  Intended for use
 * with the llama.cpp tokenizer (or any BPE-compatible tokenizer such as
 * tiktoken-cpp) where the caller provides an encode function via a
 * `std::function` callback.
 *
 * Example usage with the llama.cpp-backed `LlamaTokenizer`:
 * @code
 * auto llama_tok = std::make_shared<themis::llm::lora::LlamaTokenizer>(model_path);
 * auto estimator = std::make_unique<TiktokenEstimator>(
 *     [llama_tok](const std::string& text) -> std::size_t {
 *         return llama_tok->encode(text, false).size();   // add_bos=false
 *     });
 * handler.setTokenEstimator(std::move(estimator));
 * @endcode
 *
 * If the provided function is null the estimator falls back to
 * `CharDivisionEstimator` with ratio 4, preserving backward compatibility.
 */
class TiktokenEstimator : public TokenEstimator {
public:
    /// Signature: takes a UTF-8 string, returns the token count.
    using TokenizeFunc = std::function<std::size_t(const std::string&)>;

    /**
     * @brief Construct with an external tokenize function.
     * @param tokenize_fn  Callable returning the number of tokens for the
     *                     input string.  Must be thread-safe if estimate() is
     *                     called concurrently.  If null, falls back to
     *                     CharDivisionEstimator with ratio=4.
     */
    explicit TiktokenEstimator(TokenizeFunc tokenize_fn)
        : tokenize_fn_(std::move(tokenize_fn))
        , fallback_(4)
    {}

    std::size_t estimate(const std::string& text) const override {
        if (text.empty()) {
          return 0;
        }
        if (tokenize_fn_) {
            return tokenize_fn_(text);
        }
        return fallback_.estimate(text);
    }

private:
    TokenizeFunc          tokenize_fn_;
    CharDivisionEstimator fallback_;
};

} // namespace aql
} // namespace themis
