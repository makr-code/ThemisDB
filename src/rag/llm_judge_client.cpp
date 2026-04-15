/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            llm_judge_client.cpp                               ║
  Version:         0.0.43                                             ║
  Last Modified:   2026-04-15 04:19:03                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     348                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file llm_judge_client.cpp
 * @brief LLM Judge Client - Connects prompts to InferenceEngineEnhanced
 * 
 * This client bridges RAG Judge evaluations to the LLM inference engine,
 * enabling automated evaluation with proper caching and batching.
 */

#include "rag/llm_judge_client.h"
#include "llm/inference_engine_enhanced.h"
#include "utils/logger.h"
#include <nlohmann/json.hpp>
#include <sstream>
#include <iomanip>
#include <atomic>

using json = nlohmann::json;

namespace themis::rag::judge {

// ═══════════════════════════════════════════════════════════
// LLMJudgeClient Implementation
// ═══════════════════════════════════════════════════════════

struct LLMJudgeClient::Impl {
    Config config;
    std::shared_ptr<llm::InferenceEngineEnhanced> inference_engine;
    std::string model_id;
    
    Impl(const Config& cfg) : config(cfg) {
        // Initialize inference engine with appropriate config
        llm::InferenceEngineEnhanced::Config engine_config;
        engine_config.enable_context_caching = config.enable_caching;
        engine_config.enable_batch_processing = config.enable_batching;
        engine_config.max_batch_size = config.batch_size;
        engine_config.batch_timeout_ms = config.batch_timeout_ms;
        
        inference_engine = std::make_shared<llm::InferenceEngineEnhanced>(engine_config);
        inference_engine->start();
        
        THEMIS_INFO("LLMJudgeClient initialized with model: {}", config.model_name);
    }
    
    ~Impl() {
        if (inference_engine) {
            inference_engine->shutdown();
        }
    }
};

LLMJudgeClient::LLMJudgeClient()
    : LLMJudgeClient(Config{}) {
}

LLMJudgeClient::LLMJudgeClient(const Config& config)
    : impl_(std::make_unique<Impl>(config)) {
}

LLMJudgeClient::~LLMJudgeClient() = default;

std::string LLMJudgeClient::evaluate(const std::string& prompt) {
    auto start_time = std::chrono::steady_clock::now();
    
    try {
        // Create inference request
        llm::InferenceEngineEnhanced::EnhancedInferenceRequest request;
        request.base_request.prompt = prompt;
        request.base_request.max_tokens = impl_->config.max_tokens;
        request.base_request.temperature = impl_->config.temperature;
        request.base_request.top_p = 0.95;
        request.base_request.stop_sequences = impl_->config.stop_sequences;
        
        request.priority = impl_->config.priority;
        request.timeout = std::chrono::milliseconds(impl_->config.timeout_ms);
        request.allow_caching = impl_->config.enable_caching;
        request.preferred_model_id = impl_->config.model_name;
        request.request_id = generateRequestId();
        request.submitted_at = std::chrono::steady_clock::now();
        
        // Submit request and wait for response
        auto handle = impl_->inference_engine->submit(request);
        auto response = handle.get();
        
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            end_time - start_time);
        
        THEMIS_DEBUG("LLM evaluation completed in {}ms", duration.count());
        
        return response.text;
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("LLM evaluation failed: {}", e.what());
        throw;
    }
}

std::vector<std::string> LLMJudgeClient::evaluateBatch(
    const std::vector<std::string>& prompts
) {
    std::vector<std::string> results;
    results.reserve(prompts.size());
    
    if (!impl_->config.enable_batching || prompts.size() == 1) {
        // Sequential processing
        for (const auto& prompt : prompts) {
            results.push_back(evaluate(prompt));
        }
        return results;
    }
    
    auto start_time = std::chrono::steady_clock::now();
    
    try {
        // Submit all requests
        std::vector<llm::InferenceHandle> handles;
        handles.reserve(prompts.size());
        
        for (const auto& prompt : prompts) {
            llm::InferenceEngineEnhanced::EnhancedInferenceRequest request;
            request.base_request.prompt = prompt;
            request.base_request.max_tokens = impl_->config.max_tokens;
            request.base_request.temperature = impl_->config.temperature;
            request.base_request.top_p = 0.95;
            request.base_request.stop_sequences = impl_->config.stop_sequences;
            
            request.priority = impl_->config.priority;
            request.timeout = std::chrono::milliseconds(impl_->config.timeout_ms);
            request.allow_caching = impl_->config.enable_caching;
            request.preferred_model_id = impl_->config.model_name;
            request.request_id = generateRequestId();
            request.submitted_at = std::chrono::steady_clock::now();
            
            handles.push_back(impl_->inference_engine->submit(request));
        }
        
        // Wait for all responses
        for (auto& handle : handles) {
            auto response = handle.get();
            results.push_back(response.text);
        }
        
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            end_time - start_time);
        
        THEMIS_INFO("Batch evaluation of {} prompts completed in {}ms",
                   prompts.size(), duration.count());
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("Batch evaluation failed: {}", e.what());
        throw;
    }
    
    return results;
}

EvaluationResponse LLMJudgeClient::evaluateDimension(
    const std::string& query,
    const std::string& answer,
    const std::vector<std::pair<std::string, std::string>>& documents,
    const std::string& dimension
) {
    // Build evaluation prompt
    std::ostringstream prompt;
    
    prompt << "You are evaluating a generated answer for a RAG system.\n\n";
    prompt << "Query: " << query << "\n\n";
    prompt << "Generated Answer: " << answer << "\n\n";
    
    if (!documents.empty()) {
        prompt << "Retrieved Documents:\n";
        size_t doc_count = std::min(documents.size(), size_t(5));
        for (size_t i = 0; i < doc_count; i++) {
            prompt << "Document " << (i+1) << " (ID: " << documents[i].first << "):\n";
            prompt << documents[i].second.substr(0, 500);
            if (documents[i].second.size() > 500) {
                prompt << "...";
            }
            prompt << "\n\n";
        }
    }
    
    prompt << "Evaluate the answer for: " << dimension << "\n\n";
    prompt << "Provide your evaluation in the following JSON format:\n";
    prompt << "{\n";
    prompt << "  \"score\": <float 0.0-1.0>,\n";
    prompt << "  \"reasoning\": \"<explanation>\",\n";
    prompt << "  \"confidence\": <float 0.0-1.0>\n";
    prompt << "}\n";
    
    auto start_time = std::chrono::steady_clock::now();
    
    try {
        std::string llm_response = evaluate(prompt.str());
        
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            end_time - start_time);
        
        // Parse JSON response
        EvaluationResponse response;
        response.raw_response = llm_response;
        response.evaluation_time = duration;
        
        // Simple JSON parsing (in production, use a proper JSON library)
        parseEvaluationResponse(llm_response, response);
        
        return response;
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("Dimension evaluation failed: {}", e.what());
        
        EvaluationResponse error_response;
        error_response.score = 0.0;
        error_response.reasoning = std::string("Evaluation failed: ") + e.what();
        error_response.confidence = 0.0;
        return error_response;
    }
}

void LLMJudgeClient::setInferenceEngine(
    std::shared_ptr<llm::InferenceEngineEnhanced> engine
) {
    impl_->inference_engine = engine;
    THEMIS_INFO("Custom inference engine set");
}

void LLMJudgeClient::registerModel(
    const std::string& model_id,
    std::shared_ptr<llm::ILLMPlugin> plugin
) {
    if (impl_->inference_engine) {
        impl_->inference_engine->registerModel(model_id, plugin);
        impl_->model_id = model_id;
        THEMIS_INFO("Model registered: {}", model_id);
    }
}

LLMJudgeClient::Config LLMJudgeClient::getConfig() const {
    return impl_->config;
}

void LLMJudgeClient::setConfig(const Config& config) {
    impl_->config = config;
}

std::string LLMJudgeClient::generateRequestId() {
    static std::atomic<uint64_t> counter{0};
    auto count = counter.fetch_add(1);
    
    std::ostringstream oss;
    oss << "llm_judge_" << std::setfill('0') << std::setw(10) << count;
    return oss.str();
}

void LLMJudgeClient::parseEvaluationResponse(
    const std::string& response,
    EvaluationResponse& parsed
) {
    // Use nlohmann/json for proper JSON parsing
    try {
        json j = json::parse(response);
        
        if (j.contains("score")) {
            parsed.score = j["score"].get<double>();
        } else {
            parsed.score = 0.5;  // Default
        }
        
        if (j.contains("reasoning")) {
            parsed.reasoning = j["reasoning"].get<std::string>();
        }
        
        if (j.contains("confidence")) {
            parsed.confidence = j["confidence"].get<double>();
        } else {
            parsed.confidence = 0.5;  // Default
        }
        
    } catch (const json::exception& e) {
        // Fallback to simple parsing for non-JSON responses
        // Look for score
    size_t score_pos = response.find("\"score\"");
    if (score_pos != std::string::npos) {
        size_t colon_pos = response.find(":", score_pos);
        if (colon_pos != std::string::npos) {
            size_t comma_pos = response.find(",", colon_pos);
            if (comma_pos == std::string::npos) {
                comma_pos = response.find("}", colon_pos);
            }
            if (comma_pos != std::string::npos) {
                std::string score_str = response.substr(colon_pos + 1, 
                                                       comma_pos - colon_pos - 1);
                // Trim whitespace
                score_str.erase(0, score_str.find_first_not_of(" \t\n\r"));
                score_str.erase(score_str.find_last_not_of(" \t\n\r,") + 1);
                
                try {
                    parsed.score = std::stod(score_str);
                } catch (...) {
                    parsed.score = 0.5; // Default
                }
            }
        }
    }
    
    // Look for reasoning
    size_t reasoning_pos = response.find("\"reasoning\"");
    if (reasoning_pos != std::string::npos) {
        size_t colon_pos = response.find(":", reasoning_pos);
        if (colon_pos != std::string::npos) {
            size_t quote1 = response.find("\"", colon_pos);
            if (quote1 != std::string::npos) {
                size_t quote2 = response.find("\"", quote1 + 1);
                if (quote2 != std::string::npos) {
                    parsed.reasoning = response.substr(quote1 + 1, 
                                                      quote2 - quote1 - 1);
                }
            }
        }
    }
    }
}

} // namespace themis::rag::judge
