/**
 * @file judge_ensemble.h
 * @brief Judge ensemble with voting strategies and disagreement analysis
 * 
 * Implements multi-judge architecture with various voting strategies,
 * disagreement analysis, and consensus building.
 */

#pragma once

#include "rag/rag_judge.h"
#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace themis::rag::judge {

/**
 * @brief Voting strategy for ensemble
 */
enum class VotingStrategy {
    MAJORITY_VOTING,       ///< Simple majority
    WEIGHTED_AVERAGE,      ///< Average weighted by confidence
    CONFIDENCE_WEIGHTED,   ///< Weight by calibrated confidence
    HIERARCHICAL           ///< Cascading with disagreement resolution
};

/**
 * @brief Individual judge result in ensemble
 */
struct JudgeVote {
    std::string judge_id;
    EvaluationResult result;
    double weight;  ///< Judge weight for voting
};

/**
 * @brief Disagreement analysis result
 */
struct DisagreementAnalysis {
    double agreement_score;          ///< Overall agreement 0-1
    double cohens_kappa;             ///< Cohen's Kappa for 2 judges
    double fleiss_kappa;             ///< Fleiss' Kappa for 3+ judges
    std::vector<std::string> outlier_judges;  ///< Judges with outlier scores
    bool consensus_reached;
    double consensus_strength;       ///< 0-1
};

/**
 * @brief Ensemble evaluation result
 */
struct EnsembleResult {
    EvaluationResult aggregated_result;
    std::vector<JudgeVote> individual_votes;
    DisagreementAnalysis disagreement;
    VotingStrategy strategy_used;
    
    // Metadata
    size_t num_judges;
    std::chrono::milliseconds total_time;
};

/**
 * @brief Judge ensemble for robust evaluation
 * 
 * Creates multiple independent judge instances and aggregates
 * their evaluations using various voting strategies.
 */
class JudgeEnsemble {
public:
    /**
     * @brief Configuration for ensemble
     */
    struct Config {
        size_t num_judges = 3;
        VotingStrategy voting_strategy = VotingStrategy::WEIGHTED_AVERAGE;
        double confidence_threshold = 0.6;
        bool enable_disagreement_analysis = true;
        bool enable_outlier_detection = true;
        double outlier_threshold = 2.0;  ///< Std deviations for outlier
        bool parallel_execution = false;  ///< Execute judges in parallel
    };

    /**
     * @brief Construct ensemble with configuration
     */
    explicit JudgeEnsemble(const Config& config = {});
    
    /**
     * @brief Destructor
     */
    ~JudgeEnsemble();
    
    /**
     * @brief Evaluate using judge ensemble
     * @param query User query
     * @param documents Retrieved documents
     * @param answer Generated answer
     * @return Ensemble evaluation result
     */
    EnsembleResult evaluate(
        const std::string& query,
        const std::vector<RetrievedDocument>& documents,
        const std::string& answer
    );
    
    /**
     * @brief Aggregate individual judge votes
     * @param votes Individual judge votes
     * @param strategy Voting strategy to use
     * @return Aggregated evaluation result
     */
    EvaluationResult aggregateVotes(
        const std::vector<JudgeVote>& votes,
        VotingStrategy strategy
    );
    
    /**
     * @brief Analyze disagreement between judges
     * @param votes Individual judge votes
     * @return Disagreement analysis
     */
    DisagreementAnalysis analyzeDisagreement(
        const std::vector<JudgeVote>& votes
    );
    
    /**
     * @brief Detect outlier judges
     * @param votes Individual judge votes
     * @return List of outlier judge IDs
     */
    std::vector<std::string> detectOutliers(
        const std::vector<JudgeVote>& votes
    );
    
    /**
     * @brief Calculate inter-judge agreement
     * @param votes Individual judge votes
     * @return Agreement score 0-1
     */
    double calculateAgreement(
        const std::vector<JudgeVote>& votes
    );
    
    /**
     * @brief Calculate Cohen's Kappa for two judges
     * @param vote_a First judge vote
     * @param vote_b Second judge vote
     * @return Cohen's Kappa -1 to 1
     */
    double calculateCohensKappa(
        const JudgeVote& vote_a,
        const JudgeVote& vote_b
    );
    
    /**
     * @brief Calculate Fleiss' Kappa for multiple judges
     * @param votes All judge votes
     * @return Fleiss' Kappa -1 to 1
     */
    double calculateFleissKappa(
        const std::vector<JudgeVote>& votes
    );

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace themis::rag::judge
