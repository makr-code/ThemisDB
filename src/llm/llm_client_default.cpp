/**
 * @file llm_client_default.cpp
 * @brief Default/dummy LLM client implementation for Phase 0.4
 *
 * This is a placeholder implementation for testing and development.
 * In production, this would be replaced with OpenAI, Anthropic, or other client.
 *
 * @author ThemisDB Team
 * @date 2026-06-18
 */

#include "llm/llm_client.h"
#include <spdlog/spdlog.h>
#include <random>

namespace themis::llm {

/**
 * @brief Default LLM client implementation
 *
 * Returns mock AQL for testing; ready for real LLM integration.
 */
class DefaultLLMClient : public LLMClient {
public:
    DefaultLLMClient() : ready_(true) {
        spdlog::debug("DefaultLLMClient initialized (mock/testing mode)");
    }
    
    ~DefaultLLMClient() override = default;
    
    GenerationResult generate(
        const std::string& prompt,
        const GenerationOptions& options) override
    {
        GenerationResult result;
        
        if (!ready_) {
            result.success = false;
            result.error_message = "Client not ready";
            spdlog::warn("LLM generation failed: client not ready");
            return result;
        }
        
        // TODO (Phase 0.4 Final): Replace with real LLM API call
        // For now: return mock AQL based on prompt
        
        // Simulate generation latency
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> latency_dist(50, 200);
        int simulated_latency_ms = latency_dist(gen);
        
        spdlog::debug("LLM simulating {} ms latency", simulated_latency_ms);
        
        // Mock AQL generation: extract key terms from prompt
        std::string mock_aql = "SELECT * FROM data WHERE 1=1";
        
        if (prompt.find("user") != std::string::npos) {
            mock_aql = "SELECT id, name FROM users WHERE status = 'active'";
        } else if (prompt.find("order") != std::string::npos) {
            mock_aql = "SELECT * FROM orders WHERE created_date >= NOW() - '30d'";
        } else if (prompt.find("product") != std::string::npos) {
            mock_aql = "SELECT * FROM products WHERE category = 'electronics'";
        }
        
        result.success = true;
        result.text = mock_aql;
        result.prompt_tokens = estimateTokens(prompt);
        result.completion_tokens = estimateTokens(mock_aql);
        result.finish_reason = "length";
        
        spdlog::debug("LLM generated AQL (mock): {} tokens", result.completion_tokens);
        
        return result;
    }
    
    GenerationResult generateAQL(
        const std::string& nl_query,
        const std::string& schema_context,
        const GenerationOptions& options) override
    {
        spdlog::debug("LLM generating AQL for NL: '{}' (schema_context: {} chars)",
                      nl_query, schema_context.size());
        
        // Construct prompt from NL query + schema context
        std::string prompt = "Generate a ThemisDB AQL query for: " + nl_query;
        if (!schema_context.empty()) {
            prompt += "\n\nSchema:\n" + schema_context;
        }
        
        return generate(prompt, options);
    }
    
    size_t estimateTokens(const std::string& text) const override {
        // Rough heuristic: ~4 characters per token
        return (text.size() + 3) / 4;
    }
    
    std::string getProviderName() const override {
        return "default-mock";
    }
    
    bool isReady() const override {
        return ready_;
    }

private:
    bool ready_;
};

} // namespace themis::llm

// Factory function for creating default LLM client
std::shared_ptr<themis::llm::LLMClient> createDefaultLLMClient() {
    return std::make_shared<themis::llm::DefaultLLMClient>();
}
