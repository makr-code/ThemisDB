/**
 * @file embedded_llm.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

class EmbeddedLLM {
public:
    using GenerateFullFn = std::function<InferenceResponse(const InferenceRequest&)>;
    using EmbedFn = std::function<std::vector<float>(const std::string&)>;

    struct Config {
        std::string model_path = "models/default.gguf";
        std::string model_id = "default";
        int n_gpu_layers = 0;          // 0 = CPU only
        int n_ctx = 4096;              // Context size
        int n_batch = 4096;
        int n_threads = 4;             // CPU threads
        bool enable_caching = true;    // Response caching
        bool enable_streaming = false; // Default: no streaming
        
        // Ethical guidelines configuration
        bool enable_ethical_guidelines = true;  // Enable ethical guidelines system
        std::string ethical_guidelines_config = "config/ethical_guidelines.yaml";
    };
    
    /**
     * @brief Embedded LLM.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit EmbeddedLLM(const Config& config);
    /**
     * @brief Embedded LLM.
     * @return Return value.
     */
    explicit EmbeddedLLM(); // Default constructor
    ~EmbeddedLLM();
    
    // ═══════════════════════════════════════════════════════════
    // Simple text generation (for AQL, content analysis, etc.)
    // ═══════════════════════════════════════════════════════════
    
    std::string generate(const std::string& prompt, int max_tokens = 512);
    
    std::string generateWithParams(
        const std::string& prompt,
        float temperature = 0.7f,
        float top_p = 0.9f,
        int max_tokens = 512
    );
    
    // ═══════════════════════════════════════════════════════════
    // Chat interface (for multi-turn conversations)
    // ═══════════════════════════════════════════════════════════
    
    std::string chat(
        const std::vector<ChatMessage>& messages,
        ChatFormat format = ChatFormat::ChatML
    );
    
    /**
     * @brief Chat Simple.
     * @param[in] system_prompt Input parameter.
     * @param[in] user_message Input parameter.
     * @return Return value.
     */
    std::string chatSimple(
        const std::string& system_prompt,
        const std::string& user_message
    );
    
    /**
     * @brief ═══════════════════════════════════════════════════════════ Embeddings (for semantic search, vector DB) ═══════════════════════════════════════════════════════════
     * @param[in] text Input parameter.
     * @return Return value.
     */
    
    std::vector<float> embed(const std::string& text);
    
    /**
     * @brief Embed Batch.
     * @param[in] texts Input parameter.
     * @return Return value.
     */
    std::vector<std::vector<float>> embedBatch(const std::vector<std::string>& texts);
    
    // ═══════════════════════════════════════════════════════════
    // Streaming (for SSE, real-time UI)
    // ═══════════════════════════════════════════════════════════
    
    std::string generateStreaming(
        const std::string& prompt,
        std::function<void(const std::string& token)> callback,
        int max_tokens = 512
    );
    
    std::string generateStreamingSSE(
        const std::string& prompt,
        std::function<void(const std::string& sse_event)> callback,
        const std::string& request_id = "",
        int max_tokens = 512
    );
    
    // ═══════════════════════════════════════════════════════════
    // Output formatting (for MCP, AQL, JSON responses)
    // ═══════════════════════════════════════════════════════════
    
    json generateAsMCP(const std::string& prompt, int max_tokens = 512);
    
    json generateAsJsonMarkdown(const std::string& prompt, int max_tokens = 512);
    
    /**
     * @brief Generate Full.
     * @param[in] request Input parameter.
     * @return Return value.
     */
    InferenceResponse generateFull(const InferenceRequest& request);
    
    /**
     * @brief ═══════════════════════════════════════════════════════════ Utility methods ═══════════════════════════════════════════════════════════
     * @return True when the operation succeeds.
     */
    
    bool isReady() const;
    
    /**
     * @brief Get Model Info.
     * @return Return value.
     */
    std::string getModelInfo() const;
    
    /**
     * @brief Get Stats.
     * @return Return value.
     */
    json getStats() const;

    /**
     * @brief Set Generate Full Fn.
     * @param[in] fn Input parameter.
     */
    void setGenerateFullFn(GenerateFullFn fn);

    /**
     * @brief Set Embed Fn.
     * @param[in] fn Input parameter.
     */
    void setEmbedFn(EmbedFn fn);
    
    /**
     * @brief Clear Cache.
     */
    void clearCache();
    
    /**
     * @brief ═══════════════════════════════════════════════════════════ Ethical Guidelines Support ═══════════════════════════════════════════════════════════
     * @return Pointer to the result.
     */
    
    EthicalGuidelinesManager* getEthicalGuidelines();
    
    /**
     * @brief Has Ethical Guidelines.
     * @return True when the operation succeeds.
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

class EmbeddedLLMManager {
public:
    /**
     * @brief Instance.
     * @return Return value.
     */
    static EmbeddedLLMManager& instance();
    
    /**
     * @brief Initialize.
     * @param[in] config Input parameter.
     */
    void initialize(const EmbeddedLLM::Config& config);
    
    /**
     * @brief Get.
     * @return Return value.
     */
    EmbeddedLLM& get();
    
    /**
     * @brief Is Initialized.
     * @return True when the operation succeeds.
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
