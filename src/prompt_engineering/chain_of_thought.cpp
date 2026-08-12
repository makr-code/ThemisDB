/**
 * @file chain_of_thought.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "prompt_engineering/chain_of_thought.h"
#include <chrono>
#include <sstream>

namespace themis {
namespace prompt_engineering {

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

ChainOfThoughtBuilder::ChainOfThoughtBuilder(const CoTConfig& config)
    : config_(config) {}

// ---------------------------------------------------------------------------
// Step management
// ---------------------------------------------------------------------------

ChainOfThoughtBuilder& ChainOfThoughtBuilder::addStep(
    const std::string& content,
    const std::string& label) {

    CoTStep step;
    step.content = content;

    if (!label.empty()) {
        step.label = label;
    } else if (config_.number_steps) {
        step.label = config_.step_prefix + std::to_string(steps_.size() + 1);
    }

    steps_.push_back(std::move(step));
    return *this;
}

ChainOfThoughtBuilder& ChainOfThoughtBuilder::addReasoningStep(
    const std::string& reasoning) {
    return addStep(reasoning, "Reasoning");
}

ChainOfThoughtBuilder& ChainOfThoughtBuilder::setFinalAnswer(
    const std::string& answer) {
    final_answer_ = answer;
    return *this;
}

void ChainOfThoughtBuilder::clear() {
    steps_.clear();
    final_answer_.clear();
}

size_t ChainOfThoughtBuilder::stepCount() const {
    return steps_.size();
}

const CoTConfig& ChainOfThoughtBuilder::getConfig() const {
    return config_;
}

// ---------------------------------------------------------------------------
// Build
// ---------------------------------------------------------------------------

std::string ChainOfThoughtBuilder::build() const {
    if (steps_.empty() && final_answer_.empty()) {
        return {};
    }

    std::ostringstream out;
    bool first = true;

    for (std::size_t idx = 0; idx < steps_.size(); ++idx) {
        const auto& step = steps_[idx];

        if (!first) {
            out << config_.step_delimiter;
        }
        first = false;

        // Fire onStepBegin — callbacks are noexcept; implementations must not throw.
        if (tracer_) {
            tracer_->onStepBegin(idx, step.label);
        }

        const auto t0 = std::chrono::steady_clock::now();

        if (!step.label.empty()) {
            out << step.label << ":\n";
        }
        out << step.content;

        const auto duration =
            std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::steady_clock::now() - t0);

        // Fire onStepEnd — callbacks are noexcept; implementations must not throw.
        if (tracer_) {
            tracer_->onStepEnd(idx, step.content, duration);
        }
    }

    if (!final_answer_.empty()) {
        if (!first) {
            out << config_.step_delimiter;
        }
        out << config_.final_answer_label << "\n" << final_answer_;
    }

    return out.str();
}

// ---------------------------------------------------------------------------
// Tracer management
// ---------------------------------------------------------------------------

void ChainOfThoughtBuilder::attachTracer(
    std::shared_ptr<IChainOfThoughtTracer> tracer) {
    tracer_ = std::move(tracer);
}

void ChainOfThoughtBuilder::detachTracer() {
    tracer_.reset();
}

bool ChainOfThoughtBuilder::hasTracer() const noexcept {
    return tracer_ != nullptr;
}

// ---------------------------------------------------------------------------
// Static factory helpers
// ---------------------------------------------------------------------------

std::string ChainOfThoughtBuilder::buildZeroShot(const std::string& question) {
    return question + "\n\nLet's think step by step.";
}

std::string ChainOfThoughtBuilder::buildFewShot(
    const std::string& question,
    const std::vector<std::pair<std::string, std::string>>& examples) {

    std::ostringstream out;

    for (const auto& [ex_question, ex_answer] : examples) {
        out << "Q: " << ex_question << "\n";
        out << "A: " << ex_answer << "\n\n";
    }

    out << "Q: " << question << "\n";
    out << "A:";

    return out.str();
}

std::string ChainOfThoughtBuilder::wrapWithCoT(const std::string& prompt,
                                               bool explicit_steps) {
    std::ostringstream out;

    if (explicit_steps) {
        out << "Solve the following task by reasoning through it step by step.\n\n";
        out << "Step 1: Carefully read and understand the task.\n";
        out << "Step 2: Break down the problem into smaller sub-problems.\n";
        out << "Step 3: Solve each sub-problem in order.\n";
        out << "Step 4: Combine the results into a coherent final answer.\n\n";
    } else {
        out << "Think step by step before giving your final answer.\n\n";
    }

    out << prompt;

    return out.str();
}

} // namespace prompt_engineering
} // namespace themis
