/**
 * @file llm_meta_analyzer.cpp
 * @brief Implementation of LLM Meta Analyzer base class
 */

#include "rag/llm_meta_analyzer.h"
#include "utils/logger.h"
#include <sstream>
#include <regex>
#include <algorithm>

namespace themis::rag {

// Private implementation
struct LLMMetaAnalyzer::Impl {
    AnalysisConfig config;
    std::unordered_map<std::string, AnalysisResult> cache;
    
    // Metrics
    size_t total_calls = 0;
    size_t cache_hits = 0;
    size_t cache_misses = 0;
};

void LLMMetaAnalyzer::loadConfig(const AnalysisConfig& config) {
    if (!impl_) {
        impl_ = std::make_unique<Impl>();
    }
    impl_->config = config;
    THEMIS_INFO("LLMMetaAnalyzer config loaded: model={}, use_cot={}", 
                config.judge_model, config.use_chain_of_thought);
}

LLMMetaAnalyzer::AnalysisConfig LLMMetaAnalyzer::getConfig() const {
    if (!impl_) {
        return AnalysisConfig{};
    }
    return impl_->config;
}

void LLMMetaAnalyzer::clearCache() {
    if (impl_) {
        impl_->cache.clear();
        THEMIS_DEBUG("LLMMetaAnalyzer cache cleared");
    }
}

std::string LLMMetaAnalyzer::buildPrompt(
    const std::string& task_description,
    const std::string& input_text,
    const std::vector<std::string>& criteria
) {
    std::ostringstream prompt;
    
    prompt << "Task: " << task_description << "\n\n";
    prompt << "Input:\n" << input_text << "\n\n";
    
    if (!criteria.empty()) {
        prompt << "Evaluation Criteria:\n";
        for (size_t i = 0; i < criteria.size(); ++i) {
            prompt << (i + 1) << ". " << criteria[i] << "\n";
        }
        prompt << "\n";
    }
    
    prompt << "Please provide your analysis and a score between 0.0 and 1.0.\n";
    
    return prompt.str();
}

std::string LLMMetaAnalyzer::buildPromptWithCoT(
    const std::string& task_description,
    const std::string& input_text,
    const std::vector<std::string>& criteria,
    const std::vector<std::string>& examples
) {
    std::ostringstream prompt;
    
    prompt << "Task: " << task_description << "\n\n";
    
    if (!examples.empty()) {
        prompt << "Examples:\n";
        for (const auto& example : examples) {
            prompt << example << "\n";
        }
        prompt << "\n";
    }
    
    prompt << "Input:\n" << input_text << "\n\n";
    
    if (!criteria.empty()) {
        prompt << "Evaluation Criteria:\n";
        for (size_t i = 0; i < criteria.size(); ++i) {
            prompt << (i + 1) << ". " << criteria[i] << "\n";
        }
        prompt << "\n";
    }
    
    prompt << "Think step-by-step:\n";
    prompt << "1. First, analyze the input carefully\n";
    prompt << "2. Consider each criterion\n";
    prompt << "3. Provide reasoning for your evaluation\n";
    prompt << "4. Give a final score between 0.0 and 1.0\n\n";
    prompt << "Format your response as:\n";
    prompt << "Reasoning: [your analysis]\n";
    prompt << "Score: [0.0-1.0]\n";
    
    return prompt.str();
}

LLMMetaAnalyzer::AnalysisResult LLMMetaAnalyzer::parseResponse(
    const std::string& llm_response
) {
    AnalysisResult result;
    result.raw_response = llm_response;
    
    // Extract score
    double score = parseScore(llm_response);
    result.confidence = score;
    result.detection_positive = (score >= 0.5);
    
    // Extract reasoning
    result.reasoning = extractReasoning(llm_response);
    
    return result;
}

double LLMMetaAnalyzer::parseScore(
    const std::string& response,
    const std::string& dimension
) {
    (void)dimension;  // Placeholder until dimension-specific scoring is used
    // Try multiple patterns to extract score
    std::vector<std::regex> patterns = {
        std::regex("(?:Score|score):\\s*([0-9]*\\.?[0-9]+)"),
        std::regex("(?:Rating|rating):\\s*([0-9]*\\.?[0-9]+)"),
        std::regex("([0-9]*\\.?[0-9]+)\\s*/\\s*1\\.?0?"),
        std::regex("([0-9]*\\.?[0-9]+)\\s*out of\\s*1")
    };
    
    for (const auto& pattern : patterns) {
        std::smatch match;
        if (std::regex_search(response, match, pattern)) {
            try {
                double score = std::stod(match[1].str());
                // Normalize to 0-1 range
                if (score > 1.0 && score <= 10.0) {
                    score /= 10.0;
                } else if (score > 10.0 && score <= 100.0) {
                    score /= 100.0;
                }
                return std::clamp(score, 0.0, 1.0);
            } catch (...) {
                // Continue to next pattern
            }
        }
    }
    
    // Default to medium confidence if no score found
    THEMIS_WARN("Could not parse score from LLM response, using default 0.5");
    return 0.5;
}

std::string LLMMetaAnalyzer::extractReasoning(const std::string& response) {
    // Try to extract reasoning section
    std::regex reasoning_pattern(
        "(?:Reasoning|reasoning|Analysis|analysis):\\s*(.+?)(?:Score|score|Rating|rating|$)",
        std::regex::ECMAScript | std::regex::dotall);
    std::smatch match;
    
    if (std::regex_search(response, match, reasoning_pattern)) {
        std::string reasoning = match[1].str();
        // Trim whitespace
        reasoning.erase(0, reasoning.find_first_not_of(" \n\r\t"));
        reasoning.erase(reasoning.find_last_not_of(" \n\r\t") + 1);
        return reasoning;
    }
    
    // If no specific reasoning section, return first paragraph
    size_t first_newline = response.find("\n\n");
    if (first_newline != std::string::npos) {
        return response.substr(0, first_newline);
    }
    
    return response.substr(0, std::min(response.size(), size_t(200)));
}

std::string LLMMetaAnalyzer::callLLM(const std::string& prompt) {
    // TODO: Implement actual LLM call
    // This is a placeholder implementation that returns a hardcoded response.
    // NOTE: This placeholder affects test reliability - tests may pass with
    // unrealistic scores until actual LLM integration is complete.
    // Real implementation should call an actual LLM service (e.g., llama.cpp)
    THEMIS_DEBUG("LLM call with prompt length: {}", prompt.size());
    
    if (impl_) {
        impl_->total_calls++;
    }
    
    // Placeholder response - replace with actual LLM inference
    return "Reasoning: The input has been analyzed according to criteria.\nScore: 0.75";
}

void LLMMetaAnalyzer::exportMetrics(std::unordered_map<std::string, double>& metrics) {
    if (impl_) {
        metrics["llm_total_calls"] = static_cast<double>(impl_->total_calls);
        metrics["llm_cache_hits"] = static_cast<double>(impl_->cache_hits);
        metrics["llm_cache_misses"] = static_cast<double>(impl_->cache_misses);
        
        if (impl_->total_calls > 0) {
            metrics["llm_cache_hit_rate"] = 
                static_cast<double>(impl_->cache_hits) / impl_->total_calls;
        }
    }
}

std::string LLMMetaAnalyzer::computeCacheKey(const std::string& input) {
    // Simple hash-based cache key
    // TODO: Consider using a better hash function
    std::hash<std::string> hasher;
    return std::to_string(hasher(input));
}

} // namespace themis::rag
