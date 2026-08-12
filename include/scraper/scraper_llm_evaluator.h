/**
 * @file scraper_llm_evaluator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.11
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=5; TODO=1, Stub=1, Unimpl=0, Mock=3, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "scraper/scraper_config.h"
#include <string>
#include <vector>
#include <map>
#include <memory>

namespace themis {
namespace scraper {

// ============================================================================
// Evaluation result
// ============================================================================

/**
 * @brief Quality and relevance scores produced by the LLM evaluator.
 */
struct EvaluationResult {
    double      quality_score    = 0.0;  ///< 0.0–1.0: overall content quality
    double      gap_relevance    = 0.0;  ///< 0.0–1.0: relevance to the gap
    std::string summary;                 ///< One-sentence LLM summary of the page
    std::vector<std::string> key_entities; ///< Named entities extracted by LLM
    std::string discard_reason;          ///< Non-empty when the document should be discarded
    bool        below_threshold  = false;///< true when quality_score < threshold

    bool shouldDiscard() const {
        return below_threshold || !discard_reason.empty();
    }
};

// ============================================================================
// Interface
// ============================================================================

/**
 * @brief Scores a scraped document against the gap context.
 *
 * LLM path:  calls LLMPluginManager::instance().generate() when
 *            THEMIS_ENABLE_LLM is defined and a model is available.
 * Fallback:  heuristic keyword-frequency scoring (always available).
 */
class IScraperLLMEvaluator {
public:
    virtual ~IScraperLLMEvaluator() = default;

    /**
     * @brief Evaluate a document.
     * @param text        Plain text extracted from the scraped page.
     * @param url         Source URL (for logging / metadata).
     * @param gap         Gap context to evaluate against.
     * @param threshold   Minimum quality_score to pass; set below_threshold
     *                    when score falls short.
     * @return EvaluationResult.
     */
    virtual EvaluationResult evaluate(
        const std::string& text,
        const std::string& url,
        const GapContext&  gap,
        double             threshold) const = 0;
};

// ============================================================================
// Production evaluator
// ============================================================================

/**
 * @brief LLM-backed evaluator with keyword-match fallback.
 *
 * When THEMIS_ENABLE_LLM is defined and LLMPluginManager has a loaded model,
 * the evaluator sends a structured German-language prompt and parses the JSON
 * response.  The fallback heuristic scores documents by keyword density and
 * text length regardless of LLM availability.
 */
class ScraperLLMEvaluator : public IScraperLLMEvaluator {
public:
    ScraperLLMEvaluator() = default;
    ~ScraperLLMEvaluator() override = default;

    ScraperLLMEvaluator(const ScraperLLMEvaluator&) = delete;
    ScraperLLMEvaluator& operator=(const ScraperLLMEvaluator&) = delete;

    EvaluationResult evaluate(
        const std::string& text,
        const std::string& url,
        const GapContext&  gap,
        double             threshold) const override;

    /// Returns true when the LLM backend is reachable.
    bool isLlmAvailable() const;

private:
    /// Build the LLM prompt for quality/relevance evaluation.
    static std::string buildPrompt(const std::string& text,
                                   const GapContext&  gap);

    /// Parse the LLM JSON response into an EvaluationResult.
    static EvaluationResult parseLlmResponse(const std::string& response,
                                              double threshold);

    /// Heuristic fallback when LLM is unavailable.
    static EvaluationResult heuristicScore(const std::string& text,
                                            const GapContext&  gap,
                                            double             threshold);
};

// ============================================================================
// In-memory mock (tests)
// ============================================================================

/**
 * @brief Test double that returns pre-configured evaluation results.
 */
class InMemoryLLMEvaluator : public IScraperLLMEvaluator {
public:
    void setDefaultResult(EvaluationResult r) { default_ = std::move(r); }
    void injectResult(const std::string& url_substring, EvaluationResult r) {
        overrides_[url_substring] = std::move(r);
    }
    int callCount() const { return call_count_; }

    EvaluationResult evaluate(
        const std::string& /*text*/,
        const std::string& url,
        const GapContext&  /*gap*/,
        double             threshold) const override {
        ++call_count_;
        for (const auto& kv : overrides_) {
            if (url.find(kv.first) != std::string::npos) {
                auto r = kv.second;
                r.below_threshold = r.quality_score < threshold;
                return r;
            }
        }
        auto r = default_;
        r.below_threshold = r.quality_score < threshold;
        return r;
    }

private:
    EvaluationResult default_{0.8, 0.8, "mock summary", {}, "", false};
    std::map<std::string, EvaluationResult> overrides_;
    mutable int call_count_ = 0;
};

} // namespace scraper
} // namespace themis
