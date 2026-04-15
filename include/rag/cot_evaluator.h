/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            cot_evaluator.h                                    ║
  Version:         0.0.45                                             ║
  Last Modified:   2026-04-15 07:08:45                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     139                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file cot_evaluator.h
 * @brief Chain-of-Thought evaluation with step-by-step reasoning
 * 
 * Implements CoT evaluation where the judge provides explicit reasoning
 * steps before arriving at a final score, improving transparency and
 * consistency.
 */

#pragma once

#include <string>
#include <vector>
#include <memory>

namespace themis::rag::judge {

/**
 * @brief A single reasoning step in CoT
 */
struct ReasoningStep {
    int step_number;
    std::string question;      ///< Self-questioning
    std::string observation;   ///< What was observed
    std::string evidence;      ///< Supporting evidence
    std::string conclusion;    ///< Step conclusion
};

/**
 * @brief Chain-of-Thought evaluation result
 */
struct CoTEvaluationResult {
    double final_score;        ///< Final score 0-1
    std::vector<ReasoningStep> reasoning_steps;
    std::string final_reasoning;
    bool logic_consistent;     ///< Whether reasoning is internally consistent
    std::vector<std::string> inconsistencies;  ///< Detected logic inconsistencies
};

/**
 * @brief Chain-of-Thought evaluator
 * 
 * Evaluates answers using explicit step-by-step reasoning.
 * Provides transparency into the evaluation process and enables
 * logic validation.
 */
class CoTEvaluator {
public:
    /**
     * @brief Configuration for CoT evaluation
     */
    struct Config {
        int num_reasoning_steps = 5;  ///< Number of reasoning steps
        bool enable_self_questioning = true;
        bool enable_logic_validation = true;
        bool enable_evidence_gathering = true;
    };

    /**
     * @brief Construct evaluator with configuration
     */
    CoTEvaluator();
    explicit CoTEvaluator(const Config& config);
    
    /**
     * @brief Destructor
     */
    ~CoTEvaluator();
    
    /**
     * @brief Evaluate with chain-of-thought reasoning
     * @param query Original query
     * @param answer Generated answer
     * @param documents Retrieved documents
     * @param dimension Evaluation dimension (optional)
     * @return CoT evaluation result
     */
    CoTEvaluationResult evaluate(
        const std::string& query,
        const std::string& answer,
        const std::vector<std::pair<std::string, std::string>>& documents,
        const std::string& dimension = "overall"
    );
    
    /**
     * @brief Parse CoT response from LLM
     * @param response LLM response with CoT reasoning
     * @return Parsed reasoning steps
     */
    std::vector<ReasoningStep> parseCoTResponse(const std::string& response);
    
    /**
     * @brief Validate logic consistency in reasoning steps
     * @param steps Reasoning steps to validate
     * @return List of inconsistencies found
     */
    std::vector<std::string> validateLogicConsistency(
        const std::vector<ReasoningStep>& steps
    );
    
    /**
     * @brief Extract final score from CoT reasoning
     * @param steps Reasoning steps
     * @param response Full LLM response
     * @return Extracted score 0-1
     */
    double extractFinalScore(
        const std::vector<ReasoningStep>& steps,
        const std::string& response
    );

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace themis::rag::judge
