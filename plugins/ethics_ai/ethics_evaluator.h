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
