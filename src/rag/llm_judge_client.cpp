/**
 * @file llm_judge_client.cpp
 * @brief Implementation of LLM Judge Client
 */

#include "rag/llm_judge_client.h"
#include "utils/logger.h"
#include <nlohmann/json.hpp>
#include <regex>
#include <sstream>
#include <algorithm>

namespace themis::rag::judge {

using json = nlohmann::json;

// ═══════════════════════════════════════════════════════════
// Implementation
// ═══════════════════════════════════════════════════════════

struct LLMJudgeClient::Impl {
    Config config;
    std::shared_ptr<llm::InferenceEngineEnhanced> inference_engine;
    
    // Cache for identical prompts
    std::unordered_map<std::string, LLMJudgeResponse> cache;
    std::mutex cache_mutex;
    
    Impl(const Config& cfg, std::shared_ptr<llm::InferenceEngineEnhanced> engine)
        : config(cfg), inference_engine(engine) {
        if (!inference_engine) {
            throw std::invalid_argument("LLMJudgeClient requires valid inference engine");
        }
    }
    
    std::string generateCacheKey(const std::string& prompt, EvaluationDimension dimension) {
        return std::to_string(static_cast<int>(dimension)) + ":" + prompt;
    }
};

// ═══════════════════════════════════════════════════════════
// Constructor & Destructor
// ═══════════════════════════════════════════════════════════

LLMJudgeClient::LLMJudgeClient(
    const Config& config,
    std::shared_ptr<llm::InferenceEngineEnhanced> inference_engine
) : impl_(std::make_unique<Impl>(config, inference_engine)) {
    THEMIS_INFO("LLMJudgeClient initialized with model: {}", config.model_id);
}

LLMJudgeClient::~LLMJudgeClient() = default;

// ═══════════════════════════════════════════════════════════
// Public Methods
// ═══════════════════════════════════════════════════════════

LLMJudgeResponse LLMJudgeClient::evaluate(
    const std::string& prompt,
    EvaluationDimension dimension
) {
    auto start_time = std::chrono::steady_clock::now();
    
    // Check cache
    if (impl_->config.enable_caching) {
        std::lock_guard<std::mutex> lock(impl_->cache_mutex);
        auto cache_key = impl_->generateCacheKey(prompt, dimension);
        auto it = impl_->cache.find(cache_key);
        if (it != impl_->cache.end()) {
            THEMIS_DEBUG("LLM Judge cache hit for dimension {}", static_cast<int>(dimension));
            return it->second;
        }
    }
    
    // Format prompt for inference
    std::string formatted_prompt = formatPromptForInference(prompt, dimension);
    
    // Create inference request
    llm::InferenceEngineEnhanced::EnhancedInferenceRequest request;
    request.base_request.prompt = formatted_prompt;
    request.base_request.max_tokens = impl_->config.max_tokens;
    request.base_request.temperature = impl_->config.temperature;
    request.base_request.stop_sequences = {"\n\n", "---"};
    request.preferred_model_id = impl_->config.model_id;
    request.timeout = std::chrono::milliseconds(impl_->config.timeout_ms);
    request.allow_caching = impl_->config.enable_caching;
    
    // Submit request with retries
    LLMJudgeResponse response;
    int attempts = 0;
    
    while (attempts < impl_->config.max_retries) {
        try {
            auto handle = impl_->inference_engine->submit(request);
            auto inference_result = handle.wait();
            
            if (inference_result.success) {
                response = parseResponse(inference_result.generated_text, dimension);
                response.success = true;
                response.latency = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - start_time
                );
                
                // Cache successful response
                if (impl_->config.enable_caching) {
                    std::lock_guard<std::mutex> lock(impl_->cache_mutex);
                    auto cache_key = impl_->generateCacheKey(prompt, dimension);
                    impl_->cache[cache_key] = response;
                }
                
                return response;
            } else {
                THEMIS_WARN("Inference failed (attempt {}/{}): {}", 
                           attempts + 1, impl_->config.max_retries, 
                           inference_result.error_message);
            }
        } catch (const std::exception& e) {
            THEMIS_ERROR("Exception during inference (attempt {}/{}): {}", 
                        attempts + 1, impl_->config.max_retries, e.what());
        }
        
        attempts++;
        if (attempts < impl_->config.max_retries) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100 * attempts));
        }
    }
    
    // All retries failed
    return handleInferenceError("All retry attempts exhausted", dimension);
}

LLMJudgeResponse LLMJudgeClient::evaluateWithConfig(
    const std::string& prompt,
    EvaluationDimension dimension,
    const llm::InferenceEngineEnhanced::EnhancedInferenceRequest& request_config
) {
    auto start_time = std::chrono::steady_clock::now();
    
    try {
        auto handle = impl_->inference_engine->submit(request_config);
        auto inference_result = handle.wait();
        
        if (inference_result.success) {
            auto response = parseResponse(inference_result.generated_text, dimension);
            response.success = true;
            response.latency = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start_time
            );
            return response;
        } else {
            return handleInferenceError(inference_result.error_message, dimension);
        }
    } catch (const std::exception& e) {
        return handleInferenceError(e.what(), dimension);
    }
}

// ═══════════════════════════════════════════════════════════
// Static Parsing Methods
// ═══════════════════════════════════════════════════════════

LLMJudgeResponse LLMJudgeClient::parseResponse(
    const std::string& response,
    EvaluationDimension dimension
) {
    LLMJudgeResponse result;
    result.success = false;
    
    // Try JSON parsing first
    try {
        auto j = json::parse(response);
        
        // Extract score
        if (j.contains("score")) {
            result.score = j["score"].get<double>();
            // Normalize to 0-1 if in 1-5 range
            if (result.score > 1.0 && result.score <= 5.0) {
                result.score = (result.score - 1.0) / 4.0;
            }
        }
        
        // Extract reasoning
        if (j.contains("reasoning")) {
            result.reasoning = j["reasoning"].get<std::string>();
        } else if (j.contains("explanation")) {
            result.reasoning = j["explanation"].get<std::string>();
        }
        
        // Extract confidence
        if (j.contains("confidence")) {
            result.confidence = j["confidence"].get<double>();
        } else {
            result.confidence = 0.8;  // Default
        }
        
        result.success = true;
        return result;
    } catch (const json::exception&) {
        // Fall through to text parsing
    }
    
    // Text-based parsing
    result.score = extractScore(response);
    result.reasoning = extractReasoning(response);
    result.confidence = extractConfidence(response);
    result.success = (result.score >= 0.0);
    
    return result;
}

double LLMJudgeClient::extractScore(const std::string& response) {
    // Try to find score in various formats
    std::vector<std::regex> score_patterns = {
        std::regex(R"(score[:\s]+([0-9.]+))", std::regex::icase),
        std::regex(R"(rating[:\s]+([0-9.]+))", std::regex::icase),
        std::regex(R"(([0-9])\s*/\s*5)"),
        std::regex(R"(([0-9.]+)\s*/\s*10)"),
        std::regex(R"(^([0-9.]+)$)")  // Just a number
    };
    
    for (const auto& pattern : score_patterns) {
        std::smatch match;
        if (std::regex_search(response, match, pattern)) {
            try {
                double score = std::stod(match[1].str());
                
                // Normalize based on likely scale
                if (score <= 1.0) {
                    return score;  // Already normalized
                } else if (score <= 5.0) {
                    return (score - 1.0) / 4.0;  // 1-5 scale
                } else if (score <= 10.0) {
                    return score / 10.0;  // 0-10 scale
                } else if (score <= 100.0) {
                    return score / 100.0;  // 0-100 scale
                }
            } catch (...) {
                continue;
            }
        }
    }
    
    // Default to middle score if parsing fails
    THEMIS_WARN("Failed to extract score from response, using default 0.5");
    return 0.5;
}

std::string LLMJudgeClient::extractReasoning(const std::string& response) {
    // Try to extract reasoning section
    std::regex reasoning_pattern(R"(reasoning[:\s]+(.+?)(?:\n\n|score|rating|$))", 
                                 std::regex::icase);
    std::smatch match;
    
    if (std::regex_search(response, match, reasoning_pattern)) {
        std::string reasoning = match[1].str();
        // Trim whitespace
        reasoning.erase(0, reasoning.find_first_not_of(" \t\n\r"));
        reasoning.erase(reasoning.find_last_not_of(" \t\n\r") + 1);
        return reasoning;
    }
    
    // If no explicit reasoning section, take first paragraph
    size_t first_break = response.find("\n\n");
    if (first_break != std::string::npos) {
        return response.substr(0, first_break);
    }
    
    return response;
}

double LLMJudgeClient::extractConfidence(
    const std::string& response,
    const std::vector<double>& token_probs
) {
    // If token probabilities provided, use entropy-based confidence
    if (!token_probs.empty()) {
        double entropy = 0.0;
        for (double p : token_probs) {
            if (p > 0.0) {
                entropy -= p * std::log2(p);
            }
        }
        double max_entropy = std::log2(static_cast<double>(token_probs.size()));
        return 1.0 - (entropy / max_entropy);
    }
    
    // Try to extract confidence from text
    std::regex conf_pattern(R"(confidence[:\s]+([0-9.]+))", std::regex::icase);
    std::smatch match;
    
    if (std::regex_search(response, match, conf_pattern)) {
        try {
            double conf = std::stod(match[1].str());
            if (conf <= 1.0) {
                return conf;
            } else if (conf <= 100.0) {
                return conf / 100.0;
            }
        } catch (...) {
            // Fall through
        }
    }
    
    // Default confidence based on response quality indicators
    int quality_indicators = 0;
    if (response.find("clear") != std::string::npos) quality_indicators++;
    if (response.find("strong") != std::string::npos) quality_indicators++;
    if (response.find("evident") != std::string::npos) quality_indicators++;
    if (response.find("however") != std::string::npos) quality_indicators--;
    if (response.find("uncertain") != std::string::npos) quality_indicators--;
    
    return std::max(0.5, std::min(0.9, 0.7 + (quality_indicators * 0.1)));
}

// ═══════════════════════════════════════════════════════════
// Configuration Management
// ═══════════════════════════════════════════════════════════

LLMJudgeClient::Config LLMJudgeClient::getConfig() const {
    return impl_->config;
}

void LLMJudgeClient::setConfig(const Config& config) {
    impl_->config = config;
}

// ═══════════════════════════════════════════════════════════
// Private Helpers
// ═══════════════════════════════════════════════════════════

std::string LLMJudgeClient::formatPromptForInference(
    const std::string& prompt,
    EvaluationDimension dimension
) {
    std::ostringstream formatted;
    
    // Add instruction wrapper for better LLM understanding
    formatted << "You are an expert evaluator. Analyze the following and provide a structured response.\n\n";
    formatted << prompt;
    formatted << "\n\nProvide your response in this format:\n";
    formatted << "Reasoning: <your detailed reasoning>\n";
    formatted << "Score: <a score from 1-5>\n";
    formatted << "Confidence: <your confidence level 0-1>\n";
    
    return formatted.str();
}

LLMJudgeResponse LLMJudgeClient::handleInferenceError(
    const std::string& error_msg,
    EvaluationDimension dimension
) {
    LLMJudgeResponse error_response;
    error_response.success = false;
    error_response.error_message = error_msg;
    error_response.score = 0.5;  // Neutral score
    error_response.confidence = 0.0;
    error_response.reasoning = "Evaluation failed: " + error_msg;
    error_response.latency = std::chrono::milliseconds(0);
    
    THEMIS_ERROR("LLM Judge evaluation failed for dimension {}: {}", 
                static_cast<int>(dimension), error_msg);
    
    return error_response;
}

} // namespace themis::rag::judge
