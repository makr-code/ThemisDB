/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ethics_evaluator.h                                 ║
  Version:         0.0.45                                             ║
  Last Modified:   2026-04-15 07:10:41                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     88                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "plugins/ethics_ai/ethics_ai_types.h"
#include <memory>

namespace themis {
namespace plugins {
namespace ethics {

/**
 * @brief Ethics Evaluator
 * 
 * Evaluates ethical decisions across 5 dimensions:
 * 1. Decision Quality
 * 2. Consistency  
 * 3. Fairness
 * 4. Alignment
 * 5. Transparency
 */
class EthicsEvaluator {
public:
    EthicsEvaluator() = default;
    ~EthicsEvaluator() = default;
    
    /**
     * @brief Evaluate a decision
     * @param decision The decision to evaluate
     * @param arguments Supporting arguments
     * @return Evaluation result or error
     */
    std::variant<EthicsEvaluationResult, Status> evaluateDecision(
        const EthicalDecision& decision,
        const std::vector<EthicalArgument>& arguments
    );
    
private:
    // Dimension evaluators
    double evaluateDecisionQuality(
        const EthicalDecision& decision,
        const std::vector<EthicalArgument>& arguments
    );
    
    double evaluateConsistency(
        const EthicalDecision& decision,
        const std::vector<EthicalArgument>& arguments
    );
    
    double evaluateFairness(
        const EthicalDecision& decision,
        const std::vector<EthicalArgument>& arguments
    );
    
    double evaluateAlignment(
        const EthicalDecision& decision,
        const std::vector<EthicalArgument>& arguments
    );
    
    double evaluateTransparency(
        const EthicalDecision& decision,
        const std::vector<EthicalArgument>& arguments
    );
};

} // namespace ethics
} // namespace plugins
} // namespace themis
