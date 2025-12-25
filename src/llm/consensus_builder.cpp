#include "llm/consensus_builder.h"
#include "llm/multi_agent_orchestrator.h"
#include <algorithm>
#include <map>
#include <sstream>

namespace themis {
namespace llm {

using AgentResponse = MultiAgentOrchestrator::AgentResponse;

// ConsensusConfig methods
nlohmann::json ConsensusBuilder::ConsensusConfig::toJson() const {
    std::string strategy_str;
    switch (strategy) {
        case StrategyType::MAJORITY_VOTE: strategy_str = "MAJORITY_VOTE"; break;
        case StrategyType::WEIGHTED_AVERAGE: strategy_str = "WEIGHTED_AVERAGE"; break;
        case StrategyType::BEST_RESPONSE: strategy_str = "BEST_RESPONSE"; break;
        case StrategyType::SYNTHESIZE: strategy_str = "SYNTHESIZE"; break;
        case StrategyType::HIERARCHICAL: strategy_str = "HIERARCHICAL"; break;
    }
    
    return nlohmann::json{
        {"strategy", strategy_str},
        {"confidence_threshold", confidence_threshold},
        {"require_unanimity", require_unanimity},
        {"meta_agent_id", meta_agent_id.value_or("")},
        {"role_weights", role_weights}
    };
}

ConsensusBuilder::ConsensusConfig ConsensusBuilder::ConsensusConfig::fromJson(const nlohmann::json& j) {
    ConsensusConfig config;
    
    std::string strategy_str = j.value("strategy", "WEIGHTED_AVERAGE");
    if (strategy_str == "MAJORITY_VOTE") config.strategy = StrategyType::MAJORITY_VOTE;
    else if (strategy_str == "BEST_RESPONSE") config.strategy = StrategyType::BEST_RESPONSE;
    else if (strategy_str == "SYNTHESIZE") config.strategy = StrategyType::SYNTHESIZE;
    else if (strategy_str == "HIERARCHICAL") config.strategy = StrategyType::HIERARCHICAL;
    else config.strategy = StrategyType::WEIGHTED_AVERAGE;
    
    config.confidence_threshold = j.value("confidence_threshold", 0.7f);
    config.require_unanimity = j.value("require_unanimity", false);
    
    if (j.contains("meta_agent_id") && !j["meta_agent_id"].is_null()) {
        config.meta_agent_id = j["meta_agent_id"].get<std::string>();
    }
    
    config.role_weights = j.value("role_weights", std::map<std::string, float>{});
    
    return config;
}

// ConsensusResult methods
nlohmann::json ConsensusBuilder::ConsensusResult::toJson() const {
    return nlohmann::json{
        {"final_response", final_response},
        {"consensus_score", consensus_score},
        {"agent_contributions", agent_contributions},
        {"conflicts", conflicts},
        {"metadata", metadata}
    };
}

ConsensusBuilder::ConsensusResult ConsensusBuilder::ConsensusResult::fromJson(const nlohmann::json& j) {
    ConsensusResult result;
    result.final_response = j.value("final_response", "");
    result.consensus_score = j.value("consensus_score", 0.0f);
    result.agent_contributions = j.value("agent_contributions", std::map<std::string, float>{});
    result.conflicts = j.value("conflicts", std::vector<std::string>{});
    result.metadata = j.value("metadata", nlohmann::json::object());
    return result;
}

// Main consensus building method
ConsensusBuilder::ConsensusResult ConsensusBuilder::buildConsensus(
    const std::vector<AgentResponse>& responses,
    const ConsensusConfig& config
) const {
    if (responses.empty()) {
        return ConsensusResult{
            "",
            0.0f,
            {},
            {},
            nlohmann::json{{"error", "No responses to build consensus from"}}
        };
    }
    
    // Dispatch to appropriate strategy
    switch (config.strategy) {
        case StrategyType::MAJORITY_VOTE:
            return buildMajorityVote(responses, config);
        case StrategyType::WEIGHTED_AVERAGE:
            return buildWeightedAverage(responses, config);
        case StrategyType::BEST_RESPONSE:
            return buildBestResponse(responses, config);
        case StrategyType::SYNTHESIZE:
            return buildSynthesize(responses, config);
        case StrategyType::HIERARCHICAL:
            return buildHierarchical(responses, config);
        default:
            return buildWeightedAverage(responses, config);
    }
}

// Conflict detection
std::vector<std::string> ConsensusBuilder::detectConflicts(
    const std::vector<AgentResponse>& responses
) const {
    std::vector<std::string> conflicts;
    
    // Simple conflict detection: if responses are too dissimilar
    for (size_t i = 0; i < responses.size(); i++) {
        for (size_t j = i + 1; j < responses.size(); j++) {
            float similarity = calculateSimilarity(
                responses[i].response,
                responses[j].response
            );
            
            if (similarity < 0.3f) {
                std::stringstream ss;
                ss << "Low similarity (" << similarity << ") between "
                   << responses[i].role << " and " << responses[j].role;
                conflicts.push_back(ss.str());
            }
        }
    }
    
    return conflicts;
}

// Consensus score calculation
float ConsensusBuilder::calculateConsensusScore(
    const std::vector<AgentResponse>& responses
) const {
    if (responses.size() < 2) return 1.0f;
    
    // Calculate average pairwise similarity
    float total_similarity = 0.0f;
    int pair_count = 0;
    
    for (size_t i = 0; i < responses.size(); i++) {
        for (size_t j = i + 1; j < responses.size(); j++) {
            total_similarity += calculateSimilarity(
                responses[i].response,
                responses[j].response
            );
            pair_count++;
        }
    }
    
    return pair_count > 0 ? total_similarity / pair_count : 0.0f;
}

// Strategy implementations
ConsensusBuilder::ConsensusResult ConsensusBuilder::buildMajorityVote(
    const std::vector<AgentResponse>& responses,
    const ConsensusConfig& config
) const {
    // Group similar responses
    auto groups = groupSimilarResponses(responses);
    
    // Find majority
    std::string majority = selectMajority(groups, responses);
    
    ConsensusResult result;
    result.final_response = majority;
    result.consensus_score = calculateConsensusScore(responses);
    result.conflicts = detectConflicts(responses);
    
    // Calculate contributions
    for (const auto& resp : responses) {
        result.agent_contributions[resp.agent_id] = resp.confidence;
    }
    
    return result;
}

ConsensusBuilder::ConsensusResult ConsensusBuilder::buildWeightedAverage(
    const std::vector<AgentResponse>& responses,
    const ConsensusConfig& config
) const {
    // Calculate weighted combination based on confidence
    float total_weight = 0.0f;
    std::map<std::string, float> weights;
    
    for (const auto& resp : responses) {
        float weight = resp.confidence;
        
        // Apply role-specific weights if provided
        if (config.role_weights.count(resp.role)) {
            weight *= config.role_weights.at(resp.role);
        }
        
        weights[resp.agent_id] = weight;
        total_weight += weight;
    }
    
    // Normalize weights
    for (auto& [agent_id, weight] : weights) {
        weight /= total_weight;
    }
    
    // For now, select response with highest weight
    // In production, this would do actual weighted synthesis
    std::string best_response;
    float max_weight = 0.0f;
    
    for (const auto& resp : responses) {
        if (weights[resp.agent_id] > max_weight) {
            max_weight = weights[resp.agent_id];
            best_response = resp.response;
        }
    }
    
    ConsensusResult result;
    result.final_response = best_response;
    result.consensus_score = calculateConsensusScore(responses);
    result.agent_contributions = weights;
    result.conflicts = detectConflicts(responses);
    
    return result;
}

ConsensusBuilder::ConsensusResult ConsensusBuilder::buildBestResponse(
    const std::vector<AgentResponse>& responses,
    const ConsensusConfig& config
) const {
    // Simply select response with highest confidence
    auto best = std::max_element(responses.begin(), responses.end(),
        [](const AgentResponse& a, const AgentResponse& b) {
            return a.confidence < b.confidence;
        });
    
    ConsensusResult result;
    result.final_response = best->response;
    result.consensus_score = best->confidence;
    
    // Set contribution to 1.0 for winner, 0.0 for others
    for (const auto& resp : responses) {
        result.agent_contributions[resp.agent_id] = 
            (resp.agent_id == best->agent_id) ? 1.0f : 0.0f;
    }
    
    result.conflicts = detectConflicts(responses);
    
    return result;
}

ConsensusBuilder::ConsensusResult ConsensusBuilder::buildSynthesize(
    const std::vector<AgentResponse>& responses,
    const ConsensusConfig& config
) const {
    // STUB: In production, this would use a meta-agent to synthesize
    std::stringstream ss;
    ss << "Synthesized response from " << responses.size() << " agents:\n\n";
    
    for (const auto& resp : responses) {
        ss << "[" << resp.role << "]: " << resp.response << "\n\n";
    }
    
    ConsensusResult result;
    result.final_response = ss.str();
    result.consensus_score = calculateConsensusScore(responses);
    result.conflicts = detectConflicts(responses);
    
    // Equal contributions
    for (const auto& resp : responses) {
        result.agent_contributions[resp.agent_id] = 1.0f / responses.size();
    }
    
    return result;
}

ConsensusBuilder::ConsensusResult ConsensusBuilder::buildHierarchical(
    const std::vector<AgentResponse>& responses,
    const ConsensusConfig& config
) const {
    // STUB: Similar to synthesize for now
    return buildSynthesize(responses, config);
}

// Helper methods
float ConsensusBuilder::calculateSimilarity(const std::string& a, const std::string& b) const {
    // Use normalized Levenshtein distance for better similarity detection
    // This handles different string lengths and whitespace variations correctly
    
    if (a.empty() && b.empty()) return 1.0f;
    if (a.empty() || b.empty()) return 0.0f;
    if (a == b) return 1.0f;
    
    // Levenshtein distance calculation
    const size_t len_a = a.length();
    const size_t len_b = b.length();
    
    // Use single vector for space optimization
    std::vector<size_t> costs(len_b + 1);
    
    // Initialize first row
    for (size_t j = 0; j <= len_b; ++j) {
        costs[j] = j;
    }
    
    // Calculate edit distance
    for (size_t i = 1; i <= len_a; ++i) {
        costs[0] = i;
        size_t prev_diag = i - 1;
        
        for (size_t j = 1; j <= len_b; ++j) {
            size_t prev_costs_j = costs[j];
            
            if (a[i - 1] == b[j - 1]) {
                costs[j] = prev_diag;
            } else {
                // Use explicit min for better performance in hot path
                costs[j] = 1 + std::min(std::min(costs[j], costs[j - 1]), prev_diag);
            }
            
            prev_diag = prev_costs_j;
        }
    }
    
    // Normalize to [0, 1] range
    size_t edit_distance = costs[len_b];
    size_t max_len = std::max(len_a, len_b);
    
    return 1.0f - (static_cast<float>(edit_distance) / max_len);
}

std::map<std::string, size_t> ConsensusBuilder::groupSimilarResponses(
    const std::vector<AgentResponse>& responses
) const {
    // Stub: Group by first 50 characters
    std::map<std::string, size_t> groups;
    
    for (const auto& resp : responses) {
        std::string key = resp.response.substr(0, std::min(size_t(50), resp.response.length()));
        groups[key]++;
    }
    
    return groups;
}

std::string ConsensusBuilder::selectMajority(
    const std::map<std::string, size_t>& groups,
    const std::vector<AgentResponse>& responses
) const {
    if (groups.empty()) return "";
    
    // Find group with most votes
    auto max_group = std::max_element(groups.begin(), groups.end(),
        [](const auto& a, const auto& b) {
            return a.second < b.second;
        });
    
    // Return full response from that group
    for (const auto& resp : responses) {
        if (resp.response.find(max_group->first) == 0) {
            return resp.response;
        }
    }
    
    return responses[0].response;
}

} // namespace llm
} // namespace themis
