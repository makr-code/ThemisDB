/*
 * ThemisDB | File: classifier.h | Version: 0.0.1 | Last Modified: 2026-06-01 11:06:12
 * Author: copilot-swe-agent[bot] | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 45
 * Gap Summary: total=0; TODO=0, Stub=0, Unimpl=0, Mock=0, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * PR History (last 5): none
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
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
