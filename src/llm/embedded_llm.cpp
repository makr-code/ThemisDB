/**
 * @file embedded_llm.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=6, M=4, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "llm/embedded_llm.h"
#include "utils/error_registry.h"
#include <spdlog/spdlog.h>

namespace themis {
namespace llm {

// ═══════════════════════════════════════════════════════════
// EmbeddedLLM Implementation
// ═══════════════════════════════════════════════════════════

EmbeddedLLM::EmbeddedLLM()
    : EmbeddedLLM(Config{}) {
}

EmbeddedLLM::EmbeddedLLM(const Config& config)
    : config_(config) {
    
    // Configure LlamaWrapper
    LlamaWrapper::Config wrapper_config;
    wrapper_config.n_gpu_layers = config.n_gpu_layers;
    wrapper_config.n_ctx = config.n_ctx;
    wrapper_config.n_batch = config.n_batch > 0 ? config.n_batch : config.n_ctx;

    wrapper_ = std::make_unique<LlamaWrapper>(wrapper_config);
    
    // Initialize ethical guidelines if enabled
    if (config.enable_ethical_guidelines) {
        try {
            ethical_guidelines_ = std::make_unique<EthicalGuidelinesManager>(
                config.ethical_guidelines_config
            );
            spdlog::info("Ethical guidelines enabled: {}", config.ethical_guidelines_config);
        } catch (const std::exception& e) {
            spdlog::warn("Failed to load ethical guidelines: {}. Continuing without.", e.what());
            ethical_guidelines_ = nullptr;
        }
    }
    
    // Load model
    if (!config.model_path.empty()) {
        if (!wrapper_->loadModel(config.model_path)) {
            errors::logError(errors::ErrorCode::ERR_LLM_MODEL_LOAD_FAILED, config.model_path);
        } else {
            spdlog::info("EmbeddedLLM initialized with model: {}", config.model_path);
        }
    }
}

EmbeddedLLM::~EmbeddedLLM() {
    if (wrapper_) {
        wrapper_->unloadModel();
    }
}

void EmbeddedLLM::setGenerateFullFn(GenerateFullFn fn) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    generate_full_fn_ = std::move(fn);
}

void EmbeddedLLM::setEmbedFn(EmbedFn fn) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    embed_fn_ = std::move(fn);
}

// ═══════════════════════════════════════════════════════════
// Simple text generation
// ═══════════════════════════════════════════════════════════

std::string EmbeddedLLM::generate(const std::string& prompt, int max_tokens) {
    // Apply ethical guidelines to prompt
    std::string final_prompt = applyEthicalGuidelines(prompt);
    
    InferenceRequest request = createRequest(final_prompt, max_tokens);
    auto response = generateFull(request);
    
    // Apply ethical guidelines to response (add disclaimer if needed)
    if (hasEthicalGuidelines()) {
        auto detection_result = ethical_guidelines_->detectEthicalContext(prompt);
        response.text = ethical_guidelines_->augmentResponse(response.text, detection_result);
    }
    
    return response.text;
}

std::string EmbeddedLLM::generateWithParams(
    const std::string& prompt,
    float temperature,
    float top_p,
    int max_tokens
) {
    // Apply ethical guidelines to prompt
    std::string final_prompt = applyEthicalGuidelines(prompt);
    
    InferenceRequest request = createRequest(final_prompt, max_tokens, temperature, top_p);
    auto response = generateFull(request);
    
    // Apply ethical guidelines to response
    if (hasEthicalGuidelines()) {
        auto detection_result = ethical_guidelines_->detectEthicalContext(prompt);
        response.text = ethical_guidelines_->augmentResponse(response.text, detection_result);
    }
    
    return response.text;
}

// ═══════════════════════════════════════════════════════════
// Chat interface
// ═══════════════════════════════════════════════════════════

std::string EmbeddedLLM::chat(
    const std::vector<ChatMessage>& messages,
    ChatFormat format
) {
    // Format messages using the chat template
    std::string formatted_prompt = wrapper_->formatChatMessages(messages, format);
    
    // Generate response
    return generate(formatted_prompt);
}

std::string EmbeddedLLM::chatSimple(
    const std::string& system_prompt,
    const std::string& user_message
) {
    std::vector<ChatMessage> messages = {
        {"system", system_prompt},
        {"user", user_message}
    };
    
    return chat(messages);
}

// ═══════════════════════════════════════════════════════════
// Embeddings
// ═══════════════════════════════════════════════════════════

std::vector<float> EmbeddedLLM::embed(const std::string& text) {
    EmbedFn embed_fn;
    {
        std::lock_guard<std::mutex> lock(callback_mutex_);
        embed_fn = embed_fn_;
    }
    if (embed_fn) {
        auto result = embed_fn(text);
        if (!result.empty()) {
            return result;
        }
    }
    {
        std::lock_guard<std::mutex> lk(cache_mutex_);
        auto it = embedding_cache_.find(text);
        if (it != embedding_cache_.end()) {
            return it->second;
        }
    }
    auto embedding = wrapper_->embed(text);
    {
        std::lock_guard<std::mutex> lk(cache_mutex_);
        embedding_cache_.emplace(text, embedding);
    }
    return embedding;
}

std::vector<std::vector<float>> EmbeddedLLM::embedBatch(const std::vector<std::string>& texts) {
    std::vector<std::vector<float>> embeddings;
    embeddings.reserve(texts.size());
    
    for (const auto& text : texts) {
        embeddings.push_back(embed(text));  // reuse cached embed()
    }
    
    return embeddings;
}

// ═══════════════════════════════════════════════════════════
// Streaming
// ═══════════════════════════════════════════════════════════

std::string EmbeddedLLM::generateStreaming(
    const std::string& prompt,
    std::function<void(const std::string& token)> callback,
    int max_tokens
) {
    InferenceRequest request = createRequest(prompt, max_tokens);
    request.stream_callback = callback;
    auto response = generateFull(request);
    return response.text;
}

std::string EmbeddedLLM::generateStreamingSSE(
    const std::string& prompt,
    std::function<void(const std::string& sse_event)> callback,
    const std::string& request_id,
    int max_tokens
) {
    // Wrapper callback that formats tokens as SSE
    auto sse_callback = [&callback, &request_id](const std::string& token) {
        std::string sse_event = LlamaWrapper::formatStreamTokenAsSSE(token, request_id);
        callback(sse_event);
    };
    
    return generateStreaming(prompt, sse_callback, max_tokens);
}

// ═══════════════════════════════════════════════════════════
// Output formatting
// ═══════════════════════════════════════════════════════════

json EmbeddedLLM::generateAsMCP(const std::string& prompt, int max_tokens) {
    InferenceRequest request = createRequest(prompt, max_tokens);
    auto response = generateFull(request);
    return LlamaWrapper::formatAsMCPResponse(response);
}

json EmbeddedLLM::generateAsJsonMarkdown(const std::string& prompt, int max_tokens) {
    InferenceRequest request = createRequest(prompt, max_tokens);
    auto response = generateFull(request);
    return LlamaWrapper::formatAsJsonMarkdown(response);
}

InferenceResponse EmbeddedLLM::generateFull(const InferenceRequest& request) {
    {
        std::lock_guard<std::mutex> lock(callback_mutex_);
        if (generate_full_fn_) {
            return generate_full_fn_(request);
        }
    }
    return wrapper_->generate(request);
}

// ═══════════════════════════════════════════════════════════
// Utility methods
// ═══════════════════════════════════════════════════════════

bool EmbeddedLLM::isReady() const {
    {
        std::lock_guard<std::mutex> lock(callback_mutex_);
        if (generate_full_fn_ || embed_fn_) {
            return true;
        }
    }
    return wrapper_ && wrapper_->isModelLoaded();
}

std::string EmbeddedLLM::getModelInfo() const {
    if (!wrapper_) {
        return "No model loaded";
    }
    
    auto info = wrapper_->getModelInfo();
    if (!info) {
        return "Model info unavailable";
    }
    
    return info->model_id + " (loaded)";
}

json EmbeddedLLM::getStats() const {
    if (!wrapper_) {
        return json{{"error", "No wrapper"}};
    }
    
    return wrapper_->getPerformanceStats();
}

void EmbeddedLLM::clearCache() {
    std::size_t count;
    {
        std::lock_guard<std::mutex> lk(cache_mutex_);
        count = embedding_cache_.size();
        embedding_cache_.clear();
    }
    spdlog::info("EmbeddedLLM: embedding cache cleared ({} entries removed)", count);
}

// ═══════════════════════════════════════════════════════════
// Internal helpers
// ═══════════════════════════════════════════════════════════

InferenceRequest EmbeddedLLM::createRequest(
    const std::string& prompt,
    int max_tokens,
    float temperature,
    float top_p
) {
    InferenceRequest request;
    request.prompt = prompt;
    request.max_tokens = max_tokens;
    request.temperature = temperature;
    request.top_p = top_p;
    request.model_id = config_.model_id;
    
    return request;
}

std::string EmbeddedLLM::applyEthicalGuidelines(
    const std::string& prompt,
    const std::string& context_text) {
    
    if (!ethical_guidelines_ || !ethical_guidelines_->isEnabled()) {
        return prompt;
    }
    
    // Detect ethical context in prompt (and optionally in context)
    std::string text_to_check = context_text.empty() ? prompt : prompt + " " + context_text;
    auto detection_result = ethical_guidelines_->detectEthicalContext(text_to_check);
    
    // Augment prompt if ethical context detected or if always_apply_default is true
    return ethical_guidelines_->augmentPrompt(prompt, detection_result);
}

EthicalGuidelinesManager* EmbeddedLLM::getEthicalGuidelines() {
    return ethical_guidelines_.get();
}

bool EmbeddedLLM::hasEthicalGuidelines() const {
    return ethical_guidelines_ != nullptr && ethical_guidelines_->isEnabled();
}

// ═══════════════════════════════════════════════════════════
// EmbeddedLLMManager Implementation (Singleton)
// ═══════════════════════════════════════════════════════════

EmbeddedLLMManager& EmbeddedLLMManager::instance() {
    static EmbeddedLLMManager instance;
    return instance;
}

void EmbeddedLLMManager::initialize(const EmbeddedLLM::Config& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (initialized_) {
        spdlog::warn("EmbeddedLLMManager already initialized");
        return;
    }
    
    llm_ = std::make_unique<EmbeddedLLM>(config);
    initialized_ = true;
    
    spdlog::info("EmbeddedLLMManager initialized");
}

EmbeddedLLM& EmbeddedLLMManager::get() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!initialized_) {
        throw std::runtime_error("EmbeddedLLMManager not initialized. Call initialize() first.");
    }
    
    return *llm_;
}

bool EmbeddedLLMManager::isInitialized() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return initialized_;
}

} // namespace llm
} // namespace themis
