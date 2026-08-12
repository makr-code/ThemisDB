/**
 * @file embedded_llm.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=5; TODO=1, Stub=3, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "llm/llama_wrapper.h"
#include "llm/ethical_guidelines_manager.h"
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>
#include <functional>

namespace themis {
namespace llm {

/**
 * @brief Simple embedded LLM interface for system-wide use
 * 
 * This provides a simplified, high-level API for using LLM inference
 * throughout the ThemisDB system. It handles common patterns and provides
 * convenient methods for:
 * - AQL LLM_GENERATE() function
 * - Content analysis and summarization
 * - Chat/conversation interfaces
 * - Embeddings for semantic search
 * - SSE streaming for real-time responses
 * - MCP protocol integration
 * 
 * Thread-safe and designed for embedded use.
 */
class EmbeddedLLM {
public:
    using GenerateFullFn = std::function<InferenceResponse(const InferenceRequest&)>;
    using EmbedFn = std::function<std::vector<float>(const std::string&)>;

    /**
     * @brief Configuration for embedded LLM
     */
    struct Config {
        std::string model_path = "models/default.gguf";
        std::string model_id = "default";
        int n_gpu_layers = 0;          // 0 = CPU only
        int n_ctx = 4096;              // Context size
        /// @brief Batch size passed to llama_context. Must be >= the longest
        /// prompt that will be submitted in a single llama_decode call.
        /// Defaults to n_ctx so RAG/docs prompts are never truncated.
        int n_batch = 4096;
        int n_threads = 4;             // CPU threads
        bool enable_caching = true;    // Response caching
        bool enable_streaming = false; // Default: no streaming
        
        // Ethical guidelines configuration
        bool enable_ethical_guidelines = true;  // Enable ethical guidelines system
        std::string ethical_guidelines_config = "config/ethical_guidelines.yaml";
    };
    
    explicit EmbeddedLLM(const Config& config);
    explicit EmbeddedLLM(); // Default constructor
    ~EmbeddedLLM();
    
    // ═══════════════════════════════════════════════════════════
    // Simple text generation (for AQL, content analysis, etc.)
    // ═══════════════════════════════════════════════════════════
    
    /**
     * @brief Generate text completion (blocking)
     * @param prompt Input prompt
     * @param max_tokens Maximum tokens to generate
     * @return Generated text
     */
    std::string generate(const std::string& prompt, int max_tokens = 512);
    
    /**
     * @brief Generate with custom parameters
     */
    std::string generateWithParams(
        const std::string& prompt,
        float temperature = 0.7f,
        float top_p = 0.9f,
        int max_tokens = 512
    );
    
    // ═══════════════════════════════════════════════════════════
    // Chat interface (for multi-turn conversations)
    // ═══════════════════════════════════════════════════════════
    
    /**
     * @brief Chat completion with message history
     * @param messages Conversation history
     * @param format Chat template format (ChatML, Llama2, etc.)
     * @return Assistant's response
     */
    std::string chat(
        const std::vector<ChatMessage>& messages,
        ChatFormat format = ChatFormat::ChatML
    );
    
    /**
     * @brief Simple chat (system + user message)
     */
    std::string chatSimple(
        const std::string& system_prompt,
        const std::string& user_message
    );
    
    // ═══════════════════════════════════════════════════════════
    // Embeddings (for semantic search, vector DB)
    // ═══════════════════════════════════════════════════════════
    
    /**
     * @brief Generate embedding vector for text
     * @param text Input text
     * @return Normalized embedding vector
     */
    std::vector<float> embed(const std::string& text);
    
    /**
     * @brief Batch embed multiple texts while preserving the single-text
     *        embedding contract for every entry.
     *
     * The implementation may delegate to the configured backend one item at a
     * time when no native batch API is available. Callers can therefore rely on
     * each returned vector being equivalent to a corresponding `embed(text)`
     * invocation, including deterministic fallback behavior and normalization.
     */
    std::vector<std::vector<float>> embedBatch(const std::vector<std::string>& texts);
    
    // ═══════════════════════════════════════════════════════════
    // Streaming (for SSE, real-time UI)
    // ═══════════════════════════════════════════════════════════
    
    /**
     * @brief Generate with streaming callback
     * @param prompt Input prompt
     * @param callback Called for each generated token
     * @param max_tokens Maximum tokens
     * @return Full generated text
     */
    std::string generateStreaming(
        const std::string& prompt,
        std::function<void(const std::string& token)> callback,
        int max_tokens = 512
    );
    
    /**
     * @brief Generate streaming response formatted as SSE
     * Callback receives SSE-formatted strings ready to send
     */
    std::string generateStreamingSSE(
        const std::string& prompt,
        std::function<void(const std::string& sse_event)> callback,
        const std::string& request_id = "",
        int max_tokens = 512
    );
    
    // ═══════════════════════════════════════════════════════════
    // Output formatting (for MCP, AQL, JSON responses)
    // ═══════════════════════════════════════════════════════════
    
    /**
     * @brief Generate and format as MCP response
     */
    json generateAsMCP(const std::string& prompt, int max_tokens = 512);
    
    /**
     * @brief Generate and format as JSON with markdown
     */
    json generateAsJsonMarkdown(const std::string& prompt, int max_tokens = 512);
    
    /**
     * @brief Generate with full response metadata
     */
    InferenceResponse generateFull(const InferenceRequest& request);
    
    // ═══════════════════════════════════════════════════════════
    // Utility methods
    // ═══════════════════════════════════════════════════════════
    
    /**
     * @brief Check if model is loaded
     */
    bool isReady() const;
    
    /**
     * @brief Get model information
     */
    std::string getModelInfo() const;
    
    /**
     * @brief Get performance statistics
     */
    json getStats() const;

    /**
     * @brief Inject a generation backend override.
     *
     * When set, generation methods delegate to this callback before using the
     * built-in wrapper/fallback path.
     */
    void setGenerateFullFn(GenerateFullFn fn);

    /**
     * @brief Inject an embedding backend override.
     *
     * When set, embedding methods delegate to this callback before using the
     * built-in wrapper/fallback path.
     */
    void setEmbedFn(EmbedFn fn);
    
    /**
     * @brief Clear response cache
     *
     * Clears the in-memory embedding cache.  Subsequent calls to embed()
     * will recompute embeddings from the model.
     */
    void clearCache();
    
    // ═══════════════════════════════════════════════════════════
    // Ethical Guidelines Support
    // ═══════════════════════════════════════════════════════════
    
    /**
     * @brief Get ethical guidelines manager (if enabled)
     * @return Pointer to manager or nullptr if not enabled
     */
    EthicalGuidelinesManager* getEthicalGuidelines();
    
    /**
     * @brief Check if ethical guidelines are enabled
     */
    bool hasEthicalGuidelines() const;
    
private:
    std::unique_ptr<LlamaWrapper> wrapper_;
    Config config_;
    std::unique_ptr<EthicalGuidelinesManager> ethical_guidelines_;

    mutable std::mutex callback_mutex_;
    GenerateFullFn generate_full_fn_;
    EmbedFn embed_fn_;

    // Embedding cache: text → embedding vector (thread-safe via cache_mutex_)
    mutable std::mutex cache_mutex_;
    std::unordered_map<std::string, std::vector<float>> embedding_cache_;
    
    // Internal helpers
    InferenceRequest createRequest(
        const std::string& prompt,
        int max_tokens,
        float temperature = 0.7f,
        float top_p = 0.9f
    );
    
    // Apply ethical guidelines to prompt if enabled
    std::string applyEthicalGuidelines(
        const std::string& prompt,
        const std::string& context_text = ""
    );
};

/**
 * @brief Global embedded LLM instance accessor
 * 
 * Provides singleton-like access to embedded LLM for system-wide use.
 * Thread-safe and lazy-initialized.
 */
class EmbeddedLLMManager {
public:
    static EmbeddedLLMManager& instance();
    
    /**
     * @brief Initialize with configuration
     */
    void initialize(const EmbeddedLLM::Config& config);
    
    /**
     * @brief Get the embedded LLM instance
     */
    EmbeddedLLM& get();
    
    /**
     * @brief Check if initialized
     */
    bool isInitialized() const;
    
private:
    EmbeddedLLMManager() = default;
    ~EmbeddedLLMManager() = default;
    
    std::unique_ptr<EmbeddedLLM> llm_;
    mutable std::mutex mutex_;
    bool initialized_ = false;
};

// Convenience macros for common use cases
#define THEMIS_LLM() themis::llm::EmbeddedLLMManager::instance().get()
#define THEMIS_LLM_GENERATE(prompt) THEMIS_LLM().generate(prompt)
#define THEMIS_LLM_EMBED(text) THEMIS_LLM().embed(text)
#define THEMIS_LLM_CHAT(messages) THEMIS_LLM().chat(messages)

} // namespace llm
} // namespace themis
