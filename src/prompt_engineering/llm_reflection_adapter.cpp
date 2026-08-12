/**
 * @file llm_reflection_adapter.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.12
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=4, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "prompt_engineering/llm_reflection_adapter.h"

#include <algorithm>
#include <cctype>
#include <string>

namespace themis {
namespace prompt_engineering {

// ============================================================================
// ILLMProviderReflectionAdapter
// ============================================================================

ILLMProviderReflectionAdapter::ILLMProviderReflectionAdapter(
    std::shared_ptr<ILLMProvider> llm_provider,
    ReflectionStrategy            strategy)
    : llm_(std::move(llm_provider))
    , builder_(strategy) {}

std::string ILLMProviderReflectionAdapter::generate(
    const std::string& prompt) const {
    if (!llm_) return {};
    return llm_->complete(prompt);
}

std::string ILLMProviderReflectionAdapter::critique(
    const std::string& original_prompt,
    const std::string& response) const {
    if (!llm_) return {};
    const SelfAwareContext ctx = SelfAwareContext::fromResponse(response);
    const std::string critique_prompt =
        builder_.buildCritiquePrompt(original_prompt, response, ctx);
    return llm_->complete(critique_prompt);
}

std::string ILLMProviderReflectionAdapter::revise(
    const std::string& original_prompt,
    const std::string& response,
    const std::string& critique) const {
    if (!llm_) return response;
    const SelfAwareContext ctx = SelfAwareContext::fromResponse(response);
    const std::string revision_prompt =
        builder_.buildRevisionPrompt(original_prompt, response, critique, ctx);
    return llm_->complete(revision_prompt);
}

double ILLMProviderReflectionAdapter::score(
    const std::string& prompt,
    const std::string& response) const {
    if (scorer_) {
        return scorer_->score(prompt, response);
    }
    return heuristicScore(response);
}

std::string ILLMProviderReflectionAdapter::name() const {
    if (llm_) {
        return "llm-reflection-adapter(" + llm_->name() + ")";
    }
    return "llm-reflection-adapter(null)";
}

void ILLMProviderReflectionAdapter::setStrategy(ReflectionStrategy strategy) {
    builder_.setStrategy(strategy);
}

ReflectionStrategy ILLMProviderReflectionAdapter::getStrategy() const noexcept {
    return builder_.getStrategy();
}

void ILLMProviderReflectionAdapter::setScorer(
    std::shared_ptr<IReflectionScorer> scorer) {
    scorer_ = std::move(scorer);
}

void ILLMProviderReflectionAdapter::clearScorer() {
    scorer_.reset();
}

bool ILLMProviderReflectionAdapter::hasScorer() const noexcept {
    return scorer_ != nullptr;
}

double ILLMProviderReflectionAdapter::heuristicScore(
    const std::string& response) const {
    if (response.empty()) return 0.0;

    double score = 0.5;

    const size_t len = response.size();
    if (len > 200) score += 0.1;
    if (len > 500) score += 0.1;

    // Structural rewards.
    auto lower = response;
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char c){ return static_cast<char>(std::tolower(c)); });

    if (lower.find("1.") != std::string::npos ||
        lower.find("- ") != std::string::npos ||
        lower.find(":\n") != std::string::npos) {
        score += 0.1;
    }

    // Penalise hallucination markers.
    for (const auto& marker : ReflectionHallucinationGuard::kHallucinationMarkers) {
        if (lower.find(marker) != std::string::npos) {
            score -= 0.15;
        }
    }

    return std::max(0.0, std::min(1.0, score));
}

} // namespace prompt_engineering
} // namespace themis
