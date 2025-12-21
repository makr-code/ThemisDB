#pragma once

#include "llm/i_llm_plugin.h"
#include "llm/gguf_loader.h"
#include "llm/paged_kv_cache.h"
#include "llm/lazy_model_loader.h"
#include <memory>
#include <string>
#include <vector>

namespace themis {
namespace llm {

// llama.cpp Inference Engine - integrates GGUF loading with PagedAttention
class LlamaCppInferenceEngine {
public:
    struct Config {
        int n_ctx = 4096;           // Context length
        int n_gpu_layers = 0;       // GPU offload layers (0 = CPU only)
        int n_threads = 4;          // CPU threads
        std::string gpu_backend;    // "cuda", "metal", "vulkan", "hip", "cpu"
        bool use_mmap = true;       // Use memory-mapped loading
        bool use_mlock = false;     // Lock model in RAM
        
        // PagedAttention config
        int block_size = 16;
        int num_blocks = 4096;
        bool enable_prefix_caching = true;
    };
    
    LlamaCppInferenceEngine(const Config& config);
    ~LlamaCppInferenceEngine();
    
    // Load model from GGUF file
    bool loadModel(const std::string& model_path, const std::string& model_name);
    
    // Load model from ThemisDB (URN)
    bool loadModelFromThemisDB(const std::string& model_urn);
    
    // Unload current model
    void unloadModel();
    
    // Run inference with PagedAttention
    InferenceResponse infer(const InferenceRequest& request);
    
    // Get model info
    std::string getModelInfo() const;
    
    // Get performance stats
    struct Stats {
        size_t total_tokens_processed;
        size_t cache_hits;
        size_t cache_misses;
        double avg_latency_ms;
        size_t vram_used_mb;
    };
    Stats getStats() const;
    
private:
    Config config_;
    std::unique_ptr<GGUFLoader> gguf_loader_;
    std::unique_ptr<PagedKVCache> kv_cache_;
    std::string current_model_name_;
    bool model_loaded_;
    
    // Model tensors (memory-mapped)
    std::unordered_map<std::string, void*> tensor_ptrs_;
    
    // Statistics
    Stats stats_;
    
    // Internal inference helpers
    std::vector<float> computeAttention(
        const std::vector<float>& q,
        const std::vector<float>& k, 
        const std::vector<float>& v,
        int sequence_id
    );
    
    std::vector<float> computeFFN(
        const std::vector<float>& input,
        int layer_id
    );
    
    void setupGPUOffload();
};

} // namespace llm
} // namespace themis
