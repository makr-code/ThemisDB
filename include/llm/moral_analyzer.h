/**
 * @file moral_analyzer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "index/property_graph.h"
#include "index/vector_index.h"
#include "llm/ethical_guidelines_manager.h"
#include "llm/ai_decision_auditor.h"
#include "llm/inference_engine_enhanced.h"
#include "storage/base_entity.h"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <optional>

namespace themis {
namespace llm {

class MoralAnalyzer {
public:
    struct EthicalScenario {
        std::string id = {};
        std::string description;
        std::string domain;  // medical, legal, autonomous_systems, etc.
        std::map<std::string, int> stakeholders;  // stakeholder type -> count
        std::vector<std::string> possible_actions;
        std::vector<std::string> relevant_principles;
        std::map<std::string, double> context_weights;  // urgency, certainty, etc.
        std::string graph_id = "ethics_default";
    };
    
    struct PredictedOutcome {
        std::string description;
        double probability = 0.0;
        double utility = 0.0;  // -1.0 to 1.0
        std::map<std::string, double> stakeholder_impacts;
        std::vector<std::string> affected_principles;
    };
    
    struct EthicalArgument {
        std::string id;
        std::string content;
        std::string philosophy;
        std::string principle_basis;
        std::string argument_type;  // pro, contra, rebuttal
        double strength = 0.0;  // 0.0 to 1.0
    };
    
    struct ReasoningPath {
        std::string action_id;
        std::vector<std::string> supporting_principles;
        std::vector<std::string> opposing_principles;
        std::vector<PredictedOutcome> outcomes;
        std::vector<EthicalArgument> arguments;
        double total_score = 0.0;
        double confidence = 0.0;
    };
    
    struct EthicalDecision {
        std::string decision_id;
        std::string scenario_id;
        std::string philosophy;
        std::string recommended_action;
        std::string reasoning;
        std::vector<std::string> principle_citations;
        ReasoningPath reasoning_path;
        double confidence = 0.0;
        std::map<std::string, std::string> alternative_perspectives;
        std::string graph_id;
        
        // Evaluation metrics
        struct Metrics {
            double consistency = 0.0;
            double fairness = 0.0;
            double transparency = 1.0;  // Always transparent with graph
            double feasibility = 0.0;
            double long_term_impact = 0.0;
        } metrics;
    };
    
    struct Status {
        bool ok = true;
        std::string message;
        /**
         * @brief OK.
         * @return Return value.
         * @details Implements OK without additional internal calls.
         */
        static Status OK() { return {}; }
        /**
         * @brief Error.
         * @param[in] msg Input parameter.
         * @return Return value.
         * @details Calls: std::move().
         */
        static Status Error(std::string msg) { 
            return Status{false, std::move(msg)}; 
        }
    };
    
    explicit MoralAnalyzer(
        RocksDBWrapper& db,
        std::shared_ptr<EthicalGuidelinesManager> ethical_guidelines = nullptr,
        std::shared_ptr<VectorIndexManager> vector_index = nullptr,
        std::shared_ptr<AIDecisionAuditor> decision_auditor = nullptr,
        std::shared_ptr<InferenceEngineEnhanced> llm_engine = nullptr
    );
    
    /**
     * @brief Build Decision Graph.
     * @param[in] scenario Input parameter.
     * @return Return value.
     */
    Status buildDecisionGraph(const EthicalScenario& scenario);
    
    std::pair<Status, EthicalDecision> analyzeWithPhilosophy(
        const EthicalScenario& scenario,
        const std::string& philosophy
    );
    
    std::pair<Status, EthicalDecision> analyzeMultiPhilosophy(
        const EthicalScenario& scenario,
        const std::vector<std::string>& philosophies
    );
    
    /**
     * @brief Evaluate Deontological.
     * @param[in] scenario Input parameter.
     * @param[in] action Input parameter.
     * @return Return value.
     */
    ReasoningPath evaluateDeontological(
        const EthicalScenario& scenario,
        const std::string& action
    );
    
    /**
     * @brief Evaluate Consequentialist.
     * @param[in] scenario Input parameter.
     * @param[in] action Input parameter.
     * @return Return value.
     */
    ReasoningPath evaluateConsequentialist(
        const EthicalScenario& scenario,
        const std::string& action
    );
    
    /**
     * @brief Evaluate Virtue Ethics.
     * @param[in] scenario Input parameter.
     * @param[in] action Input parameter.
     * @return Return value.
     */
    ReasoningPath evaluateVirtueEthics(
        const EthicalScenario& scenario,
        const std::string& action
    );
    
    /**
     * @brief Predict Outcomes.
     * @param[in] scenario Input parameter.
     * @param[in] action Input parameter.
     * @return Return value.
     */
    std::vector<PredictedOutcome> predictOutcomes(
        const EthicalScenario& scenario,
        const std::string& action
    );
    
    /**
     * @brief Calculate Expected Utility.
     * @param[in] outcomes Input parameter.
     * @return Return value.
     */
    double calculateExpectedUtility(
        const std::vector<PredictedOutcome>& outcomes
    );
    
    /**
     * @brief Generate Arguments.
     * @param[in] scenario Input parameter.
     * @param[in] action Input parameter.
     * @param[in] philosophy Input parameter.
     * @return Return value.
     */
    std::vector<EthicalArgument> generateArguments(
        const EthicalScenario& scenario,
        const std::string& action,
        const std::string& philosophy
    );
    
    std::vector<std::string> recommendPhilosophies(
        const EthicalScenario& scenario,
        bool use_llm = true
    );
    
    std::pair<Status, std::vector<std::string>> detectEthicalImplicationsViaLLM(
        const EthicalScenario& scenario
    );
    
    EthicalDecision synthesizeDecision(
        const std::vector<std::pair<std::string, ReasoningPath>>& paths
    );
    
    /**
     * @brief Check Consistency.
     * @param[in] decision Input parameter.
     * @return Return value.
     */
    double checkConsistency(const EthicalDecision& decision);
    
    /**
     * @brief Assess Fairness.
     * @param[in] decision Input parameter.
     * @return Return value.
     */
    double assessFairness(const EthicalDecision& decision);
    
    std::vector<std::pair<EthicalScenario, EthicalDecision>> 
    retrieveSimilarScenarios(
        const EthicalScenario& scenario,
        int limit = 5
    );
    
    Status storeDecision(
        const EthicalDecision& decision,
        const std::vector<float>& scenario_embedding = {},
        const std::string& user_id = "ethics_system"
    );
    
    /**
     * @brief Export Decision Graph DOT.
     * @param[in] scenario_id Identifier of the scenario.
     * @return Return value.
     */
    std::string exportDecisionGraphDOT(const std::string& scenario_id);
    
    /**
     * @brief Get Reasoning Explanation.
     * @param[in] decision Input parameter.
     * @return Return value.
     */
    std::string getReasoningExplanation(const EthicalDecision& decision);
    
    /**
     * @brief Clear Decision Graph.
     * @param[in] scenario_id Identifier of the scenario.
     * @return Return value.
     */
    Status clearDecisionGraph(const std::string& scenario_id);

private:
    // Core components
    RocksDBWrapper& db_;
    std::unique_ptr<PropertyGraphManager> graph_manager_;
    std::shared_ptr<EthicalGuidelinesManager> ethical_guidelines_;
    std::shared_ptr<VectorIndexManager> vector_index_;
    std::shared_ptr<AIDecisionAuditor> decision_auditor_;
    std::shared_ptr<InferenceEngineEnhanced> llm_engine_;  // For semantic analysis
    
    // Helper methods
    
    /**
     * @brief Extract Keywords.
     * @param[in] decision Input parameter.
     * @return Return value.
     */
    std::vector<std::string> extractKeywords(const EthicalDecision& decision);
    
    /**
     * @brief Add Scenario Node.
     * @param[in] scenario Input parameter.
     * @return Return value.
     */
    Status addScenarioNode(const EthicalScenario& scenario);
    
    /**
     * @brief Add Stakeholder Nodes.
     * @param[in] scenario Input parameter.
     * @param[in] scenario_node_id Identifier of the scenario node.
     * @return Return value.
     */
    Status addStakeholderNodes(
        const EthicalScenario& scenario,
        const std::string& scenario_node_id
    );
    
    /**
     * @brief Add Principle Nodes.
     * @param[in] scenario Input parameter.
     * @param[in] scenario_node_id Identifier of the scenario node.
     * @param[in] philosophy Input parameter.
     * @return Return value.
     */
    Status addPrincipleNodes(
        const EthicalScenario& scenario,
        const std::string& scenario_node_id,
        const std::string& philosophy
    );
    
    /**
     * @brief Add Action Nodes.
     * @param[in] scenario Input parameter.
     * @param[in] scenario_node_id Identifier of the scenario node.
     * @return Return value.
     */
    Status addActionNodes(
        const EthicalScenario& scenario,
        const std::string& scenario_node_id
    );
    
    /**
     * @brief Add Outcome Nodes.
     * @param[in] action_id Identifier of the action.
     * @param[in] outcomes Input parameter.
     * @return Return value.
     */
    Status addOutcomeNodes(
        const std::string& action_id,
        const std::vector<PredictedOutcome>& outcomes
    );
    
    /**
     * @brief Add Argument Nodes.
     * @param[in] action_id Identifier of the action.
     * @param[in] arguments Input parameter.
     * @return Return value.
     */
    Status addArgumentNodes(
        const std::string& action_id,
        const std::vector<EthicalArgument>& arguments
    );
    
    /**
     * @brief Traverse Reasoning Path.
     * @param[in] scenario_id Identifier of the scenario.
     * @param[in] action_id Identifier of the action.
     * @param[in] philosophy Input parameter.
     * @return Return value.
     */
    ReasoningPath traverseReasoningPath(
        const std::string& scenario_id,
        const std::string& action_id,
        const std::string& philosophy
    );
    
    /**
     * @brief Load Principles For Philosophy.
     * @param[in] philosophy Input parameter.
     * @return Return value.
     */
    std::vector<std::string> loadPrinciplesForPhilosophy(
        const std::string& philosophy
    );
    
    /**
     * @brief Score Action By Principles.
     * @param[in] action Input parameter.
     * @param[in] principles Input parameter.
     * @param[in] philosophy Input parameter.
     * @return Return value.
     */
    double scoreActionByPrinciples(
        const std::string& action,
        const std::vector<std::string>& principles,
        const std::string& philosophy
    );
    
    std::map<std::string, double> calculateStakeholderImpacts(
        const EthicalScenario& scenario,
        const std::string& action
    );
    
    /**
     * @brief Format Decision Text.
     * @param[in] decision Input parameter.
     * @return Return value.
     */
    std::string formatDecisionText(
        const EthicalDecision& decision
    );
    
    /**
     * @brief Validate Scenario.
     * @param[in] scenario Input parameter.
     * @return True when the operation succeeds.
     */
    bool validateScenario(const EthicalScenario& scenario);
};

} // namespace llm
} // namespace themis
