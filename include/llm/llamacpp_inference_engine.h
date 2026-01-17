#pragma once

#include "llm/i_llm_plugin.h"
#include "llm/gguf_loader.h"
#include "llm/paged_kv_cache.h"
#include "llm/lazy_model_loader.h"
#include "llm/llm_model_storage.h"
#include "llm/lora_framework/lora_storage_service.h"
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
        
        // ThemisDB integration (optional)
        std::shared_ptr<LLMModelStorage> model_storage;        // Model storage service
        std::shared_ptr<lora::LoRAStorageService> lora_storage; // LoRa storage service
    };
    
    LlamaCppInferenceEngine(const Config& config);
    ~LlamaCppInferenceEngine();
    
    // Load model from GGUF file
    bool loadModel(const std::string& model_path, const std::string& model_name);
    
    // Load model from ThemisDB (URN)
    bool loadModelFromThemisDB(const std::string& model_urn);
    
    // Load LoRa adapter from ThemisDB
    bool loadAdapterFromThemisDB(const std::string& adapter_id);
    
    // LoRa adapter management
    bool loadAndApplyLoRAAdapter(const std::string& adapter_id, float scale = 1.0f);
    bool applyMultipleAdapters(const std::vector<std::pair<std::string, float>>& adapters);
    bool removeAdapter(const std::string& adapter_id);
    void clearAllAdapters();
    bool isAdapterActive(const std::string& adapter_id) const;
    std::vector<std::string> getActiveAdapters() const;
    bool validateAdapterApplication(const std::string& adapter_id);
    
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
    
    // LoRa adapter tracking
    std::unordered_map<std::string, int> active_adapters_;  // adapter_id -> adapter_handle
    std::unordered_map<std::string, float> adapter_scales_;  // adapter_id -> scale
    std::unordered_map<std::string, std::string> adapter_temp_files_;  // adapter_id -> temp file path
    int next_adapter_handle_id_;  // Counter for unique adapter handles
    void* model_handle_;  // llama_model* handle
    void* context_handle_;  // llama_context* handle
    
    // Statistics
    Stats stats_;
    
    // Temporary file path for streamed models
    std::string temp_model_path_;
    
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
    
    // Helper function for streaming models from blob store
    bool streamModelFromBlobStore(
        const storage::BlobRef& blob_ref,
        const std::string& output_path,
        std::shared_ptr<storage::BlobStorageManager> blob_manager
    );
    
    // Helper function to decrypt model file
    bool decryptModelFile(
        const std::string& encrypted_path,
        const std::string& output_path,
        const LLMModelMetadata& metadata
    );
    
    // Helper to get blob reference from model metadata
    std::optional<storage::BlobRef> getBlobReferenceFromMetadata(
        const std::string& model_id
    );
    
    // LoRa adapter helper methods
    std::string convertAdapterToLlamaCppFormat(const lora::AdapterWeights& weights);
    std::string getTempAdapterPath(const std::string& adapter_id);
    void cleanupTempAdapterFiles();
    bool saveAdapterToTempFile(const std::string& temp_path, const lora::AdapterWeights& weights);
};

} // namespace llm
} // namespace themis
