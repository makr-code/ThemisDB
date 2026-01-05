#pragma once

#include "llm/llm_plugin_interface.h"
#include "llm/model_loader.h"
#include "llm/multi_lora_manager.h"
#include <mutex>
#include <unordered_map>
#include <memory>

// Forward declarations for llama.cpp types
struct llama_model;
struct llama_context;
typedef int32_t llama_token;

namespace themis {
namespace llm {

/**
 * @brief Chat role enumeration for type-safe message roles
 */
enum class ChatRole {
    System,     // System message (instructions, persona)
    User,       // User message (query, input)
    Assistant   // Assistant message (response, output)
};

/**
 * @brief Chat message structure for multi-turn conversations
 */
struct ChatMessage {
    std::string role;      // "system", "user", "assistant" (kept as string for compatibility)
    std::string content;   // Message content
    
    // Helper constructor for enum-based creation
    ChatMessage(ChatRole r, const std::string& c) 
        : content(c) {
        switch (r) {
            case ChatRole::System: role = "system"; break;
            case ChatRole::User: role = "user"; break;
            case ChatRole::Assistant: role = "assistant"; break;
        }
    }
    
    // Default constructor for string-based creation (backwards compatibility)
    ChatMessage(const std::string& r, const std::string& c)
        : role(r), content(c) {}
    
    ChatMessage() = default;
};

/**
 * @brief Chat template format options
 */
enum class ChatFormat {
    ChatML,      // ChatML format: <|im_start|>role\ncontent<|im_end|>
    Llama2,      // Llama-2 format: [INST] content [/INST]
    Vicuna,      // Vicuna format: USER: content\nASSISTANT:
    Alpaca       // Alpaca format: ### Instruction:\ncontent\n### Response:
};

} // namespace llm
} // namespace themis

/**
 * @file llama_wrapper.h
 * @brief Reference implementation of LLM plugin using llama.cpp backend
 * 
 * This plugin demonstrates:
 * - Loading GGUF models (quantized llama.cpp format)
 * - LoRA adapter management
 * - GPU acceleration (CUDA/Metal/Vulkan)
 * - Zero-copy integration with ThemisDB vector storage
 * - Ollama-style lazy model loading
 * - vLLM-style multi-LoRA management
 * 
 * Based on AI_ECOSYSTEM_SHARDING_ARCHITECTURE.md v1.3.0 design.
 */

namespace themis {
namespace llm {

/**
 * @brief llama.cpp plugin implementation
 * 
 * This is a reference implementation showing how to create
 * an LLM plugin for ThemisDB. It wraps llama.cpp functionality
 * into the ILLMPlugin interface.
 * 
 * Uses LazyModelLoader (Ollama-style) and MultiLoRAManager (vLLM-style)
 * for efficient resource management.
 * 
 * Note: Actual llama.cpp integration will be done in v1.3.0.
 * This provides the plugin structure and API design.
 */
class LlamaWrapper : public ILLMPlugin {
public:
    /**
     * @brief Configuration for llama.cpp backend
     */
    struct Config {
        // GPU settings
        int n_gpu_layers = 32;        // Number of layers to offload to GPU
        bool use_mmap = true;         // Memory-map model file
        bool use_mlock = false;       // Lock memory (prevent swapping)
        
        // Context settings
        int n_ctx = 4096;             // Context length
        int n_batch = 512;            // Batch size for prompt processing
        int n_threads = 8;            // CPU threads (for layers not on GPU)
        
        // Memory management
        size_t max_vram_mb = 14336;   // Max VRAM to use (14GB default)
        bool unified_memory = false;  // Use CUDA unified memory
        
        // Performance optimizations (llama.cpp features)
        bool use_flash_attn = true;   // Flash Attention for 15-25% speedup
        
        // Lazy loading (Ollama-style)
        LazyModelLoader::Config lazy_loader_config;
        
        // Multi-LoRA (vLLM-style)
        MultiLoRAManager::Config multi_lora_config;
    };
    
    explicit LlamaWrapper(const Config& config);
    ~LlamaWrapper() override;
    
    // Prevent copying
    LlamaWrapper(const LlamaWrapper&) = delete;
    LlamaWrapper& operator=(const LlamaWrapper&) = delete;
    
    // ═══════════════════════════════════════════════════════════
    // Model Management
    // ═══════════════════════════════════════════════════════════
    
    bool loadModel(
        const std::string& model_path,
        const json& config = {}
    ) override;
    
    void unloadModel() override;
    
    std::optional<ModelInfo> getModelInfo() const override;
    
    bool isModelLoaded() const override;
    
    // ═══════════════════════════════════════════════════════════
    // LoRA Management
    // ═══════════════════════════════════════════════════════════
    
    bool loadLoRA(
        const std::string& lora_id,
        const std::string& lora_path,
        float scale = 1.0f
    ) override;
    
    bool unloadLoRA(const std::string& lora_id) override;
    
    std::vector<LoRAInfo> listLoRAs() const override;
    
    // ═══════════════════════════════════════════════════════════
    // Inference
    // ═══════════════════════════════════════════════════════════
    
    InferenceResponse generate(const InferenceRequest& request) override;
    
    InferenceResponse generateRAG(
        const RAGContext& rag_context,
        const InferenceRequest& request
    ) override;
    
    std::vector<float> embed(const std::string& text) override;
    
    // ═══════════════════════════════════════════════════════════
    // Capabilities
    // ═══════════════════════════════════════════════════════════
    
    LLMCapabilities getCapabilities() const override;
    
    json getMemoryStats() const override;
    
    json getPerformanceStats() const override;
    
    // ═══════════════════════════════════════════════════════════
    // Distributed Features
    // ═══════════════════════════════════════════════════════════
    
    std::vector<uint8_t> exportLoRA(const std::string& lora_id) override;
    
    bool importLoRA(
        const std::string& lora_id,
        const std::vector<uint8_t>& data
    ) override;

    // Non-ILLMPlugin convenience method for tests
    std::string getName() const { return "llamacpp"; }
    
private:
    Config config_;
    
    // Ollama-style lazy model loader
    std::unique_ptr<LazyModelLoader> model_loader_;
    
    // vLLM-style multi-LoRA manager
    std::unique_ptr<MultiLoRAManager> lora_manager_;
    
    // Current active model
    std::string current_model_id_;
    std::string current_model_path_;
    
    // Statistics
    struct Stats {
        size_t total_inferences = 0;
        size_t total_tokens_generated = 0;
        double total_inference_time_ms = 0.0;
    };
    Stats stats_;
    
    // Thread safety
    mutable std::mutex mutex_;
    
    // Helper methods
    std::string formatPromptForRAG(
        const RAGContext& rag_context,
        const InferenceRequest& request
    );
    
    void updateStatistics(const InferenceResponse& response);
    
    std::string extractModelId(const std::string& model_path);
    
    // Internal llama.cpp helper functions
    std::vector<llama_token> tokenizeInternal(
        llama_model* model,
        const std::string& text,
        bool add_bos
    );
    
    std::string detokenizeInternal(
        llama_context* ctx,
        const std::vector<llama_token>& tokens
    );
    
    llama_token sampleTokenInternal(
        llama_context* ctx,
        llama_model* model,
        float* logits,
        int32_t n_vocab,
        float temperature,
        float top_p
    );
    
    // Chat formatting helpers
    std::string formatChatMessages(
        const std::vector<ChatMessage>& messages,
        ChatFormat format = ChatFormat::ChatML
    );
    
    std::string formatChatML(const std::vector<ChatMessage>& messages);
    std::string formatLlama2(const std::vector<ChatMessage>& messages);
    std::string formatVicuna(const std::vector<ChatMessage>& messages);
    std::string formatAlpaca(const std::vector<ChatMessage>& messages);
    
public:
    // ═══════════════════════════════════════════════════════════
    // Output Formatting Helpers (MCP, SSE, AQL)
    // ═══════════════════════════════════════════════════════════
    
    /**
     * @brief Format response as JSON for MCP protocol
     * Converts InferenceResponse to MCP-compatible JSON format
     */
    static json formatAsMCPResponse(const InferenceResponse& response);
    
    /**
     * @brief Format response as SSE (Server-Sent Events) data
     * Returns SSE-formatted string: "data: {...}\n\n"
     */
    static std::string formatAsSSE(const InferenceResponse& response);
    
    /**
     * @brief Format response as JSON with embedded markdown
     * Useful for rich text responses with code blocks
     */
    static json formatAsJsonMarkdown(const InferenceResponse& response);
    
    /**
     * @brief Format streaming token as SSE event
     * For real-time token streaming via Server-Sent Events
     */
    static std::string formatStreamTokenAsSSE(const std::string& token, const std::string& request_id = "");
};

} // namespace llm
} // namespace themis
