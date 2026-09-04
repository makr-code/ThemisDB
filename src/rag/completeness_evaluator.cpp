/**
 * @file completeness_evaluator.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "rag/completeness_evaluator.h"
#include "rag/llm_judge_integration.h"
#include "rag/response_parser.h"
#include "utils/logger.h"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <set>
#include <sstream>
#include <regex>
#include <mutex>

namespace themis::rag::judge {

using json = nlohmann::json;

struct CompletenessEvaluator::Impl {
    Config config;
    std::unique_ptr<LLMJudgeIntegration> llm_integration;
    ResponseParser parser;
    mutable std::mutex state_mutex;  // Protect shared state access
    
    // Optimization: Extract answer words once to avoid repeated parsing
    // Builds a set of lowercase words from answer for O(log n) lookup
    static std::set<std::string> extractAnswerWords(const std::string& answer) {
        std::set<std::string> words;
        std::string answer_lower = answer;
        std::transform(answer_lower.begin(), answer_lower.end(), 
                      answer_lower.begin(), ::tolower);
        
        std::istringstream stream(answer_lower);
        std::string word = {};
        while (stream >> word) {
            // Remove punctuation from word end
            while (!word.empty() && !std::isalnum(word.back())) {
                word.pop_back();
            }
            if (word.length() > 3) {  // Filter short words
                words.insert(word);
            }
        }
        return words;
    }
    
    // Check if aspect is covered in answer using pre-extracted words
    // Complexity: O(n_terms × log n_words) instead of O(n_terms × n_answer_length)
    bool isAspectCoveredOptimized(const std::string& aspect, 
                                  const std::set<std::string>& answer_words) {
        std::string aspect_lower = aspect;
        std::transform(aspect_lower.begin(), aspect_lower.end(), 
                      aspect_lower.begin(), ::tolower);
        
        std::istringstream stream(aspect_lower);
        std::vector<std::string> key_terms;
        std::string word = {};
        
        while (stream >> word) {
            // Remove punctuation from word end
            while (!word.empty() && !std::isalnum(word.back())) {
                word.pop_back();
            }
            if (word.length() > 3) {  // Filter short words
                key_terms.push_back(word);
            }
        }
        
        if (key_terms.empty()) {
            return false;
        }
        
        // Check how many key terms are present in answer words
        size_t found_count = 0;
        for (const auto& term : key_terms) {
            if (answer_words.count(term)) {
                found_count++;
            }
        }
        
        // Consider covered if majority of key terms are present
        return found_count >= (key_terms.size() * 0.6);
    }
    
    // Check if aspect is covered in answer
    bool isAspectCovered(const std::string& aspect, const std::string& answer) {
        auto answer_words = extractAnswerWords(answer);
        return isAspectCoveredOptimized(aspect, answer_words);
    }
    
    // Calculate coverage score using pre-extracted words
    // Complexity: O(n_terms × log n_words) instead of O(n_terms × n_answer_length)
    double calculateCoverageScoreOptimized(const std::string& aspect,
                                          const std::set<std::string>& answer_words) {
        std::string aspect_lower = aspect;
        std::transform(aspect_lower.begin(), aspect_lower.end(), 
                      aspect_lower.begin(), ::tolower);
        
        std::istringstream stream(aspect_lower);
        std::vector<std::string> key_terms;
        std::string word = {};
        
        while (stream >> word) {
            // Remove punctuation from word end
            while (!word.empty() && !std::isalnum(word.back())) {
                word.pop_back();
            }
            if (word.length() > 3) {
                key_terms.push_back(word);
            }
        }
        
        if (key_terms.empty()) {
            return 0.0;
        }
        
        size_t found_count = 0;
        for (const auto& term : key_terms) {
            if (answer_words.count(term)) {
                found_count++;
            }
        }
        
        return static_cast<double>(found_count) / key_terms.size();
    }
    
    // Calculate coverage score for an aspect
    double calculateCoverageScore(const std::string& aspect, const std::string& answer) {
        auto answer_words = extractAnswerWords(answer);
        return calculateCoverageScoreOptimized(aspect, answer_words);
    }
};

CompletenessEvaluator::CompletenessEvaluator()
    : CompletenessEvaluator(Config{}) {
}

CompletenessEvaluator::CompletenessEvaluator(const Config& config)
    : impl_(std::make_unique<Impl>()) {
    impl_->config = config;
    
    // Initialize LLM integration
    LLMJudgeIntegration::Config llm_config;
    llm_config.model_name = "default";
    llm_config.temperature = 0.3;
    llm_config.max_tokens = 512;
    impl_->llm_integration = std::make_unique<LLMJudgeIntegration>(llm_config);
    
    THEMIS_DEBUG("CompletenessEvaluator initialized");
}

CompletenessEvaluator::~CompletenessEvaluator() = default;

std::vector<QueryAspect> CompletenessEvaluator::extractQueryAspects(const std::string& query) {
    std::vector<QueryAspect> aspects;
    aspects.reserve(10);  // Reasonable estimate for typical queries
    
    if (query.empty()) {
        return aspects;
    }
    
    // Prompt for LLM-based aspect extraction
    std::string prompt = R"(Analyze the following query and extract key aspects that should be covered in a complete answer.
Mark each aspect as required or optional.

Query: )" + query + R"(

Output format:
{
  "aspects": [
    {"text": "aspect1", "required": true},
    {"text": "aspect2", "required": false}
  ]
}

Aspects:)";
    
    try {
        std::string llm_response = impl_->llm_integration->evaluateDimension(
            prompt, EvaluationDimension::COMPLETENESS
        );
        
        // Parse JSON response
        json response_json = impl_->parser.parseJSONResponse(llm_response);
        
        if (response_json.contains("aspects") && response_json["aspects"].is_array()) {
            for (const auto& aspect_json : response_json["aspects"]) {
                if (aspect_json.contains("text")) {
                    QueryAspect aspect;
                    aspect.aspect_text = aspect_json["text"].get<std::string>();
                    aspect.is_required = aspect_json.value("required", true);
                    aspect.is_covered = false;  // Will be determined later
                    aspect.coverage_score = 0.0;
                    aspects.push_back(aspect);
                }
            }
        }

        if (aspects.empty()) {
            QueryAspect aspect;
            aspect.aspect_text = query;
            aspect.is_required = true;
            aspect.is_covered = false;
            aspect.coverage_score = 0.0;
            aspects.push_back(aspect);
        }
    } catch (const std::exception& e) {
        THEMIS_WARN("Aspect extraction failed: {}", e.what());
        
        // Fallback: simple aspect extraction based on question words
        QueryAspect aspect;
        aspect.aspect_text = query;
        aspect.is_required = true;
        aspect.is_covered = false;
        aspect.coverage_score = 0.0;
        aspects.push_back(aspect);
    }
    
    THEMIS_DEBUG("Extracted {} aspects from query", aspects.size());
    return aspects;
}

std::pair<DepthLevel, double> CompletenessEvaluator::assessDepth(
    const std::string& answer,
    [[maybe_unused]] const std::vector<QueryAspect>& aspects
) {
    static_cast<void>(aspects);
    if (answer.empty()) {
        return {DepthLevel::SHALLOW, 0.0};
    }
    
    // Count indicators of depth:
    // - Length (as proxy for detail)
    // - Number of examples (words like "example", "for instance", "such as")
    // - Evidence markers ("because", "therefore", "research shows", "according to")
    // - Multiple sentences per aspect
    
    size_t answer_length = answer.length();
    
    // Example indicators
    std::regex example_regex(R"(\b(example|instance|such as|like|including|e\.g\.|for example)\b)", 
                             std::regex::icase);
    auto examples_begin = std::sregex_iterator(answer.begin(), answer.end(), example_regex);
    auto examples_end = std::sregex_iterator();
    size_t example_count = std::distance(examples_begin, examples_end);
    
    // Evidence indicators
    std::regex evidence_regex(R"(\b(because|therefore|thus|hence|research|study|according to|shows that)\b)",
                              std::regex::icase);
    auto evidence_begin = std::sregex_iterator(answer.begin(), answer.end(), evidence_regex);
    auto evidence_end = std::sregex_iterator();
    size_t evidence_count = std::distance(evidence_begin, evidence_end);
    
    // Sentence count
    std::regex sentence_regex(R"([^.!?]+[.!?])");
    auto sentences_begin = std::sregex_iterator(answer.begin(), answer.end(), sentence_regex);
    auto sentences_end = std::sregex_iterator();
    size_t sentence_count = std::distance(sentences_begin, sentences_end);
    
    // Calculate depth score (0-1)
    double depth_score = 0.0;
    
    // Length component (up to 0.4)
    depth_score += std::min(0.4, answer_length / 1000.0);
    
    // Examples component (up to 0.3)
    depth_score += std::min(0.3, example_count * 0.1);
    
    // Evidence component (up to 0.3)
    depth_score += std::min(0.3, evidence_count * 0.1);
    
    // Ensure in range [0, 1]
    depth_score = std::min(1.0, depth_score);
    
    // Determine depth level
    DepthLevel level = {};
    if (depth_score >= 0.7) {
        level = DepthLevel::DEEP;
    } else if (depth_score >= 0.4) {
        level = DepthLevel::MEDIUM;
    } else {
        level = DepthLevel::SHALLOW;
    }
    
    THEMIS_DEBUG("Depth assessment: level={}, score={:.2f}, examples={}, evidence={}, sentences={}", 
                 static_cast<int>(level), depth_score, example_count, evidence_count, sentence_count);
    
    return {level, depth_score};
}

std::vector<std::string> CompletenessEvaluator::detectMissingInformation(
    [[maybe_unused]] const std::string& answer,
    [[maybe_unused]] const std::string& query,
    const std::vector<QueryAspect>& aspects
) {
    std::vector<std::string> missing_info;
    static_cast<void>(answer);
    static_cast<void>(query);
    
    if (!impl_->config.enable_gap_detection) {
        return missing_info;
    }
    
    // Identify uncovered aspects
    for (const auto& aspect : aspects) {
        if (!aspect.is_covered || aspect.coverage_score < 0.5) {
            missing_info.push_back(aspect.aspect_text);
        }
    }
    
    THEMIS_DEBUG("Detected {} missing information items", missing_info.size());
    return missing_info;
}

CompletenessResult CompletenessEvaluator::evaluate(
    const std::string& answer,
    const std::string& query
) {
    CompletenessResult result = {};
    
    if (answer.empty() || query.empty()) {
        result.completeness_score = 0.0;
        result.explanation = "Empty answer or query.";
        return result;
    }
    
    // Step 1: Extract query aspects
    result.aspects = extractQueryAspects(query);
    result.total_aspects_count = result.aspects.size();
    
    if (result.aspects.empty()) {
        result.completeness_score = 0.5;  // Neutral when no aspects identified
        result.explanation = "No specific aspects identified in query.";
        return result;
    }
    
    // Step 2: Check aspect coverage
    // Optimization: Extract answer words once to avoid repeated parsing in loop
    // This reduces complexity from O(n_aspects × n_terms × n_answer) 
    // to O(n_answer + n_aspects × n_terms × log n_unique_words)
    auto answer_words = impl_->extractAnswerWords(answer);
    
    result.covered_aspects_count = 0;
    double required_coverage = 0.0;
    double optional_coverage = 0.0;
    size_t required_count = 0;
    size_t optional_count = 0;
    
    for (auto& aspect : result.aspects) {
        aspect.is_covered = impl_->isAspectCoveredOptimized(aspect.aspect_text, answer_words);
        aspect.coverage_score = impl_->calculateCoverageScoreOptimized(aspect.aspect_text, answer_words);
        
        if (aspect.is_covered) {
            result.covered_aspects_count++;
        }
        
        if (aspect.is_required) {
            required_coverage += aspect.coverage_score;
            required_count++;
        } else {
            optional_coverage += aspect.coverage_score;
            optional_count++;
        }
    }
    
    // Calculate weighted coverage score
    double required_avg = required_count > 0 ? required_coverage / required_count : 1.0;
    double optional_avg = optional_count > 0 ? optional_coverage / optional_count : 1.0;
    
    result.weighted_coverage_score = 
        required_avg * impl_->config.required_aspect_weight +
        optional_avg * impl_->config.optional_aspect_weight;
    
    // Step 3: Assess depth
    if (impl_->config.enable_depth_assessment) {
        auto [depth_level, depth_score] = assessDepth(answer, result.aspects);
        result.depth_level = depth_level;
        result.depth_score = depth_score;
    } else {
        result.depth_level = DepthLevel::MEDIUM;
        result.depth_score = 0.5;
    }
    
    // Step 4: Detect missing information
    result.missing_information = detectMissingInformation(answer, query, result.aspects);
    
    // Step 5: Calculate overall completeness score
    // Weighted combination: coverage (70%) + depth (30%)
    result.completeness_score = result.weighted_coverage_score * 0.7 + result.depth_score * 0.3;
    
    // Ensure score is in [0, 1]
    result.completeness_score = std::min(1.0, std::max(0.0, result.completeness_score));
    
    // Generate explanation
    std::ostringstream explanation = {};
    explanation << "Completeness Score: " << result.completeness_score << "\n";
    explanation << "Aspect Coverage: " << result.covered_aspects_count << "/" 
                << result.total_aspects_count << " aspects covered\n";
    explanation << "Weighted Coverage: " << result.weighted_coverage_score << "\n";
    explanation << "Depth: ";
    switch (result.depth_level) {
        case DepthLevel::SHALLOW: explanation << "Shallow"; break;
        case DepthLevel::MEDIUM: explanation << "Medium"; break;
        case DepthLevel::DEEP: explanation << "Deep"; break;
    }
    explanation << " (score: " << result.depth_score << ")\n";
    
    if (!result.missing_information.empty()) {
        explanation << "Missing aspects: ";
        for (size_t i = 0; i < std::min(result.missing_information.size(), size_t(3)); ++i) {
            if (i > 0) {
              explanation << ", ";
            }
            explanation << result.missing_information[i];
        }
        if (result.missing_information.size() > 3) {
            explanation << " and " << (result.missing_information.size() - 3) << " more";
        }
        explanation << "\n";
    }
    
    if (result.completeness_score >= 0.8) {
        explanation << "Answer comprehensively covers all query aspects.";
    } else if (result.completeness_score >= 0.6) {
        explanation << "Answer covers most query aspects with reasonable depth.";
    } else {
        explanation << "Answer has incomplete coverage or lacks sufficient depth.";
    }
    
    result.explanation = explanation.str();
    
    THEMIS_INFO("Completeness evaluation complete: score={:.2f}, coverage={}/{}, depth={:.2f}", 
                result.completeness_score, result.covered_aspects_count, 
                result.total_aspects_count, result.depth_score);
    
    return result;
}

} // namespace themis::rag::judge

