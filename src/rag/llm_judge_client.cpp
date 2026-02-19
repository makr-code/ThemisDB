/**
 * @file llm_judge_client.cpp
 * @brief Implementation of LLM Judge client for InferenceEngineEnhanced
 */

#include "rag/llm_judge_client.h"
#include "llm/inference_engine_enhanced.h"
#include "utils/logger.h"
#include <chrono>
#include <sstream>
#include <algorithm>

namespace themis::rag::judge {

using namespace themis::llm;

// ═══════════════════════════════════════════════════════════
// Implementation
// ═══════════════════════════════════════════════════════════

struct LLMJudgeClient::Impl {
    Config config;
    std::shared_ptr<InferenceEngineEnhanced> engine;
    bool owns_engine = false;  // Track if we created the engine
    
    Impl(const Config& cfg) : config(cfg) {}
    
    Impl(std::shared_ptr<InferenceEngineEnhanced> eng) 
        : engine(eng), owns_engine(false) {}
    
    /**
     * @brief Convert client config to inference request
     */
    InferenceEngineEnhanced::EnhancedInferenceRequest createRequest(
        const std::string& prompt,
        const Config& cfg
    ) {
        InferenceEngineEnhanced::EnhancedInferenceRequest req;
        
        // Base request settings
        req.base_request.prompt = prompt;
        req.base_request.max_tokens = cfg.max_tokens;
        req.base_request.temperature = cfg.temperature;
        req.base_request.top_p = 0.95;
        req.base_request.top_k = 40;
        
        // Enhanced settings
        req.priority = cfg.priority;
        req.timeout = std::chrono::milliseconds(cfg.request_timeout_ms);
        req.allow_caching = cfg.enable_caching;
        req.preferred_model_id = cfg.preferred_model_id;
        
        // Generate unique request ID
        static std::atomic<uint64_t> request_counter{0};
        req.request_id = "llm_judge_" + std::to_string(request_counter++);
        req.submitted_at = std::chrono::steady_clock::now();
        
        return req;
    }
    
    /**
     * @brief Convert inference response to judge response
     */
    LLMJudgeResponse convertResponse(
        const InferenceResponse& inf_response,
        double generation_time_ms
    ) {
        LLMJudgeResponse response;
        response.text = inf_response.text;
        response.generation_time_ms = generation_time_ms;
        response.from_cache = false;  // TODO: Get from response metadata
        response.model_id = "default";  // TODO: Get from response metadata
        
        // Convert token probabilities if available
        // Note: InferenceResponse would need to be extended to include this
        // For now, leave empty as it's not in the base struct
        
        return response;
    }
    
    /**
     * @brief Extract probabilities for specific tokens (stub for now)
     * 
     * Full implementation requires:
     * 1. Extending InferenceResponse to include logits
     * 2. Finding token IDs for target_tokens in vocabulary
     * 3. Computing softmax over vocabulary
     * 4. Extracting probabilities for target token IDs
     */
    std::vector<double> extractProbs(
        const InferenceResponse& response,
        const std::vector<std::string>& target_tokens
    ) {
        // Stub implementation - returns uniform distribution
        // In production, this would:
        // 1. Get logits from response
        // 2. Look up token IDs for target_tokens
        // 3. Apply softmax
        // 4. Return probabilities for those tokens
        
        std::vector<double> probs(target_tokens.size());
        double uniform_prob = 1.0 / target_tokens.size();
        std::fill(probs.begin(), probs.end(), uniform_prob);
        
        THEMIS_DEBUG("Token probability extraction not yet implemented, returning uniform distribution");
        return probs;
    }
};

// ═══════════════════════════════════════════════════════════
// Constructor & Destructor
// ═══════════════════════════════════════════════════════════

LLMJudgeClient::LLMJudgeClient(std::shared_ptr<InferenceEngineEnhanced> engine)
    : impl_(std::make_unique<Impl>(engine)) {
    THEMIS_INFO("LLMJudgeClient initialized with external engine");
}

LLMJudgeClient::LLMJudgeClient(const Config& config)
    : impl_(std::make_unique<Impl>(config)) {
    // Don't create engine here - let it be injected
    // This allows for mock testing and deferred initialization
    THEMIS_INFO("LLMJudgeClient initialized (engine not set)");
}

LLMJudgeClient::~LLMJudgeClient() = default;

// ═══════════════════════════════════════════════════════════
// Public Methods
// ═══════════════════════════════════════════════════════════

LLMJudgeResponse LLMJudgeClient::generate(const std::string& prompt) {
    return generate(prompt, impl_->config);
}

LLMJudgeResponse LLMJudgeClient::generate(
    const std::string& prompt,
    const Config& config
) {
    if (!impl_->engine) {
        THEMIS_WARN("No inference engine set, returning empty response");
        LLMJudgeResponse response;
        response.text = "{}";  // Empty JSON for compatibility
        response.generation_time_ms = 0.0;
        response.from_cache = false;
        return response;
    }
    
    auto start_time = std::chrono::steady_clock::now();
    
    try {
        // Create request
        auto request = impl_->createRequest(prompt, config);
        
        // Submit to engine
        auto handle = impl_->engine->submit(request);
        
        // Wait for result with timeout
        auto inf_response = handle.get_result();
        
        // Calculate generation time
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
            end_time - start_time
        );
        double generation_time_ms = duration.count() / 1000.0;
        
        // Convert and return
        auto response = impl_->convertResponse(inf_response, generation_time_ms);
        
        THEMIS_DEBUG("LLM generation completed in {:.2f}ms", generation_time_ms);
        return response;
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("LLM generation failed: {}", e.what());
        
        // Return error response
        LLMJudgeResponse response;
        response.text = "{}";
        response.generation_time_ms = 0.0;
        response.from_cache = false;
        return response;
    }
}

std::vector<LLMJudgeResponse> LLMJudgeClient::generateMultiple(
    const std::string& prompt,
    size_t num_samples,
    const Config& config
) {
    std::vector<LLMJudgeResponse> responses;
    responses.reserve(num_samples);
    
    // Generate each sample with slightly different temperature for diversity
    for (size_t i = 0; i < num_samples; i++) {
        Config sample_config = config;
        // Add small temperature variation for diversity
        sample_config.temperature = config.temperature + (i * 0.05);
        
        auto response = generate(prompt, sample_config);
        responses.push_back(response);
    }
    
    THEMIS_DEBUG("Generated {} samples", responses.size());
    return responses;
}

std::vector<double> LLMJudgeClient::extractTokenProbabilities(
    const std::string& prompt,
    const std::vector<std::string>& target_tokens,
    const Config& config
) {
    if (!impl_->engine) {
        THEMIS_WARN("No inference engine set, returning uniform distribution");
        std::vector<double> probs(target_tokens.size());
        double uniform = 1.0 / target_tokens.size();
        std::fill(probs.begin(), probs.end(), uniform);
        return probs;
    }
    
    // For now, use generate and stub extraction
    // Full implementation requires InferenceResponse to include logits
    Config prob_config = config;
    prob_config.extract_token_probs = true;
    
    auto response = generate(prompt, prob_config);
    
    // Stub: Return uniform distribution
    // TODO: Extract actual probabilities from response
    return std::vector<double>(target_tokens.size(), 1.0 / target_tokens.size());
}

void LLMJudgeClient::setEngine(std::shared_ptr<InferenceEngineEnhanced> engine) {
    impl_->engine = engine;
    impl_->owns_engine = false;
    THEMIS_INFO("Inference engine set");
}

std::shared_ptr<InferenceEngineEnhanced> LLMJudgeClient::getEngine() const {
    return impl_->engine;
}

bool LLMJudgeClient::isReady() const {
    return impl_->engine != nullptr;
}

void LLMJudgeClient::setConfig(const Config& config) {
    impl_->config = config;
}

LLMJudgeClient::Config LLMJudgeClient::getConfig() const {
    return impl_->config;
}

void LLMJudgeClient::clearCache() {
    if (impl_->engine) {
        impl_->engine->clearCache();
        THEMIS_INFO("Inference cache cleared");
    }
}

void LLMJudgeClient::prewarmCache(const std::vector<std::string>& prompts) {
    if (impl_->engine) {
        impl_->engine->prewarmCache(prompts);
        THEMIS_INFO("Cache prewarmed with {} prompts", prompts.size());
    }
}

} // namespace themis::rag::judge
