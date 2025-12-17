#pragma once

#include "llm/llm_plugin_interface.h"
#include <mutex>
#include <unordered_map>
#include <memory>

/**
 * @file llamacpp_plugin.h
 * @brief Reference implementation of LLM plugin using llama.cpp backend
 * 
 * This plugin demonstrates:
 * - Loading GGUF models (quantized llama.cpp format)
 * - LoRA adapter management
 * - GPU acceleration (CUDA/Metal/Vulkan)
 * - Zero-copy integration with ThemisDB vector storage
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
 * Note: Actual llama.cpp integration will be done in v1.3.0.
 * This provides the plugin structure and API design.
 */
class LlamaCppPlugin : public ILLMPlugin {
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
        
        // LoRA settings
        int lora_cache_slots = 8;     // Max cached LoRA adapters
        size_t lora_cache_vram_mb = 512;
    };
    
    explicit LlamaCppPlugin(const Config& config = Config{});
    ~LlamaCppPlugin() override;
    
    // Prevent copying
    LlamaCppPlugin(const LlamaCppPlugin&) = delete;
    LlamaCppPlugin& operator=(const LlamaCppPlugin&) = delete;
    
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
    
private:
    Config config_;
    
    // Model state (opaque pointers to llama.cpp structures)
    // In actual implementation, these would be llama_model*, llama_context*, etc.
    struct ModelState {
        void* model_ptr = nullptr;      // llama_model*
        void* context_ptr = nullptr;    // llama_context*
        ModelInfo info;
        bool loaded = false;
    };
    ModelState model_state_;
    
    // LoRA cache
    struct LoRACacheEntry {
        LoRAInfo info;
        void* adapter_ptr = nullptr;  // llama_lora_adapter*
        size_t last_used_timestamp = 0;
    };
    std::unordered_map<std::string, LoRACacheEntry> lora_cache_;
    
    // Statistics
    struct Stats {
        size_t total_inferences = 0;
        size_t total_tokens_generated = 0;
        double total_inference_time_ms = 0.0;
        size_t cache_hits = 0;
        size_t cache_misses = 0;
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
    
    void evictLRULoRA();
};

} // namespace llm
} // namespace themis
