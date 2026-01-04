#include "llm/embedded_llm.h"
#include <spdlog/spdlog.h>

namespace themis {
namespace llm {

// ═══════════════════════════════════════════════════════════
// EmbeddedLLM Implementation
// ═══════════════════════════════════════════════════════════

EmbeddedLLM::EmbeddedLLM(const Config& config)
    : config_(config) {
    
    // Configure LlamaWrapper
    LlamaWrapper::Config wrapper_config;
    wrapper_config.n_gpu_layers = config.n_gpu_layers;
    wrapper_config.n_ctx = config.n_ctx;
    
    wrapper_ = std::make_unique<LlamaWrapper>(wrapper_config);
    
    // Load model
    if (!config.model_path.empty()) {
        if (!wrapper_->loadModel(config.model_path)) {
            spdlog::error("Failed to load model: {}", config.model_path);
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

// ═══════════════════════════════════════════════════════════
// Simple text generation
// ═══════════════════════════════════════════════════════════

std::string EmbeddedLLM::generate(const std::string& prompt, int max_tokens) {
    InferenceRequest request = createRequest(prompt, max_tokens);
    auto response = wrapper_->generate(request);
    return response.text;
}

std::string EmbeddedLLM::generateWithParams(
    const std::string& prompt,
    float temperature,
    float top_p,
    int max_tokens
) {
    InferenceRequest request = createRequest(prompt, max_tokens, temperature, top_p);
    auto response = wrapper_->generate(request);
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
    return wrapper_->embed(text);
}

std::vector<std::vector<float>> EmbeddedLLM::embedBatch(const std::vector<std::string>& texts) {
    std::vector<std::vector<float>> embeddings;
    embeddings.reserve(texts.size());
    
    for (const auto& text : texts) {
        embeddings.push_back(wrapper_->embed(text));
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
    
    auto response = wrapper_->generate(request);
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
    auto response = wrapper_->generate(request);
    return LlamaWrapper::formatAsMCPResponse(response);
}

json EmbeddedLLM::generateAsJsonMarkdown(const std::string& prompt, int max_tokens) {
    InferenceRequest request = createRequest(prompt, max_tokens);
    auto response = wrapper_->generate(request);
    return LlamaWrapper::formatAsJsonMarkdown(response);
}

InferenceResponse EmbeddedLLM::generateFull(const InferenceRequest& request) {
    return wrapper_->generate(request);
}

// ═══════════════════════════════════════════════════════════
// Utility methods
// ═══════════════════════════════════════════════════════════

bool EmbeddedLLM::isReady() const {
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
    // TODO: Implement cache clearing if caching is enabled
    spdlog::info("Cache clearing requested");
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
