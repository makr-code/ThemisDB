#include "llm/safety/classifier.h"

#include <algorithm>
#include <future>
#include <regex>
#include <thread>

namespace themis::llm::safety {

namespace {

std::string toLower(std::string_view in) {
    std::string out(in.begin(), in.end());
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return out;
}

bool containsAny(const std::string& text, const std::vector<std::string>& needles) {
    for (const auto& needle : needles) {
        if (text.find(needle) != std::string::npos) {
            return true;
        }
    }
    return false;
}

} // namespace

SafetyClassifier::SafetyClassifier(InferenceFn inference_fn)
    : inference_fn_(std::move(inference_fn)) {}

void SafetyClassifier::setInferenceFn(InferenceFn inference_fn) {
    inference_fn_ = std::move(inference_fn);
}

bool SafetyClassifier::hasInferenceFn() const {
    return static_cast<bool>(inference_fn_);
}

SafetyClassification SafetyClassifier::classify(std::string_view text) const {
    if (inference_fn_) {
        if (auto inferred = inference_fn_(text); inferred.has_value()) {
            if (inferred->confidence < 0.0) {
                inferred->confidence = 0.0;
            }
            if (inferred->confidence > 1.0) {
                inferred->confidence = 1.0;
            }
            if (inferred->source.empty()) {
                inferred->source = "inference";
            }
            return *inferred;
        }
    }

    return fallbackClassify(text);
}

std::vector<SafetyClassification> SafetyClassifier::classifyBatch(
    const std::vector<std::string>& texts,
    std::size_t max_parallelism) const
{
    std::vector<SafetyClassification> out(texts.size());
    if (texts.empty()) {
        return out;
    }

    const std::size_t hw = std::max<std::size_t>(1, std::thread::hardware_concurrency());
    const std::size_t workers = max_parallelism == 0 ? hw : std::max<std::size_t>(1, max_parallelism);

    std::size_t index = 0;
    while (index < texts.size()) {
        const std::size_t chunk = std::min(workers, texts.size() - index);
        std::vector<std::future<SafetyClassification>> futures;
        futures.reserve(chunk);

        for (std::size_t i = 0; i < chunk; ++i) {
            const std::size_t pos = index + i;
            futures.emplace_back(std::async(std::launch::async, [this, &texts, pos]() {
                return classify(texts[pos]);
            }));
        }

        for (std::size_t i = 0; i < chunk; ++i) {
            out[index + i] = futures[i].get();
        }

        index += chunk;
    }

    return out;
}

SafetyClassification SafetyClassifier::fallbackClassify(std::string_view text) const {
    static const std::vector<std::string> block_signals = {
        "build a bomb", "bypass authentication", "steal credentials", "drop table", "exfiltrate"
    };
    static const std::vector<std::string> review_signals = {
        "admin password", "token", "exploit", "payload", "disable security"
    };

    const std::string lowered = toLower(text);

    if (containsAny(lowered, block_signals)) {
        return {.label = SafetyLabel::BLOCK,
                .confidence = 0.95,
                .rationale = "Matched high-risk policy signal",
                .source = "rule_based"};
    }

    if (containsAny(lowered, review_signals)) {
        return {.label = SafetyLabel::REVIEW,
                .confidence = 0.70,
                .rationale = "Matched medium-risk policy signal",
                .source = "rule_based"};
    }

    return {.label = SafetyLabel::SAFE,
            .confidence = 0.90,
            .rationale = "No risky signal detected",
            .source = "rule_based"};
}

} // namespace themis::llm::safety
