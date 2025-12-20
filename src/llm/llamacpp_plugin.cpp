#include "llm/llamacpp_plugin.h"
#include <spdlog/spdlog.h>
#include <chrono>
#include <sstream>
#include <filesystem>
#include <llama.h>

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
    auto* cached = model_loader_->getOrLoadModel(
        current_model_id_,
        current_model_path_
    );
    if (!cached) {
        throw std::runtime_error("Model failed to load");
    }

    auto* lmodel = reinterpret_cast<llama_model*>(cached->model_handle);
    auto* lctx = reinterpret_cast<llama_context*>(cached->context_handle);
    if (!lmodel || !lctx) {
        throw std::runtime_error("Invalid llama.cpp handles");
    }

    // Tokenize prompt
    std::vector<llama_token> prompt_tokens;
    prompt_tokens.resize(4096);
    int n_prompt = llama_tokenize(
        lmodel,
        request.prompt.c_str(),
        request.prompt.size(),
        prompt_tokens.data(),
        prompt_tokens.size(),
        true,
        false
    );
    if (n_prompt < 0) {
        throw std::runtime_error("Prompt tokenization failed");
    }
    prompt_tokens.resize(static_cast<size_t>(n_prompt));

    // Evaluate prompt
    int n_threads = std::max(1, config_.n_threads);
    if (llama_eval(lctx, prompt_tokens.data(), n_prompt, 0, n_threads) != 0) {
        throw std::runtime_error("llama_eval failed");
    }

    // Sampling parameters
    float temperature = request.temperature;
    float top_p = request.top_p;
    int32_t top_k = request.top_k;
    float repeat_penalty = request.repeat_penalty;

    std::vector<llama_token> generated;
    generated.reserve(request.max_tokens);

    // Simple generation loop
    for (int i = 0; i < request.max_tokens; ++i) {
        // Get logits for last token
        const float* logits = llama_get_logits(lctx);
        if (!logits) {
            break;
        }

        // Sample next token
        llama_token token = 0;
        // Basic greedy/top-k/top-p sampling using helpers
        std::vector<llama_token_data> candidates;
        candidates.reserve(llama_n_vocab(lmodel));
        const int n_vocab = llama_n_vocab(lmodel);
        for (int t = 0; t < n_vocab; ++t) {
            candidates.emplace_back(llama_token_data{(llama_token)t, logits[t], 0.0f});
        }
        llama_token_data_array arr = { candidates.data(), candidates.size(), false };

        if (repeat_penalty != 1.0f && !generated.empty()) {
            llama_sample_repetition_penalty(lctx, &arr, generated.data(), generated.size(), repeat_penalty);
        }
        if (top_k > 0) {
            llama_sample_top_k(lctx, &arr, top_k, 1);
        }
        if (top_p < 1.0f) {
            llama_sample_top_p(lctx, &arr, top_p, 1);
        }
        if (temperature != 0.0f && temperature != 1.0f) {
            llama_sample_temperature(lctx, &arr, temperature);
        }
        token = llama_sample_token(lctx, &arr);

        // Stop conditions
        if (token == llama_token_eos(lmodel)) {
            break;
        }

        generated.push_back(token);

        // Decode token (feed back)
        if (llama_eval(lctx, &token, 1, prompt_tokens.size() + generated.size() - 1, n_threads) != 0) {
            break;
        }
    }

    // Detokenize to string
    std::string output;
    output.reserve(generated.size() * 4);
    for (auto tk : generated) {
        const char* piece = llama_token_to_piece(lmodel, tk);
        if (piece) {
            output.append(piece);
        }
    }

    InferenceResponse response;
    response.text = output;
    response.model_used = current_model_id_;
    if (request.lora_adapter_id) {
        response.lora_used = *request.lora_adapter_id;
    }
    response.tokens_prompt = n_prompt;
    response.tokens_generated = static_cast<int>(generated.size());

    auto end_time = std::chrono::high_resolution_clock::now();
    response.inference_time_ms = std::chrono::duration<float, std::milli>(end_time - start_time).count();
    response.tokens_per_second = response.tokens_generated / (response.inference_time_ms / 1000.0f);

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
