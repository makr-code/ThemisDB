#ifndef THEMIS_CONSENSUS_BUILDER_H
#define THEMIS_CONSENSUS_BUILDER_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <optional>
#include <nlohmann/json.hpp>

namespace themis {
namespace llm {

// Forward declaration
struct AgentResponse;

/**
 * @brief ConsensusBuilder - Fuses multiple agent responses into consensus
 * 
 * Strategies:
 * - MAJORITY_VOTE: Democratic voting
 * - WEIGHTED_AVERAGE: Confidence-weighted
 * - BEST_RESPONSE: Highest confidence wins
 * - SYNTHESIZE: Meta-agent synthesizes all responses
 * - HIERARCHICAL: Meta-agent makes final decision
 * 
 * Handles conflict detection and resolution.
 */
class ConsensusBuilder {
public:
    enum class StrategyType {
        MAJORITY_VOTE,        // Democratic voting
        WEIGHTED_AVERAGE,     // Weighted by confidence
        BEST_RESPONSE,        // Highest confidence wins
        SYNTHESIZE,           // LLM synthesizes all responses
        HIERARCHICAL          // Meta-agent decides
    };

    struct ConsensusConfig {
        StrategyType strategy;
        float confidence_threshold;
        bool require_unanimity;
        std::optional<std::string> meta_agent_id;  // For HIERARCHICAL/SYNTHESIZE
        std::map<std::string, float> role_weights; // For WEIGHTED_AVERAGE
        
        nlohmann::json toJson() const;
        static ConsensusConfig fromJson(const nlohmann::json& j);
    };

    struct ConsensusResult {
        std::string final_response;
        float consensus_score;  // 0.0-1.0, how much agents agree
        std::map<std::string, float> agent_contributions;
        std::vector<std::string> conflicts;  // Detected disagreements
        nlohmann::json metadata;
        
        nlohmann::json toJson() const;
        static ConsensusResult fromJson(const nlohmann::json& j);
    };

    /**
     * @brief Construct ConsensusBuilder
     */
    ConsensusBuilder() = default;

    ~ConsensusBuilder() = default;

    /**
     * @brief Build consensus from multiple agent responses
     * @param responses Vector of agent responses
     * @param config Consensus configuration
     * @return Consensus result
     */
    ConsensusResult buildConsensus(
        const std::vector<AgentResponse>& responses,
        const ConsensusConfig& config
    ) const;

    /**
     * @brief Detect conflicts between responses
     * @param responses Vector of agent responses
     * @return Vector of conflict descriptions
     */
    std::vector<std::string> detectConflicts(
        const std::vector<AgentResponse>& responses
    ) const;

    /**
     * @brief Calculate consensus score
     * @param responses Vector of agent responses
     * @return Consensus score (0.0-1.0)
     */
    float calculateConsensusScore(
        const std::vector<AgentResponse>& responses
    ) const;

private:
    // Strategy implementations
    ConsensusResult buildMajorityVote(
        const std::vector<AgentResponse>& responses,
        const ConsensusConfig& config
    ) const;

    ConsensusResult buildWeightedAverage(
        const std::vector<AgentResponse>& responses,
        const ConsensusConfig& config
    ) const;

    ConsensusResult buildBestResponse(
        const std::vector<AgentResponse>& responses,
        const ConsensusConfig& config
    ) const;

    ConsensusResult buildSynthesize(
        const std::vector<AgentResponse>& responses,
        const ConsensusConfig& config
    ) const;

    ConsensusResult buildHierarchical(
        const std::vector<AgentResponse>& responses,
        const ConsensusConfig& config
    ) const;

    // Helper methods
    float calculateSimilarity(const std::string& a, const std::string& b) const;
    std::map<std::string, size_t> groupSimilarResponses(
        const std::vector<AgentResponse>& responses
    ) const;
    std::string selectMajority(
        const std::map<std::string, size_t>& groups,
        const std::vector<AgentResponse>& responses
    ) const;
};

} // namespace llm
} // namespace themis

#endif // THEMIS_CONSENSUS_BUILDER_H
