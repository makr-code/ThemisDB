/**
 * @file classifier.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=13, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "llm/safety/classifier.h"

#include <algorithm>
#include <cctype>
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
    while (static_cast<size_t>(index) < texts.size()) {
        const std::size_t chunk = std::min(workers, static_cast<int>(texts.size()) - index);
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
        SafetyClassification out;
        out.label = SafetyLabel::BLOCK;
        out.confidence = 0.95;
        out.rationale = "Matched high-risk policy signal";
        out.source = "rule_based";
        return out;
    }

    if (containsAny(lowered, review_signals)) {
        SafetyClassification out;
        out.label = SafetyLabel::REVIEW;
        out.confidence = 0.70;
        out.rationale = "Matched medium-risk policy signal";
        out.source = "rule_based";
        return out;
    }

    SafetyClassification out;
    out.label = SafetyLabel::SAFE;
    out.confidence = 0.90;
    out.rationale = "No risky signal detected";
    out.source = "rule_based";
    return out;
}

} // namespace themis::llm::safety
