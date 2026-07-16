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

/**
 * @brief Options for LLM generation
 */
struct GenerationOptions {
    /// Maximum tokens in response
    int max_tokens = 256;
    
    /// Temperature (0.0 = deterministic, 1.0 = creative)
    float temperature = 0.7f;
    
    /// Top-k sampling (0 = disabled)
    int top_k = 0;
    
    /// Top-p nucleus sampling (0 = disabled)
    float top_p = 0.9f;
    
    /// Stop sequences (generation stops when any is encountered)
    std::vector<std::string> stop_sequences;
    
    /// Timeout for generation in milliseconds
    uint32_t timeout_ms = 10000;
};

/**
 * @brief Result of LLM generation
 */
struct GenerationResult {
    /// Whether generation succeeded
    bool success = false;
    
    /// Generated text (if success == true)
    std::string text;
    
    /// Error message (if success == false)
    std::string error_message;
    
    /// Number of tokens in prompt (for counting)
    size_t prompt_tokens = 0;
    
    /// Number of tokens in completion
    size_t completion_tokens = 0;
    
    /// Reason generation stopped (e.g., "length", "stop_sequence", "error")
    std::string finish_reason;
};

/**
 * @brief Abstract LLM inference client
 *
 * Implementations should:
 * - Handle provider-specific auth/setup
 * - Rate limiting and retries
 * - Token counting
 * - Error handling
 */
class LLMClient {
public:
    virtual ~LLMClient() = default;
    
    /**
     * @brief Generate text from prompt
     *
     * @param prompt The input prompt (may include context, examples, instructions)
     * @param options Generation parameters
     * @return GenerationResult with text on success, error_message on failure
     *
     * @note Thread-safe; can be called concurrently
     * @note Should respect timeout_ms in options
     */
    virtual GenerationResult generate(
        const std::string& prompt,
        const GenerationOptions& options = {}
    ) = 0;
    
    /**
     * @brief Generate AQL from natural language
     *
     * Convenience method specifically for NL→AQL translation.
     * Implementations may apply domain-specific prompting.
     *
     * @param nl_query Natural language query
     * @param schema_context Collection/field constraints for context
     * @param options Generation parameters
     * @return GenerationResult with AQL text on success
     */
    virtual GenerationResult generateAQL(
        const std::string& nl_query,
        const std::string& schema_context = "",
        const GenerationOptions& options = {}
    ) = 0;
    
    /**
     * @brief Estimate tokens in text
     *
     * Used for token budgeting in validation loops.
     *
     * @param text Text to tokenize
     * @return Number of tokens (approximate for some implementations)
     */
    virtual size_t estimateTokens(const std::string& text) const = 0;
    
    /**
     * @brief Get provider name (e.g., "openai", "ollama", "mock")
     */
    virtual std::string getProviderName() const = 0;
    
    /**
     * @brief Check if client is ready to generate
     *
     * Validates auth, connectivity, etc.
     *
     * @return true if client can generate, false if not ready
     */
    virtual bool isReady() const = 0;
};

} // namespace themis::llm
