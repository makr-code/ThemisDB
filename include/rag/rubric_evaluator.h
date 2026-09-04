/**
 * @file rubric_evaluator.h
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
 * @brief Score level in a rubric
 */
struct RubricLevel {
    int score = 0;                    ///< Numeric score (1-5)
    std::string description;      ///< Level description
    std::vector<std::string> examples;  ///< Example indicators
    std::vector<std::string> criteria;  ///< Specific criteria
};

/**
 * @brief Rubric for a specific dimension
 */
struct DimensionRubric {
    std::string dimension_name;
    std::string description;
    double weight;                ///< Weight in overall score
    std::vector<RubricLevel> levels;  ///< Scoring levels (1-5)
};

/**
 * @brief Complete evaluation rubric
 */
struct EvaluationRubric {
    std::string name;
    std::string description;
    std::string domain;  ///< Domain (e.g., "medical", "legal", "general")
    std::vector<DimensionRubric> dimensions;
    
    // Metadata
    std::string version;
    std::string author;
};

/**
 * @brief Rubric-based evaluation result
 */
struct RubricEvaluationResult {
    double overall_score = 0;  ///< 0-1 normalized from 1-5 scale
    std::unordered_map<std::string, double> dimension_scores;
    std::unordered_map<std::string, int> dimension_levels;  ///< 1-5 level per dimension
    std::unordered_map<std::string, std::string> dimension_reasoning;
    std::string overall_reasoning;
    std::string rubric_name;
};

/**
 * @brief Rubric evaluator
 * 
 * Evaluates answers using structured rubrics with defined scoring levels.
 * Supports YAML-based rubric definitions and custom domain rubrics.
 */
class RubricEvaluator {
public:
    /**
     * @brief Configuration for rubric evaluation
     */
    struct Config {
        bool strict_level_matching = true;  ///< Require exact level match
        bool enable_consistency_check = true;
        double consistency_threshold = 0.8;
    };

    /**
     * @brief Construct evaluator with configuration
     */
    RubricEvaluator();
    explicit RubricEvaluator(const Config& config);
    
    /**
     * @brief Destructor
     */
    ~RubricEvaluator();
    
    /**
     * @brief Load rubric from YAML string
     * @param yaml_content YAML rubric specification
     * @return true if loaded successfully
     */
    bool loadRubricFromYAML(const std::string& yaml_content);
    
    /**
     * @brief Load rubric from YAML file
     * @param filepath Path to YAML file
     * @return true if loaded successfully
     */
    bool loadRubricFromFile(const std::string& filepath);
    
    /**
     * @brief Set active rubric
     * @param rubric Rubric to use for evaluation
     */
    void setRubric(const EvaluationRubric& rubric);
    
    /**
     * @brief Get active rubric
     * @return Current rubric
     */
    const EvaluationRubric& getRubric() const;
    
    /**
     * @brief Evaluate answer using active rubric
     * @param query Original query
     * @param answer Generated answer
     * @param documents Retrieved documents
     * @return Rubric-based evaluation result
     */
    RubricEvaluationResult evaluate(
        const std::string& query,
        const std::string& answer,
        const std::vector<std::pair<std::string, std::string>>& documents
    );
    
    /**
     * @brief Create default general-purpose rubric
     * @return Default rubric
     */
    static EvaluationRubric createDefaultRubric();
    
    /**
     * @brief Validate rubric structure
     * @param rubric Rubric to validate
     * @return true if valid
     */
    static bool validateRubric(const EvaluationRubric& rubric);
    
    /**
     * @brief Convert 1-5 score to 0-1 normalized score
     * @param level_score Score on 1-5 scale
     * @return Normalized score 0-1
     */
    static double normalizeScore(int level_score);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace themis::rag::judge
