#include "llm/llamacpp_plugin.h"
#include <spdlog/spdlog.h>
#include <chrono>
#include <sstream>
#include <filesystem>

namespace themis {
namespace llm {

LlamaCppPlugin::LlamaCppPlugin(const Config& config)
    : config_(config) {
    
    // Initialize lazy model loader (Ollama-style)
    model_loader_ = std::make_unique<LazyModelLoader>(config_.lazy_loader_config);
    
    // Initialize multi-LoRA manager (vLLM-style)
    lora_manager_ = std::make_unique<MultiLoRAManager>(config_.multi_lora_config);
    
    spdlog::info("LlamaCppPlugin initialized:");
    spdlog::info("  GPU layers: {}, Context: {}", 
                 config_.n_gpu_layers, config_.n_ctx);
    spdlog::info("  Lazy loading: enabled (Ollama-style)");
    spdlog::info("  Multi-LoRA: enabled (vLLM-style)");
}

LlamaCppPlugin::~LlamaCppPlugin() {
    unloadModel();
}

// ═══════════════════════════════════════════════════════════
// Model Management
// ═══════════════════════════════════════════════════════════

bool LlamaCppPlugin::loadModel(
    const std::string& model_path,
    const json& config
) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    spdlog::info("Loading model (lazy): {}", model_path);
    
    // Extract model ID from path
    current_model_id_ = extractModelId(model_path);
    current_model_path_ = model_path;
    
    // Use lazy model loader (Ollama-style)
    // Model loads on-demand during first inference
    json load_config = config;
    if (!config.contains("n_gpu_layers")) {
        load_config["n_gpu_layers"] = config_.n_gpu_layers;
    }
    if (!config.contains("n_ctx")) {
        load_config["n_ctx"] = config_.n_ctx;
    }
    
    // Trigger lazy load (or get from cache)
    auto* model = model_loader_->getOrLoadModel(
        current_model_id_,
        model_path,
        load_config
    );
    
    if (!model) {
        spdlog::error("Failed to load model: {}", model_path);
        return false;
    }
    
    spdlog::info("Model ready: {} (lazy loaded)", current_model_id_);
    return true;
}

void LlamaCppPlugin::unloadModel() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (current_model_id_.empty()) {
        return;
    }
    
    spdlog::info("Unloading model: {}", current_model_id_);
    
    // Unload via lazy loader
    model_loader_->unloadModel(current_model_id_, true);
    
    current_model_id_.clear();
    current_model_path_.clear();
    
    spdlog::info("Model unloaded");
}

std::optional<ModelInfo> LlamaCppPlugin::getModelInfo() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (current_model_id_.empty()) {
        return std::nullopt;
    }
    
    return model_loader_->getModelInfo(current_model_id_);
}

bool LlamaCppPlugin::isModelLoaded() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return !current_model_id_.empty() && 
           model_loader_->isModelLoaded(current_model_id_);
}

// ═══════════════════════════════════════════════════════════
// LoRA Management
// ═══════════════════════════════════════════════════════════

bool LlamaCppPlugin::loadLoRA(
    const std::string& lora_id,
    const std::string& lora_path,
    float scale
) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (current_model_id_.empty()) {
        spdlog::error("Cannot load LoRA: no model loaded");
        return false;
    }
    
    spdlog::info("Loading LoRA (lazy): {}", lora_id);
    
    // Use multi-LoRA manager (vLLM-style)
    return lora_manager_->loadLoRA(lora_id, lora_path, current_model_id_, scale);
}

bool LlamaCppPlugin::unloadLoRA(const std::string& lora_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    return lora_manager_->unloadLoRA(lora_id);
}

std::vector<LoRAInfo> LlamaCppPlugin::listLoRAs() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return lora_manager_->listLoRAs();
}

// ═══════════════════════════════════════════════════════════
// Inference
// ═══════════════════════════════════════════════════════════

InferenceResponse LlamaCppPlugin::generate(const InferenceRequest& request) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (current_model_id_.empty()) {
        throw std::runtime_error("No model loaded");
    }
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    spdlog::debug("Generating response for prompt: {} (max_tokens={})",
                  request.prompt.substr(0, 50), request.max_tokens);
    
    // Ensure model is loaded (lazy loading trigger)
    auto* model = model_loader_->getOrLoadModel(
        current_model_id_,
        current_model_path_
    );
    
    if (!model) {
        throw std::runtime_error("Model failed to load");
    }
    
    // Apply LoRA if specified (vLLM-style)
    if (request.lora_adapter_id) {
        auto* lora = lora_manager_->getLoRA(*request.lora_adapter_id);
        if (lora) {
            lora_manager_->applyLoRA(*request.lora_adapter_id, model->context_handle);
        } else {
            spdlog::warn("LoRA not found: {}", *request.lora_adapter_id);
        }
    }
    
    // TODO: Actual inference implementation in v1.3.0
    // This is a stub showing the structure
    
    InferenceResponse response;
    response.text = "This is a placeholder response. Actual llama.cpp integration "
                    "will be implemented in v1.3.0. Model and LoRA are managed via "
                    "LazyModelLoader (Ollama-style) and MultiLoRAManager (vLLM-style).";
    response.model_used = current_model_id_;
    
    if (request.lora_adapter_id) {
        response.lora_used = *request.lora_adapter_id;
    }
    
    // Simulate token generation
    response.tokens_prompt = static_cast<int>(request.prompt.size() / 4);
    response.tokens_generated = 20;
    
    auto end_time = std::chrono::high_resolution_clock::now();
    response.inference_time_ms = std::chrono::duration<float, std::milli>(
        end_time - start_time).count();
    response.tokens_per_second = response.tokens_generated / 
                                 (response.inference_time_ms / 1000.0f);
    
    updateStatistics(response);
    
    return response;
}

InferenceResponse LlamaCppPlugin::generateRAG(
    const RAGContext& rag_context,
    const InferenceRequest& request
) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (current_model_id_.empty()) {
        throw std::runtime_error("No model loaded");
    }
    
    spdlog::debug("Generating RAG response with {} documents",
                  rag_context.documents.size());
    
    // Format prompt with RAG context
    std::string formatted_prompt = formatPromptForRAG(rag_context, request);
    
    // Create modified request with formatted prompt
    InferenceRequest rag_request = request;
    rag_request.prompt = formatted_prompt;
    
    // Unlock for actual generation (generate will lock again)
    mutex_.unlock();
    auto response = generate(rag_request);
    mutex_.lock();
    
    // Add RAG metadata to response
    response.metadata["rag_enabled"] = true;
    response.metadata["num_documents"] = rag_context.documents.size();
    
    return response;
}

std::vector<float> LlamaCppPlugin::embed(const std::string& text) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (current_model_id_.empty()) {
        throw std::runtime_error("No model loaded");
    }
    
    // TODO: Actual embedding implementation
    // For now, return a dummy vector
    
    spdlog::debug("Generating embedding for text: {}", text.substr(0, 50));
    
    // Return 768-dimensional dummy vector
    return std::vector<float>(768, 0.0f);
}

// ═══════════════════════════════════════════════════════════
// Capabilities
// ═══════════════════════════════════════════════════════════

LLMCapabilities LlamaCppPlugin::getCapabilities() const {
    LLMCapabilities caps;
    
    caps.supports_instruct = true;
    caps.supports_chat = true;
    caps.supports_completion = true;
    
    caps.supports_lora = true;
    caps.supports_quantization = true;
    caps.supports_streaming = true;
    caps.supports_batching = true;
    
    caps.gpu_accelerated = (config_.n_gpu_layers > 0);
    caps.supports_cuda = true;
    caps.supports_metal = true;
    caps.supports_vulkan = true;
    
    caps.supports_zero_copy = config_.unified_memory;
    
    return caps;
}

json LlamaCppPlugin::getMemoryStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    json stats;
    
    // Get stats from lazy model loader
    stats["model_loader"] = model_loader_->getMemoryStats();
    stats["model_loader_cache"] = model_loader_->getCacheStats();
    
    // Get stats from multi-LoRA manager
    stats["lora_manager"] = lora_manager_->getMemoryStats();
    stats["lora_manager_cache"] = lora_manager_->getCacheStats();
    
    // Combined totals
    auto model_stats = model_loader_->getMemoryStats();
    auto lora_stats = lora_manager_->getMemoryStats();
    
    stats["total_vram_mb"] = model_stats["vram_used_mb"].get<size_t>() + 
                             lora_stats["vram_used_mb"].get<size_t>();
    stats["max_vram_mb"] = config_.max_vram_mb;
    
    return stats;
}

json LlamaCppPlugin::getPerformanceStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    json stats;
    stats["total_inferences"] = stats_.total_inferences;
    stats["total_tokens_generated"] = stats_.total_tokens_generated;
    
    if (stats_.total_inferences > 0) {
        stats["avg_inference_time_ms"] = 
            stats_.total_inference_time_ms / stats_.total_inferences;
        stats["avg_tokens_per_inference"] = 
            static_cast<double>(stats_.total_tokens_generated) / stats_.total_inferences;
    }
    
    // Include model loader and LoRA manager stats
    stats["model_loader_stats"] = model_loader_->getCacheStats();
    stats["lora_manager_stats"] = lora_manager_->getCacheStats();
    
    return stats;
}

// ═══════════════════════════════════════════════════════════
// Distributed Features
// ═══════════════════════════════════════════════════════════

std::vector<uint8_t> LlamaCppPlugin::exportLoRA(const std::string& lora_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    spdlog::info("Exporting LoRA for cross-shard transfer: {}", lora_id);
    
    // Delegate to multi-LoRA manager
    return lora_manager_->exportLoRA(lora_id);
}

bool LlamaCppPlugin::importLoRA(
    const std::string& lora_id,
    const std::vector<uint8_t>& data
) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (current_model_id_.empty()) {
        spdlog::error("Cannot import LoRA: no model loaded");
        return false;
    }
    
    spdlog::info("Importing LoRA from remote shard: {} ({} bytes)",
                 lora_id, data.size());
    
    // Delegate to multi-LoRA manager
    return lora_manager_->importLoRA(lora_id, data, current_model_id_);
}

// ═══════════════════════════════════════════════════════════
// Helper Methods
// ═══════════════════════════════════════════════════════════

std::string LlamaCppPlugin::formatPromptForRAG(
    const RAGContext& rag_context,
    const InferenceRequest& request
) {
    // Build RAG prompt using template
    std::ostringstream oss;
    
    // Add system prompt if provided
    if (request.system_prompt) {
        oss << *request.system_prompt << "\n\n";
    }
    
    // Add context documents
    oss << "Context:\n";
    for (size_t i = 0; i < rag_context.documents.size(); ++i) {
        const auto& doc = rag_context.documents[i];
        oss << "[Document " << (i + 1) << "]\n";
        oss << doc.content << "\n\n";
    }
    
    // Add user query
    oss << "Question: " << rag_context.query << "\n\n";
    oss << "Answer based on the context provided above:";
    
    return oss.str();
}

void LlamaCppPlugin::updateStatistics(const InferenceResponse& response) {
    stats_.total_inferences++;
    stats_.total_tokens_generated += response.tokens_generated;
    stats_.total_inference_time_ms += response.inference_time_ms;
}

std::string LlamaCppPlugin::extractModelId(const std::string& model_path) {
    // Extract filename without extension as model ID
    std::filesystem::path p(model_path);
    return p.stem().string();
}

} // namespace llm
} // namespace themis
