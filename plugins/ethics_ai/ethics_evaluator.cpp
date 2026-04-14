/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ethics_evaluator.cpp                               ║
  Version:         0.0.40                                             ║
  Last Modified:   2026-04-14 06:58:30                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     194                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "ethics_evaluator.h"
#include <algorithm>
#include <numeric>

namespace themis {
namespace plugins {
namespace ethics {

std::variant<EthicsEvaluationResult, Status> EthicsEvaluator::evaluateDecision(
    const EthicalDecision& decision,
    const std::vector<EthicalArgument>& arguments) {
    
    EthicsEvaluationResult result;
    
    // Evaluate each dimension
    result.decision_quality_score = evaluateDecisionQuality(decision, arguments);
    result.consistency_score = evaluateConsistency(decision, arguments);
    result.fairness_score = evaluateFairness(decision, arguments);
    result.alignment_score = evaluateAlignment(decision, arguments);
    result.transparency_score = evaluateTransparency(decision, arguments);
    
    // Calculate overall score (weighted average)
    const double weights[5] = {0.25, 0.20, 0.20, 0.20, 0.15};
    const double scores[5] = {
        result.decision_quality_score,
        result.consistency_score,
        result.fairness_score,
        result.alignment_score,
        result.transparency_score
    };
    
    result.overall_score = 0.0;
    for (int i = 0; i < 5; ++i) {
        result.overall_score += weights[i] * scores[i];
    }
    
    // Store detailed metrics
    result.detailed_metrics["decision_quality"] = result.decision_quality_score;
    result.detailed_metrics["consistency"] = result.consistency_score;
    result.detailed_metrics["fairness"] = result.fairness_score;
    result.detailed_metrics["alignment"] = result.alignment_score;
    result.detailed_metrics["transparency"] = result.transparency_score;
    result.detailed_metrics["num_arguments"] = static_cast<double>(arguments.size());
    result.detailed_metrics["confidence"] = decision.confidence;
    result.detailed_metrics["consensus_level"] = decision.consensus_level;
    
    return result;
}

double EthicsEvaluator::evaluateDecisionQuality(
    const EthicalDecision& decision,
    const std::vector<EthicalArgument>& arguments) {
    
    double score = 0.5; // Base score
    
    // Factor 1: Confidence
    score += decision.confidence * 0.3;
    
    // Factor 2: Number of supporting arguments
    if (!arguments.empty()) {
        double arg_factor = std::min(arguments.size() / 5.0, 1.0) * 0.2;
        score += arg_factor;
    }
    
    // Factor 3: Consensus level
    score += decision.consensus_level * 0.2;
    
    return std::min(score, 1.0);
}

double EthicsEvaluator::evaluateConsistency(
    const EthicalDecision& decision,
    const std::vector<EthicalArgument>& arguments) {
    
    double score = 0.6; // Base score
    
    // Check if arguments align with the same philosophy
    if (!arguments.empty() && !decision.primary_philosophy.empty()) {
        size_t aligned_count = 0;
        for (const auto& arg : arguments) {
            if (arg.philosophy_school == decision.primary_philosophy) {
                aligned_count++;
            }
        }
        double alignment_ratio = static_cast<double>(aligned_count) / arguments.size();
        score += alignment_ratio * 0.3;
    } else {
        score += 0.3; // Default if no arguments
    }
    
    return std::min(score, 1.0);
}

double EthicsEvaluator::evaluateFairness(
    const EthicalDecision& decision,
    const std::vector<EthicalArgument>& arguments) {
    
    double score = 0.65; // Base score
    
    // Factor 1: Multi-philosophy consideration
    if (decision.supporting_philosophies.size() > 1) {
        score += 0.2;
    }
    
    // Factor 2: Argument diversity
    if (!arguments.empty()) {
        std::set<std::string> unique_schools;
        for (const auto& arg : arguments) {
            unique_schools.insert(arg.philosophy_school);
        }
        double diversity = static_cast<double>(unique_schools.size()) / 
                          std::max(size_t(1), arguments.size());
        score += diversity * 0.15;
    }
    
    return std::min(score, 1.0);
}

double EthicsEvaluator::evaluateAlignment(
    const EthicalDecision& decision,
    const std::vector<EthicalArgument>& arguments) {
    
    double score = 0.6; // Base score
    
    // Factor 1: Argument chains used
    if (!decision.argument_chain_ids.empty()) {
        score += 0.2;
    }
    
    // Factor 2: Strong arguments
    if (!arguments.empty()) {
        size_t strong_args = 0;
        for (const auto& arg : arguments) {
            if (arg.strength == ArgumentStrength::STRONG || 
                arg.strength == ArgumentStrength::DECISIVE) {
                strong_args++;
            }
        }
        double strong_ratio = static_cast<double>(strong_args) / arguments.size();
        score += strong_ratio * 0.2;
    }
    
    return std::min(score, 1.0);
}

double EthicsEvaluator::evaluateTransparency(
    const EthicalDecision& decision,
    const std::vector<EthicalArgument>& arguments) {
    
    double score = 0.5; // Base score
    
    // Factor 1: Decision text completeness
    if (!decision.decision_text.empty()) {
        score += 0.2;
    }
    
    // Factor 2: Primary philosophy specified
    if (!decision.primary_philosophy.empty()) {
        score += 0.15;
    }
    
    // Factor 3: Supporting philosophies documented
    if (!decision.supporting_philosophies.empty()) {
        score += 0.15;
    }
    
    return std::min(score, 1.0);
}

} // namespace ethics
} // namespace plugins
} // namespace themis
