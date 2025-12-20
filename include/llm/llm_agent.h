#ifndef THEMIS_LLM_AGENT_H
#define THEMIS_LLM_AGENT_H

#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <nlohmann/json.hpp>

namespace rocksdb {
    class TransactionDB;
}

namespace themis {
namespace llm {

/**
 * @brief LLMAgent - Represents a specialized agent with role and LoRA adapter
 * 
 * Features:
 * - Role-based specialization (legal, technical, business, etc.)
 * - LoRA adapter support for domain-specific fine-tuning
 * - Context management and chain-of-thought reasoning
 * - Peer communication for iterative refinement
 * 
 * Each agent represents a "perspective" or "expert" that collaborates
 * with other agents to solve complex problems.
 */
class LLMAgent {
public:
    struct AgentConfig {
        std::string agent_id;
        std::string role;              // "legal_expert", "technical_analyst", etc.
        std::string lora_adapter_id;   // LoRA adapter for specialization
        std::string base_model;        // "mistral-7b", "llama-3-8b"
        int max_context_length;
        float temperature;
        nlohmann::json role_instructions; // System prompt template
        nlohmann::json metadata;
        
        nlohmann::json toJson() const;
        static AgentConfig fromJson(const nlohmann::json& j);
    };

    struct AgentRequest {
        std::string prompt;
        nlohmann::json context;        // Additional context
        std::vector<std::string> peer_responses; // For iterative refinement
        int max_tokens = 2048;
        float temperature = 0.7;
        
        nlohmann::json toJson() const;
        static AgentRequest fromJson(const nlohmann::json& j);
    };

    struct AgentResult {
        std::string response;
        std::vector<std::string> reasoning_steps;
        float confidence;
        nlohmann::json metadata;
        int token_count;
        int latency_ms;
        
        nlohmann::json toJson() const;
        static AgentResult fromJson(const nlohmann::json& j);
    };

    /**
     * @brief Construct LLMAgent
     * @param config Agent configuration
     * @param db RocksDB TransactionDB instance
     */
    explicit LLMAgent(const AgentConfig& config, rocksdb::TransactionDB* db);

    ~LLMAgent() = default;

    /**
     * @brief Process request and generate response
     * @param request Agent request with prompt and context
     * @return Agent result with response and reasoning
     */
    AgentResult processRequest(const AgentRequest& request);

    /**
     * @brief Validate response quality
     * @param response Response to validate
     * @return True if response meets quality criteria
     */
    bool validateResponse(const std::string& response) const;

    /**
     * @brief Get agent ID
     * @return Agent identifier
     */
    std::string getId() const { return config_.agent_id; }

    /**
     * @brief Get agent role
     * @return Role identifier
     */
    std::string getRole() const { return config_.role; }

    /**
     * @brief Get agent configuration
     * @return Agent configuration
     */
    const AgentConfig& getConfig() const { return config_; }

    /**
     * @brief Update agent configuration
     * @param config New configuration
     */
    void updateConfig(const AgentConfig& config);

    /**
     * @brief Get agent statistics
     * @return JSON with statistics (requests, avg_latency, etc.)
     */
    nlohmann::json getStats() const;

private:
    AgentConfig config_;
    rocksdb::TransactionDB* db_;
    
    // Statistics
    size_t total_requests_ = 0;
    int64_t total_tokens_ = 0;
    int64_t total_latency_ms_ = 0;
    
    // Helper methods
    std::string buildSystemPrompt() const;
    std::string formatPromptWithContext(const AgentRequest& request) const;
    std::vector<std::string> extractReasoningSteps(const std::string& response) const;
    float estimateConfidence(const std::string& response) const;
    
    // Stub for LLM inference (will be implemented with llama.cpp in v1.5.0)
    std::string generateResponse(const std::string& prompt, const AgentRequest& request);
    
    int64_t getCurrentTimestampMs() const;
};

} // namespace llm
} // namespace themis

#endif // THEMIS_LLM_AGENT_H
