#include "llm/multi_agent_orchestrator.h"
#include "llm/llm_agent.h"
#include "llm/agent_role_registry.h"
#include "llm/consensus_builder.h"
#include <chrono>
#include <random>
#include <sstream>
#include <iomanip>

namespace themis {
namespace llm {

// AgentResponse methods
nlohmann::json MultiAgentOrchestrator::AgentResponse::toJson() const {
    return nlohmann::json{
        {"agent_id", agent_id},
        {"role", role},
        {"response", response},
        {"reasoning_steps", reasoning_steps},
        {"confidence", confidence},
        {"metadata", metadata},
        {"timestamp_ms", timestamp_ms},
        {"latency_ms", latency_ms}
    };
}

MultiAgentOrchestrator::AgentResponse MultiAgentOrchestrator::AgentResponse::fromJson(const nlohmann::json& j) {
    AgentResponse ar;
    ar.agent_id = j.value("agent_id", "");
    ar.role = j.value("role", "");
    ar.response = j.value("response", "");
    ar.reasoning_steps = j.value("reasoning_steps", std::vector<std::string>{});
    ar.confidence = j.value("confidence", 0.0f);
    ar.metadata = j.value("metadata", nlohmann::json::object());
    ar.timestamp_ms = j.value("timestamp_ms", 0L);
    ar.latency_ms = j.value("latency_ms", 0);
    return ar;
}

// Task methods
nlohmann::json MultiAgentOrchestrator::Task::toJson() const {
    std::string type_str;
    switch (type) {
        case TaskType::PARALLEL: type_str = "PARALLEL"; break;
        case TaskType::SEQUENTIAL: type_str = "SEQUENTIAL"; break;
        case TaskType::HIERARCHICAL: type_str = "HIERARCHICAL"; break;
    }
    
    return nlohmann::json{
        {"id", id},
        {"description", description},
        {"type", type_str},
        {"required_roles", required_roles},
        {"context", context}
    };
}

MultiAgentOrchestrator::Task MultiAgentOrchestrator::Task::fromJson(const nlohmann::json& j) {
    Task t;
    t.id = j.value("id", "");
    t.description = j.value("description", "");
    
    std::string type_str = j.value("type", "PARALLEL");
    if (type_str == "SEQUENTIAL") t.type = TaskType::SEQUENTIAL;
    else if (type_str == "HIERARCHICAL") t.type = TaskType::HIERARCHICAL;
    else t.type = TaskType::PARALLEL;
    
    t.required_roles = j.value("required_roles", std::vector<std::string>{});
    t.context = j.value("context", nlohmann::json::object());
    return t;
}

// OrchestratedResult methods
nlohmann::json MultiAgentOrchestrator::OrchestratedResult::toJson() const {
    nlohmann::json responses_json = nlohmann::json::array();
    for (const auto& ar : agent_responses) {
        responses_json.push_back(ar.toJson());
    }
    
    return nlohmann::json{
        {"session_id", session_id},
        {"agent_responses", responses_json},
        {"synthesized_result", synthesized_result},
        {"reasoning_trace", reasoning_trace},
        {"overall_confidence", overall_confidence},
        {"total_duration_ms", total_duration_ms}
    };
}

MultiAgentOrchestrator::OrchestratedResult MultiAgentOrchestrator::OrchestratedResult::fromJson(const nlohmann::json& j) {
    OrchestratedResult result;
    result.session_id = j.value("session_id", "");
    
    if (j.contains("agent_responses")) {
        for (const auto& ar_json : j["agent_responses"]) {
            result.agent_responses.push_back(AgentResponse::fromJson(ar_json));
        }
    }
    
    result.synthesized_result = j.value("synthesized_result", "");
    result.reasoning_trace = j.value("reasoning_trace", nlohmann::json::object());
    result.overall_confidence = j.value("overall_confidence", 0.0f);
    result.total_duration_ms = j.value("total_duration_ms", 0L);
    return result;
}

// Constructor
MultiAgentOrchestrator::MultiAgentOrchestrator(
    rocksdb::TransactionDB* db,
    std::shared_ptr<AgentRoleRegistry> role_registry,
    std::shared_ptr<ConsensusBuilder> consensus_builder
) : db_(db),
    role_registry_(role_registry),
    consensus_builder_(consensus_builder) {
}

// Main orchestration method
MultiAgentOrchestrator::OrchestratedResult MultiAgentOrchestrator::processTasks(
    const std::vector<Task>& tasks
) {
    auto start_time = std::chrono::steady_clock::now();
    
    std::vector<AgentResponse> all_responses;
    
    // Process tasks based on their type
    for (const auto& task : tasks) {
        std::vector<AgentResponse> task_responses;
        
        switch (task.type) {
            case TaskType::PARALLEL:
                task_responses = executeParallel({task});
                break;
            case TaskType::SEQUENTIAL:
                task_responses = executeSequential({task});
                break;
            case TaskType::HIERARCHICAL:
                task_responses = executeHierarchical({task});
                break;
        }
        
        all_responses.insert(all_responses.end(), task_responses.begin(), task_responses.end());
    }
    
    // Synthesize results
    auto result = synthesizeResults(all_responses);
    
    auto end_time = std::chrono::steady_clock::now();
    result.total_duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time
    ).count();
    
    return result;
}

// Task decomposition
std::vector<MultiAgentOrchestrator::Task> MultiAgentOrchestrator::decomposeProblem(
    const std::string& complex_prompt
) {
    // Simple keyword-based decomposition (stub implementation)
    // In production, this would use an LLM or more sophisticated NLP
    std::vector<Task> tasks;
    
    // Check for multi-perspective analysis keywords
    bool needs_legal = (complex_prompt.find("legal") != std::string::npos ||
                       complex_prompt.find("contract") != std::string::npos ||
                       complex_prompt.find("compliance") != std::string::npos);
    
    bool needs_technical = (complex_prompt.find("technical") != std::string::npos ||
                           complex_prompt.find("code") != std::string::npos ||
                           complex_prompt.find("performance") != std::string::npos);
    
    bool needs_business = (complex_prompt.find("business") != std::string::npos ||
                          complex_prompt.find("financial") != std::string::npos ||
                          complex_prompt.find("strategic") != std::string::npos);
    
    // Create tasks based on identified needs
    if (needs_legal) {
        tasks.push_back({
            generateSessionId(),
            "Analyze from legal perspective: " + complex_prompt,
            TaskType::PARALLEL,
            {"legal_expert"},
            nlohmann::json::object()
        });
    }
    
    if (needs_technical) {
        tasks.push_back({
            generateSessionId(),
            "Analyze from technical perspective: " + complex_prompt,
            TaskType::PARALLEL,
            {"technical_analyst"},
            nlohmann::json::object()
        });
    }
    
    if (needs_business) {
        tasks.push_back({
            generateSessionId(),
            "Analyze from business perspective: " + complex_prompt,
            TaskType::PARALLEL,
            {"business_strategist"},
            nlohmann::json::object()
        });
    }
    
    // If no specific perspective identified, use general analysis
    if (tasks.empty()) {
        tasks.push_back({
            generateSessionId(),
            complex_prompt,
            TaskType::PARALLEL,
            {"general_analyst"},
            nlohmann::json::object()
        });
    }
    
    return tasks;
}

// Result synthesis
MultiAgentOrchestrator::OrchestratedResult MultiAgentOrchestrator::synthesizeResults(
    const std::vector<AgentResponse>& responses
) {
    OrchestratedResult result;
    result.session_id = generateSessionId();
    result.agent_responses = responses;
    
    // Build consensus using ConsensusBuilder
    if (consensus_builder_ && !responses.empty()) {
        auto consensus = consensus_builder_->buildConsensus(responses, {
            ConsensusBuilder::StrategyType::WEIGHTED_AVERAGE,
            0.7f,
            false,
            std::nullopt,
            {}
        });
        
        result.synthesized_result = consensus.final_response;
        result.overall_confidence = consensus.consensus_score;
        result.reasoning_trace = nlohmann::json{
            {"consensus_strategy", "WEIGHTED_AVERAGE"},
            {"agent_contributions", consensus.agent_contributions},
            {"conflicts", consensus.conflicts}
        };
    } else {
        // Fallback: simple concatenation
        std::string combined;
        float total_confidence = 0.0f;
        
        for (const auto& resp : responses) {
            combined += "[" + resp.role + "]: " + resp.response + "\n\n";
            total_confidence += resp.confidence;
        }
        
        result.synthesized_result = combined;
        result.overall_confidence = responses.empty() ? 0.0f : total_confidence / responses.size();
    }
    
    return result;
}

// Agent management
void MultiAgentOrchestrator::registerAgent(std::shared_ptr<LLMAgent> agent) {
    auto agent_id = agent->getId();
    auto role = agent->getRole();
    
    agents_[agent_id] = agent;
    role_to_agents_[role].push_back(agent_id);
}

std::shared_ptr<LLMAgent> MultiAgentOrchestrator::getAgent(const std::string& agent_id) const {
    auto it = agents_.find(agent_id);
    return (it != agents_.end()) ? it->second : nullptr;
}

std::vector<std::shared_ptr<LLMAgent>> MultiAgentOrchestrator::getAgentsByRole(
    const std::string& role
) const {
    std::vector<std::shared_ptr<LLMAgent>> result;
    
    auto it = role_to_agents_.find(role);
    if (it != role_to_agents_.end()) {
        for (const auto& agent_id : it->second) {
            auto agent = getAgent(agent_id);
            if (agent) {
                result.push_back(agent);
            }
        }
    }
    
    return result;
}

std::vector<std::string> MultiAgentOrchestrator::listAgents() const {
    std::vector<std::string> result;
    for (const auto& [agent_id, _] : agents_) {
        result.push_back(agent_id);
    }
    return result;
}

// Private helper methods
MultiAgentOrchestrator::AgentResponse MultiAgentOrchestrator::executeTask(
    const Task& task,
    std::shared_ptr<LLMAgent> agent
) {
    auto start_time = std::chrono::steady_clock::now();
    
    LLMAgent::AgentRequest request{
        task.description,
        task.context,
        {}
    };
    
    auto agent_result = agent->processRequest(request);
    
    auto end_time = std::chrono::steady_clock::now();
    auto latency = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time
    ).count();
    
    return AgentResponse{
        agent->getId(),
        agent->getRole(),
        agent_result.response,
        agent_result.reasoning_steps,
        agent_result.confidence,
        agent_result.metadata,
        getCurrentTimestampMs(),
        static_cast<int>(latency)
    };
}

std::vector<MultiAgentOrchestrator::AgentResponse> MultiAgentOrchestrator::executeParallel(
    const std::vector<Task>& tasks
) {
    std::vector<AgentResponse> responses;
    
    for (const auto& task : tasks) {
        if (task.required_roles.empty()) continue;
        
        auto role = task.required_roles[0];
        auto agents = getAgentsByRole(role);
        
        if (!agents.empty()) {
            auto response = executeTask(task, agents[0]);
            responses.push_back(response);
        }
    }
    
    return responses;
}

std::vector<MultiAgentOrchestrator::AgentResponse> MultiAgentOrchestrator::executeSequential(
    const std::vector<Task>& tasks
) {
    // Similar to parallel for now (stub)
    return executeParallel(tasks);
}

std::vector<MultiAgentOrchestrator::AgentResponse> MultiAgentOrchestrator::executeHierarchical(
    const std::vector<Task>& tasks
) {
    // Similar to parallel for now (stub)
    return executeParallel(tasks);
}

std::string MultiAgentOrchestrator::generateSessionId() const {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    
    std::stringstream ss;
    ss << std::hex;
    for (int i = 0; i < 32; i++) {
        ss << dis(gen);
    }
    return ss.str();
}

int64_t MultiAgentOrchestrator::getCurrentTimestampMs() const {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

} // namespace llm
} // namespace themis
