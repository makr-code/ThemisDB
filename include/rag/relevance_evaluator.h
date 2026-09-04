/**
 * @file relevance_evaluator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include <string>
#include <vector>
#include <memory>

namespace themis::rag::judge {

/**
 * @brief Query intent type
 */
enum class QueryIntent {
    INFORMATIONAL,   ///< Seeking information/knowledge
    NAVIGATIONAL,    ///< Looking for specific resource
    TRANSACTIONAL,   ///< Wanting to perform action
    CONVERSATIONAL,  ///< Follow-up or clarification
    UNKNOWN
};

/**
 * @brief Relevance evaluation result
 */
struct RelevanceResult {
    double relevance_score = 0;           ///< Overall score 0-1
    std::vector<std::string> reverse_questions;
    double question_similarity_score; ///< Similarity to original query
    QueryIntent detected_intent;
    double intent_alignment_score;
    double noise_ratio;               ///< 0-1, lower is better
    std::vector<std::string> irrelevant_segments;
    std::string explanation;
};

/**
 * @brief Relevance evaluator
 * 
 * Evaluates answer relevance through:
 * 1. Reverse question generation
 * 2. Query intent analysis
 * 3. Noise detection
 */
class RelevanceEvaluator {
public:
    /**
     * @brief Configuration for relevance evaluation
     */
    struct Config {
        size_t num_reverse_questions = 3;
        double similarity_threshold = 0.7;
        bool enable_intent_analysis = true;
        bool enable_noise_detection = true;
    };

    /**
     * @brief Construct evaluator with configuration
     */
    RelevanceEvaluator();
    explicit RelevanceEvaluator(const Config& config);
    
    /**
     * @brief Destructor
     */
    ~RelevanceEvaluator();
    
    /**
     * @brief Evaluate relevance of an answer
     * @param answer Generated answer
     * @param query Original query
     * @return Relevance evaluation result
     */
    RelevanceResult evaluate(
        const std::string& answer,
        const std::string& query
    );
    
    /**
     * @brief Generate questions that this answer would address
     * @param answer Generated answer
     * @return List of reverse-generated questions
     */
    std::vector<std::string> generateReverseQuestions(const std::string& answer);
    
    /**
     * @brief Analyze query intent
     * @param query Query to analyze
     * @return Detected intent type
     */
    QueryIntent analyzeIntent(const std::string& query);
    
    /**
     * @brief Detect irrelevant information in answer
     * @param answer Generated answer
     * @param query Original query
     * @return List of irrelevant segments
     */
    std::vector<std::string> detectNoise(
        const std::string& answer,
        const std::string& query
    );
    
    /**
     * @brief Calculate semantic similarity between query and questions
     * @param query Original query
     * @param questions Generated questions
     * @return Similarity score 0-1
     */
    double calculateSemanticSimilarity(
        const std::string& query,
        const std::vector<std::string>& questions
    );

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace themis::rag::judge
