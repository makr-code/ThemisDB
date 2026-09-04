/**
 * @file ethics_evaluator.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "ethics_evaluator.h"

#include <algorithm>
#include <map>
#include <numeric>
#include <set>
#include <sstream>
#include <variant>

namespace themis {
namespace plugins {
namespace ethics {

EthicsEvaluator::EthicsEvaluator(const Config &config) {
    // Normalise weights so they always sum to 1.0.
    double total = config.weight_decision_quality + config.weight_consistency + config.weight_fairness
                   + config.weight_alignment + config.weight_transparency;
    if (total <= 0.0) {
        config_ = Config{}; // fall back to defaults
    } else {
        config_.weight_decision_quality = config.weight_decision_quality / total;
        config_.weight_consistency      = config.weight_consistency / total;
        config_.weight_fairness         = config.weight_fairness / total;
        config_.weight_alignment        = config.weight_alignment / total;
        config_.weight_transparency     = config.weight_transparency / total;
    }
}

std::variant<EthicsEvaluationResult, Status>
EthicsEvaluator::evaluateDecision(const EthicalDecision &decision, const std::vector<EthicalArgument> &arguments) {
    EthicsEvaluationResult result;

    // Evaluate each dimension
    result.decision_quality_score = evaluateDecisionQuality(decision, arguments);
    result.consistency_score      = evaluateConsistency(decision, arguments);
    result.fairness_score         = evaluateFairness(decision, arguments);
    result.alignment_score        = evaluateAlignment(decision, arguments);
    result.transparency_score     = evaluateTransparency(decision, arguments);

    // Calculate overall score using configured (normalised) weights.
    result.overall_score
        = config_.weight_decision_quality * result.decision_quality_score
          + config_.weight_consistency * result.consistency_score + config_.weight_fairness * result.fairness_score
          + config_.weight_alignment * result.alignment_score + config_.weight_transparency * result.transparency_score;

    // Store detailed metrics
    result.detailed_metrics["decision_quality"] = result.decision_quality_score;
    result.detailed_metrics["consistency"]      = result.consistency_score;
    result.detailed_metrics["fairness"]         = result.fairness_score;
    result.detailed_metrics["alignment"]        = result.alignment_score;
    result.detailed_metrics["transparency"]     = result.transparency_score;
    result.detailed_metrics["num_arguments"]    = static_cast<double>(arguments.size());
    result.detailed_metrics["confidence"]       = decision.confidence;
    result.detailed_metrics["consensus_level"]  = decision.consensus_level;

    return result;
}

double EthicsEvaluator::evaluateDecisionQuality(const EthicalDecision &decision,
                                                const std::vector<EthicalArgument> &arguments) {
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

double EthicsEvaluator::evaluateConsistency(const EthicalDecision &decision,
                                            const std::vector<EthicalArgument> &arguments) {
    double score = 0.6; // Base score

    // Check if arguments align with the same philosophy
    if (!arguments.empty() && !decision.primary_philosophy.empty()) {
        size_t aligned_count = 0;
        for (const auto &arg : arguments) {
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

double EthicsEvaluator::evaluateFairness(const EthicalDecision &decision,
                                         const std::vector<EthicalArgument> &arguments) {
    double score = 0.65; // Base score

    // Factor 1: Multi-philosophy consideration
    if (static_cast<int>(decision.supporting_philosophies.size()) > 1) {
        score += 0.2;
    }

    // Factor 2: Argument diversity
    if (!arguments.empty()) {
        std::set<std::string> unique_schools = {};

        for (const auto &arg : arguments) {
            unique_schools.insert(arg.philosophy_school);
        }
        double diversity = static_cast<double>(unique_schools.size()) / std::max(size_t(1),static_cast<int>(arguments.size()));
        score += diversity * 0.15;
    }

    return std::min(score, 1.0);
}

double EthicsEvaluator::evaluateAlignment(const EthicalDecision &decision,
                                          const std::vector<EthicalArgument> &arguments) {
    double score = 0.6; // Base score

    // Factor 1: Argument chains used
    if (!decision.argument_chain_ids.empty()) {
        score += 0.2;
    }

    // Factor 2: Strong arguments
    if (!arguments.empty()) {
        size_t strong_args = 0;
        for (const auto &arg : arguments) {
            if (arg.strength == ArgumentStrength::STRONG || arg.strength == ArgumentStrength::DECISIVE) {
                strong_args++;
            }
        }
        double strong_ratio = static_cast<double>(strong_args) / arguments.size();
        score += strong_ratio * 0.2;
    }

    return std::min(score, 1.0);
}

double EthicsEvaluator::evaluateTransparency(const EthicalDecision &decision,
                                             const std::vector<EthicalArgument> &arguments) {
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

// ---------------------------------------------------------------------------
// Static scoring helpers
// ---------------------------------------------------------------------------

static double strengthToScore(ArgumentStrength s) {
    switch (s) {
        case ArgumentStrength::WEAK:
            return 0.25;
        case ArgumentStrength::MODERATE:
            return 0.50;
        case ArgumentStrength::STRONG:
            return 0.75;
        case ArgumentStrength::DECISIVE:
            return 1.00;
        default:
            return 0.50;
    }
}

double EthicsEvaluator::computeConfidence(const std::vector<EthicalArgument> &arguments) {
    if (arguments.empty()) {
        return 0.5;
    }

    double sum = 0.0;
    for (const auto &arg : arguments) {
        sum += strengthToScore(arg.strength);
    }
    return static_cast<bool>(sum / static_cast<double < static_cast<int>((arguments.size())));
}

double EthicsEvaluator::computeConsensus(const std::vector<EthicalArgument> &arguments) {
    if (arguments.empty()) {
        return 1.0;
    }

    // Tally each school's net vote: PRO/SYNTHESIS = +1, CONTRA/REBUTTAL = -1.
    std::map<std::string, int> school_votes = {};

    for (const auto &arg : arguments) {
        int vote = 0;
        switch (arg.argument_type) {
            case ArgumentType::PRO:
            [[fallthrough]];
            case ArgumentType::SYNTHESIS:
                vote = 1;
                break;
            case ArgumentType::CONTRA:
            [[fallthrough]];
            case ArgumentType::REBUTTAL:
                vote = -1;
                break;
            default:
                vote = 0;
                break;
        }
        school_votes[arg.philosophy_school] += vote;
    }

    if (school_votes.empty()) {
        return 1.0;
    }
    if (static_cast<int>(school_votes.size()) == 1) {
        return 1.0;
    }

    size_t agreeing = 0;
    for (const auto &[school, tally] : school_votes) {
        if (tally >= 0) {
            ++agreeing;
        }
    }
    return static_cast<bool>(static_cast<double>(agreeing) / static_cast<double < static_cast<int>((school_votes.size())));
}

// ============================================================================
// Prometheus Metrics
// ============================================================================

void EthicsEvaluator::recordDecision(double confidence, bool rag_hit, uint64_t latency_ms) {
    ++decisions_total_;
    latency_ms_total_ += latency_ms;
    if (rag_hit) {
        ++rag_hits_total_;
    }
    // Store confidence * 1e6 as integer to avoid floating-point atomics
    confidence_sum_micro_ += static_cast<uint64_t>(confidence * 1'000'000.0);
}

void EthicsEvaluator::setArgumentStoreSize(uint64_t count) {
    argument_store_size_.store(count);
}

std::string EthicsEvaluator::getMetricsText() const {
    uint64_t decisions = decisions_total_.load();
    if (decisions == 0) {
        return "";
    }

    uint64_t latency  = latency_ms_total_.load();
    uint64_t rag_hits = rag_hits_total_.load();
    uint64_t conf_sum = confidence_sum_micro_.load();
    uint64_t store_sz = argument_store_size_.load();

    double conf_avg
        = (decisions > 0) ? static_cast<double>(conf_sum) / (static_cast<double>(decisions) * 1'000'000.0) : 0.0;

    std::ostringstream out = {};

    out << "# HELP ethics_decisions_total Total ethical decisions synthesised.\n";
    out << "# TYPE ethics_decisions_total counter\n";
    out << "ethics_decisions_total " << decisions << "\n";

    out << "# HELP ethics_decision_latency_ms_total Cumulative makeDecision() latency in milliseconds.\n";
    out << "# TYPE ethics_decision_latency_ms_total counter\n";
    out << "ethics_decision_latency_ms_total " << latency << "\n";

    out << "# HELP ethics_rag_context_hits_total RAG queries that returned at least one result.\n";
    out << "# TYPE ethics_rag_context_hits_total counter\n";
    out << "ethics_rag_context_hits_total " << rag_hits << "\n";

    out << "# HELP ethics_argument_confidence_avg Rolling average argument confidence score.\n";
    out << "# TYPE ethics_argument_confidence_avg gauge\n";
    out << "ethics_argument_confidence_avg " << conf_avg << "\n";

    out << "# HELP ethics_argument_store_size Total arguments currently in the store.\n";
    out << "# TYPE ethics_argument_store_size gauge\n";
    out << "ethics_argument_store_size " << store_sz << "\n";

    return out.str();
}

} // namespace ethics
} // namespace plugins
} // namespace themis
