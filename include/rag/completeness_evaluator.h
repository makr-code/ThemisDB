/**
 * @file completeness_evaluator.h
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
#include <unordered_map>

namespace themis::rag::judge {

/**
 * @brief Query aspect information
 */
struct QueryAspect {
    std::string aspect_text;
    bool is_required;  ///< Required vs optional aspect
    bool is_covered;   ///< Whether covered in answer
    double coverage_score;  ///< 0-1 indicating how well covered
};

/**
 * @brief Depth level of answer
 */
enum class DepthLevel {
    SHALLOW,   ///< Surface-level, minimal details
    MEDIUM,    ///< Moderate detail with some examples
    DEEP       ///< Comprehensive with examples and evidence
};

/**
 * @brief Completeness evaluation result
 */
struct CompletenessResult {
    double completeness_score = 0;     ///< Overall score 0-1
    std::vector<QueryAspect> aspects;
    size_t covered_aspects_count;
    size_t total_aspects_count;
    double weighted_coverage_score;
    DepthLevel depth_level;
    double depth_score;
    std::vector<std::string> missing_information;
    std::string explanation;
};

/**
 * @brief Completeness evaluator
 * 
 * Evaluates answer completeness through:
 * 1. Aspect coverage analysis
 * 2. Depth assessment
 * 3. Missing information detection
 */
class CompletenessEvaluator {
public:
    /**
     * @brief Configuration for completeness evaluation
     */
    struct Config {
        double required_aspect_weight = 0.7;
        double optional_aspect_weight = 0.3;
        bool enable_depth_assessment = true;
        bool enable_gap_detection = true;
    };

    /**
     * @brief Construct evaluator with configuration
     */
    CompletenessEvaluator();
    explicit CompletenessEvaluator(const Config& config);
    
    /**
     * @brief Destructor
     */
    ~CompletenessEvaluator();
    
    /**
     * @brief Evaluate completeness of an answer
     * @param answer Generated answer
     * @param query Original query
     * @return Completeness evaluation result
     */
    CompletenessResult evaluate(
        const std::string& answer,
        const std::string& query
    );
    
    /**
     * @brief Extract aspects from query
     * @param query Query to analyze
     * @return List of query aspects
     */
    std::vector<QueryAspect> extractQueryAspects(const std::string& query);
    
    /**
     * @brief Assess depth of answer
     * @param answer Generated answer
     * @param aspects Query aspects
     * @return Depth level and score
     */
    std::pair<DepthLevel, double> assessDepth(
        const std::string& answer,
        [[maybe_unused]] const std::vector<QueryAspect>& aspects
    );
    
    /**
     * @brief Detect missing information
     * @param answer Generated answer
     * @param query Original query
     * @param aspects Identified query aspects
     * @return List of missing information items
     */
    std::vector<std::string> detectMissingInformation(
        [[maybe_unused]] const std::string& answer,
        [[maybe_unused]] const std::string& query,
        const std::vector<QueryAspect>& aspects
    );

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace themis::rag::judge
