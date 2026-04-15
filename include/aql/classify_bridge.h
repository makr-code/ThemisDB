/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            classify_bridge.h                                  ║
  Version:         0.0.10                                             ║
  Last Modified:   2026-04-15 05:33:19                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     120                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • f62f9c89c4  2026-03-14  feat(aql): wire detectIntentWithNativeNLP() to IClassifyF... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

namespace themis {
namespace aql {

/**
 * @brief Result returned by a classify operation.
 *
 * Mirrors the CLASSIFY(text, categories) -> {category, confidence, scores}
 * AQL function output.
 */
struct ClassifyResult {
    /// Winning category label (empty string on failure).
    std::string category;

    /// Confidence in [0, 1] for the winning category.
    double confidence = 0.0;

    /// Per-category scores (label -> score).
    std::unordered_map<std::string, double> scores;
};

/**
 * @brief Abstract interface for zero-shot text classification.
 *
 * Concrete implementations can delegate to the AQL CLASSIFY function, an
 * embedded model, or any other classification back-end.  Inject a pointer
 * into DocsAssistantFunctions via setClassifier() to activate native NLP
 * intent detection.
 */
class IClassifyFn {
public:
    virtual ~IClassifyFn() = default;

    /**
     * @brief Classify @p text into one of @p categories.
     *
     * @param text       Input text to classify.
     * @param categories Candidate category labels.
     * @return ClassifyResult with the best-matching category and its score.
     */
    virtual ClassifyResult classify(const std::string& text,
                                    const std::vector<std::string>& categories) const = 0;
};

/**
 * @brief No-op fallback that always returns an empty / unknown result.
 *
 * Used as the default when no real classifier has been injected, preserving
 * the original behaviour (fall through to the LLM path).
 */
class NullClassifyFn final : public IClassifyFn {
public:
    ClassifyResult classify(const std::string& /*text*/,
                            const std::vector<std::string>& /*categories*/) const override {
        return ClassifyResult{};   // category = "", confidence = 0.0
    }
};

/**
 * @brief Concrete IClassifyFn that delegates to the AQL FunctionRegistry.
 *
 * Calls CLASSIFY(text, categories) through the global FunctionRegistry and
 * converts the JSON result into a ClassifyResult.  Registered in the AQL
 * module initialiser so that DocsAssistantFunctions can use it automatically
 * once wired via setClassifier().
 *
 * Expected JSON shape from CLASSIFY:
 * @code
 * {
 *   "category":   "<label>",
 *   "confidence": 0.92,
 *   "scores":     { "configuration": 0.92, "troubleshooting": 0.05, ... }
 * }
 * @endcode
 */
class AQLFunctionClassifyBridge final : public IClassifyFn {
public:
    ClassifyResult classify(const std::string& text,
                            const std::vector<std::string>& categories) const override;
};

/**
 * @brief Register AQLFunctionClassifyBridge as the active classifier in
 *        DocsAssistantFunctions and ensure the global singleton is wired.
 *
 * Call this once during AQL module initialisation (e.g. from the same site
 * that calls registerBuiltinFunctions()).
 */
void registerClassifyBridge();

} // namespace aql
} // namespace themis
