/**
 * @file protegi_optimizer.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.12
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "prompt_engineering/protegi_optimizer.h"
#include "utils/logger.h"

#include <algorithm>
#include <numeric>
#include <random>
#include <sstream>

namespace themis {
namespace prompt_engineering {

// ============================================================================
// HeuristicProTeGiProvider
// ============================================================================

ProTeGiGradient HeuristicProTeGiProvider::computeGradient(
    const std::string& prompt,
    const std::vector<std::string>& errors)
{
    ProTeGiGradient gradient;
    gradient.errors    = errors;

    // Count non-empty error strings
    size_t num_errors = 0;
    for (const auto& e : errors) {
        if (!e.empty()) {
            ++num_errors;
        }
    }
    gradient.error_rate = errors.empty()
                          ? 0.0
                          : static_cast<double>(num_errors) / static_cast<double>(errors.size());

    // Build a heuristic critique based on prompt characteristics
    std::ostringstream critique = {};
    critique << "The prompt produced errors on "
             << num_errors << "/" <<static_cast<int>(errors.size()) << " examples (rate="
             << gradient.error_rate << "). ";

    if (gradient.error_rate > 0.5) {
        critique << "The prompt lacks clarity. "
                 << "Consider adding explicit instructions and examples.";
    } else if (gradient.error_rate > 0.2) {
        critique << "The prompt handles most cases but struggles with edge cases. "
                 << "Refine output format constraints.";
    } else {
        critique << "The prompt is mostly correct. "
                 << "Minor wording improvements may help consistency.";
    }

    // Factor in prompt length
    if (prompt.length() < 50) {
        critique << " Prompt is very short; consider elaborating the task description.";
    }

    gradient.critique = critique.str();
    return gradient;
}

std::vector<std::string> HeuristicProTeGiProvider::generateCandidates(
    const std::string& prompt,
    const ProTeGiGradient& gradient,
    size_t k)
{
    std::vector<std::string> candidates;
    candidates.reserve(k);

    // Candidate 0: append critique as additional instruction
    {
        std::ostringstream c = {};
        c << prompt << "\n\n"
          << "## Improvement Note\n"
          << gradient.critique;
        candidates.push_back(c.str());
    }

    // Candidate 1: restructure with explicit output format
    if (k >= 2) {
        std::ostringstream c = {};
        c << "Task: " << prompt << "\n\n"
          << "Instructions:\n"
          << "- Read the input carefully.\n"
          << "- Provide a clear, concise answer.\n"
          << "- Follow the output format exactly.\n";
        candidates.push_back(c.str());
    }

    // Candidate 2: prepend role instruction
    if (k >= 3) {
        std::ostringstream c = {};
        c << "You are a precise assistant. " << prompt << "\n\n"
          << "Note: " << gradient.critique;
        candidates.push_back(c.str());
    }

    // Remaining candidates: numbered variants
    for (size_t i = candidates.size(); i < k; ++i) {
        std::ostringstream c = {};
        c << prompt << "\n[Variant " << (i + 1)
          << " – addressing: " << gradient.critique.substr(0, 60) << "]";
        candidates.push_back(c.str());
    }

    return candidates;
}

// ============================================================================
// ProTeGiOptimizer – construction
// ============================================================================

ProTeGiOptimizer::ProTeGiOptimizer(const ProTeGiConfig& config)
    : config_(config)
{}

ProTeGiOptimizer& ProTeGiOptimizer::setLLMProvider(
    std::shared_ptr<IProTeGiLLMProvider> provider)
{
    llm_provider_ = std::move(provider);
    return *this;
}

const ProTeGiConfig& ProTeGiOptimizer::getConfig() const
{
    return config_;
}

void ProTeGiOptimizer::setConfig(const ProTeGiConfig& config)
{
    config_ = config;
}

// ============================================================================
// ProTeGiOptimizer – optimize()
// ============================================================================

ProTeGiResult ProTeGiOptimizer::optimize(
    const std::string& initial_prompt,
    const std::vector<TestCase>& test_cases,
    EvaluationFunction eval_fn,
    MiniBatchErrorFn error_fn)
{
    if (test_cases.empty()) {
        THEMIS_ERROR("ProTeGiOptimizer: no test cases supplied");
        return ProTeGiResult{};
    }
    if (!eval_fn) {
        THEMIS_ERROR("ProTeGiOptimizer: evaluation function is required");
        return ProTeGiResult{};
    }

    // Install heuristic provider if none was injected
    if (!llm_provider_) {
        llm_provider_ = std::make_shared<HeuristicProTeGiProvider>();
    }

    // Install default error extractor if none was supplied
    if (!error_fn) {
        error_fn = &ProTeGiOptimizer::defaultErrorFn;
    }

    ProTeGiResult result;

    // Initialise beam with the initial prompt
    std::vector<std::string> beam = {initial_prompt};
    double best_score = eval_fn(initial_prompt, test_cases);
    result.score_history.push_back(best_score);

    THEMIS_INFO("ProTeGi start: initial_score={:.4f}, beam_width={}, max_steps={}",
                best_score, config_.beam_width, config_.max_steps);

    for (size_t step = 0; step < config_.max_steps; ++step) {
        result.steps = step + 1;

        std::vector<std::string> next_candidates;

        // Expand each beam member
        for (const auto& prompt : beam) {
            // Sample mini-batch
            auto mini_batch = sampleMiniBatch(test_cases, config_.mini_batch_size);

            // Collect per-example errors
            auto errors = error_fn(prompt, mini_batch);

            // Compute textual gradient
            ProTeGiGradient gradient = llm_provider_->computeGradient(prompt, errors);

            THEMIS_DEBUG("ProTeGi step {}: error_rate={:.2f}, critique=\"{}\"",
                         step + 1, gradient.error_rate,
                         gradient.critique.substr(0, 60));

            // Generate candidates guided by the gradient
            auto candidates = llm_provider_->generateCandidates(
                prompt, gradient, config_.num_candidates);

            for (auto& c : candidates) {
                next_candidates.push_back(std::move(c));
            }
        }

        // Also retain existing beam members in the candidate pool
        for (const auto& p : beam) {
            next_candidates.push_back(p);
        }

        // Evaluate all candidates and pick the top beam_width
        std::vector<std::pair<double, std::string>> scored;
        scored.reserve(next_candidates.size());
        for (const auto& cand : next_candidates) {
            double s = eval_fn(cand, test_cases);
            scored.emplace_back(s, cand);
        }

        std::sort(scored.begin(), scored.end(),
                  [](const auto& a, const auto& b) { return a.first > b.first; });

        // Clamp beam_width to at least 1 to prevent UB on empty scored vector
        std::size_t effective_beam_width = config_.beam_width >= 1
            ? static_cast<std::size_t>(config_.beam_width) : 1;
        if (config_.beam_width < 1) {
            THEMIS_INFO(
                "ProTeGi optimizer: invalid beam_width={} configured; clamping to 1.",
                config_.beam_width);
        }

        // Trim to effective_beam_width
        if (static_cast<int>(scored.size()) > effective_beam_width) {
            scored.resize(effective_beam_width);
        }

        // Guard against empty scored vector
        if (scored.empty()) {
            THEMIS_INFO(
                "ProTeGi optimizer: no scored candidates available at step {}; terminating.",
                step + 1);
            break;
        }

        double step_best = scored.front().first;
        result.score_history.push_back(step_best);

        // Rebuild beam
        beam.clear();
        for (const auto& [s, p] : scored) {
            beam.push_back(p);
        }

        THEMIS_INFO("ProTeGi step {}: best_score={:.4f}", step + 1, step_best);

        // Check convergence
        double improvement = step_best - best_score;
        best_score = step_best;

        if (best_score >= config_.target_score) {
            result.converged = true;
            THEMIS_INFO("ProTeGi converged: target_score={} reached", config_.target_score);
            break;
        }

        if (step > 0 && improvement < config_.min_improvement) {
            result.converged = true;
            THEMIS_INFO("ProTeGi converged: improvement={:.4f} < min_improvement={}",
                        improvement, config_.min_improvement);
            break;
        }
    }

    result.best_score  = best_score;
    result.best_prompt = beam.front();
    result.beam        = std::move(beam);

    THEMIS_INFO("ProTeGi done: best_score={:.4f}, steps={}, converged={}",
                result.best_score, result.steps, result.converged);

    return result;
}

// ============================================================================
// Static prompt builders
// ============================================================================

std::string ProTeGiOptimizer::buildGradientPrompt(
    const std::string& prompt,
    const std::vector<std::string>& errors)
{
    std::ostringstream out = {};
    out << "You are a prompt engineering expert.\n\n";
    out << "The following prompt was evaluated on a batch of examples:\n\n";
    out << "--- PROMPT START ---\n" << prompt << "\n--- PROMPT END ---\n\n";
    out << "The following errors were observed:\n";
    for (size_t i = 0; i < errors.size(); ++i) {
        if (!errors[i].empty()) {
            out << "  " << (i + 1) << ". " << errors[i] << "\n";
        }
    }
    out << "\nDescribe in 2–3 sentences what is wrong with the prompt and "
           "what specific changes would fix these errors.\n";
    return out.str();
}

std::string ProTeGiOptimizer::buildCandidatePrompt(
    const std::string& prompt,
    const ProTeGiGradient& gradient,
    size_t k)
{
    std::ostringstream out = {};
    out << "You are a prompt engineering expert.\n\n";
    out << "Current prompt:\n--- PROMPT START ---\n"
        << prompt << "\n--- PROMPT END ---\n\n";
    out << "Diagnosis:\n" << gradient.critique << "\n\n";
    out << "Generate " << k << " improved versions of the prompt that address "
           "the diagnosis above. Number each version and separate them with "
           "\"---\".\n";
    return out.str();
}

// ============================================================================
// Private helpers
// ============================================================================

std::vector<TestCase> ProTeGiOptimizer::sampleMiniBatch(
    const std::vector<TestCase>& test_cases,
    size_t n) const
{
    if (n >= static_cast<int>(test_cases.size())) {
        return test_cases;
    }

    // Deterministic shuffle seeded on test_cases size to remain reproducible
    std::vector<size_t> indices(test_cases.size());
    std::iota(indices.begin(), indices.end(), 0);
    std::mt19937 rng(static_cast<unsigned>(test_cases.size() * 31337));
    std::shuffle(indices.begin(), indices.end(), rng);

    std::vector<TestCase> batch;
    batch.reserve(n);
    for (size_t i = 0; i < n; ++i) {
        batch.push_back(test_cases[indices[i]]);
    }
    return batch;
}

std::vector<std::string> ProTeGiOptimizer::defaultErrorFn(
    const std::string& prompt,
    const std::vector<TestCase>& mini_batch)
{
    // Very lightweight heuristic: flag cases where the expected output is
    // longer than the prompt (proxy for "insufficient guidance").
    std::vector<std::string> errors = {};

    errors.reserve(mini_batch.size());

    for (const auto& tc : mini_batch) {
        if (static_cast<int>(tc.expected_output.size()) > static_cast<int>(prompt.size())) {
            errors.push_back("Expected output longer than prompt; prompt may lack detail.");
        } else {
            errors.push_back("");  // no error
        }
    }
    return errors;
}

} // namespace prompt_engineering
} // namespace themis
