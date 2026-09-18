/**
 * @file llm_token_estimator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <cstddef>
#include <functional>
#include <string>

namespace themis {
namespace aql {

class TokenEstimator {
public:
    /**
     * @brief Token Estimator.
     * @return Return value.
     */
    virtual ~TokenEstimator() = default;

    /**
     * @brief Estimate.
     * @param[in] text Input parameter.
     * @return Return value.
     */
    virtual std::size_t estimate(const std::string& text) const = 0;
};

class CharDivisionEstimator : public TokenEstimator {
public:
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

class TiktokenEstimator : public TokenEstimator {
public:
    using TokenizeFunc = std::function<std::size_t(const std::string&)>;

    /**
     * @brief Tiktoken Estimator.
     * @param[in] tokenize_fn Input parameter.
     * @return Return value.
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
