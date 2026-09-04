/**
 * @file reflection_tuner.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.12
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=4, M=7, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "prompt_engineering/reflection_tuner.h"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <sstream>
#include <cctype>

namespace themis {
namespace prompt_engineering {

// ============================================================================
// File-local helpers
// ============================================================================

namespace {

std::string toLower(const std::string& s) {
    std::string out = s;
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return out;
}

// Linguistic markers used to infer self-reported confidence.
const std::vector<std::string> kUncertaintyMarkers = {
    "i think", "i believe", "possibly", "perhaps", "maybe",
    "i'm not certain", "i'm not sure", "uncertain", "likely",
    "probably", "might", "could be", "i'm unsure",
    "i suspect", "i'm not confident", "i cannot confirm",
    "it seems", "it appears"
};

const std::vector<std::string> kConfidenceMarkers = {
    "definitely", "certainly", "absolutely", "clearly", "obviously",
    "without doubt", "i'm confident", "i'm certain", "exactly",
    "precisely", "i know", "i'm sure", "it is clear"
};

} // anonymous namespace

// ============================================================================
// ReflectionHallucinationGuard — static data
// ============================================================================

const std::vector<std::string> ReflectionHallucinationGuard::kHallucinationMarkers = {
    "i cannot verify",
    "i'm not sure about",
    "this may be incorrect",
    "however, i should note",
    "i may be wrong",
    "actually, let me reconsider",
    "wait, i made an error",
    "i need to correct myself",
    "my previous response was incorrect",
    "i hallucinated",
    "this is not accurate",
    "i fabricated",
    "i confabulated",
    "i cannot confirm",
    "this might not be accurate"
};

// ============================================================================
// SelfAwareContext
// ============================================================================

SelfAwareContext SelfAwareContext::fromResponse(const std::string& response) {
    SelfAwareContext ctx;
    const std::string lower = toLower(response);

    size_t uncertainty_count = 0;
    size_t confidence_count  = 0;

    for (const auto& marker : kUncertaintyMarkers) {
        if (lower.find(marker) != std::string::npos) {
            ctx.uncertainty_markers.push_back(marker);
            ++uncertainty_count;
        }
    }
    for (const auto& marker : kConfidenceMarkers) {
        if (lower.find(marker) != std::string::npos) {
            ++confidence_count;
        }
    }

    ctx.has_uncertain_claims = !ctx.uncertainty_markers.empty();

    const size_t total = confidence_count + uncertainty_count;
    if (total == 0) {
        ctx.confidence = 0.7;  // neutral — no signals detected
    } else {
        ctx.confidence = static_cast<double>(confidence_count) /
                         static_cast<double>(total);
        ctx.confidence = std::max(0.1, std::min(1.0, ctx.confidence));
    }

    return ctx;
}

nlohmann::json SelfAwareContext::toJson() const {
    nlohmann::json j;
    j["confidence"]          = confidence;
    j["domain"]              = domain;
    j["has_uncertain_claims"] = has_uncertain_claims;
    j["uncertainty_markers"] = uncertainty_markers;
    return j;
}

// ============================================================================
// ReflectionResult
// ============================================================================

nlohmann::json ReflectionResult::toJson() const {
    nlohmann::json j;
    j["final_response"]                = final_response;
    j["converged"]                     = converged;
    j["halted_by_hallucination_guard"] = halted_by_hallucination_guard;
    j["total_iterations"]              = total_iterations;
    j["initial_quality"]               = initial_quality;
    j["final_quality"]                 = final_quality;
    j["quality_improvement"]           = quality_improvement;
    j["quality_trajectory"]            = quality_trajectory;
    j["self_aware_context"]            = self_aware_context.toJson();

    nlohmann::json steps_arr = nlohmann::json::array();
    for (const auto& step : steps) {
        nlohmann::json s;
        s["iteration"]               = step.iteration;
        s["response"]                = step.response;
        s["critique"]                = step.critique;
        s["quality_score"]           = step.quality_score;
        s["quality_delta"]           = step.quality_delta;
        s["hallucination_suspected"] = step.hallucination_suspected;
        s["metadata"]                = step.metadata;
        steps_arr.push_back(s);
    }
    j["steps"]    = steps_arr;
    j["metadata"] = metadata;
    return j;
}

// ============================================================================
// DynamicReflectionPromptBuilder
// ============================================================================

DynamicReflectionPromptBuilder::DynamicReflectionPromptBuilder(
    ReflectionStrategy strategy)
    : strategy_(strategy) {}

void DynamicReflectionPromptBuilder::setStrategy(ReflectionStrategy strategy) {
    strategy_ = strategy;
}

ReflectionStrategy DynamicReflectionPromptBuilder::getStrategy() const noexcept {
    return strategy_;
}

std::string DynamicReflectionPromptBuilder::buildSelfAwareContextHeader(
    const SelfAwareContext& ctx) const {
    // Only inject a header when it carries actionable information.
    if (ctx.uncertainty_markers.empty() &&
        ctx.confidence >= 0.4 &&
        ctx.confidence <= 0.9) {
        return {};
    }

    std::ostringstream out = {};
    out << "[Self-Awareness Context]\n";

    if (ctx.has_uncertain_claims) {
        out << "Your previous response contained uncertainty markers (";
        for (size_t i = 0; i <static_cast<int>(ctx.uncertainty_markers.size()); ++i) {
            if (i > 0) {
              out << ", ";
            }
            out << '"' << ctx.uncertainty_markers[i] << '"';
        }
        out << "). Pay special attention to verifying factual claims.\n";
    }

    if (ctx.confidence < 0.4) {
        out << "Your self-reported confidence is low ("
            << static_cast<int>(ctx.confidence * 100)
            << "%). Be extra critical of unverified claims in the response.\n";
    } else if (ctx.confidence > 0.9) {
        out << "Your self-reported confidence is high ("
            << static_cast<int>(ctx.confidence * 100)
            << "%). Check for potential overconfidence that may mask inaccuracies.\n";
    }

    out << '\n';
    return out.str();
}

std::string DynamicReflectionPromptBuilder::buildCritiquePrompt(
    const std::string& original_prompt,
    const std::string& response,
    const SelfAwareContext& ctx) const {

    std::ostringstream out = {};
    out << buildSelfAwareContextHeader(ctx);

    switch (strategy_) {
    case ReflectionStrategy::SELF_REFINE:
        out << "Review the following response and provide constructive criticism.\n\n";
        out << "Original task:\n" << original_prompt << "\n\n";
        out << "Response to critique:\n" << response << "\n\n";
        out << "Identify:\n"
            << "1. Factual inaccuracies or unverified claims\n"
            << "2. Missing important information\n"
            << "3. Logical inconsistencies\n"
            << "4. Areas where the response could be more precise or helpful\n\n";
        out << "Provide a concise, actionable critique:";
        break;

    case ReflectionStrategy::REFLEXION:
        out << "You are reflecting on your previous response to improve future performance.\n\n";
        out << "Task you were given:\n" << original_prompt << "\n\n";
        out << "Your response:\n" << response << "\n\n";
        out << "Reflect on:\n"
            << "- What worked well in your response?\n"
            << "- What failed or was suboptimal?\n"
            << "- What specific knowledge or reasoning was missing?\n"
            << "- What would you do differently?\n\n";
        out << "Verbal reflection:";
        break;

    case ReflectionStrategy::CONSTITUTIONAL:
        out << "Review the following response for quality and accuracy.\n\n";
        out << "Task:\n" << original_prompt << "\n\n";
        out << "Response:\n" << response << "\n\n";
        out << "Critique based on helpfulness, harmlessness, and honesty:";
        break;

    case ReflectionStrategy::SOCRATIC:
        out << "Using Socratic questioning, challenge the following response.\n\n";
        out << "Task:\n" << original_prompt << "\n\n";
        out << "Claim / Response:\n" << response << "\n\n";
        out << "Question the underlying assumptions and ask probing questions "
            << "that would expose weaknesses in the reasoning:";
        break;
    }

    return out.str();
}

std::string DynamicReflectionPromptBuilder::buildRevisionPrompt(
    const std::string& original_prompt,
    const std::string& response,
    const std::string& critique,
    const SelfAwareContext& ctx) const {

    std::ostringstream out = {};
    out << buildSelfAwareContextHeader(ctx);

    switch (strategy_) {
    case ReflectionStrategy::SELF_REFINE:
    [[fallthrough]];\n    case ReflectionStrategy::REFLEXION:
    [[fallthrough]];\n    case ReflectionStrategy::SOCRATIC:
        out << "Revise your previous response by addressing the critique below.\n\n";
        out << "Original task:\n" << original_prompt << "\n\n";
        out << "Previous response:\n" << response << "\n\n";
        out << "Critique:\n" << critique << "\n\n";
        out << "Provide an improved response that addresses the critique. "
            << "Preserve what was correct in the original response:\n";
        break;

    case ReflectionStrategy::CONSTITUTIONAL:
        out << "Revise your response to address the constitutional critique.\n\n";
        out << "Original task:\n" << original_prompt << "\n\n";
        out << "Original response:\n" << response << "\n\n";
        out << "Constitutional critique:\n" << critique << "\n\n";
        out << "Write a revised response that is more helpful, harmless, and honest:\n";
        break;
    }

    return out.str();
}

std::string DynamicReflectionPromptBuilder::buildConstitutionalCritiquePrompt(
    const std::string& response,
    const std::vector<std::string>& principles) const {

    std::ostringstream out = {};
    out << "Critique the following response against each constitutional principle.\n\n";
    out << "Response:\n" << response << "\n\n";
    out << "Constitutional principles:\n";
    for (size_t i = 0; i < principles.size(); ++i) {
        out << (i + 1) << ". " << principles[i] << '\n';
    }
    out << "\nFor each principle, state whether the response complies and "
        << "explain any violations:\n";
    return out.str();
}

std::string DynamicReflectionPromptBuilder::buildSocraticPrompt(
    const std::string& claim,
    size_t iteration) const {

    static const std::vector<std::string> kSocraticQuestions = {
        "What is the fundamental assumption underlying this claim?",
        "Can you provide evidence that directly supports this?",
        "What would a counterexample to this claim look like?",
        "How would you defend this position to a skeptic?",
        "What information would change your conclusion?",
    };

    std::ostringstream out = {};
    out << "Consider the following claim:\n" << claim << "\n\n";
    out << kSocraticQuestions[iteration % kSocraticQuestions.size()] << '\n';
    out << "\nAnswer this question honestly, then revise your claim accordingly:";
    return out.str();
}

// ============================================================================
// ReflectionHallucinationGuard
// ============================================================================

ReflectionHallucinationGuard::ReflectionHallucinationGuard(
    double divergence_threshold, size_t window)
    : divergence_threshold_(divergence_threshold)
    , window_(window > 0 ? window : 1) {}

bool ReflectionHallucinationGuard::detectHallucinationSignals(
    const std::string& response,
    const std::string& critique) const {

    const std::string lower_resp   = toLower(response);
    const std::string lower_crit   = toLower(critique);

    for (const auto& marker : kHallucinationMarkers) {
        if (lower_crit.find(marker) != std::string::npos) {
            return true;
        }
    }

    // Self-correction patterns in the response itself also signal issues.
    static const std::vector<std::string> kSelfCorrection = {
        "actually,", "wait,", "let me reconsider",
        "i was wrong", "to correct myself", "i made an error"
    };
    for (const auto& pattern : kSelfCorrection) {
        if (lower_resp.find(pattern) != std::string::npos) {
            return true;
        }
    }

    return false;
}

bool ReflectionHallucinationGuard::isDiverging(
    const std::vector<double>& trajectory) const {

    if (static_cast<int>(trajectory.size()) < window_ + 1) {
        return false;
    }

    const size_t n = trajectory.size();
    const double earlier = trajectory[n - window_ - 1];

    double recent_avg = 0.0;
    for (size_t i = n - window_; i < n; ++i) {
        recent_avg += trajectory[i];
    }
    recent_avg /= static_cast<double>(window_);

    return (earlier - recent_avg) > divergence_threshold_;
}

bool ReflectionHallucinationGuard::shouldHalt(
    const std::vector<ReflectionStep>& steps) const {

    if (steps.empty()) {
      return false;
    }

    if (steps.back().hallucination_suspected) {
        return true;
    }

    std::vector<double> trajectory = {};

    trajectory.reserve(steps.size());
    for (const auto& step : steps) {
        trajectory.push_back(step.quality_score);
    }
    return isDiverging(trajectory);
}

// ============================================================================
// ReflectionTuner
// ============================================================================

ReflectionTuner::ReflectionTuner(const ReflectionConfig& config)
    : config_(config)
    , prompt_builder_(config.strategy)
    , hallucination_guard_(config.divergence_threshold, config.divergence_window) {}

void ReflectionTuner::setReflectionProvider(
    std::shared_ptr<IReflectionProvider> provider) {
    provider_ = std::move(provider);
}

void ReflectionTuner::clearReflectionProvider() {
    provider_.reset();
}

bool ReflectionTuner::hasReflectionProvider() const noexcept {
    return provider_ != nullptr;
}

const ReflectionConfig& ReflectionTuner::getConfig() const noexcept {
    return config_;
}

void ReflectionTuner::setConfig(const ReflectionConfig& config) {
    config_ = config;
    prompt_builder_.setStrategy(config.strategy);
    hallucination_guard_ = ReflectionHallucinationGuard(
        config.divergence_threshold, config.divergence_window);
}

const DynamicReflectionPromptBuilder&
ReflectionTuner::getPromptBuilder() const noexcept {
    return prompt_builder_;
}

const ReflectionHallucinationGuard&
ReflectionTuner::getHallucinationGuard() const noexcept {
    return hallucination_guard_;
}

double ReflectionTuner::computeHeuristicScore(const std::string& /*prompt*/,
                                               const std::string& response) const {
    if (response.empty()) {
      return 0.0;
    }

    double score = 0.5;

    // Reward longer, more complete responses.
    const size_t len = response.size();
    if (len > 200) {
      score += 0.1;
    }
    if (len > 500) {
      score += 0.1;
    }

    // Penalise hallucination markers found in the response text.
    const std::string lower = toLower(response);
    for (const auto& marker : ReflectionHallucinationGuard::kHallucinationMarkers) {
        if (lower.find(marker) != std::string::npos) {
            score -= 0.15;
        }
    }

    // Reward structured content (numbered lists, bullet points, labelled sections).
    if (lower.find("1.") != std::string::npos ||
        lower.find("- ") != std::string::npos ||
        lower.find(":\n") != std::string::npos) {
        score += 0.1;
    }

    return std::max(0.0, std::min(1.0, score));
}

SelfAwareContext ReflectionTuner::extractContext(
    const std::vector<ReflectionStep>& steps) const {
    if (steps.empty()) return SelfAwareContext{};
    return SelfAwareContext::fromResponse(steps.back().response);
}

ReflectionStep ReflectionTuner::runIteration(
    const std::string& prompt,
    const std::string& current_response,
    size_t iteration,
    const SelfAwareContext& ctx) {

    ReflectionStep step;
    step.iteration = iteration;

    std::string critique = {};
    std::string revised_response = {};
    double      score = 0.0;

    if (provider_) {
        // Use the concrete provider for critique, revision, and scoring.
        critique         = provider_->critique(prompt, current_response);
        revised_response = provider_->revise(prompt, current_response, critique);
        score            = provider_->score(prompt, revised_response);

        // Store the critique prompt in metadata for observability.
        step.metadata["critique_prompt"] =
            prompt_builder_.buildCritiquePrompt(prompt, current_response, ctx);
    } else {
        // Fallback: template-based critique + heuristic score.
        // The generated critique prompt is returned in metadata so that callers
        // who drive an external LLM can forward it manually.
        std::string critique_prompt = {};
        if (config_.strategy == ReflectionStrategy::CONSTITUTIONAL &&
            !config_.constitutional_principles.empty()) {
            critique_prompt = prompt_builder_.buildConstitutionalCritiquePrompt(
                current_response, config_.constitutional_principles);
        } else {
            critique_prompt = prompt_builder_.buildCritiquePrompt(
                prompt, current_response, ctx);
        }

        // Heuristic fallback critique based on self-aware context.
        std::ostringstream crit_out = {};
        if (ctx.has_uncertain_claims) {
            crit_out << "The response contains uncertainty markers (";
            for (size_t i = 0; i <static_cast<int>(ctx.uncertainty_markers.size()); ++i) {
                if (i > 0) {
                  crit_out << ", ";
                }
                crit_out << '"' << ctx.uncertainty_markers[i] << '"';
            }
            crit_out << "). Consider providing more definitive statements or "
                     << "explicitly acknowledging limitations.";
        } else {
            crit_out << "Consider whether the response is complete, accurate, "
                     << "and directly addresses all aspects of the original task. "
                     << "Check for any missing details or logical gaps.";
        }
        critique         = crit_out.str();
        revised_response = current_response;  // cannot revise without an LLM
        score            = computeHeuristicScore(prompt, revised_response);

        step.metadata["critique_prompt"]  = critique_prompt;
        step.metadata["revision_prompt"]  =
            prompt_builder_.buildRevisionPrompt(prompt, current_response, critique, ctx);
    }

    step.critique                = critique;
    step.response                = revised_response;
    step.quality_score           = score;
    step.hallucination_suspected =
        hallucination_guard_.detectHallucinationSignals(revised_response, critique);

    return step;
}

bool ReflectionTuner::shouldConverge(const ReflectionResult& result,
                                      const ReflectionStep& step) const {
    if (step.quality_score >= config_.convergence_threshold) {
        return true;
    }
    if (static_cast<int>(result.steps.size()) >= 2 &&
        std::abs(step.quality_delta) < config_.min_delta_improvement) {
        return true;
    }
    return false;
}

ReflectionResult ReflectionTuner::tuneFromPrompt(const std::string& prompt) {
    const std::string initial_response =
        provider_ ? provider_->generate(prompt) : prompt;
    return tune(prompt, initial_response);
}

ReflectionResult ReflectionTuner::tune(const std::string& prompt,
                                        const std::string& initial_response) {
    ReflectionResult result;
    result.metadata["strategy"]       = static_cast<int>(config_.strategy);
    result.metadata["max_iterations"] = config_.max_iterations;
    result.metadata["provider"]       =
        provider_ ? provider_->name() : std::string("fallback");

    std::string current_response = initial_response;

    // Score the initial response before any reflection.
    const double initial_score =
        provider_ ? provider_->score(prompt, initial_response)
                  : computeHeuristicScore(prompt, initial_response);

    result.initial_quality = initial_score;
    result.quality_trajectory.push_back(initial_score);

    double prev_score = initial_score;

    for (size_t iter = 0; iter < config_.max_iterations; ++iter) {
        const SelfAwareContext ctx =
            config_.include_self_aware_context
                ? SelfAwareContext::fromResponse(current_response)
                : SelfAwareContext{};

        ReflectionStep step = runIteration(prompt, current_response, iter, ctx);
        step.quality_delta  = step.quality_score - prev_score;

        result.steps.push_back(step);
        result.quality_trajectory.push_back(step.quality_score);

        current_response = step.response;
        prev_score       = step.quality_score;

        if (config_.hallucination_guard_enabled &&
            hallucination_guard_.shouldHalt(result.steps)) {
            result.halted_by_hallucination_guard = true;
            break;
        }

        if (shouldConverge(result, step)) {
            result.converged = true;
            break;
        }
    }

    result.final_response   = current_response;
    result.total_iterations = result.steps.size();
    result.final_quality    = result.steps.empty()
                                  ? initial_score
                                  : result.steps.back().quality_score;
    result.quality_improvement = result.final_quality - result.initial_quality;
    result.self_aware_context  = extractContext(result.steps);

    return result;
}

} // namespace prompt_engineering
} // namespace themis

