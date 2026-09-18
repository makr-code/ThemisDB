/**
 * @file classifier.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <cstddef>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace themis::llm::safety {

enum class SafetyLabel {
    SAFE,
    REVIEW,
    BLOCK
};

struct SafetyClassification {
    SafetyLabel label = SafetyLabel::SAFE;
    double confidence = 0.0;
    std::string rationale;
    std::string source = "rule_based";
};

class SafetyClassifier {
public:
    using InferenceFn = std::function<std::optional<SafetyClassification>(std::string_view)>;

    explicit SafetyClassifier(InferenceFn inference_fn = nullptr);

    /**
     * @brief Inject an inference function used by classify().
     * @param[in] inference_fn Input parameter.
     */
    void setInferenceFn(InferenceFn inference_fn);
    /**
     * @brief Has Inference Fn.
     * @return True when the operation succeeds.
     */
    bool hasInferenceFn() const;

    /**
     * @brief Classify the semantic intent of a query.
     * @param[in] text Input parameter.
     * @return Return value.
     */
    SafetyClassification classify(std::string_view text) const;
    std::vector<SafetyClassification> classifyBatch(
        const std::vector<std::string>& texts,
        std::size_t max_parallelism = 0) const;

private:
    /**
     * @brief Fallback Classify.
     * @param[in] text Input parameter.
     * @return Return value.
     */
    SafetyClassification fallbackClassify(std::string_view text) const;

    InferenceFn inference_fn_;
};

} // namespace themis::llm::safety
