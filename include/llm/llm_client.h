/**
 * @file llm_client.h
 * @brief Abstract interface for LLM inference clients
 *
 * Defines the contract for generating text/AQL via LLM backends.
 * Implementations can wrap OpenAI, Anthropic, local Ollama, etc.
 *
 * @author ThemisDB Team
 * @date 2026-06-18
 */

#pragma once

#include <string>
#include <vector>
#include <memory>

namespace themis::llm {

struct GenerationOptions {
    int max_tokens = 256;
    
    float temperature = 0.7f;
    
    int top_k = 0;
    
    float top_p = 0.9f;
    
    std::vector<std::string> stop_sequences;
    
    uint32_t timeout_ms = 10000;
};

struct GenerationResult {
    bool success = false;
    
    std::string text;
    
    std::string error_message;
    
    size_t prompt_tokens = 0;
    
    size_t completion_tokens = 0;
    
    std::string finish_reason;
};

class LLMClient {
public:
    /**
     * @brief LLMClient.
     * @return Return value.
     */
    virtual ~LLMClient() = default;
    
    virtual GenerationResult generate(
        const std::string& prompt,
        const GenerationOptions& options = {}
    ) = 0;
    
    virtual GenerationResult generateAQL(
        const std::string& nl_query,
        const std::string& schema_context = "",
        const GenerationOptions& options = {}
    ) = 0;
    
    /**
     * @brief Estimate Tokens.
     * @param[in] text Input parameter.
     * @return Return value.
     */
    virtual size_t estimateTokens(const std::string& text) const = 0;
    
    /**
     * @brief Get Provider Name.
     * @return Return value.
     */
    virtual std::string getProviderName() const = 0;
    
    /**
     * @brief Is Ready.
     * @return True when the operation succeeds.
     */
    virtual bool isReady() const = 0;
};

} // namespace themis::llm
