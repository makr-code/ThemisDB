#include "llm/llamacpp_inference_engine.h"
#include <stdexcept>
#include <cmath>
#include <algorithm>
#include <spdlog/spdlog.h>
#include <fstream>
#include <filesystem>

namespace themis {
namespace llm {

namespace fs = std::filesystem;

LlamaCppInferenceEngine::LlamaCppInferenceEngine(const Config& config)
    : config_(config), model_loaded_(false) {
    
    // Initialize PagedKVCache
    PagedKVCache::Config kv_config;
    kv_config.block_size = config.block_size;
    kv_config.num_blocks = config.num_blocks;
    kv_config.enable_prefix_caching = config.enable_prefix_caching;
    
    // TODO: Pass actual PagedBlockManager instance
    kv_cache_ = std::make_unique<PagedKVCache>(kv_config, nullptr);
    
    // Setup GPU offload if requested
    if (config_.n_gpu_layers > 0) {
        setupGPUOffload();
    }
    
    stats_ = {};
}

LlamaCppInferenceEngine::~LlamaCppInferenceEngine() {
    unloadModel();
}

bool LlamaCppInferenceEngine::loadModel(const std::string& model_path, 
                                         const std::string& model_name) {
    // Create GGUF loader
    gguf_loader_ = std::make_unique<GGUFLoader>();
    
    // Parse GGUF file
    if (!gguf_loader_->parseFile(model_path)) {
        return false;
    }
    
    current_model_name_ = model_name;
    
    // Memory-map all tensors
    const auto& metadata = gguf_loader_->getMetadata();
    for (const auto& tensor : metadata.tensors) {
        void* ptr = gguf_loader_->mmapTensor(tensor.name);
        if (ptr) {
            tensor_ptrs_[tensor.name] = ptr;
        }
    }
    
    model_loaded_ = true;
    return true;
}

bool LlamaCppInferenceEngine::loadModelFromThemisDB(const std::string& model_urn) {
    spdlog::info("Loading model from ThemisDB: {}", model_urn);
    
    // Check if model storage is configured
    if (!config_.model_storage) {
        spdlog::error("Model storage not configured");
        return false;
    }
    
    try {
        // 1. Query metadata from LLMModelStorage
        auto metadata_opt = config_.model_storage->loadModel(model_urn);
        if (!metadata_opt) {
            spdlog::error("Model {} not found in metadata store", model_urn);
            return false;
        }
        
        auto& metadata = *metadata_opt;
        spdlog::info("Found model metadata: name={}, architecture={}, format={}, size={}MB",
                     metadata.model_name, metadata.architecture, metadata.format,
                     metadata.size_bytes / (1024 * 1024));
        
        // 2. Get model file path or blob reference
        std::string model_path;
        
        // Check if model has a local file path
        if (!metadata.file_path.empty() && fs::exists(metadata.file_path)) {
            spdlog::info("Using local model file: {}", metadata.file_path);
            model_path = metadata.file_path;
        }
        // Otherwise, try to load from blob storage
        else {
            // Get blob storage manager from model storage config
            auto blob_manager = config_.model_storage->getConfig().blob_manager;
            if (!blob_manager) {
                spdlog::error("Blob storage manager not configured");
                return false;
            }
            
            // Get blob reference from metadata
            auto blob_ref_opt = getBlobReferenceFromMetadata(model_urn);
            if (!blob_ref_opt) {
                spdlog::error("Model {} has no blob reference and no local file", model_urn);
                return false;
            }
            
            auto& blob_ref = *blob_ref_opt;
            spdlog::info("Model stored in blob storage: type={}, uri={}, size={}MB",
                         static_cast<int>(blob_ref.type), blob_ref.uri,
                         blob_ref.size_bytes / (1024 * 1024));
            
            // 3. Stream model from blob store to temporary file
            temp_model_path_ = "/tmp/themis_model_" + model_urn + ".gguf";
            
            // Check if decryption is needed
            bool needs_decryption = metadata.custom_metadata.contains("encryption_enabled") &&
                                    metadata.custom_metadata["encryption_enabled"].get<bool>();
            
            if (needs_decryption) {
                spdlog::info("Model is encrypted, will decrypt during loading");
                // TODO: Implement decryption during streaming
                spdlog::warn("Decryption not yet fully implemented, attempting plain load");
            }
            
            // Stream model to temporary file
            if (!streamModelFromBlobStore(blob_ref, temp_model_path_, blob_manager)) {
                spdlog::error("Failed to stream model from blob store");
                return false;
            }
            
            model_path = temp_model_path_;
            spdlog::info("Model streamed to: {}", model_path);
        }
        
        // 4. Load model using existing loadModel function
        bool success = loadModel(model_path, metadata.model_name);
        
        if (success) {
            spdlog::info("Model {} loaded successfully from ThemisDB", model_urn);
            
            // 5. Update usage statistics
            config_.model_storage->updateUsageStats(model_urn, 0);
        } else {
            spdlog::error("Failed to load model {} from path: {}", model_urn, model_path);
        }
        
        return success;
    } catch (const std::exception& e) {
        spdlog::error("Exception loading model from ThemisDB: {}", e.what());
        return false;
    }
}

bool LlamaCppInferenceEngine::loadAdapterFromThemisDB(const std::string& adapter_id) {
    spdlog::info("Loading LoRa adapter from ThemisDB: {}", adapter_id);
    
    // Check if LoRa storage is configured
    if (!config_.lora_storage) {
        spdlog::error("LoRa storage not configured");
        return false;
    }
    
    try {
        // 1. Load adapter metadata
        auto metadata_opt = config_.lora_storage->loadMetadata(adapter_id);
        if (!metadata_opt) {
            spdlog::error("Adapter {} not found in metadata store", adapter_id);
            return false;
        }
        
        auto& metadata = *metadata_opt;
        spdlog::info("Found adapter metadata: base_model={}, version={}, rank={}",
                     metadata.base_model_id, metadata.version, metadata.hyperparameters.rank);
        
        // 2. Load adapter weights
        auto weights_opt = config_.lora_storage->loadAdapter(adapter_id);
        if (!weights_opt) {
            spdlog::error("Failed to load adapter weights for {}", adapter_id);
            return false;
        }
        
        auto& weights = *weights_opt;
        spdlog::info("Loaded adapter weights: format={}, size={}KB",
                     weights.format, weights.size_bytes / 1024);
        
        // 3. TODO: Apply adapter to loaded model
        // This would require integration with llama.cpp's LoRa support
        spdlog::warn("LoRa adapter application not yet fully implemented");
        spdlog::info("Adapter {} loaded successfully (weights available)", adapter_id);
        
        return true;
    } catch (const std::exception& e) {
        spdlog::error("Exception loading adapter from ThemisDB: {}", e.what());
        return false;
    }
}

void LlamaCppInferenceEngine::unloadModel() {
    if (gguf_loader_) {
        // Unmap all tensors
        for (auto& [name, ptr] : tensor_ptrs_) {
            gguf_loader_->unmapTensor(ptr);
        }
        tensor_ptrs_.clear();
    }
    
    // Clean up temporary model file if it exists
    if (!temp_model_path_.empty() && fs::exists(temp_model_path_)) {
        try {
            fs::remove(temp_model_path_);
            spdlog::info("Cleaned up temporary model file: {}", temp_model_path_);
        } catch (const std::exception& e) {
            spdlog::warn("Failed to clean up temporary model file: {}", e.what());
        }
        temp_model_path_.clear();
    }
    
    gguf_loader_.reset();
    current_model_name_.clear();
    model_loaded_ = false;
}

InferenceResponse LlamaCppInferenceEngine::infer(const InferenceRequest& request) {
    if (!model_loaded_) {
        throw std::runtime_error("No model loaded");
    }
    
    InferenceResponse response;
    response.request_id = request.request_id;
    response.model_id = request.model_id;
    response.metadata["request_id"] = !request.request_id.empty() ? request.request_id : request.metadata.value("request_id", "");
    
    // Real inference using GGUF loader and model tensors
    // In a real implementation, this would:
    // 1. Tokenize prompt using loaded model
    // 2. Generate embeddings
    // 3. Process through transformer layers with PagedAttention KV cache
    // 4. Generate output tokens
    // 5. Detokenize
    
    // For now, use simplified implementation with placeholder
    // This will be replaced with actual llama.cpp inference when model loading is complete
    response.text = "[Generated response from " + current_model_name_ + 
                    " for: " + request.prompt + "]";
    response.tokens_generated = 50;
    response.inference_time_ms = 150.0f;
    response.latency_ms = static_cast<int64_t>(response.inference_time_ms);
    response.tokens_per_second = response.tokens_generated / (response.inference_time_ms / 1000.0f);
    
    // Update stats
    stats_.total_tokens_processed += response.tokens_generated;
    stats_.avg_latency_ms = (stats_.avg_latency_ms + response.inference_time_ms) / 2.0;
    
    return response;
}

std::string LlamaCppInferenceEngine::getModelInfo() const {
    if (!model_loaded_) {
        return "No model loaded";
    }
    
    const auto& metadata = gguf_loader_->getMetadata();
    return "Model: " + current_model_name_ + 
           ", Architecture: " + metadata.architecture +
           ", Version: " + metadata.version +
           ", Tensors: " + std::to_string(metadata.tensors.size());
}

LlamaCppInferenceEngine::Stats LlamaCppInferenceEngine::getStats() const {
    return stats_;
}

std::vector<float> LlamaCppInferenceEngine::computeAttention(
    const std::vector<float>& q,
    const std::vector<float>& k,
    const std::vector<float>& v,
    int sequence_id) {
    
    // Simplified attention computation
    // In real implementation:
    // 1. Retrieve KV cache from PagedKVCache
    // 2. Compute attention scores
    // 3. Apply softmax
    // 4. Compute weighted values
    // 5. Store new KV in cache
    
    std::vector<float> output(q.size());
    // Stub: just return input
    output = q;
    
    return output;
}

std::vector<float> LlamaCppInferenceEngine::computeFFN(
    const std::vector<float>& input,
    int layer_id) {
    
    // Simplified FFN computation
    // In real implementation:
    // 1. Gate projection (SwiGLU)
    // 2. Up projection
    // 3. Activation
    // 4. Down projection
    
    std::vector<float> output = input;
    return output;
}

void LlamaCppInferenceEngine::setupGPUOffload() {
    // TODO: Setup GPU backend based on config_.gpu_backend
    // - CUDA: cuBLAS, cuDNN
    // - Metal: Metal Performance Shaders
    // - Vulkan: Kompute
    // - HIP: hipBLAS
    
    // For now, stub
}

bool LlamaCppInferenceEngine::streamModelFromBlobStore(
    const storage::BlobRef& blob_ref,
    const std::string& output_path,
    std::shared_ptr<storage::BlobStorageManager> blob_manager
) {
    try {
        spdlog::info("Streaming model from blob store to: {}", output_path);
        
        // Retrieve blob from storage
        auto blob_data = blob_manager->get(blob_ref);
        if (!blob_data) {
            spdlog::error("Failed to retrieve blob: {}", blob_ref.id);
            return false;
        }
        
        spdlog::info("Retrieved blob data: {} bytes", blob_data->size());
        
        // Write to output file
        std::ofstream output_file(output_path, std::ios::binary);
        if (!output_file) {
            spdlog::error("Failed to open output file: {}", output_path);
            return false;
        }
        
        output_file.write(reinterpret_cast<const char*>(blob_data->data()), blob_data->size());
        output_file.close();
        
        if (!output_file.good()) {
            spdlog::error("Error writing to output file: {}", output_path);
            return false;
        }
        
        spdlog::info("Model streamed successfully: {} bytes written", blob_data->size());
        
        // Verify file size
        auto file_size = fs::file_size(output_path);
        if (file_size != blob_data->size()) {
            spdlog::error("File size mismatch: expected {}, got {}", blob_data->size(), file_size);
            return false;
        }
        
        // TODO: Verify checksum if available
        if (!blob_ref.hash_sha256.empty()) {
            spdlog::info("TODO: Verify SHA256 checksum: {}", blob_ref.hash_sha256);
        }
        
        return true;
    } catch (const std::exception& e) {
        spdlog::error("Exception streaming model from blob store: {}", e.what());
        return false;
    }
}

std::optional<storage::BlobRef> LlamaCppInferenceEngine::getBlobReferenceFromMetadata(
    const std::string& model_id
) {
    if (!config_.model_storage) {
        return std::nullopt;
    }
    
    // This is a helper to get blob reference from the storage impl
    // We need to access the storage implementation to get the blob ref
    // For now, we'll reconstruct it from the metadata
    
    auto metadata_opt = config_.model_storage->loadModel(model_id);
    if (!metadata_opt) {
        return std::nullopt;
    }
    
    auto& metadata = *metadata_opt;
    
    // Check custom metadata for blob reference
    if (metadata.custom_metadata.contains("blob_ref_uri")) {
        storage::BlobRef ref;
        ref.id = metadata.custom_metadata.value("blob_ref_id", model_id);
        ref.uri = metadata.custom_metadata["blob_ref_uri"];
        ref.type = static_cast<storage::BlobStorageType>(
            metadata.custom_metadata.value("blob_ref_type", 0)
        );
        ref.hash_sha256 = metadata.custom_metadata.value("blob_ref_hash", "");
        ref.size_bytes = metadata.custom_metadata.value("blob_ref_size", 0);
        ref.compressed = metadata.custom_metadata.value("blob_ref_compressed", false);
        if (ref.compressed) {
            ref.compression_type = metadata.custom_metadata.value("blob_ref_compression", "");
        }
        
        return ref;
    }
    
    return std::nullopt;
}

} // namespace llm
} // namespace themis
