/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ethics_evaluator.h                                 ║
  Version:         0.0.24                                             ║
  Last Modified:   2026-02-22 08:12:13                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     92                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 00c723d27  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 03329d86d  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31e8b8df0  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 0d722b04c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
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
