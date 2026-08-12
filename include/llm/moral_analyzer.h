/**
 * @file moral_analyzer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/**
 * @brief Moral Analyzer for graph-based ethical reasoning
 * 
 * This class provides advanced moral reasoning capabilities using ThemisDB's
 * property graph to represent ethical dilemmas, principles, stakeholders,
 * actions, and outcomes. It integrates with the EthicalGuidelinesManager
 * for principled decision-making.
 * 
 * Features:
 * - Graph-based representation of ethical scenarios
 * - Multi-philosophy reasoning (Kant, Utilitarian, Virtue Ethics, etc.)
 * - Decision graph construction and traversal
 * - Principle-based evaluation
 * - Stakeholder impact analysis
 * - Outcome prediction and utility calculation
 * - Transparent reasoning chains
 * 
 * Graph Structure:
 * - Node Types: Scenario, Stakeholder, Principle, Action, Outcome, Argument
 * - Edge Types: involves, applies_to, considers, leads_to, supports, opposes, based_on
 * 
 * Example Usage:
 * ```cpp
 * // Create analyzer
 * MoralAnalyzer analyzer(db, ethical_guidelines_mgr);
 * 
 * // Build ethical scenario
 * EthicalScenario scenario;
 * scenario.description = "Autonomous vehicle must choose...";
 * scenario.domain = "autonomous_systems";
 * scenario.stakeholders = {{"passenger", 1}, {"pedestrian", 5}};
 * 
 * // Analyze with specific philosophy
 * auto result = analyzer.analyzeWithPhilosophy(scenario, "kant");
 * 
 * // Or use multi-philosophy ensemble
 * auto ensemble = analyzer.analyzeMultiPhilosophy(scenario, 
 *     {"kant", "utilitarian", "virtue"});
 * ```
 * 
 * Thread-safe for concurrent use.
 */
class MoralAnalyzer {
public:
    /**
     * @brief Ethical scenario representation
     */
    struct EthicalScenario {
        std::string id;
        std::string description;
        std::string domain;  // medical, legal, autonomous_systems, etc.
        std::map<std::string, int> stakeholders;  // stakeholder type -> count
        std::vector<std::string> possible_actions;
        std::vector<std::string> relevant_principles;
        std::map<std::string, double> context_weights;  // urgency, certainty, etc.
        std::string graph_id = "ethics_default";
    };
    
    /**
     * @brief Predicted outcome of an action
     */
    struct PredictedOutcome {
        std::string description;
        double probability = 0.0;
        double utility = 0.0;  // -1.0 to 1.0
        std::map<std::string, double> stakeholder_impacts;
        std::vector<std::string> affected_principles;
    };
    
    /**
     * @brief Ethical argument for or against an action
     */
    struct EthicalArgument {
        std::string id;
        std::string content;
        std::string philosophy;
        std::string principle_basis;
        std::string argument_type;  // pro, contra, rebuttal
        double strength = 0.0;  // 0.0 to 1.0
    };
    
    /**
     * @brief Reasoning path through the decision graph
     */
    struct ReasoningPath {
        std::string action_id;
        std::vector<std::string> supporting_principles;
        std::vector<std::string> opposing_principles;
        std::vector<PredictedOutcome> outcomes;
        std::vector<EthicalArgument> arguments;
        double total_score = 0.0;
        double confidence = 0.0;
    };
    
    /**
     * @brief Final ethical decision with full reasoning
     */
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
    
    /**
     * @brief Status of operations
     */
    struct Status {
        bool ok = true;
        std::string message;
        static Status OK() { return {}; }
        static Status Error(std::string msg) { 
            return Status{false, std::move(msg)}; 
        }
    };
    
    /**
     * @brief Construct MoralAnalyzer
     * 
     * @param db RocksDB wrapper for storage
     * @param ethical_guidelines Optional ethical guidelines manager
     * @param vector_index Optional vector index for similarity search
     * @param decision_auditor Optional auditor for compliance logging
     * @param llm_engine Optional LLM inference engine for semantic analysis
     */
    explicit MoralAnalyzer(
        RocksDBWrapper& db,
        std::shared_ptr<EthicalGuidelinesManager> ethical_guidelines = nullptr,
        std::shared_ptr<VectorIndexManager> vector_index = nullptr,
        std::shared_ptr<AIDecisionAuditor> decision_auditor = nullptr,
        std::shared_ptr<InferenceEngineEnhanced> llm_engine = nullptr
    );
    
    /**
     * @brief Build decision graph for ethical scenario
     * 
     * Creates a property graph representation with:
     * - Scenario node with description and context
     * - Stakeholder nodes with risk levels
     * - Principle nodes from relevant ethical frameworks
     * - Action nodes for possible choices
     * - Outcome nodes for predicted consequences
     * - Edges representing relationships
     * 
     * @param scenario The ethical scenario
     * @return Status indicating success or failure
     */
    Status buildDecisionGraph(const EthicalScenario& scenario);
    
    /**
     * @brief Analyze scenario using specific philosophical framework
     * 
     * @param scenario The ethical scenario
     * @param philosophy Philosophy to use (kant, utilitarian, virtue, etc.)
     * @return Pair of Status and EthicalDecision
     */
    std::pair<Status, EthicalDecision> analyzeWithPhilosophy(
        const EthicalScenario& scenario,
        const std::string& philosophy
    );
    
    /**
     * @brief Analyze scenario using multiple philosophies
     * 
     * Generates decisions from multiple philosophical perspectives and
     * synthesizes them into a balanced recommendation.
     * 
     * @param scenario The ethical scenario
     * @param philosophies List of philosophies to consider
     * @return Pair of Status and EthicalDecision with synthesis
     */
    std::pair<Status, EthicalDecision> analyzeMultiPhilosophy(
        const EthicalScenario& scenario,
        const std::vector<std::string>& philosophies
    );
    
    /**
     * @brief Evaluate action using deontological (Kantian) ethics
     * 
     * Checks:
     * - Universalizability of maxims
     * - Respect for persons as ends
     * - Compliance with categorical imperative
     * - Adherence to moral duties
     * 
     * @param scenario The scenario context
     * @param action The action to evaluate
     * @return Reasoning path with deontological evaluation
     */
    ReasoningPath evaluateDeontological(
        const EthicalScenario& scenario,
        const std::string& action
    );
    
    /**
     * @brief Evaluate action using consequentialist (utilitarian) ethics
     * 
     * Calculates:
     * - Expected utility across all outcomes
     * - Aggregate happiness/well-being
     * - Distribution of benefits and harms
     * - Overall consequence optimization
     * 
     * @param scenario The scenario context
     * @param action The action to evaluate
     * @return Reasoning path with consequentialist evaluation
     */
    ReasoningPath evaluateConsequentialist(
        const EthicalScenario& scenario,
        const std::string& action
    );
    
    /**
     * @brief Evaluate action using virtue ethics
     * 
     * Considers:
     * - Character virtues (wisdom, courage, justice, temperance)
     * - What a virtuous person would do
     * - Development of moral character
     * - Practical wisdom (phronesis)
     * 
     * @param scenario The scenario context
     * @param action The action to evaluate
     * @return Reasoning path with virtue ethics evaluation
     */
    ReasoningPath evaluateVirtueEthics(
        const EthicalScenario& scenario,
        const std::string& action
    );
    
    /**
     * @brief Predict outcomes of an action
     * 
     * Uses:
     * - Historical data from similar scenarios
     * - Causal reasoning
     * - Stakeholder impact analysis
     * - Probabilistic modeling
     * 
     * @param scenario The scenario context
     * @param action The action to predict outcomes for
     * @return Vector of predicted outcomes
     */
    std::vector<PredictedOutcome> predictOutcomes(
        const EthicalScenario& scenario,
        const std::string& action
    );
    
    /**
     * @brief Calculate expected utility of an action
     * 
     * Uses utilitarian calculus:
     * EU(action) = Σ(P(outcome_i) * U(outcome_i))
     * 
     * @param outcomes Predicted outcomes with probabilities and utilities
     * @return Expected utility value
     */
    double calculateExpectedUtility(
        const std::vector<PredictedOutcome>& outcomes
    );
    
    /**
     * @brief Generate ethical arguments for/against action
     * 
     * Creates structured arguments based on:
     * - Relevant ethical principles
     * - Stakeholder perspectives
     * - Philosophical frameworks
     * - Historical precedents
     * 
     * @param scenario The scenario context
     * @param action The action to argue about
     * @param philosophy Philosophy to use for arguments
     * @return Vector of ethical arguments
     */
    std::vector<EthicalArgument> generateArguments(
        const EthicalScenario& scenario,
        const std::string& action,
        const std::string& philosophy
    );
    
    /**
     * @brief Recommend philosophies for an ethical scenario (Quick Win #1)
     * 
     * Analyzes scenario description and context to recommend appropriate
     * philosophical frameworks. Uses rule-based keyword matching to identify
     * which philosophies are most relevant.
     * 
     * Uses multi-modal detection:
     * 1. Keyword matching (regex) for explicit terms:
     *    - "duty", "obligation" → Kantian deontology
     *    - "greatest good", "utility", "consequences" → Utilitarianism
     *    - "character", "virtue", "excellence" → Virtue ethics
     *    - "care", "relationship", "compassion" → Care ethics
     *    - "justice", "fairness", "rights" → Rawlsian justice
     * 2. Domain-specific heuristics
     * 3. LLM semantic analysis for implicit ethical implications (optional)
     * 
     * The LLM component can detect ethical concerns without direct keyword markers,
     * such as power dynamics, vulnerability, and subtle moral tensions.
     * 
     * Falls back to multi-philosophy ensemble if no clear match.
     * 
     * @param scenario The ethical scenario to analyze
     * @param use_llm Whether to use LLM for semantic analysis (default: true)
     * @return Vector of recommended philosophy names (ordered by relevance)
     */
    std::vector<std::string> recommendPhilosophies(
        const EthicalScenario& scenario,
        bool use_llm = true
    );
    
    /**
     * @brief Use LLM to detect implicit ethical implications
     * 
     * Analyzes scenarios for ethical concerns that lack direct keyword markers:
     * - Implicit moral tensions (fairness without "fair", harm without "harm")
     * - Power dynamics and vulnerability contexts
     * - Domain-specific ethical questions (medical consent, AI bias, data privacy)
     * - Cultural and contextual nuances
     * 
     * This complements regex/keyword matching by using NLP and semantic understanding
     * to identify subtle ethical dimensions that traditional pattern matching might miss.
     * 
     * @param scenario The ethical scenario to analyze
     * @return Pair of Status and vector of detected philosophies with confidence
     */
    std::pair<Status, std::vector<std::string>> detectEthicalImplicationsViaLLM(
        const EthicalScenario& scenario
    );
    
    /**
     * @brief Synthesize decision from multiple reasoning paths
     * 
     * Combines multiple philosophical perspectives using:
     * - Reflective equilibrium
     * - Weighted voting
     * - Common ground identification
     * - Trade-off analysis
     * 
     * @param paths Reasoning paths from different philosophies
     * @return Synthesized ethical decision
     */
    EthicalDecision synthesizeDecision(
        const std::vector<std::pair<std::string, ReasoningPath>>& paths
    );
    
    /**
     * @brief Check consistency of decision with principles
     * 
     * Verifies:
     * - Decision follows stated principles
     * - No contradictions in reasoning
     * - Similar cases treated similarly
     * - Temporal consistency
     * 
     * @param decision The decision to check
     * @return Consistency score (0.0 to 1.0)
     */
    double checkConsistency(const EthicalDecision& decision);
    
    /**
     * @brief Assess fairness of decision
     * 
     * Measures:
     * - Demographic parity
     * - Equal treatment
     * - Impartial consideration
     * - Distribution of benefits/harms
     * 
     * @param decision The decision to assess
     * @return Fairness score (0.0 to 1.0)
     */
    double assessFairness(const EthicalDecision& decision);
    
    /**
     * @brief Retrieve similar past scenarios
     * 
     * Uses:
     * - Vector similarity search
     * - Graph pattern matching
     * - Domain filtering
     * - Outcome analysis
     * 
     * @param scenario Current scenario
     * @param limit Maximum number of similar scenarios
     * @return Vector of similar scenarios with decisions
     */
    std::vector<std::pair<EthicalScenario, EthicalDecision>> 
    retrieveSimilarScenarios(
        const EthicalScenario& scenario,
        int limit = 5
    );
    
    /**
     * @brief Store decision in multi-model architecture with audit trail
     * 
     * Stores decision across:
     * - Graph: Full reasoning chains and decision structure
     * - Vector: Scenario embeddings for similarity search
     * - Relational: Keywords and metadata for structured queries
     * - Audit Trail: Complete compliance logging
     * 
     * @param decision The ethical decision to store
     * @param scenario_embedding Optional embedding vector for similarity search
     * @param user_id Identity of the actor storing the decision (used in audit log)
     * @return Status indicating success or failure
     */
    Status storeDecision(
        const EthicalDecision& decision,
        const std::vector<float>& scenario_embedding = {},
        const std::string& user_id = "ethics_system"
    );
    
    /**
     * @brief Export decision graph as DOT format for visualization
     * 
     * @param scenario_id Scenario to export
     * @return DOT format string
     */
    std::string exportDecisionGraphDOT(const std::string& scenario_id);
    
    /**
     * @brief Get reasoning chain as human-readable text
     * 
     * @param decision The decision with reasoning
     * @return Formatted reasoning explanation
     */
    std::string getReasoningExplanation(const EthicalDecision& decision);
    
    /**
     * @brief Clear decision graph for scenario
     * 
     * @param scenario_id Scenario to clear
     * @return Status indicating success or failure
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
     * @brief Extract keywords from decision for relational storage
     */
    std::vector<std::string> extractKeywords(const EthicalDecision& decision);
    
    /**
     * @brief Add scenario node to graph
     */
    Status addScenarioNode(const EthicalScenario& scenario);
    
    /**
     * @brief Add stakeholder nodes and connect to scenario
     */
    Status addStakeholderNodes(
        const EthicalScenario& scenario,
        const std::string& scenario_node_id
    );
    
    /**
     * @brief Add principle nodes from guidelines
     */
    Status addPrincipleNodes(
        const EthicalScenario& scenario,
        const std::string& scenario_node_id,
        const std::string& philosophy
    );
    
    /**
     * @brief Add action nodes for possible choices
     */
    Status addActionNodes(
        const EthicalScenario& scenario,
        const std::string& scenario_node_id
    );
    
    /**
     * @brief Add outcome nodes for predicted consequences
     */
    Status addOutcomeNodes(
        const std::string& action_id,
        const std::vector<PredictedOutcome>& outcomes
    );
    
    /**
     * @brief Add argument nodes for ethical reasoning
     */
    Status addArgumentNodes(
        const std::string& action_id,
        const std::vector<EthicalArgument>& arguments
    );
    
    /**
     * @brief Traverse graph to find reasoning path
     */
    ReasoningPath traverseReasoningPath(
        const std::string& scenario_id,
        const std::string& action_id,
        const std::string& philosophy
    );
    
    /**
     * @brief Load principles for philosophy from guidelines
     */
    std::vector<std::string> loadPrinciplesForPhilosophy(
        const std::string& philosophy
    );
    
    /**
     * @brief Score action based on principle adherence
     */
    double scoreActionByPrinciples(
        const std::string& action,
        const std::vector<std::string>& principles,
        const std::string& philosophy
    );
    
    /**
     * @brief Calculate utility impact on stakeholders
     */
    std::map<std::string, double> calculateStakeholderImpacts(
        const EthicalScenario& scenario,
        const std::string& action
    );
    
    /**
     * @brief Format decision as structured text
     */
    std::string formatDecisionText(
        const EthicalDecision& decision
    );
    
    /**
     * @brief Validate scenario structure
     */
    bool validateScenario(const EthicalScenario& scenario);
};

} // namespace llm
} // namespace themis
