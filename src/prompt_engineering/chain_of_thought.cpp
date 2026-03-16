/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            chain_of_thought.cpp                               ║
  Version:         0.0.1                                              ║
  Last Modified:   2026-03-16 04:17:19                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     157                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • d135ff3ad  2026-03-09  feat(prompt_engineering): implement ChainOfThoughtBuilder... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "prompt_engineering/chain_of_thought.h"
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

    for (const auto& step : steps_) {
        if (!first) {
            out << config_.step_delimiter;
        }
        first = false;

        if (!step.label.empty()) {
            out << step.label << ":\n";
        }
        out << step.content;
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
