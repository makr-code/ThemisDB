#ifndef THEMIS_MULTI_AGENT_ORCHESTRATOR_H
#define THEMIS_MULTI_AGENT_ORCHESTRATOR_H

#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <mutex>
#include <nlohmann/json.hpp>

namespace rocksdb {
    class TransactionDB;
}

namespace themis {
namespace llm {

class LLMAgent;
class AgentRoleRegistry;
class ConsensusBuilder;

/**
 * @brief Multi-Agent Orchestrator - Coordinates multiple LLM agents for complex reasoning tasks
 * 
 * Features:
 * - Task decomposition: Break complex problems into subtasks
 * - Agent selection: Choose appropriate agents based on roles
 * - Parallel/sequential execution: Optimize task processing
 * - Result synthesis: Combine agent outputs into coherent response
 * 
 * Design inspired by:
 * - AutoGen (Microsoft Research)
 * - LangGraph (LangChain)
 * - MetaGPT
 * - Mixture of Agents (Together AI)
 */
class MultiAgentOrchestrator {
public:
    enum class TaskType {
        PARALLEL,      // All agents work simultaneously
        SEQUENTIAL,    // Agents work one after another
        HIERARCHICAL   // Meta-agent coordinates sub-agents
    };

    struct Task {
        std::string id;
        std::string description;
        TaskType type;
        std::vector<std::string> required_roles;
        nlohmann::json context;
        
        nlohmann::json toJson() const;
        static Task fromJson(const nlohmann::json& j);
    };

    struct AgentResponse {
        std::string agent_id;
        std::string role;
        std::string response;
        std::vector<std::string> reasoning_steps;
        float confidence;
        nlohmann::json metadata;
        int64_t timestamp_ms;
        int latency_ms;
        
        nlohmann::json toJson() const;
        static AgentResponse fromJson(const nlohmann::json& j);
    };

    struct OrchestratedResult {
        std::string session_id;
        std::vector<AgentResponse> agent_responses;
        std::string synthesized_result;
        nlohmann::json reasoning_trace;
        float overall_confidence;
        int64_t total_duration_ms;
        
        nlohmann::json toJson() const;
        static OrchestratedResult fromJson(const nlohmann::json& j);
    };

    /**
     * @brief Construct MultiAgentOrchestrator
     * @param db RocksDB TransactionDB instance
     * @param role_registry Agent role registry
     * @param consensus_builder Consensus builder
     */
    explicit MultiAgentOrchestrator(
        rocksdb::TransactionDB* db,
        std::shared_ptr<AgentRoleRegistry> role_registry,
        std::shared_ptr<ConsensusBuilder> consensus_builder
    );

    ~MultiAgentOrchestrator() = default;

    /**
     * @brief Process tasks using multi-agent collaboration
     * @param tasks Vector of tasks to process
     * @return Orchestrated result with synthesized response
     */
    OrchestratedResult processTasks(const std::vector<Task>& tasks);

    /**
     * @brief Decompose complex prompt into subtasks
     * @param complex_prompt Complex user prompt
     * @return Vector of subtasks with assigned roles
     */
    std::vector<Task> decomposeProblem(const std::string& complex_prompt);

    /**
     * @brief Synthesize multiple agent responses
     * @param responses Vector of agent responses
     * @return Orchestrated result with consensus
     */
    OrchestratedResult synthesizeResults(const std::vector<AgentResponse>& responses);

    /**
     * @brief Register an agent with the orchestrator
     * @param agent Shared pointer to LLMAgent
     */
    void registerAgent(std::shared_ptr<LLMAgent> agent);

    /**
     * @brief Get agent by ID
     * @param agent_id Agent identifier
     * @return Agent if found, nullptr otherwise
     */
    std::shared_ptr<LLMAgent> getAgent(const std::string& agent_id) const;

    /**
     * @brief Get agents by role
     * @param role Role identifier
     * @return Vector of agents with matching role
     */
    std::vector<std::shared_ptr<LLMAgent>> getAgentsByRole(const std::string& role) const;

    /**
     * @brief List all registered agents
     * @return Vector of agent IDs
     */
    std::vector<std::string> listAgents() const;

private:
    rocksdb::TransactionDB* db_;
    std::shared_ptr<AgentRoleRegistry> role_registry_;
    std::shared_ptr<ConsensusBuilder> consensus_builder_;
    
    // Thread safety mutex
    mutable std::mutex mutex_;
    
    // Agent pool: agent_id -> LLMAgent
    std::map<std::string, std::shared_ptr<LLMAgent>> agents_;
    
    // Role mapping: role -> vector of agent_ids
    std::map<std::string, std::vector<std::string>> role_to_agents_;
    
    // Helper methods
    std::vector<Task> decomposeByKeywords(const std::string& prompt);
    std::vector<Task> decomposeByTemplate(const std::string& prompt);
    AgentResponse executeTask(const Task& task, std::shared_ptr<LLMAgent> agent);
    std::vector<AgentResponse> executeParallel(const std::vector<Task>& tasks);
    std::vector<AgentResponse> executeSequential(const std::vector<Task>& tasks);
    std::vector<AgentResponse> executeHierarchical(const std::vector<Task>& tasks);
    
    std::string generateSessionId() const;
    int64_t getCurrentTimestampMs() const;
};

} // namespace llm
} // namespace themis

#endif // THEMIS_MULTI_AGENT_ORCHESTRATOR_H
