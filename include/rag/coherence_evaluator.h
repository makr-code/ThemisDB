/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            coherence_evaluator.h                              ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:46:37                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     127                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file coherence_evaluator.h
 * @brief Coherence evaluation for RAG outputs
 * 
 * Evaluates logical structure and quality of generated answers through
 * logical flow analysis, structural coherence, linguistic quality, and
 * internal consistency checking.
 */

#pragma once

#include <string>
#include <vector>
#include <memory>

namespace themis::rag::judge {

/**
 * @brief Coherence evaluation result
 */
struct CoherenceResult {
    double coherence_score;         ///< Overall score 0-1
    double logical_flow_score;      ///< Argument structure and transitions (30%)
    double structural_score;        ///< Organization and structure (20%)
    double linguistic_score;        ///< Grammar, clarity, readability (20%)
    double consistency_score;       ///< Internal consistency (30%)
    bool has_contradictions;
    std::vector<std::string> contradictions;
    std::string explanation;
};

/**
 * @brief Coherence evaluator
 * 
 * Evaluates answer coherence through:
 * 1. Logical flow analysis
 * 2. Structural coherence
 * 3. Linguistic quality
 * 4. Internal consistency
 */
class CoherenceEvaluator {
public:
    /**
     * @brief Configuration for coherence evaluation
     */
    struct Config {
        double logical_flow_weight = 0.30;
        double structural_weight = 0.20;
        double linguistic_weight = 0.20;
        double consistency_weight = 0.30;
        bool enable_contradiction_detection = true;
    };

    /**
     * @brief Construct evaluator with configuration
     */
    CoherenceEvaluator();
    explicit CoherenceEvaluator(const Config& config);
    
    /**
     * @brief Destructor
     */
    ~CoherenceEvaluator();
    
    /**
     * @brief Evaluate coherence of an answer
     * @param answer Generated answer
     * @return Coherence evaluation result
     */
    CoherenceResult evaluate(const std::string& answer);
    
    /**
     * @brief Analyze logical flow and transitions
     * @param answer Generated answer
     * @return Logical flow score 0-1
     */
    double analyzeLogicalFlow(const std::string& answer);
    
    /**
     * @brief Assess structural coherence
     * @param answer Generated answer
     * @return Structural score 0-1
     */
    double assessStructure(const std::string& answer);
    
    /**
     * @brief Evaluate linguistic quality
     * @param answer Generated answer
     * @return Linguistic score 0-1
     */
    double evaluateLinguisticQuality(const std::string& answer);
    
    /**
     * @brief Check for internal contradictions
     * @param answer Generated answer
     * @return List of detected contradictions
     */
    std::vector<std::string> detectContradictions(const std::string& answer);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace themis::rag::judge
