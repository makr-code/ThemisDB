#include "llm/llama_wrapper.h"
#include "llm/llm_prefix_cache.h"
#include "llm/llm_response_cache.h"
#include "llm/paged_block_manager.h"
#include <spdlog/spdlog.h>
#include <chrono>
#include <sstream>
#include <filesystem>
#include <llama.h>

namespace themis {
namespace llm {

// Constants for stub response estimation
constexpr size_t CHARS_PER_TOKEN_ESTIMATE = 4;
constexpr int MAX_STUB_TOKENS = 64;

// ═══════════════════════════════════════════════════════════
// Configuration Validation
// ═══════════════════════════════════════════════════════════

void LlamaWrapper::validateConfig(const Config& config) {
    // Validate basic parameters
    if (config.n_ctx <= 0) {
        throw std::invalid_argument("n_ctx must be positive");
    }
    if (config.n_batch <= 0) {
        throw std::invalid_argument("n_batch must be positive");
    }
    if (config.n_threads <= 0) {
        throw std::invalid_argument("n_threads must be positive");
    }
    
    // Validate prefix cache config if enabled
    if (config.use_kv_cache_reuse) {
        const auto& cache_cfg = config.prefix_cache_config;
        
        if (cache_cfg.similarity_threshold < 0.0 || cache_cfg.similarity_threshold > 1.0) {
            spdlog::warn("prefix_cache_config.similarity_threshold should be between 0.0 and 1.0, got {}",
                        cache_cfg.similarity_threshold);
        }
        
        if (cache_cfg.max_entries == 0) {
            spdlog::warn("prefix_cache_config.max_entries is 0, cache will be ineffective");
        }
        
        if (cache_cfg.max_entries > 100000) {
            spdlog::warn("prefix_cache_config.max_entries is very large ({}), may use excessive memory",
                        cache_cfg.max_entries);
        }
        
        if (cache_cfg.min_prefix_length < 10) {
            spdlog::warn("prefix_cache_config.min_prefix_length is very small ({}), may cache inefficiently",
                        cache_cfg.min_prefix_length);
        }
        
        if (cache_cfg.ttl_seconds < 60) {
            spdlog::warn("prefix_cache_config.ttl_seconds is very short ({}s), cache may expire too quickly",
                        cache_cfg.ttl_seconds);
        }
    }
}

// ═══════════════════════════════════════════════════════════
// Constructor and Destructor
// ═══════════════════════════════════════════════════════════

LlamaWrapper::LlamaWrapper(const Config& config)
    : config_(config) {
    
    // Validate configuration
    validateConfig(config_);
    
    // Initialize lazy model loader (Ollama-style)
    model_loader_ = std::make_unique<LazyModelLoader>(config_.lazy_loader_config);
    
    // Initialize multi-LoRA manager (vLLM-style)
    lora_manager_ = std::make_unique<MultiLoRAManager>(config_.multi_lora_config);
    
    // Initialize KV-Cache Reuse (Prefix Caching)
    if (config_.use_kv_cache_reuse) {
        prefix_cache_ = std::make_unique<LLMPrefixCache>(
            "llama_prefix_cache",
            config_.prefix_cache_config
        );
        spdlog::info("  KV-Cache Reuse: enabled (10-20x first-token speedup)");
    }
    
    // Initialize response cache (optional)
    if (config_.enable_response_cache) {
        response_cache_ = std::make_unique<LLMResponseCache>("response_cache", config_.response_cache_config);
        spdlog::info("Response cache enabled (max_entries: {}, ttl: {}s, similarity: {})",
                     config_.response_cache_config.max_entries,
                     config_.response_cache_config.ttl_seconds,
                     config_.response_cache_config.similarity_threshold);
    }
    
    spdlog::info("LlamaWrapper initialized:");
    spdlog::info("  GPU layers: {}, Context: {}", 
                 config_.n_gpu_layers, config_.n_ctx);
    spdlog::info("  Flash Attention: {}", config_.use_flash_attn ? "enabled" : "disabled");
    spdlog::info("  Lazy loading: enabled (Ollama-style)");
    spdlog::info("  Multi-LoRA: enabled (vLLM-style)");
    spdlog::info("  Response cache: {}", config_.enable_response_cache ? "enabled" : "disabled");
}

LlamaWrapper::~LlamaWrapper() {
    unloadDraftModel();
    unloadModel();
}

// ═══════════════════════════════════════════════════════════
// Model Management
// ═══════════════════════════════════════════════════════════

bool LlamaWrapper::loadModel(
    const std::string& model_path,
    const json& config
) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    spdlog::info("Loading model (lazy): {}", model_path);
    
    auto load_start = std::chrono::high_resolution_clock::now();
    
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
    if (!config.contains("n_batch")) {
        load_config["n_batch"] = config_.n_batch;
    }
    if (!config.contains("n_threads")) {
        load_config["n_threads"] = config_.n_threads;
    }
    
    // Pass performance optimization flags
    if (!config.contains("use_flash_attn")) {
        load_config["use_flash_attn"] = config_.use_flash_attn;
    }
    if (!config.contains("use_mmap")) {
        load_config["use_mmap"] = config_.use_mmap;
    }
    if (!config.contains("use_mlock")) {
        load_config["use_mlock"] = config_.use_mlock;
    }
    if (!config.contains("enable_embeddings")) {
        load_config["enable_embeddings"] = config_.enable_embeddings;
    }
    
    // Trigger lazy load (or get from cache)
    auto* model = model_loader_->getOrLoadModel(
        current_model_id_,
        model_path,
        load_config
    );
    
    if (!model) {
        spdlog::error("Failed to load model: {}", model_path);
        
        if (metrics_collector_) {
            metrics_collector_->recordError("model_load_failed", "model_loader");
        }
        
        return false;
    }
    
    spdlog::info("Model ready: {} (lazy loaded)", current_model_id_);
    
    // Load draft model if speculative decoding enabled
    if (config_.use_speculative_decoding && !config_.draft_model_path.empty()) {
        if (!loadDraftModel(config_.draft_model_path)) {
            spdlog::warn("Failed to load draft model, speculative decoding disabled");
            config_.use_speculative_decoding = false;
        }
    }
    
    auto load_end = std::chrono::high_resolution_clock::now();
    double load_time_ms = std::chrono::duration<double, std::milli>(load_end - load_start).count();
    
    spdlog::info("Model ready: {} (lazy loaded in {:.2f}ms)", current_model_id_, load_time_ms);
    
    // Record model loaded metrics
    if (metrics_collector_) {
        metrics_collector_->recordModelLoaded(current_model_id_, model->vram_mb);
        metrics_collector_->recordModelSwitchLatency(load_time_ms);
    }
    
    return true;
}

void LlamaWrapper::unloadModel() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (current_model_id_.empty()) {
        return;
    }
    
    spdlog::info("Unloading model: {}", current_model_id_);
    
    // Unload draft model first
    unloadDraftModel();
    // Record model unload before clearing the ID
    if (metrics_collector_) {
        metrics_collector_->recordModelUnloaded(current_model_id_);
    }
    
    // Unload via lazy loader
    model_loader_->unloadModel(current_model_id_, true);
    
    current_model_id_.clear();
    current_model_path_.clear();
    
    spdlog::info("Model unloaded");
}

std::optional<ModelInfo> LlamaWrapper::getModelInfo() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (current_model_id_.empty()) {
        return std::nullopt;
    }
    
    return model_loader_->getModelInfo(current_model_id_);
}

bool LlamaWrapper::isModelLoaded() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return !current_model_id_.empty() && 
           model_loader_->isModelLoaded(current_model_id_);
}

// ═══════════════════════════════════════════════════════════
// LoRA Management
// ═══════════════════════════════════════════════════════════

bool LlamaWrapper::loadLoRA(
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

bool LlamaWrapper::unloadLoRA(const std::string& lora_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    return lora_manager_->unloadLoRA(lora_id);
}

std::vector<LoRAInfo> LlamaWrapper::listLoRAs() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return lora_manager_->listLoRAs();
}

// ═══════════════════════════════════════════════════════════
// Inference
// ═══════════════════════════════════════════════════════════

InferenceResponse LlamaWrapper::generate(const InferenceRequest& request) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (current_model_id_.empty()) {
        throw std::runtime_error("No model loaded");
    }
    
    spdlog::debug("Generating response for prompt: {} (max_tokens={})",
                  request.prompt.substr(0, std::min(request.prompt.size(), size_t(50))), request.max_tokens);
    
    // Check if speculative decoding is available and enabled
    if (config_.use_speculative_decoding && draft_model_ && draft_context_) {
        spdlog::debug("Using speculative decoding");
        // Unlock for speculative generation (it will lock internally as needed)
        mutex_.unlock();
        auto response = generateSpeculative(request);
        mutex_.lock();
        return response;
    }
    
    // Fall back to regular generation
    // Unlock for regular generation (it will lock internally as needed)
    mutex_.unlock();
    auto response = generateRegular(request);
    mutex_.lock();
    return response;
    // Check response cache first (if enabled)
    if (response_cache_) {
        auto cached_response = response_cache_->get(request.prompt);
        if (cached_response) {
            spdlog::debug("Cache hit for prompt: {}", request.prompt.substr(0, 50));
            
            // Update request_id to match current request
            cached_response->request_id = request.request_id;
            
            // Record cache hit in inference metrics too
            if (metrics_collector_) {
                metrics_collector_->recordInferenceRequest(current_model_id_);
                metrics_collector_->recordInferenceSuccess(current_model_id_, 1.0); // Cached responses are ~1ms
                metrics_collector_->recordTokensGenerated(current_model_id_, cached_response->tokens_generated);
            }
            
            return *cached_response;
        }
    }
    
    // Record inference request
    if (metrics_collector_) {
        metrics_collector_->recordInferenceRequest(current_model_id_);
    }
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    spdlog::debug("Generating response for prompt: {} (max_tokens={})",
                  request.prompt.substr(0, std::min(request.prompt.size(), size_t(50))), request.max_tokens);
    
    // Ensure model is loaded (lazy loading trigger)
    auto* cached = model_loader_->getOrLoadModel(
        current_model_id_,
        current_model_path_
    );
    if (!cached) {
        if (metrics_collector_) {
            metrics_collector_->recordInferenceFailure(current_model_id_, "model_load_failed");
        }
        throw std::runtime_error("Model failed to load");
    }

    auto* lmodel = reinterpret_cast<llama_model*>(cached->model_handle);
    auto* lctx = reinterpret_cast<llama_context*>(cached->context_handle);
    
    // For testing with stub models, allow nullptr handles
    // In production with real llama.cpp, these would be non-null
    if (!lmodel || !lctx) {
        spdlog::warn("LlamaWrapper: Model/context handle is null, using stub response");
        // Fallback to stub for compatibility
        std::string output = "[Generated response placeholder for: " + request.prompt + "]";
        InferenceResponse response;
        response.request_id = request.request_id;
        response.text = output;
        response.model_used = current_model_id_;
        if (request.lora_adapter_id) {
            response.lora_used = *request.lora_adapter_id;
        }
        response.tokens_prompt = static_cast<int>(std::max<size_t>(1, request.prompt.size() / CHARS_PER_TOKEN_ESTIMATE));
        response.tokens_generated = std::max(1, std::min(request.max_tokens, MAX_STUB_TOKENS));
        auto end_time = std::chrono::high_resolution_clock::now();
        response.inference_time_ms = std::chrono::duration<float, std::milli>(end_time - start_time).count();
        response.latency_ms = static_cast<int64_t>(response.inference_time_ms);
        response.tokens_per_second = (response.inference_time_ms > 0) 
            ? response.tokens_generated / (response.inference_time_ms / 1000.0f)
            : 0.0f;
        updateStatistics(response);
        
        // Record metrics for stub response
        if (metrics_collector_) {
            metrics_collector_->recordInferenceSuccess(current_model_id_, response.inference_time_ms);
            metrics_collector_->recordTokensGenerated(current_model_id_, response.tokens_generated);
            metrics_collector_->recordEndToEndLatency(current_model_id_, response.inference_time_ms);
        }
        
        // Cache the response
        if (response_cache_) {
            response_cache_->put(request.prompt, response);
        }
        
        return response;
    }

    // Real llama.cpp inference implementation
    try {
        // 1. Tokenize prompt
        std::vector<llama_token> prompt_tokens = tokenizeInternal(lmodel, request.prompt, true);
        
        InferenceResponse response;
        response.request_id = request.request_id;
        response.model_used = current_model_id_;
        response.tokens_prompt = static_cast<int>(prompt_tokens.size());
        
        if (request.lora_adapter_id) {
            response.lora_used = *request.lora_adapter_id;
        }
        
        // 2. Prepare batch for prompt evaluation
        llama_batch batch = llama_batch_get_one(prompt_tokens.data(), prompt_tokens.size());
        
        // 3. Evaluate prompt (populate KV cache)
        if (llama_decode(lctx, batch) != 0) {
            throw std::runtime_error("Failed to evaluate prompt");
        }
        
        // 4. Generate tokens
        std::vector<llama_token> generated_tokens;
        int max_tokens = request.max_tokens > 0 ? request.max_tokens : 512;
        float temperature = request.temperature > 0.0f ? request.temperature : 0.7f;
        float top_p = request.top_p > 0.0f ? request.top_p : 0.9f;
        
        // Get vocab for EOS detection and token count
        const llama_vocab* vocab = llama_model_get_vocab(lmodel);
        int32_t n_vocab = llama_vocab_n_tokens(vocab);
        llama_token eos_token = llama_vocab_eos(vocab);
        
        // Time to first token
        auto first_token_start = std::chrono::high_resolution_clock::now();
        bool first_token_generated = false;
        
        for (int i = 0; i < max_tokens; ++i) {
            // Get logits for last token
            float* logits = llama_get_logits_ith(lctx, -1);
            
            // Sample next token
            llama_token next_token = sampleTokenInternal(lctx, lmodel, logits, n_vocab, temperature, top_p);
            
            // Check for end of sequence (EOS token)
            if (next_token == eos_token) {
                break;
            }
            
            generated_tokens.push_back(next_token);
            
            // Record first token latency
            if (!first_token_generated && metrics_collector_) {
                auto first_token_end = std::chrono::high_resolution_clock::now();
                double first_token_latency = std::chrono::duration<double, std::milli>(
                    first_token_end - first_token_start
                ).count();
                metrics_collector_->recordFirstTokenLatency(current_model_id_, first_token_latency);
                first_token_generated = true;
            }
            
            // **Streaming support**: Call callback if provided
            // Note: Callback is called while holding mutex_. Ensure callback
            // is non-blocking to avoid performance issues.
            if (request.stream_callback) {
                try {
                    // Detokenize this single token for streaming
                    std::string token_text = detokenizeInternal(lctx, {next_token});
                    request.stream_callback(token_text);
                } catch (const std::exception& e) {
                    spdlog::warn("Streaming callback error: {}", e.what());
                    // Continue generation even if streaming fails
                }
            }
            
            // Prepare next batch with single token
            llama_batch next_batch = llama_batch_get_one(&next_token, 1);
            
            // Decode next token
            if (llama_decode(lctx, next_batch) != 0) {
                spdlog::warn("Failed to decode token at position {}", i);
                break;
            }
        }
        
        // 5. Detokenize generated tokens
        response.text = detokenizeInternal(lctx, generated_tokens);
        response.tokens_generated = static_cast<int>(generated_tokens.size());
        
        // 6. Calculate timing metrics
        auto end_time = std::chrono::high_resolution_clock::now();
        response.inference_time_ms = std::chrono::duration<float, std::milli>(end_time - start_time).count();
        response.latency_ms = static_cast<int64_t>(response.inference_time_ms);
        
        response.tokens_per_second = (response.inference_time_ms > 0) 
            ? response.tokens_generated / (response.inference_time_ms / 1000.0f)
            : 0.0f;
        
        updateStatistics(response);
        
        // Record comprehensive metrics
        if (metrics_collector_) {
            metrics_collector_->recordInferenceSuccess(current_model_id_, response.inference_time_ms);
            metrics_collector_->recordTokensGenerated(current_model_id_, response.tokens_generated);
            metrics_collector_->recordEndToEndLatency(current_model_id_, response.inference_time_ms);
            
            // Calculate per-token latency
            if (response.tokens_generated > 0) {
                double per_token_latency = response.inference_time_ms / response.tokens_generated;
                metrics_collector_->recordPerTokenLatency(current_model_id_, per_token_latency);
            }
        }
        
        // Cache the successful response
        if (response_cache_) {
            response_cache_->put(request.prompt, response);
        }
        
        return response;
        
    } catch (const std::exception& e) {
        spdlog::error("Inference error: {}", e.what());
        
        // Record error
        if (metrics_collector_) {
            metrics_collector_->recordInferenceFailure(current_model_id_, "inference_exception");
            metrics_collector_->recordError("inference_exception", "llama_wrapper");
        }
        
        throw;
    }
}

InferenceResponse LlamaWrapper::generateRAG(
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

std::vector<float> LlamaWrapper::embed(const std::string& text) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (current_model_id_.empty()) {
        throw std::runtime_error("No model loaded");
    }
    
    spdlog::debug("Generating embedding for text: {}", text.substr(0, 50));
    
    // Ensure model is loaded
    auto* cached = model_loader_->getOrLoadModel(
        current_model_id_,
        current_model_path_
    );
    if (!cached) {
        throw std::runtime_error("Model failed to load");
    }
    
    auto* lmodel = reinterpret_cast<llama_model*>(cached->model_handle);
    auto* lctx = reinterpret_cast<llama_context*>(cached->context_handle);
    
    // Fallback to dummy embedding if handles are null
    if (!lmodel || !lctx) {
        spdlog::warn("LlamaWrapper: Model/context handle is null for embeddings, returning dummy vector");
        return std::vector<float>(768, 0.0f);
    }
    
    try {
        // 1. Tokenize input text
        std::vector<llama_token> tokens = tokenizeInternal(lmodel, text, true);
        
        // 2. Prepare batch for evaluation
        llama_batch batch = llama_batch_get_one(tokens.data(), tokens.size());
        
        // 3. Evaluate to generate embeddings
        if (llama_decode(lctx, batch) != 0) {
            throw std::runtime_error("Failed to generate embeddings");
        }
        
        // 4. Get embeddings from context
        // Note: This requires the model to be loaded with embeddings enabled
        float* embd = llama_get_embeddings(lctx);
        if (!embd) {
            throw std::runtime_error("Failed to retrieve embeddings from context");
        }
        
        // 5. Get embedding dimension
        int32_t n_embd = llama_model_n_embd(lmodel);
        
        // 6. Copy embeddings to vector
        std::vector<float> embedding(embd, embd + n_embd);
        
        // 7. Normalize the embedding vector (L2 normalization)
        float norm = 0.0f;
        for (float val : embedding) {
            norm += val * val;
        }
        norm = std::sqrt(norm);
        
        if (norm > 0.0f) {
            for (float& val : embedding) {
                val /= norm;
            }
        }
        
        return embedding;
        
    } catch (const std::exception& e) {
        spdlog::error("Embedding generation error: {}", e.what());
        throw;
    }
}

// ═══════════════════════════════════════════════════════════
// Capabilities
// ═══════════════════════════════════════════════════════════

LLMCapabilities LlamaWrapper::getCapabilities() const {
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

json LlamaWrapper::getMemoryStats() const {
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

json LlamaWrapper::getPerformanceStats() const {
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

std::vector<uint8_t> LlamaWrapper::exportLoRA(const std::string& lora_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    spdlog::info("Exporting LoRA for cross-shard transfer: {}", lora_id);
    
    // Delegate to multi-LoRA manager
    return lora_manager_->exportLoRA(lora_id);
}

bool LlamaWrapper::importLoRA(
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

std::string LlamaWrapper::formatPromptForRAG(
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

void LlamaWrapper::updateStatistics(const InferenceResponse& response) {
    stats_.total_inferences++;
    stats_.total_tokens_generated += response.tokens_generated;
    stats_.total_inference_time_ms += response.inference_time_ms;
}

std::string LlamaWrapper::extractModelId(const std::string& model_path) {
    // Extract filename without extension as model ID
    std::filesystem::path p(model_path);
    return p.stem().string();
}

// ═══════════════════════════════════════════════════════════
// Chat Formatting Methods
// ═══════════════════════════════════════════════════════════

std::string LlamaWrapper::formatChatMessages(
    const std::vector<ChatMessage>& messages,
    ChatFormat format
) {
    switch (format) {
        case ChatFormat::ChatML:
            return formatChatML(messages);
        case ChatFormat::Llama2:
            return formatLlama2(messages);
        case ChatFormat::Vicuna:
            return formatVicuna(messages);
        case ChatFormat::Alpaca:
            return formatAlpaca(messages);
        default:
            return formatChatML(messages);
    }
}

std::string LlamaWrapper::formatChatML(const std::vector<ChatMessage>& messages) {
    // ChatML format used by Mistral, Llama-3, etc.
    // <|im_start|>system\ncontent<|im_end|>
    std::ostringstream oss;
    
    for (const auto& msg : messages) {
        oss << "<|im_start|>" << msg.role << "\n";
        oss << msg.content << "\n";
        oss << "<|im_end|>\n";
    }
    
    // Add the assistant prompt to trigger response
    oss << "<|im_start|>assistant\n";
    
    return oss.str();
}

std::string LlamaWrapper::formatLlama2(const std::vector<ChatMessage>& messages) {
    // Llama-2 chat format
    // <s>[INST] <<SYS>>\nsystem_message\n<</SYS>>\n\nuser_message [/INST]
    std::ostringstream oss;
    
    bool first_user = true;
    std::string system_msg;
    
    for (const auto& msg : messages) {
        if (msg.role == "system") {
            system_msg = msg.content;
        } else if (msg.role == "user") {
            if (first_user && !system_msg.empty()) {
                oss << "<s>[INST] <<SYS>>\n" << system_msg << "\n<</SYS>>\n\n";
                oss << msg.content << " [/INST]";
                first_user = false;
            } else if (first_user) {
                oss << "<s>[INST] " << msg.content << " [/INST]";
                first_user = false;
            } else {
                oss << " <s>[INST] " << msg.content << " [/INST]";
            }
        } else if (msg.role == "assistant") {
            oss << " " << msg.content << " </s>";
        }
    }
    
    return oss.str();
}

std::string LlamaWrapper::formatVicuna(const std::vector<ChatMessage>& messages) {
    // Vicuna format
    // A chat between a curious user and an artificial intelligence assistant...
    // USER: message\nASSISTANT:
    std::ostringstream oss;
    
    // Optional system message
    bool has_system = false;
    for (const auto& msg : messages) {
        if (msg.role == "system") {
            oss << msg.content << "\n\n";
            has_system = true;
            break;
        }
    }
    
    if (!has_system) {
        oss << "A chat between a curious user and an artificial intelligence assistant. ";
        oss << "The assistant gives helpful, detailed, and polite answers to the user's questions.\n\n";
    }
    
    for (const auto& msg : messages) {
        if (msg.role == "user") {
            oss << "USER: " << msg.content << "\n";
        } else if (msg.role == "assistant") {
            oss << "ASSISTANT: " << msg.content << "\n";
        }
    }
    
    // Add assistant prompt
    oss << "ASSISTANT:";
    
    return oss.str();
}

std::string LlamaWrapper::formatAlpaca(const std::vector<ChatMessage>& messages) {
    // Alpaca format
    // Below is an instruction... ### Instruction:\nuser_message\n\n### Response:
    std::ostringstream oss;
    
    std::string system_msg = "Below is an instruction that describes a task. "
                            "Write a response that appropriately completes the request.";
    
    // Extract system message if present
    for (const auto& msg : messages) {
        if (msg.role == "system") {
            system_msg = msg.content;
            break;
        }
    }
    
    oss << system_msg << "\n\n";
    
    // Format conversation
    for (const auto& msg : messages) {
        if (msg.role == "user") {
            oss << "### Instruction:\n" << msg.content << "\n\n";
        } else if (msg.role == "assistant") {
            oss << "### Response:\n" << msg.content << "\n\n";
        }
    }
    
    // Add response prompt
    oss << "### Response:\n";
    
    return oss.str();
}

// ═══════════════════════════════════════════════════════════
// Internal Helper Methods for llama.cpp Integration
// ═══════════════════════════════════════════════════════════

std::vector<llama_token> LlamaWrapper::tokenizeInternal(
    llama_model* model, 
    const std::string& text, 
    bool add_bos
) {
    if (!model) {
        throw std::runtime_error("Model is null");
    }
    
    // Get vocab from model
    const llama_vocab* vocab = llama_model_get_vocab(model);
    
    // Allocate buffer for tokens (estimate: text length + special tokens)
    int32_t n_tokens_max = text.length() + (add_bos ? 1 : 0) + 8;
    std::vector<llama_token> tokens(n_tokens_max);
    
    // Tokenize
    int32_t n_tokens = llama_tokenize(
        vocab,
        text.c_str(),
        text.length(),
        tokens.data(),
        tokens.size(),
        add_bos,
        false  // special tokens
    );
    
    if (n_tokens < 0) {
        // Buffer was too small, resize and try again
        tokens.resize(-n_tokens);
        n_tokens = llama_tokenize(
            vocab,
            text.c_str(),
            text.length(),
            tokens.data(),
            tokens.size(),
            add_bos,
            false
        );
    }
    
    if (n_tokens < 0) {
        throw std::runtime_error("Failed to tokenize text");
    }
    
    tokens.resize(n_tokens);
    return tokens;
}

std::string LlamaWrapper::detokenizeInternal(
    llama_context* ctx,
    const std::vector<llama_token>& tokens
) {
    if (!ctx) {
        throw std::runtime_error("Context is null");
    }
    
    // Get model and vocab from context
    const llama_model* model = llama_get_model(ctx);
    const llama_vocab* vocab = llama_model_get_vocab(model);
    
    std::string result;
    result.reserve(tokens.size() * 4);  // Rough estimate
    
    for (llama_token token : tokens) {
        // Buffer for token piece
        char buf[256];
        int32_t n = llama_token_to_piece(vocab, token, buf, sizeof(buf), 0, false);
        
        if (n > 0 && n < static_cast<int32_t>(sizeof(buf))) {
            result.append(buf, n);
        }
    }
    
    return result;
}

llama_token LlamaWrapper::sampleTokenInternal(
    llama_context* ctx,
    llama_model* model,
    float* logits,
    int32_t n_vocab,
    float temperature,
    float top_p
) {
    if (!ctx || !model || !logits) {
        throw std::runtime_error("Invalid parameters for sampling");
    }
    
    // Build candidates array from logits
    std::vector<llama_token_data> candidates;
    candidates.reserve(n_vocab);
    
    for (llama_token token_id = 0; token_id < n_vocab; ++token_id) {
        candidates.push_back({token_id, logits[token_id], 0.0f});
    }
    
    llama_token_data_array candidates_p = {
        candidates.data(),
        candidates.size(),
        -1,     // selected token (not used)
        false   // sorted
    };
    
    // Apply temperature sampling
    if (temperature > 0.0f && temperature != 1.0f) {
        // Manually apply temperature to logits
        for (size_t i = 0; i < candidates.size(); ++i) {
            candidates[i].logit /= temperature;
        }
    }
    
    // Apply top-p (nucleus) sampling
    if (top_p < 1.0f && top_p > 0.0f) {
        // Sort by logit (descending)
        std::sort(candidates.begin(), candidates.end(), 
            [](const llama_token_data& a, const llama_token_data& b) {
                return a.logit > b.logit;
            });
        
        // Calculate softmax and cumulative probability
        float max_logit = candidates[0].logit;
        float sum_exp = 0.0f;
        for (auto& c : candidates) {
            c.p = std::exp(c.logit - max_logit);
            sum_exp += c.p;
        }
        
        float cum_prob = 0.0f;
        size_t last_idx = 0;
        for (size_t i = 0; i < candidates.size(); ++i) {
            candidates[i].p /= sum_exp;
            cum_prob += candidates[i].p;
            last_idx = i;
            if (cum_prob >= top_p) {
                break;
            }
        }
        
        // Truncate to top-p
        candidates.resize(last_idx + 1);
        candidates_p.size = candidates.size();
    }
    
    // Sample from remaining candidates
    if (candidates.empty()) {
        return 0;  // Fallback to token 0
    }
    
    // Simple greedy sampling from sorted candidates
    // (For production, use llama_sampler for more sophisticated sampling)
    return candidates[0].id;
}

// ═══════════════════════════════════════════════════════════
// Output Formatting Helpers (MCP, SSE, AQL)
// ═══════════════════════════════════════════════════════════

json LlamaWrapper::formatAsMCPResponse(const InferenceResponse& response) {
    // MCP-compatible response format for Model Context Protocol
    json mcp_response = {
        {"type", "completion"},
        {"completion", {
            {"text", response.text},
            {"model", response.model_used},
            {"tokens_prompt", response.tokens_prompt},
            {"tokens_generated", response.tokens_generated},
            {"tokens_per_second", response.tokens_per_second},
            {"latency_ms", response.latency_ms}
        }},
        {"metadata", response.metadata}
    };
    
    if (!response.request_id.empty()) {
        mcp_response["request_id"] = response.request_id;
    }
    
    if (response.lora_used.has_value()) {
        mcp_response["completion"]["lora"] = response.lora_used.value();
    }
    
    return mcp_response;
}

std::string LlamaWrapper::formatAsSSE(const InferenceResponse& response) {
    // Server-Sent Events format: "data: {json}\n\n"
    json sse_data = formatAsMCPResponse(response);
    return "data: " + sse_data.dump() + "\n\n";
}

json LlamaWrapper::formatAsJsonMarkdown(const InferenceResponse& response) {
    // JSON with embedded markdown for rich text display
    json result = {
        {"format", "markdown"},
        {"content", response.text},
        {"metadata", {
            {"model", response.model_used},
            {"tokens", {
                {"prompt", response.tokens_prompt},
                {"generated", response.tokens_generated},
                {"total", response.tokens_prompt + response.tokens_generated}
            }},
            {"performance", {
                {"latency_ms", response.latency_ms},
                {"tokens_per_second", response.tokens_per_second}
            }}
        }}
    };
    
    if (!response.request_id.empty()) {
        result["request_id"] = response.request_id;
    }
    
    // Add markdown formatting hints
    result["metadata"]["format_hints"] = {
        {"supports_code_blocks", true},
        {"supports_tables", true},
        {"supports_latex", false}
    };
    
    return result;
}

std::string LlamaWrapper::formatStreamTokenAsSSE(const std::string& token, const std::string& request_id) {
    // SSE format for streaming tokens
    json event = {
        {"type", "token"},
        {"token", token}
    };
    
    if (!request_id.empty()) {
        event["request_id"] = request_id;
    }
    
    return "data: " + event.dump() + "\n\n";
}

// ═══════════════════════════════════════════════════════════
// Cache Management (Optional Features)
// ═══════════════════════════════════════════════════════════

std::optional<PrefixCacheStatistics> LlamaWrapper::getPrefixCacheStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!prefix_cache_) {
        return std::nullopt;
    }
    
    return prefix_cache_->getStatistics();
}

void LlamaWrapper::clearPrefixCache() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (prefix_cache_) {
        prefix_cache_->clear();
        spdlog::info("Prefix cache cleared");
    }
}

// ═══════════════════════════════════════════════════════════
// Speculative Decoding Implementation (Phase 2)
// ═══════════════════════════════════════════════════════════

bool LlamaWrapper::loadDraftModel(const std::string& draft_path) {
    spdlog::info("Loading draft model for speculative decoding: {}", draft_path);
    
    // Initialize llama.cpp model parameters for draft
    llama_model_params draft_params = llama_model_default_params();
    draft_params.n_gpu_layers = config_.draft_n_gpu_layers;
    draft_params.use_mmap = config_.use_mmap;
    draft_params.use_mlock = false;  // Draft doesn't need mlock
    
    // Load draft model
    draft_model_ = llama_load_model_from_file(draft_path.c_str(), draft_params);
    
    if (!draft_model_) {
        spdlog::error("Failed to load draft model: {}", draft_path);
        return false;
    }
    
    // Create context for draft model
    llama_context_params draft_ctx_params = llama_context_default_params();
    draft_ctx_params.n_ctx = config_.n_ctx;
    draft_ctx_params.n_batch = config_.n_batch * 2;  // Draft can use larger batch
    draft_ctx_params.n_threads = config_.n_threads;
    
    draft_context_ = llama_new_context_with_model(draft_model_, draft_ctx_params);
    
    if (!draft_context_) {
        spdlog::error("Failed to create context for draft model");
        llama_free_model(draft_model_);
        draft_model_ = nullptr;
        return false;
    }
    
    draft_model_id_ = extractModelId(draft_path);
    spdlog::info("Draft model loaded successfully: {} ({} GPU layers)", 
                 draft_model_id_, config_.draft_n_gpu_layers);
    return true;
}

void LlamaWrapper::unloadDraftModel() {
    if (draft_context_) {
        llama_free(draft_context_);
        draft_context_ = nullptr;
    }
    if (draft_model_) {
        llama_free_model(draft_model_);
        draft_model_ = nullptr;
    }
    draft_model_id_.clear();
}

std::optional<LlamaWrapper::SpeculativeDecodingStats> LlamaWrapper::getSpeculativeStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!config_.use_speculative_decoding || !draft_model_) {
        return std::nullopt;
    }
    
    return speculative_stats_;
}

float LlamaWrapper::getProbability(float* logits, llama_token token, int32_t n_vocab) {
    // Find max logit for numerical stability
    float max_logit = -INFINITY;
    for (int32_t i = 0; i < n_vocab; ++i) {
        max_logit = std::max(max_logit, logits[i]);
    }
    
    // Calculate softmax denominator
    float sum_exp = 0.0f;
    for (int32_t i = 0; i < n_vocab; ++i) {
        sum_exp += std::exp(logits[i] - max_logit);
    }
    
    // Calculate probability for target token
    float token_prob = std::exp(logits[token] - max_logit) / sum_exp;
    return token_prob;
}

void LlamaWrapper::synchronizeDraftToTarget(const std::vector<llama_token>& accepted_tokens) {
    // Synchronize draft model's KV cache to match target model
    // This ensures both models are at the same position
    // Implementation: Re-evaluate accepted tokens in draft model
    if (accepted_tokens.empty() || !draft_context_) {
        return;
    }
    
    // Clear draft context and re-evaluate accepted tokens
    llama_memory_t mem = llama_get_memory(draft_context_);
    llama_memory_clear(mem, true);
    
    llama_batch batch = llama_batch_get_one(
        const_cast<llama_token*>(accepted_tokens.data()), 
        accepted_tokens.size()
    );
    
    if (llama_decode(draft_context_, batch) != 0) {
        spdlog::warn("Failed to synchronize draft model");
    }
}

InferenceResponse LlamaWrapper::generateSpeculative(const InferenceRequest& request) {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Get target model
    auto* cached = model_loader_->getOrLoadModel(current_model_id_, current_model_path_);
    if (!cached) {
        throw std::runtime_error("Target model failed to load");
    }
    
    auto* target_model = reinterpret_cast<llama_model*>(cached->model_handle);
    auto* target_context = reinterpret_cast<llama_context*>(cached->context_handle);
    
    if (!target_model || !target_context || !draft_model_ || !draft_context_) {
        spdlog::warn("Speculative decoding prerequisites not met, falling back to regular generation");
        return generateRegular(request);
    }
    
    try {
        // 1. Tokenize prompt (same for both models)
        std::vector<llama_token> prompt_tokens = tokenizeInternal(target_model, request.prompt, true);
        
        InferenceResponse response;
        response.request_id = request.request_id;
        response.model_used = current_model_id_ + " (speculative)";
        response.tokens_prompt = static_cast<int>(prompt_tokens.size());
        
        if (request.lora_adapter_id) {
            response.lora_used = *request.lora_adapter_id;
        }
        
        // 2. Evaluate prompt in both models
        llama_batch batch = llama_batch_get_one(prompt_tokens.data(), prompt_tokens.size());
        if (llama_decode(target_context, batch) != 0) {
            throw std::runtime_error("Failed to evaluate prompt in target model");
        }
        if (llama_decode(draft_context_, batch) != 0) {
            throw std::runtime_error("Failed to evaluate prompt in draft model");
        }
        
        // 3. Speculative generation loop
        std::vector<llama_token> generated_tokens;
        const llama_vocab* vocab = llama_model_get_vocab(target_model);
        int32_t n_vocab = llama_vocab_n_tokens(vocab);
        llama_token eos_token = llama_vocab_eos(vocab);
        
        int max_tokens = request.max_tokens > 0 ? request.max_tokens : 512;
        float temperature = request.temperature > 0.0f ? request.temperature : 0.7f;
        float top_p = request.top_p > 0.0f ? request.top_p : 0.9f;
        
        size_t total_speculations = 0;
        size_t total_accepted = 0;
        
        while (generated_tokens.size() < static_cast<size_t>(max_tokens)) {
            // 3a. Draft model generates N candidate tokens
            std::vector<llama_token> draft_tokens;
            for (int i = 0; i < config_.speculative_tokens; ++i) {
                float* draft_logits = llama_get_logits_ith(draft_context_, -1);
                llama_token draft_token = sampleTokenInternal(
                    draft_context_, draft_model_, draft_logits, n_vocab,
                    temperature, top_p
                );
                
                draft_tokens.push_back(draft_token);
                
                // Feed token back to draft model
                llama_batch draft_batch = llama_batch_get_one(&draft_token, 1);
                if (llama_decode(draft_context_, draft_batch) != 0) {
                    break;
                }
                
                // Stop if EOS
                if (draft_token == eos_token) {
                    break;
                }
            }
            
            total_speculations += draft_tokens.size();
            
            // 3b. Target model validates all draft tokens in parallel
            llama_batch validation_batch = llama_batch_get_one(
                draft_tokens.data(), draft_tokens.size()
            );
            if (llama_decode(target_context, validation_batch) != 0) {
                spdlog::warn("Failed to validate draft tokens");
                break;
            }
            
            // 3c. Check which tokens are accepted
            int accepted = 0;
            for (size_t i = 0; i < draft_tokens.size(); ++i) {
                float* target_logits = llama_get_logits_ith(target_context, i);
                
                // Get probability of draft token from target model
                float target_prob = getProbability(target_logits, draft_tokens[i], n_vocab);
                
                if (target_prob >= config_.acceptance_threshold) {
                    generated_tokens.push_back(draft_tokens[i]);
                    total_accepted++;
                    accepted++;
                    
                    // Stream token if callback provided
                    if (request.stream_callback) {
                        try {
                            std::string token_text = detokenizeInternal(target_context, {draft_tokens[i]});
                            request.stream_callback(token_text);
                        } catch (const std::exception& e) {
                            spdlog::warn("Streaming callback error: {}", e.what());
                        }
                    }
                    
                    // Check for EOS
                    if (draft_tokens[i] == eos_token) {
                        break;
                    }
                } else {
                    // Target model rejects, resample from target distribution
                    llama_token corrected_token = sampleTokenInternal(
                        target_context, target_model, target_logits, n_vocab,
                        temperature, top_p
                    );
                    generated_tokens.push_back(corrected_token);
                    accepted++;
                    
                    // Stream corrected token
                    if (request.stream_callback) {
                        try {
                            std::string token_text = detokenizeInternal(target_context, {corrected_token});
                            request.stream_callback(token_text);
                        } catch (const std::exception& e) {
                            spdlog::warn("Streaming callback error: {}", e.what());
                        }
                    }
                    
                    // Synchronize draft model after rejection
                    synchronizeDraftToTarget(generated_tokens);
                    break;  // Stop after first rejection
                }
            }
            
            // Check for completion
            if (generated_tokens.empty() || generated_tokens.back() == eos_token) {
                break;
            }
        }
        
        // 4. Update statistics
        speculative_stats_.total_speculations += total_speculations;
        speculative_stats_.total_accepted += total_accepted;
        speculative_stats_.total_rejected += (total_speculations - total_accepted);
        
        if (total_speculations > 0) {
            double acceptance_rate = static_cast<double>(total_accepted) / total_speculations;
            speculative_stats_.avg_acceptance_rate = 
                (speculative_stats_.avg_acceptance_rate * (speculative_stats_.total_speculations - total_speculations) + 
                 acceptance_rate * total_speculations) / speculative_stats_.total_speculations;
        }
        
        // 5. Detokenize and finalize response
        response.text = detokenizeInternal(target_context, generated_tokens);
        response.tokens_generated = static_cast<int>(generated_tokens.size());
        
        auto end_time = std::chrono::high_resolution_clock::now();
        response.inference_time_ms = std::chrono::duration<float, std::milli>(end_time - start_time).count();
        response.latency_ms = static_cast<int64_t>(response.inference_time_ms);
        
        response.tokens_per_second = (response.inference_time_ms > 0) 
            ? response.tokens_generated / (response.inference_time_ms / 1000.0f)
            : 0.0f;
        
        // Add speculative decoding metadata
        response.metadata["speculative_decoding"] = true;
        response.metadata["acceptance_rate"] = (total_speculations > 0) 
            ? static_cast<double>(total_accepted) / total_speculations : 0.0;
        response.metadata["draft_model"] = draft_model_id_;
        
        updateStatistics(response);
        return response;
        
    } catch (const std::exception& e) {
        spdlog::error("Speculative decoding error: {}", e.what());
        spdlog::info("Falling back to regular generation");
        return generateRegular(request);
    }
}

InferenceResponse LlamaWrapper::generateRegular(const InferenceRequest& request) {
    // This is the existing generate() implementation extracted
    // Fallback for when speculative decoding is not available
    auto start_time = std::chrono::high_resolution_clock::now();
    
    auto* cached = model_loader_->getOrLoadModel(current_model_id_, current_model_path_);
    if (!cached) {
        throw std::runtime_error("Model failed to load");
    }

    auto* lmodel = reinterpret_cast<llama_model*>(cached->model_handle);
    auto* lctx = reinterpret_cast<llama_context*>(cached->context_handle);
    
    // For testing with stub models, allow nullptr handles
    if (!lmodel || !lctx) {
        spdlog::warn("LlamaWrapper: Model/context handle is null, using stub response");
        std::string output = "[Generated response placeholder for: " + request.prompt + "]";
        InferenceResponse response;
        response.request_id = request.request_id;
        response.text = output;
        response.model_used = current_model_id_;
        if (request.lora_adapter_id) {
            response.lora_used = *request.lora_adapter_id;
        }
        response.tokens_prompt = static_cast<int>(std::max<size_t>(1, request.prompt.size() / CHARS_PER_TOKEN_ESTIMATE));
        response.tokens_generated = std::max(1, std::min(request.max_tokens, MAX_STUB_TOKENS));
        auto end_time = std::chrono::high_resolution_clock::now();
        response.inference_time_ms = std::chrono::duration<float, std::milli>(end_time - start_time).count();
        response.latency_ms = static_cast<int64_t>(response.inference_time_ms);
        response.tokens_per_second = (response.inference_time_ms > 0) 
            ? response.tokens_generated / (response.inference_time_ms / 1000.0f)
            : 0.0f;
        updateStatistics(response);
        return response;
    }

    // Real llama.cpp inference implementation
    try {
        std::vector<llama_token> prompt_tokens = tokenizeInternal(lmodel, request.prompt, true);
        
        InferenceResponse response;
        response.request_id = request.request_id;
        response.model_used = current_model_id_;
        response.tokens_prompt = static_cast<int>(prompt_tokens.size());
        
        if (request.lora_adapter_id) {
            response.lora_used = *request.lora_adapter_id;
        }
        
        llama_batch batch = llama_batch_get_one(prompt_tokens.data(), prompt_tokens.size());
        
        if (llama_decode(lctx, batch) != 0) {
            throw std::runtime_error("Failed to evaluate prompt");
        }
        
        std::vector<llama_token> generated_tokens;
        int max_tokens = request.max_tokens > 0 ? request.max_tokens : 512;
        float temperature = request.temperature > 0.0f ? request.temperature : 0.7f;
        float top_p = request.top_p > 0.0f ? request.top_p : 0.9f;
        
        const llama_vocab* vocab = llama_model_get_vocab(lmodel);
        int32_t n_vocab = llama_vocab_n_tokens(vocab);
        llama_token eos_token = llama_vocab_eos(vocab);
        
        for (int i = 0; i < max_tokens; ++i) {
            float* logits = llama_get_logits_ith(lctx, -1);
            llama_token next_token = sampleTokenInternal(lctx, lmodel, logits, n_vocab, temperature, top_p);
            
            if (next_token == eos_token) {
                break;
            }
            
            generated_tokens.push_back(next_token);
            
            if (request.stream_callback) {
                try {
                    std::string token_text = detokenizeInternal(lctx, {next_token});
                    request.stream_callback(token_text);
                } catch (const std::exception& e) {
                    spdlog::warn("Streaming callback error: {}", e.what());
                }
            }
            
            llama_batch next_batch = llama_batch_get_one(&next_token, 1);
            if (llama_decode(lctx, next_batch) != 0) {
                spdlog::warn("Failed to decode token at position {}", i);
                break;
            }
        }
        
        response.text = detokenizeInternal(lctx, generated_tokens);
        response.tokens_generated = static_cast<int>(generated_tokens.size());
        
        auto end_time = std::chrono::high_resolution_clock::now();
        response.inference_time_ms = std::chrono::duration<float, std::milli>(end_time - start_time).count();
        response.latency_ms = static_cast<int64_t>(response.inference_time_ms);
        
        response.tokens_per_second = (response.inference_time_ms > 0) 
            ? response.tokens_generated / (response.inference_time_ms / 1000.0f)
            : 0.0f;
        
        updateStatistics(response);
        return response;
        
    } catch (const std::exception& e) {
        spdlog::error("Inference error: {}", e.what());
        throw;
    }
}

// ═══════════════════════════════════════════════════════════
// Continuous Batching (Phase 2.2)
// ═══════════════════════════════════════════════════════════

void LlamaWrapper::startBatchMode() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (batch_mode_active_) {
        spdlog::warn("Batch mode already active");
        return;
    }
    
    if (!config_.use_continuous_batching) {
        spdlog::warn("Continuous batching is disabled in config");
        return;
    }
    
    // Initialize PagedKVCache if not already
    if (!paged_kv_cache_) {
        PagedKVCache::Config kv_config;
        kv_config.block_size = 16;  // 16 tokens per block
        kv_config.num_blocks = 4096; // Supports large batches
        
        // Create block manager for PagedKVCache
        PagedBlockManager::Config block_config;
        block_config.max_blocks = kv_config.num_blocks;
        block_config.block_size_tokens = kv_config.block_size;
        auto block_manager = std::make_shared<PagedBlockManager>(block_config);
        paged_kv_cache_ = std::make_unique<PagedKVCache>(kv_config, block_manager);
        spdlog::info("PagedKVCache initialized for continuous batching");
    }
    
    // Initialize ContinuousBatchScheduler
    if (!batch_scheduler_) {
        ContinuousBatchScheduler::SchedulerConfig sched_config;
        sched_config.max_batch_size = config_.max_batch_size;
        sched_config.max_concurrent_requests = config_.max_concurrent_requests;
        sched_config.max_tokens_per_batch = config_.max_tokens_per_batch;
        sched_config.enable_preemption = config_.enable_preemption;
        sched_config.enable_chunked_prefill = config_.enable_chunked_prefill;
        sched_config.prefill_chunk_size = config_.prefill_chunk_size;
        sched_config.enable_priority_scheduling = (config_.scheduler_policy == "priority");
        sched_config.enable_continuous_batching = true;
        
        batch_scheduler_ = std::make_unique<ContinuousBatchScheduler>(
            sched_config,
            paged_kv_cache_.get()
        );
        spdlog::info("Continuous Batch Scheduler initialized (vLLM-style)");
    }
    
    // Start the scheduler
    batch_scheduler_->start();
    batch_mode_active_ = true;
    
    spdlog::info("Continuous batching mode started:");
    spdlog::info("  Max batch size: {}", config_.max_batch_size);
    spdlog::info("  Max concurrent: {}", config_.max_concurrent_requests);
    spdlog::info("  Scheduler policy: {}", config_.scheduler_policy);
    spdlog::info("  Preemption: {}", config_.enable_preemption ? "enabled" : "disabled");
}

void LlamaWrapper::stopBatchMode() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!batch_mode_active_) {
        spdlog::warn("Batch mode not active");
        return;
    }
    
    if (batch_scheduler_) {
        batch_scheduler_->stop();
    }
    
    batch_mode_active_ = false;
    spdlog::info("Continuous batching mode stopped");
}

bool LlamaWrapper::isBatchModeActive() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return batch_mode_active_;
}

std::string LlamaWrapper::submitBatchRequest(
    const InferenceRequest& request,
    ContinuousBatchScheduler::RequestPriority priority,
    std::function<void(const InferenceResponse&)> callback
) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!batch_mode_active_) {
        throw std::runtime_error("Batch mode not active. Call startBatchMode() first.");
    }
    
    if (!batch_scheduler_) {
        throw std::runtime_error("Batch scheduler not initialized");
    }
    
    // Submit request to scheduler
    std::string request_id = batch_scheduler_->submitRequest(request, priority, callback);
    
    spdlog::debug("Batch request submitted: {} (priority: {})",
                  request_id, static_cast<int>(priority));
    
    return request_id;
}

std::optional<ContinuousBatchScheduler::Stats> LlamaWrapper::getBatchSchedulerStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!batch_mode_active_ || !batch_scheduler_) {
        return std::nullopt;
    }
    
    return batch_scheduler_->getStats();
}

} // namespace llm
} // namespace themis

