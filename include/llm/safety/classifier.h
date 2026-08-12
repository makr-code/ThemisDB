/**
 * @file classifier.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/** @brief Safety classifier. */
class SafetyClassifier {
public:
    using InferenceFn = std::function<std::optional<SafetyClassification>(std::string_view)>;

    explicit SafetyClassifier(InferenceFn inference_fn = nullptr);

    void setInferenceFn(InferenceFn inference_fn);
    bool hasInferenceFn() const;

    SafetyClassification classify(std::string_view text) const;
    std::vector<SafetyClassification> classifyBatch(
        const std::vector<std::string>& texts,
        std::size_t max_parallelism = 0) const;

private:
    SafetyClassification fallbackClassify(std::string_view text) const;

    InferenceFn inference_fn_;
};

} // namespace themis::llm::safety
