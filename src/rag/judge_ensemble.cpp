/**
 * @file judge_ensemble.cpp
 * @brief Implementation of judge ensemble
 */

#include "rag/judge_ensemble.h"
#include "utils/logger.h"
#include <algorithm>
#include <numeric>
#include <cmath>
#include <chrono>

namespace themis::rag::judge {

struct JudgeEnsemble::Impl {
    Config config;
    std::vector<std::unique_ptr<RAGJudge>> judges;
    
    // Calculate mean of scores
    double calculateMean(const std::vector<double>& scores) {
        if (scores.empty()) return 0.0;
        return std::accumulate(scores.begin(), scores.end(), 0.0) / scores.size();
    }
    
    // Calculate standard deviation
    double calculateStdDev(const std::vector<double>& scores, double mean) {
        if (scores.size() <= 1) return 0.0;
        
        double sum_sq_diff = 0.0;
        for (double score : scores) {
            double diff = score - mean;
            sum_sq_diff += diff * diff;
        }
        
        return std::sqrt(sum_sq_diff / (scores.size() - 1));
    }
};

JudgeEnsemble::JudgeEnsemble(const Config& config)
    : impl_(std::make_unique<Impl>()) {
    impl_->config = config;
    
    // Create judge instances
    RAGJudgeConfig judge_config;
    judge_config.mode = EvaluationMode::BALANCED;
    
    for (size_t i = 0; i < config.num_judges; ++i) {
        impl_->judges.push_back(std::make_unique<RAGJudge>(judge_config));
    }
    
    THEMIS_INFO("JudgeEnsemble initialized with {} judges, strategy: {}",
                config.num_judges, static_cast<int>(config.voting_strategy));
}

JudgeEnsemble::~JudgeEnsemble() = default;

EnsembleResult JudgeEnsemble::evaluate(
    const std::string& query,
    const std::vector<RetrievedDocument>& documents,
    const std::string& answer
) {
    auto start_time = std::chrono::steady_clock::now();
    
    EnsembleResult result;
    result.num_judges = impl_->config.num_judges;
    result.strategy_used = impl_->config.voting_strategy;
    
    THEMIS_DEBUG("Starting ensemble evaluation with {} judges", impl_->config.num_judges);
    
    // Collect votes from all judges
    for (size_t i = 0; i < impl_->judges.size(); ++i) {
        JudgeVote vote;
        vote.judge_id = "judge_" + std::to_string(i);
        vote.result = impl_->judges[i]->evaluate(query, documents, answer);
        vote.weight = 1.0;  // Default equal weight
        
        result.individual_votes.push_back(vote);
    }
    
    // Detect outliers
    if (impl_->config.enable_outlier_detection) {
        auto outliers = detectOutliers(result.individual_votes);
        THEMIS_DEBUG("Detected {} outlier judges", outliers.size());
    }
    
    // Analyze disagreement
    if (impl_->config.enable_disagreement_analysis) {
        result.disagreement = analyzeDisagreement(result.individual_votes);
    }
    
    // Aggregate votes
    result.aggregated_result = aggregateVotes(
        result.individual_votes,
        impl_->config.voting_strategy
    );
    
    auto end_time = std::chrono::steady_clock::now();
    result.total_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time
    );
    
    THEMIS_INFO("Ensemble evaluation complete: overall_score={:.2f}, agreement={:.2f}, time={}ms",
                result.aggregated_result.overall_score, 
                result.disagreement.agreement_score,
                result.total_time.count());
    
    return result;
}

EvaluationResult JudgeEnsemble::aggregateVotes(
    const std::vector<JudgeVote>& votes,
    VotingStrategy strategy
) {
    if (votes.empty()) {
        return EvaluationResult{};
    }
    
    EvaluationResult aggregated;
    
    switch (strategy) {
        case VotingStrategy::MAJORITY_VOTING: {
            // Simple average (equivalent to majority for continuous scores)
            double faith_sum = 0, rel_sum = 0, comp_sum = 0, coh_sum = 0;
            
            for (const auto& vote : votes) {
                faith_sum += vote.result.faithfulness_score;
                rel_sum += vote.result.relevance_score;
                comp_sum += vote.result.completeness_score;
                coh_sum += vote.result.coherence_score;
            }
            
            size_t n = votes.size();
            aggregated.faithfulness_score = faith_sum / n;
            aggregated.relevance_score = rel_sum / n;
            aggregated.completeness_score = comp_sum / n;
            aggregated.coherence_score = coh_sum / n;
            aggregated.overall_score = (aggregated.faithfulness_score * 0.3 +
                                       aggregated.relevance_score * 0.25 +
                                       aggregated.completeness_score * 0.25 +
                                       aggregated.coherence_score * 0.2);
            break;
        }
        
        case VotingStrategy::WEIGHTED_AVERAGE:
        case VotingStrategy::CONFIDENCE_WEIGHTED: {
            // Weight by confidence
            double total_weight = 0.0;
            double faith_sum = 0, rel_sum = 0, comp_sum = 0, coh_sum = 0;
            
            for (const auto& vote : votes) {
                double weight = vote.result.confidence * vote.weight;
                total_weight += weight;
                
                faith_sum += vote.result.faithfulness_score * weight;
                rel_sum += vote.result.relevance_score * weight;
                comp_sum += vote.result.completeness_score * weight;
                coh_sum += vote.result.coherence_score * weight;
            }
            
            if (total_weight > 0) {
                aggregated.faithfulness_score = faith_sum / total_weight;
                aggregated.relevance_score = rel_sum / total_weight;
                aggregated.completeness_score = comp_sum / total_weight;
                aggregated.coherence_score = coh_sum / total_weight;
                aggregated.overall_score = (aggregated.faithfulness_score * 0.3 +
                                           aggregated.relevance_score * 0.25 +
                                           aggregated.completeness_score * 0.25 +
                                           aggregated.coherence_score * 0.2);
            }
            break;
        }
        
        case VotingStrategy::HIERARCHICAL: {
            // Use median for robustness against outliers
            std::vector<double> faith_scores, rel_scores, comp_scores, coh_scores;
            
            for (const auto& vote : votes) {
                faith_scores.push_back(vote.result.faithfulness_score);
                rel_scores.push_back(vote.result.relevance_score);
                comp_scores.push_back(vote.result.completeness_score);
                coh_scores.push_back(vote.result.coherence_score);
            }
            
            auto median = [](std::vector<double>& scores) {
                std::sort(scores.begin(), scores.end());
                size_t mid = scores.size() / 2;
                if (scores.size() % 2 == 0) {
                    return (scores[mid-1] + scores[mid]) / 2.0;
                }
                return scores[mid];
            };
            
            aggregated.faithfulness_score = median(faith_scores);
            aggregated.relevance_score = median(rel_scores);
            aggregated.completeness_score = median(comp_scores);
            aggregated.coherence_score = median(coh_scores);
            aggregated.overall_score = (aggregated.faithfulness_score * 0.3 +
                                       aggregated.relevance_score * 0.25 +
                                       aggregated.completeness_score * 0.25 +
                                       aggregated.coherence_score * 0.2);
            break;
        }
    }
    
    // Calculate average confidence
    double confidence_sum = 0.0;
    for (const auto& vote : votes) {
        confidence_sum += vote.result.confidence;
    }
    aggregated.confidence = confidence_sum / votes.size();
    
    aggregated.judge_model = "ensemble";
    
    return aggregated;
}

DisagreementAnalysis JudgeEnsemble::analyzeDisagreement(
    const std::vector<JudgeVote>& votes
) {
    DisagreementAnalysis analysis;
    
    if (votes.size() < 2) {
        analysis.agreement_score = 1.0;
        analysis.consensus_reached = true;
        analysis.consensus_strength = 1.0;
        return analysis;
    }
    
    // Calculate agreement score
    analysis.agreement_score = calculateAgreement(votes);
    
    // Calculate Kappa metrics
    if (votes.size() == 2) {
        analysis.cohens_kappa = calculateCohensKappa(votes[0], votes[1]);
        analysis.fleiss_kappa = 0.0;
    } else {
        analysis.cohens_kappa = 0.0;
        analysis.fleiss_kappa = calculateFleissKappa(votes);
    }
    
    // Detect outliers
    if (impl_->config.enable_outlier_detection) {
        analysis.outlier_judges = detectOutliers(votes);
    }
    
    // Determine consensus
    analysis.consensus_reached = analysis.agreement_score >= 0.7;
    analysis.consensus_strength = analysis.agreement_score;
    
    THEMIS_DEBUG("Disagreement analysis: agreement={:.2f}, kappa={:.2f}, outliers={}",
                 analysis.agreement_score, 
                 votes.size() == 2 ? analysis.cohens_kappa : analysis.fleiss_kappa,
                 analysis.outlier_judges.size());
    
    return analysis;
}

std::vector<std::string> JudgeEnsemble::detectOutliers(
    const std::vector<JudgeVote>& votes
) {
    std::vector<std::string> outliers;
    
    if (votes.size() < 3) {
        return outliers;  // Need at least 3 judges for outlier detection
    }
    
    // Collect overall scores
    std::vector<double> scores;
    for (const auto& vote : votes) {
        scores.push_back(vote.result.overall_score);
    }
    
    // Calculate mean and std dev
    double mean = impl_->calculateMean(scores);
    double std_dev = impl_->calculateStdDev(scores, mean);
    
    if (std_dev == 0.0) {
        return outliers;  // All scores identical
    }
    
    // Identify outliers (beyond threshold * std dev)
    for (size_t i = 0; i < votes.size(); ++i) {
        double z_score = std::abs((scores[i] - mean) / std_dev);
        if (z_score > impl_->config.outlier_threshold) {
            outliers.push_back(votes[i].judge_id);
        }
    }
    
    return outliers;
}

double JudgeEnsemble::calculateAgreement(
    const std::vector<JudgeVote>& votes
) {
    if (votes.size() < 2) {
        return 1.0;
    }
    
    // Calculate pairwise score differences
    std::vector<double> differences;
    
    for (size_t i = 0; i < votes.size(); ++i) {
        for (size_t j = i + 1; j < votes.size(); ++j) {
            double diff = std::abs(votes[i].result.overall_score - 
                                  votes[j].result.overall_score);
            differences.push_back(diff);
        }
    }
    
    // Agreement = 1 - average difference
    double avg_diff = impl_->calculateMean(differences);
    return std::max(0.0, 1.0 - avg_diff);
}

double JudgeEnsemble::calculateCohensKappa(
    const JudgeVote& vote_a,
    const JudgeVote& vote_b
) {
    // Simplified Cohen's Kappa based on score agreement
    // For continuous scores, we discretize into categories
    
    auto categorize = [](double score) -> int {
        if (score >= 0.8) return 4;  // Excellent
        if (score >= 0.7) return 3;  // Good
        if (score >= 0.6) return 2;  // Fair
        if (score >= 0.5) return 1;  // Poor
        return 0;  // Very Poor
    };
    
    int cat_a = categorize(vote_a.result.overall_score);
    int cat_b = categorize(vote_b.result.overall_score);
    
    // Observed agreement
    double p_o = (cat_a == cat_b) ? 1.0 : 0.0;
    
    // Expected agreement (simplified - assume uniform distribution)
    double p_e = 0.2;  // 1/5 categories
    
    // Cohen's Kappa = (p_o - p_e) / (1 - p_e)
    return (p_o - p_e) / (1.0 - p_e);
}

double JudgeEnsemble::calculateFleissKappa(
    const std::vector<JudgeVote>& votes
) {
    // Simplified Fleiss' Kappa for multiple judges
    // Based on score variance
    
    std::vector<double> scores;
    for (const auto& vote : votes) {
        scores.push_back(vote.result.overall_score);
    }
    
    double mean = impl_->calculateMean(scores);
    double variance = 0.0;
    
    for (double score : scores) {
        double diff = score - mean;
        variance += diff * diff;
    }
    variance /= scores.size();
    
    // Convert variance to agreement measure
    // Low variance = high agreement
    double max_variance = 0.25;  // Maximum expected variance
    double agreement = 1.0 - std::min(variance / max_variance, 1.0);
    
    // Map to Kappa scale (-1 to 1)
    return 2.0 * agreement - 1.0;
}

} // namespace themis::rag::judge
