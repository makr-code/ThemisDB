/**
 * @file moral_analyzer.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=4, H=2, M=9, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "llm/moral_analyzer.h"
#include <sstream>
#include <algorithm>
#include <cmath>
#include <random>
#include <chrono>
#include <nlohmann/json.hpp>

namespace themis {
namespace llm {

using json = nlohmann::json;

MoralAnalyzer::MoralAnalyzer(
    RocksDBWrapper& db,
    std::shared_ptr<EthicalGuidelinesManager> ethical_guidelines,
    std::shared_ptr<VectorIndexManager> vector_index,
    std::shared_ptr<AIDecisionAuditor> decision_auditor,
    std::shared_ptr<InferenceEngineEnhanced> llm_engine
) : db_(db), 
    ethical_guidelines_(ethical_guidelines),
    vector_index_(vector_index),
    decision_auditor_(decision_auditor),
    llm_engine_(llm_engine) {
    graph_manager_ = std::make_unique<PropertyGraphManager>(db);
}

MoralAnalyzer::Status MoralAnalyzer::buildDecisionGraph(
    const EthicalScenario& scenario
) {
    if (!validateScenario(scenario)) {
        return Status::Error("Invalid scenario structure");
    }
    
    // Add scenario node
    auto status = addScenarioNode(scenario);
    if (!status.ok) {
      return status;
    }
    
    std::string scenario_node_id = "scenario_" + scenario.id;
    
    // Add stakeholder nodes
    status = addStakeholderNodes(scenario, scenario_node_id);
    if (!status.ok) {
      return status;
    }
    
    // Add action nodes
    status = addActionNodes(scenario, scenario_node_id);
    if (!status.ok) {
      return status;
    }
    
    return Status::OK();
}

MoralAnalyzer::Status MoralAnalyzer::addScenarioNode(
    const EthicalScenario& scenario
) {
    BaseEntity scenario_entity("scenario_" + scenario.id);
    scenario_entity.setField("type", "scenario");
    scenario_entity.setField("description", scenario.description);
    scenario_entity.setField("domain", scenario.domain);
    scenario_entity.setField("_labels", 
        std::string(json(std::vector<std::string>{"Scenario", scenario.domain}).dump()));
    
    auto status = graph_manager_->addNode(scenario_entity, scenario.graph_id);
    
    if (!status.ok) {
        return Status::Error("Failed to add scenario node: " + status.message);
    }
    
    return Status::OK();
}

MoralAnalyzer::Status MoralAnalyzer::addStakeholderNodes(
    const EthicalScenario& scenario,
    const std::string& scenario_node_id
) {
    for (const auto& [stakeholder_type, count] : scenario.stakeholders) {
        std::string stakeholder_id = "stakeholder_" + scenario.id + "_" + stakeholder_type;
        
        BaseEntity stakeholder_entity(stakeholder_id);
        stakeholder_entity.setField("type", "stakeholder");
        stakeholder_entity.setField("stakeholder_type", stakeholder_type);
        stakeholder_entity.setField("count", count);
        stakeholder_entity.setField("_labels", 
            std::string(json(std::vector<std::string>{"Stakeholder"}).dump()));
        
        auto status = graph_manager_->addNode(
            stakeholder_entity, 
            scenario.graph_id
        );
        
        if (!status.ok) {
            return Status::Error("Failed to add stakeholder node");
        }
        
        // Create edge: scenario -> involves -> stakeholder
        BaseEntity involves_edge("involves_" + scenario.id + "_" + stakeholder_type);
        involves_edge.setField("_from", scenario_node_id);
        involves_edge.setField("_to", stakeholder_id);
        involves_edge.setField("_type", "involves");
        
        auto edge_status = graph_manager_->addEdge(
            involves_edge,
            scenario.graph_id
        );
        
        if (!edge_status.ok) {
            return Status::Error("Failed to add involves edge");
        }
    }
    
    return Status::OK();
}

MoralAnalyzer::Status MoralAnalyzer::addPrincipleNodes(
    const EthicalScenario& scenario,
    const std::string& scenario_node_id,
    const std::string& philosophy
) {
    auto principles = loadPrinciplesForPhilosophy(philosophy);
    
    for (const auto& principle_id : principles) {
        std::string node_id = "principle_" + principle_id;
        
        BaseEntity principle_entity(node_id);
        principle_entity.setField("type", "principle");
        principle_entity.setField("principle_id", principle_id);
        principle_entity.setField("philosophy", philosophy);
        principle_entity.setField("_labels", 
            std::string(json(std::vector<std::string>{"Principle", philosophy}).dump()));
        
        auto status = graph_manager_->addNode(
            principle_entity,
            scenario.graph_id
        );
        
        if (!status.ok) {
            return Status::Error("Failed to add principle node");
        }
        
        // Create edge: principle -> applies_to -> scenario
        BaseEntity applies_edge("applies_" + principle_id + "_" + scenario.id);
        applies_edge.setField("_from", node_id);
        applies_edge.setField("_to", scenario_node_id);
        applies_edge.setField("_type", "applies_to");
        
        auto edge_status = graph_manager_->addEdge(
            applies_edge,
            scenario.graph_id
        );
        
        if (!edge_status.ok) {
            return Status::Error("Failed to add applies_to edge");
        }
    }
    
    return Status::OK();
}

MoralAnalyzer::Status MoralAnalyzer::addActionNodes(
    const EthicalScenario& scenario,
    const std::string& scenario_node_id
) {
    for (const auto& action : scenario.possible_actions) {
        std::string action_id = "action_" + scenario.id + "_" + action;
        
        BaseEntity action_entity(action_id);
        action_entity.setField("type", "action");
        action_entity.setField("description", action);
        action_entity.setField("_labels", std::string("Action"));
        
        auto status = graph_manager_->addNode(
            action_entity,
            scenario.graph_id
        );
        
        if (!status.ok) {
            return Status::Error("Failed to add action node");
        }
        
        // Create edge: scenario -> considers -> action
        BaseEntity considers_edge("considers_" + scenario.id + "_" + action);
        considers_edge.setField("_from", scenario_node_id);
        considers_edge.setField("_to", action_id);
        considers_edge.setField("_type", "considers");
        
        auto edge_status = graph_manager_->addEdge(
            considers_edge,
            scenario.graph_id
        );
        
        if (!edge_status.ok) {
            return Status::Error("Failed to add considers edge");
        }
    }
    
    return Status::OK();
}

MoralAnalyzer::Status MoralAnalyzer::addOutcomeNodes(
    const std::string& action_id,
    const std::vector<PredictedOutcome>& outcomes
) {
    for (size_t i = 0; i < outcomes.size(); ++i) {
        const auto& outcome = outcomes[i];
        std::string outcome_id = action_id + "_outcome_" + std::to_string(i);
        
        BaseEntity outcome_entity(outcome_id);
        outcome_entity.setField("type", "outcome");
        outcome_entity.setField("description", outcome.description);
        outcome_entity.setField("probability", outcome.probability);
        outcome_entity.setField("utility", outcome.utility);
        outcome_entity.setField("_labels", std::string("Outcome"));
        
        // Extract graph_id from action_id (assuming format: action_<scenario_id>_...)
        // For now, use default graph
        std::string graph_id = "ethics_default";
        
        auto status = graph_manager_->addNode(outcome_entity, graph_id);
        
        if (!status.ok) {
            return Status::Error("Failed to add outcome node");
        }
        
        // Create edge: action -> leads_to -> outcome
        BaseEntity leads_edge(action_id + "_leads_" + std::to_string(i));
        leads_edge.setField("_from", action_id);
        leads_edge.setField("_to", outcome_id);
        leads_edge.setField("_type", "leads_to");
        
        auto edge_status = graph_manager_->addEdge(leads_edge, graph_id);
        
        if (!edge_status.ok) {
            return Status::Error("Failed to add leads_to edge");
        }
    }
    
    return Status::OK();
}

MoralAnalyzer::Status MoralAnalyzer::addArgumentNodes(
    const std::string& action_id,
    const std::vector<EthicalArgument>& arguments
) {
    std::string graph_id = "ethics_default";
    
    for (const auto& arg : arguments) {
        std::string arg_node_id = "argument_" + arg.id;
        
        BaseEntity arg_entity(arg_node_id);
        arg_entity.setField("type", "argument");
        arg_entity.setField("content", arg.content);
        arg_entity.setField("philosophy", arg.philosophy);
        arg_entity.setField("principle_basis", arg.principle_basis);
        arg_entity.setField("argument_type", arg.argument_type);
        arg_entity.setField("strength", arg.strength);
        arg_entity.setField("_labels", std::string("Argument"));
        
        auto status = graph_manager_->addNode(arg_entity, graph_id);
        
        if (!status.ok) {
            return Status::Error("Failed to add argument node");
        }
        
        // Create edge based on argument type
        std::string edge_type = (arg.argument_type == "pro") ? "supports" : "opposes";
        BaseEntity arg_edge("arg_edge_" + arg.id);
        arg_edge.setField("_from", arg_node_id);
        arg_edge.setField("_to", action_id);
        arg_edge.setField("_type", edge_type);
        
        auto edge_status = graph_manager_->addEdge(arg_edge, graph_id);
        
        if (!edge_status.ok) {
            return Status::Error("Failed to add argument edge");
        }
    }
    
    return Status::OK();
}

std::pair<MoralAnalyzer::Status, MoralAnalyzer::EthicalDecision> 
MoralAnalyzer::analyzeWithPhilosophy(
    const EthicalScenario& scenario,
    const std::string& philosophy
) {
    EthicalDecision decision;
    decision.scenario_id = scenario.id;
    decision.philosophy = philosophy;
    decision.graph_id = scenario.graph_id;
    
    // Build decision graph
    auto build_status = buildDecisionGraph(scenario);
    if (!build_status.ok) {
        return {build_status, decision};
    }
    
    // Add principle nodes for this philosophy
    std::string scenario_node_id = "scenario_" + scenario.id;
    auto principle_status = addPrincipleNodes(scenario, scenario_node_id, philosophy);
    if (!principle_status.ok) {
        return {principle_status, decision};
    }
    
    // Evaluate each action with the selected philosophy
    std::vector<std::pair<std::string, ReasoningPath>> action_evaluations;
    
    for (const auto& action : scenario.possible_actions) {
        ReasoningPath path;
        
        if (philosophy == "kant" || philosophy == "deontological") {
            path = evaluateDeontological(scenario, action);
        } else if (philosophy == "utilitarian" || philosophy == "consequentialist") {
            path = evaluateConsequentialist(scenario, action);
        } else if (philosophy == "virtue") {
            path = evaluateVirtueEthics(scenario, action);
        } else {
            // Default to consequentialist
            path = evaluateConsequentialist(scenario, action);
        }
        
        path.action_id = action;
        action_evaluations.push_back({action, path});
    }
    
    // Select best action
    auto best_it = std::max_element(
        action_evaluations.begin(),
        action_evaluations.end(),
        [](const auto& a, const auto& b) {
            return a.second.total_score < b.second.total_score;
        }
    );
    
    if (best_it != action_evaluations.end()) {
        decision.recommended_action = best_it->first;
        decision.reasoning_path = best_it->second;
        decision.confidence = best_it->second.confidence;
    }
    
    // Generate reasoning text
    decision.reasoning = formatDecisionText(decision);
    
    // Extract principle citations
    decision.principle_citations = decision.reasoning_path.supporting_principles;
    
    // Calculate metrics
    decision.metrics.consistency = checkConsistency(decision);
    decision.metrics.fairness = assessFairness(decision);
    decision.metrics.transparency = 1.0;  // Graph-based is always transparent
    
    // Generate decision ID
    decision.decision_id = "decision_" + scenario.id + "_" + philosophy;
    
    return {Status::OK(), decision};
}

std::pair<MoralAnalyzer::Status, MoralAnalyzer::EthicalDecision>
MoralAnalyzer::analyzeMultiPhilosophy(
    const EthicalScenario& scenario,
    const std::vector<std::string>& philosophies
) {
    std::vector<std::pair<std::string, ReasoningPath>> all_paths;
    std::map<std::string, std::string> alternative_perspectives;
    
    // Analyze with each philosophy
    for (const auto& philosophy : philosophies) {
        auto [status, decision] = analyzeWithPhilosophy(scenario, philosophy);
        
        if (status.ok) {
            all_paths.push_back({philosophy, decision.reasoning_path});
            alternative_perspectives[philosophy] = decision.recommended_action;
        }
    }
    
    if (all_paths.empty()) {
        return {Status::Error("No philosophy analysis succeeded"), EthicalDecision{}};
    }
    
    // Synthesize decision from multiple perspectives
    EthicalDecision synthesized = synthesizeDecision(all_paths);
    synthesized.scenario_id = scenario.id;
    synthesized.graph_id = scenario.graph_id;
    synthesized.philosophy = "multi_philosophy_synthesis";
    synthesized.alternative_perspectives = alternative_perspectives;
    synthesized.decision_id = "decision_" + scenario.id + "_synthesis";
    
    return {Status::OK(), synthesized};
}

MoralAnalyzer::ReasoningPath MoralAnalyzer::evaluateDeontological(
    const EthicalScenario& scenario,
    const std::string& action
) {
    ReasoningPath path;
    path.action_id = action;
    
    // Load Kantian principles
    std::vector<std::string> kant_principles = {
        "categorical_imperative",
        "respect_for_persons",
        "universalizability",
        "human_dignity"
    };
    
    // Check if action respects these principles
    // NOTE: This uses keyword-based scoring. In production, replace with:
    // - LLM-based semantic analysis
    // - Integration with EthicalGuidelinesManager
    // - Rule-based reasoning engine
    double score = 0.0;
    for (const auto& principle : kant_principles) {
        double principle_score = scoreActionByPrinciples(action, {principle}, "kant");
        
        if (principle_score > 0.5) {
            path.supporting_principles.push_back(principle);
            score += principle_score;
        } else {
            path.opposing_principles.push_back(principle);
        }
    }
    
    path.total_score = score / kant_principles.size();
    path.confidence = 0.8;  // Deontological rules tend to be clear
    
    // Generate arguments
    EthicalArgument arg;
    arg.id = scenario.id + "_kant_" + action;
    arg.philosophy = "kant";
    arg.argument_type = (path.total_score > 0.5) ? "pro" : "contra";
    arg.strength = std::abs(path.total_score - 0.5) * 2.0;
    
    if (path.total_score > 0.5) {
        arg.content = "This action respects the categorical imperative and treats persons as ends in themselves.";
    } else {
        arg.content = "This action violates the categorical imperative by treating persons as mere means.";
    }
    
    path.arguments.push_back(arg);
    
    return path;
}

MoralAnalyzer::ReasoningPath MoralAnalyzer::evaluateConsequentialist(
    const EthicalScenario& scenario,
    const std::string& action
) {
    ReasoningPath path;
    path.action_id = action;
    
    // Predict outcomes
    path.outcomes = predictOutcomes(scenario, action);
    
    // Calculate expected utility
    double expected_utility = calculateExpectedUtility(path.outcomes);
    
    path.total_score = (expected_utility + 1.0) / 2.0;  // Normalize to 0-1
    path.confidence = 0.7;  // Outcome prediction has uncertainty
    
    // Utilitarian principles
    if (expected_utility > 0) {
        path.supporting_principles.push_back("maximize_utility");
        path.supporting_principles.push_back("greatest_good");
    } else {
        path.opposing_principles.push_back("minimize_harm");
    }
    
    // Generate argument
    EthicalArgument arg;
    arg.id = scenario.id + "_util_" + action;
    arg.philosophy = "utilitarian";
    arg.argument_type = (expected_utility > 0) ? "pro" : "contra";
    arg.strength = std::abs(expected_utility);
    
    std::ostringstream oss;
    oss << "Expected utility: " << expected_utility 
        << ". This action " << (expected_utility > 0 ? "maximizes" : "does not maximize")
        << " overall well-being.";
    arg.content = oss.str();
    
    path.arguments.push_back(arg);
    
    return path;
}

MoralAnalyzer::ReasoningPath MoralAnalyzer::evaluateVirtueEthics(
    const EthicalScenario& scenario,
    const std::string& action
) {
    ReasoningPath path;
    path.action_id = action;
    
    // Check alignment with virtues
    std::vector<std::string> virtues = {
        "wisdom", "courage", "justice", "temperance", "compassion"
    };
    
    double score = 0.0;
    for (const auto& virtue : virtues) {
        double virtue_score = scoreActionByPrinciples(action, {virtue}, "virtue");
        
        if (virtue_score > 0.5) {
            path.supporting_principles.push_back(virtue);
            score += virtue_score;
        }
    }
    
    path.total_score = score / virtues.size();
    path.confidence = 0.75;
    
    // Generate argument
    EthicalArgument arg;
    arg.id = scenario.id + "_virtue_" + action;
    arg.philosophy = "virtue";
    arg.argument_type = (path.total_score > 0.5) ? "pro" : "contra";
    arg.strength = path.total_score;
    arg.content = "A virtuous person would " + 
                  std::string(path.total_score > 0.5 ? "take" : "avoid") + 
                  " this action.";
    
    path.arguments.push_back(arg);
    
    return path;
}

std::vector<MoralAnalyzer::PredictedOutcome> MoralAnalyzer::predictOutcomes(
    const EthicalScenario& scenario,
    const std::string& action
) {
    std::vector<PredictedOutcome> outcomes;
    
    // NOTE: Basic outcome prediction for demonstration.
    // In production, replace with:
    // - Historical case database lookups
    // - Causal inference models
    // - LLM-based outcome prediction
    // - Domain-specific predictive models
    
    PredictedOutcome positive;
    positive.description = "Positive outcome of " + action;
    positive.probability = 0.6;
    positive.utility = 0.7;
    positive.stakeholder_impacts = calculateStakeholderImpacts(scenario, action);
    
    PredictedOutcome negative;
    negative.description = "Negative outcome of " + action;
    negative.probability = 0.4;
    negative.utility = -0.3;
    negative.stakeholder_impacts = calculateStakeholderImpacts(scenario, action);
    
    outcomes.push_back(positive);
    outcomes.push_back(negative);
    
    return outcomes;
}

double MoralAnalyzer::calculateExpectedUtility(
    const std::vector<PredictedOutcome>& outcomes
) {
    double expected_utility = 0.0;
    
    for (const auto& outcome : outcomes) {
        expected_utility += outcome.probability * outcome.utility;
    }
    
    return expected_utility;
}

std::vector<MoralAnalyzer::EthicalArgument> MoralAnalyzer::generateArguments(
    const EthicalScenario& scenario,
    const std::string& action,
    const std::string& philosophy
) {
    std::vector<EthicalArgument> arguments;
    
    // Generate pro argument
    EthicalArgument pro;
    pro.id = scenario.id + "_" + philosophy + "_pro_" + action;
    pro.philosophy = philosophy;
    pro.argument_type = "pro";
    pro.strength = 0.7;
    pro.content = "From " + philosophy + " perspective, this action is justified.";
    
    arguments.push_back(pro);
    
    // Generate contra argument
    EthicalArgument contra;
    contra.id = scenario.id + "_" + philosophy + "_contra_" + action;
    contra.philosophy = philosophy;
    contra.argument_type = "contra";
    contra.strength = 0.3;
    contra.content = "From " + philosophy + " perspective, concerns exist about this action.";
    
    arguments.push_back(contra);
    
    return arguments;
}

MoralAnalyzer::EthicalDecision MoralAnalyzer::synthesizeDecision(
    const std::vector<std::pair<std::string, ReasoningPath>>& paths
) {
    EthicalDecision synthesized;
    
    // Count votes for each action
    std::map<std::string, double> action_scores;
    
    for (const auto& [philosophy, path] : paths) {
        action_scores[path.action_id] += path.total_score;
    }
    
    // Select action with highest total score
    auto best = std::max_element(
        action_scores.begin(),
        action_scores.end(),
        [](const auto& a, const auto& b) {
            return a.second < b.second;
        }
    );
    
    if (best != action_scores.end()) {
        synthesized.recommended_action = best->first;
        
        // Find the reasoning path for this action
        for (const auto& [philosophy, path] : paths) {
            if (path.action_id == best->first) {
                synthesized.reasoning_path = path;
                break;
            }
        }
    }
    
    // Calculate average confidence
    double total_confidence = 0.0;
    for (const auto& [_, path] : paths) {
        total_confidence += path.confidence;
    }
    // Guard against division by zero
    if (!paths.empty()) {
        synthesized.confidence = total_confidence / paths.size();
    } else {
        synthesized.confidence = 0.0;
    }
    
    // Generate synthesis reasoning
    std::ostringstream oss;
    oss << "After considering " << paths.size() << " philosophical perspectives, "
        << "the recommended action is: " << synthesized.recommended_action << ". ";
    
    for (const auto& [philosophy, path] : paths) {
        oss << philosophy << " ethics scores it at " << path.total_score << ". ";
    }
    
    synthesized.reasoning = oss.str();
    
    return synthesized;
}

double MoralAnalyzer::checkConsistency(const EthicalDecision& decision) {
    // Check if decision is consistent with stated principles
    double consistency = 0.8;  // Base consistency
    
    // Penalize if no principles cited
    if (decision.principle_citations.empty()) {
        consistency -= 0.2;
    }
    
    // Reward if multiple principles support the decision
    if (decision.principle_citations.size() >= 3) {
        consistency += 0.1;
    }
    
    return std::min(1.0, std::max(0.0, consistency));
}

double MoralAnalyzer::assessFairness(const EthicalDecision& decision) {
    // Assess fairness based on stakeholder impacts
    double fairness = 0.7;  // Base fairness
    
    // Check if decision considers all stakeholders
    if (decision.reasoning.find("stakeholder") != std::string::npos) {
        fairness += 0.2;
    }
    
    // Check if reasoning mentions fair treatment
    if (decision.reasoning.find("fair") != std::string::npos ||
        decision.reasoning.find("equal") != std::string::npos) {
        fairness += 0.1;
    }
    
    return std::min(1.0, fairness);
}

std::vector<std::pair<MoralAnalyzer::EthicalScenario, MoralAnalyzer::EthicalDecision>>
MoralAnalyzer::retrieveSimilarScenarios(
    const EthicalScenario& /*scenario*/,
    int /*limit*/
) {
    // In production: Use vector similarity search
    // For now: Return empty
    return {};
}

MoralAnalyzer::Status MoralAnalyzer::storeDecision(
    const EthicalDecision& decision,
    const std::vector<float>& scenario_embedding,
    const std::string& user_id
) {
    
    // 1. GRAPH STORAGE: Store full decision structure with reasoning chains
    BaseEntity decision_entity(decision.decision_id);
    decision_entity.setField("type", "decision");
    decision_entity.setField("scenario_id", decision.scenario_id);
    decision_entity.setField("philosophy", decision.philosophy);
    decision_entity.setField("recommended_action", decision.recommended_action);
    decision_entity.setField("reasoning", decision.reasoning);
    decision_entity.setField("confidence", decision.confidence);
    decision_entity.setField("_labels", std::string("Decision"));
    
    // Add principle citations to graph
    decision_entity.setField("principle_citations", std::string(json(decision.principle_citations).dump()));
    
    // Add alternative perspectives
    json alt_perspectives_json = json::object();
    for (const auto& [phil, perspective] : decision.alternative_perspectives) {
        alt_perspectives_json[phil] = perspective;
    }
    decision_entity.setField("alternative_perspectives", alt_perspectives_json.dump());
    
    // Add metrics
    decision_entity.setField("metric_consistency", decision.metrics.consistency);
    decision_entity.setField("metric_fairness", decision.metrics.fairness);
    decision_entity.setField("metric_transparency", decision.metrics.transparency);
    decision_entity.setField("metric_feasibility", decision.metrics.feasibility);
    decision_entity.setField("metric_long_term_impact", decision.metrics.long_term_impact);
    
    auto status = graph_manager_->addNode(decision_entity, decision.graph_id);
    
    if (!status.ok) {
        return Status::Error("Failed to store decision in graph: " + status.message);
    }
    
    // Create edge: decision -> based_on -> scenario
    BaseEntity based_on_edge("based_on_" + decision.decision_id);
    based_on_edge.setField("_from", decision.decision_id);
    based_on_edge.setField("_to", "scenario_" + decision.scenario_id);
    based_on_edge.setField("_type", "based_on");
    
    auto edge_status = graph_manager_->addEdge(based_on_edge, decision.graph_id);
    
    if (!edge_status.ok) {
        return Status::Error("Failed to store decision edge in graph");
    }
    
    // 2. VECTOR STORAGE: Store scenario embedding for similarity search
    if (vector_index_ && !scenario_embedding.empty()) {
        BaseEntity vector_entity(decision.scenario_id + "_embedding");
        vector_entity.setField("embedding", scenario_embedding);
        vector_entity.setField("scenario_id", decision.scenario_id);
        vector_entity.setField("decision_id", decision.decision_id);
        vector_entity.setField("philosophy", decision.philosophy);
        vector_entity.setField("domain", "ethics_scenarios");
        
        auto vec_status = vector_index_->addEntity(vector_entity, "embedding");
        if (!vec_status.ok) {
            // Log warning but don't fail - vector storage is optional
            // In production: Log to monitoring system
        }
    }
    
    // 3. RELATIONAL STORAGE: Store keywords and metadata for structured queries
    auto keywords = extractKeywords(decision);
    
    BaseEntity metadata_entity("decision_metadata_" + decision.decision_id);
    metadata_entity.setField("decision_id", decision.decision_id);
    metadata_entity.setField("scenario_id", decision.scenario_id);
    metadata_entity.setField("philosophy", decision.philosophy);
    metadata_entity.setField("recommended_action", decision.recommended_action);
    metadata_entity.setField("confidence", decision.confidence);
    metadata_entity.setField("keywords", std::string(json(keywords).dump()));
    metadata_entity.setField("timestamp", std::chrono::system_clock::now().time_since_epoch().count());
    metadata_entity.setField("principle_count", static_cast<int>(decision.principle_citations.size()));
    metadata_entity.setField("metrics_avg", 
        (decision.metrics.consistency + decision.metrics.fairness + 
         decision.metrics.transparency + decision.metrics.feasibility + 
         decision.metrics.long_term_impact) / 5.0);
    
    bool metadata_stored = db_.put("ethics_metadata:" + decision.decision_id, 
                                     metadata_entity.serialize());
    if (!metadata_stored) {
        return Status::Error("Failed to store decision metadata");
    }
    
    // 4. AUDIT TRAIL: Log decision for compliance and transparency
    if (decision_auditor_) {
        AIDecisionAudit audit;
        audit.decision_id = decision.decision_id;
        audit.user_id = user_id;  // Use configurable user_id
        audit.session_id = decision.graph_id;
        audit.timestamp = std::chrono::system_clock::now();
        audit.query = "Ethical scenario: " + decision.scenario_id;
        
        // Context
        json context_json;
        context_json["scenario_id"] = decision.scenario_id;
        context_json["philosophy"] = decision.philosophy;
        context_json["graph_id"] = decision.graph_id;
        context_json["principles"] = decision.principle_citations;
        audit.context = context_json;
        
        // Model information
        audit.model_name = "MoralAnalyzer";
        audit.model_version = "1.0";
        json model_params;
        model_params["philosophy"] = decision.philosophy;
        model_params["metrics"] = {
            {"consistency", decision.metrics.consistency},
            {"fairness", decision.metrics.fairness},
            {"transparency", decision.metrics.transparency},
            {"feasibility", decision.metrics.feasibility},
            {"long_term_impact", decision.metrics.long_term_impact}
        };
        audit.model_params = model_params;
        
        // Output
        audit.response = decision.recommended_action;
        audit.confidence_score = static_cast<float>(decision.confidence);
        
        // Collect alternatives
        for (const auto& [phil, perspective] : decision.alternative_perspectives) {
            audit.alternatives.push_back(phil + ": " + perspective);
        }
        
        // Explainability
        audit.explanation = decision.reasoning;
        audit.reasoning_steps = decision.principle_citations;
        
        json key_factors;
        key_factors["action"] = decision.recommended_action;
        key_factors["confidence"] = decision.confidence;
        key_factors["principles_applied"] = decision.principle_citations.size();
        key_factors["keywords"] = keywords;
        audit.key_factors = key_factors;
        
        // Human review flag for low confidence decisions
        audit.requires_human_review = (decision.confidence < 0.7);
        
        // Log the audit entry
        decision_auditor_->logDecision(audit);
        {
            // Log warning but don't fail - audit is important but shouldn't block
            // In production: Alert monitoring system
        }
    }
    
    return Status::OK();
}

std::vector<std::string> MoralAnalyzer::extractKeywords(const EthicalDecision& decision) {
    /**
     * Basic keyword extraction from ethical decision
     * 
     * Strategy:
     * - Extract significant words from principles and reasoning
     * - Filter common stopwords (limited list - could be expanded)
     * - Add philosophy, action, and metric-based tags
     * - No stemming or lemmatization (future enhancement)
     * - Case-insensitive matching
     * 
     * Limitations:
     * - Basic tokenization (space-separated only)
     * - Limited stopword list (can be extended as needed)
     * - No linguistic processing (no stemming, NER, etc.)
     * - English-only (no locale support)
     */
    
    std::vector<std::string> keywords;
    
    // Common English stopwords (basic list - can be expanded)
    static const std::unordered_set<std::string> stopwords = {
        "should", "would", "could", "their", "there", "where",
        "this", "that", "with", "from", "about", "have", "been",
        "will", "what", "when", "which", "them", "then", "than"
    };
    
    // Add philosophy as keyword
    keywords.push_back(decision.philosophy);
    
    // Add action as keyword
    keywords.push_back(decision.recommended_action);
    
    // Extract keywords from principles
    for (const auto& principle : decision.principle_citations) {
        // Simple keyword extraction: split on spaces and take significant words
        std::istringstream iss(principle);
        std::string word;
        while (iss >> word) {
            // Convert to lowercase (locale-agnostic for ASCII)
            for (auto& c : word) {
                if (c >= 'A' && c <= 'Z') {
                    c = c - 'A' + 'a';
                }
            }
            
            // Remove punctuation
            word.erase(std::remove_if(word.begin(), word.end(), 
                [](unsigned char c) { return std::ispunct(c); }), word.end());
            
            // Add if significant (longer than 4 chars, not stopword)
            if (word.length() > 4 && stopwords.find(word) == stopwords.end()) {
                keywords.push_back(word);
            }
        }
    }
    
    // Add metrics as keywords if they're notable
    if (decision.metrics.consistency > 0.8) {
      keywords.push_back("high_consistency");
    }
    if (decision.metrics.fairness > 0.8) {
      keywords.push_back("high_fairness");
    }
    if (decision.metrics.feasibility > 0.8) {
      keywords.push_back("highly_feasible");
    }
    if (decision.metrics.long_term_impact > 0.8) {
      keywords.push_back("high_impact");
    }
    
    // Remove duplicates
    std::sort(keywords.begin(), keywords.end());
    keywords.erase(std::unique(keywords.begin(), keywords.end()), keywords.end());
    
    return keywords;
}

std::string MoralAnalyzer::exportDecisionGraphDOT(const std::string& scenario_id) {
    std::ostringstream dot;
    
    dot << "digraph EthicalDecision {" << std::endl;
    dot << "  rankdir=TB;" << std::endl;
    dot << "  node [shape=box];" << std::endl;
    
    // Add scenario node
    dot << "  scenario [label=\"Scenario\\n" << scenario_id << "\"];" << std::endl;
    
    // In production: Query graph and generate full DOT representation
    
    dot << "}" << std::endl;
    
    return dot.str();
}

std::string MoralAnalyzer::getReasoningExplanation(
    const EthicalDecision& decision
) {
    std::ostringstream explanation;
    
    explanation << "Ethical Decision Analysis\n";
    explanation << "========================\n\n";
    explanation << "Scenario: " << decision.scenario_id << "\n";
    explanation << "Philosophy: " << decision.philosophy << "\n";
    explanation << "Recommended Action: " << decision.recommended_action << "\n";
    explanation << "Confidence: " << (decision.confidence * 100) << "%\n\n";
    
    explanation << "Reasoning:\n";
    explanation << decision.reasoning << "\n\n";
    
    explanation << "Supporting Principles:\n";
    for (const auto& principle : decision.principle_citations) {
        explanation << "  - " << principle << "\n";
    }
    
    explanation << "\nMetrics:\n";
    explanation << "  Consistency: " << (decision.metrics.consistency * 100) << "%\n";
    explanation << "  Fairness: " << (decision.metrics.fairness * 100) << "%\n";
    explanation << "  Transparency: " << (decision.metrics.transparency * 100) << "%\n";
    
    if (!decision.alternative_perspectives.empty()) {
        explanation << "\nAlternative Perspectives:\n";
        for (const auto& [phil, action] : decision.alternative_perspectives) {
            explanation << "  " << phil << ": " << action << "\n";
        }
    }
    
    return explanation.str();
}

MoralAnalyzer::Status MoralAnalyzer::clearDecisionGraph(
    const std::string& /*scenario_id*/
) {
    // In production: Delete all nodes and edges related to this scenario
    return Status::OK();
}

std::vector<std::string> MoralAnalyzer::loadPrinciplesForPhilosophy(
    const std::string& philosophy
) {
    // Use ethical guidelines manager if available
    if (ethical_guidelines_) {
        // In production: Query from ethical guidelines manager
    }
    
    // Default principles by philosophy
    if (philosophy == "kant" || philosophy == "deontological") {
        return {"categorical_imperative", "respect_for_persons", "universalizability"};
    } else if (philosophy == "utilitarian" || philosophy == "consequentialist") {
        return {"maximize_utility", "greatest_good", "impartial_consideration"};
    } else if (philosophy == "virtue") {
        return {"wisdom", "courage", "justice", "temperance", "compassion"};
    }
    
    return {};
}

double MoralAnalyzer::scoreActionByPrinciples(
    const std::string& action,
    const std::vector<std::string>& principles,
    const std::string& /*philosophy*/
) {
    // NOTE: Basic keyword-based scoring for demonstration.
    // In production, replace with:
    // - LLM semantic similarity
    // - Trained classification model
    // - Integration with EthicalGuidelinesManager
    // - Philosophy-specific reasoning rules
    
    double score = 0.5;  // Neutral baseline
    
    // Simple keyword matching
    std::string action_lower = action;
    std::transform(action_lower.begin(), action_lower.end(), 
                   action_lower.begin(), ::tolower);
    
    // Deterministic scoring based on keyword matches
    int matches = 0;
    for (const auto& principle : principles) {
        std::string principle_lower = principle;
        std::transform(principle_lower.begin(), principle_lower.end(),
                      principle_lower.begin(), ::tolower);
        if (action_lower.find(principle_lower) != std::string::npos) {
            matches++;
        }
    }
    
    // Adjust score based on match ratio (deterministic, reproducible)
    if (!principles.empty()) {
        double match_ratio = static_cast<double>(matches) / principles.size();
        score = 0.5 + (match_ratio * 0.4);  // Range: 0.5 to 0.9
    }
    
    return std::min(1.0, std::max(0.0, score));
}

std::map<std::string, double> MoralAnalyzer::calculateStakeholderImpacts(
    const EthicalScenario& scenario,
    const std::string& action
) {
    std::map<std::string, double> impacts;
    
    // NOTE: Basic impact calculation for demonstration.
    // In production, replace with:
    // - Stakeholder analysis models
    // - Historical outcome data
    // - Domain-specific impact assessment
    // - LLM-based impact prediction
    
    // Use deterministic impact calculation for reproducible/auditable decisions
    for (const auto& [stakeholder_type, count] : scenario.stakeholders) {
        // Base impact proportional to stakeholder count (deterministic)
        double impact = static_cast<double>(count) / 10.0;
        
        // Check if action mentions this stakeholder type (simple heuristic)
        std::string action_lower = action;
        std::string stakeholder_lower = stakeholder_type;
        std::transform(action_lower.begin(), action_lower.end(), action_lower.begin(), ::tolower);
        std::transform(stakeholder_lower.begin(), stakeholder_lower.end(), stakeholder_lower.begin(), ::tolower);
        
        if (action_lower.find(stakeholder_lower) != std::string::npos) {
            impact *= 1.5;  // Directly mentioned stakeholders have higher impact
        }
        
        impacts[stakeholder_type] = std::min(1.0, std::max(-1.0, impact));
    }
    
    return impacts;
}

std::string MoralAnalyzer::formatDecisionText(const EthicalDecision& decision) {
    std::ostringstream oss;
    
    oss << "From the perspective of " << decision.philosophy << " ethics, "
        << "the recommended action is: " << decision.recommended_action << ". ";
    
    if (!decision.reasoning_path.supporting_principles.empty()) {
        oss << "This decision is supported by the following principles: ";
        for (size_t i = 0; i < decision.reasoning_path.supporting_principles.size(); ++i) {
            if (i > 0) {
              oss << ", ";
            }
            oss << decision.reasoning_path.supporting_principles[i];
        }
        oss << ". ";
    }
    
    if (!decision.reasoning_path.opposing_principles.empty()) {
        oss << "However, some principles suggest caution: ";
        for (size_t i = 0; i < decision.reasoning_path.opposing_principles.size(); ++i) {
            if (i > 0) {
              oss << ", ";
            }
            oss << decision.reasoning_path.opposing_principles[i];
        }
        oss << ". ";
    }
    
    if (!decision.reasoning_path.outcomes.empty()) {
        oss << "Expected outcomes include: ";
        for (size_t i = 0; i < decision.reasoning_path.outcomes.size(); ++i) {
            if (i > 0) {
              oss << ", ";
            }
            oss << decision.reasoning_path.outcomes[i].description
                << " (probability: " << decision.reasoning_path.outcomes[i].probability << ")";
        }
        oss << ". ";
    }
    
    return oss.str();
}

bool MoralAnalyzer::validateScenario(const EthicalScenario& scenario) {
    if (scenario.id.empty()) {
      return false;
    }
    if (scenario.description.empty()) {
      return false;
    }
    if (scenario.possible_actions.empty()) {
      return false;
    }
    return true;
}

std::vector<std::string> MoralAnalyzer::recommendPhilosophies(
    const EthicalScenario& scenario,
    bool use_llm
) {
    std::vector<std::string> recommendations;
    
    // Convert description to lowercase for case-insensitive matching
    std::string desc_lower = scenario.description;
    std::transform(desc_lower.begin(), desc_lower.end(), 
                   desc_lower.begin(), ::tolower);
    
    // Convert domain to lowercase
    std::string domain_lower = scenario.domain;
    std::transform(domain_lower.begin(), domain_lower.end(),
                   domain_lower.begin(), ::tolower);
    
    // Helper lambda for checking if text contains a keyword
    auto contains = [](const std::string& text, const std::string& keyword) {
        return text.find(keyword) != std::string::npos;
    };
    
    // Rule-based recommendations based on keywords
    
    // Deontological (Kantian) ethics - duty, rules, obligations
    if (contains(desc_lower, "duty") || contains(desc_lower, "obligation") ||
        contains(desc_lower, "rule") || contains(desc_lower, "principle") ||
        contains(desc_lower, "categorical") || contains(desc_lower, "universal")) {
        recommendations.push_back("kant");
    }
    
    // Consequentialist (Utilitarian) ethics - outcomes, utility, greatest good
    if (contains(desc_lower, "greatest good") || contains(desc_lower, "utility") ||
        contains(desc_lower, "consequence") || contains(desc_lower, "outcome") ||
        contains(desc_lower, "maximize") || contains(desc_lower, "benefit") ||
        contains(desc_lower, "harm")) {
        recommendations.push_back("utilitarian");
    }
    
    // Virtue ethics - character, virtue, excellence
    if (contains(desc_lower, "character") || contains(desc_lower, "virtue") ||
        contains(desc_lower, "excellence") || contains(desc_lower, "wisdom") ||
        contains(desc_lower, "courage") || contains(desc_lower, "integrity")) {
        recommendations.push_back("virtue");
    }
    
    // Care ethics - relationships, care, compassion
    if (contains(desc_lower, "care") || contains(desc_lower, "relationship") ||
        contains(desc_lower, "compassion") || contains(desc_lower, "empathy") ||
        contains(desc_lower, "nurture") || contains(desc_lower, "connection")) {
        recommendations.push_back("care_ethics");
    }
    
    // Rawlsian justice - fairness, justice, rights, equality
    if (contains(desc_lower, "justice") || contains(desc_lower, "fairness") ||
        contains(desc_lower, "right") || contains(desc_lower, "equality") ||
        contains(desc_lower, "equal") || contains(desc_lower, "impartial")) {
        recommendations.push_back("rawls");
    }
    
    // Domain-specific recommendations
    if (contains(domain_lower, "medical") || contains(domain_lower, "healthcare")) {
        // Medical ethics often involves care ethics and deontology
        if (std::find(recommendations.begin(), recommendations.end(), "care_ethics") == 
            recommendations.end()) {
            recommendations.push_back("care_ethics");
        }
    }
    
    if (contains(domain_lower, "autonomous") || contains(domain_lower, "ai") ||
        contains(domain_lower, "technology")) {
        // Technology ethics often involves consequentialism
        if (std::find(recommendations.begin(), recommendations.end(), "utilitarian") == 
            recommendations.end()) {
            recommendations.push_back("utilitarian");
        }
    }
    
    if (contains(domain_lower, "legal") || contains(domain_lower, "justice")) {
        // Legal ethics involves Rawlsian justice
        if (std::find(recommendations.begin(), recommendations.end(), "rawls") == 
            recommendations.end()) {
            recommendations.push_back("rawls");
        }
    }
    
    // Use LLM for semantic analysis of implicit ethical implications
    if (use_llm && llm_engine_) {
        auto [status, llm_recommendations] = detectEthicalImplicationsViaLLM(scenario);
        if (status.ok) {
            // Merge LLM recommendations with keyword-based ones
            for (const auto& rec : llm_recommendations) {
                if (std::find(recommendations.begin(), recommendations.end(), rec) == 
                    recommendations.end()) {
                    recommendations.push_back(rec);
                }
            }
        }
        // NOTE: If LLM fails, we continue with keyword-based recommendations
    }
    
    // If no clear match, recommend multi-philosophy ensemble
    if (recommendations.empty()) {
        recommendations.push_back("kant");
        recommendations.push_back("utilitarian");
        recommendations.push_back("virtue");
    }
    
    // Remove duplicates while preserving order
    std::vector<std::string> unique_recs;
    for (const auto& rec : recommendations) {
        if (std::find(unique_recs.begin(), unique_recs.end(), rec) == unique_recs.end()) {
            unique_recs.push_back(rec);
        }
    }
    
    return unique_recs;
}

std::pair<MoralAnalyzer::Status, std::vector<std::string>> 
MoralAnalyzer::detectEthicalImplicationsViaLLM(
    const EthicalScenario& scenario
) {
    if (!llm_engine_) {
        return {Status::Error("LLM engine not available"), {}};
    }
    
    // Construct prompt for ethical implication detection
    std::stringstream prompt;
    prompt << "Analyze the following ethical scenario and identify implicit moral concerns "
           << "that may not be explicitly stated with keywords like 'duty', 'harm', or 'fairness'.\n\n"
           << "Scenario Description: " << scenario.description << "\n"
           << "Domain: " << scenario.domain << "\n\n"
           << "Identify:\n"
           << "1. Implicit power dynamics or vulnerabilities\n"
           << "2. Contextual ethical tensions (even without explicit ethical keywords)\n"
           << "3. Domain-specific ethical concerns (e.g., medical consent, AI bias, privacy)\n"
           << "4. Cultural or social nuances that raise ethical questions\n\n"
           << "For each ethical concern, suggest the most relevant philosophical framework:\n"
           << "- Kantian deontology (for duty, rights, respect for persons)\n"
           << "- Utilitarian consequentialism (for outcomes, maximizing welfare)\n"
           << "- Virtue ethics (for character, moral excellence)\n"
           << "- Care ethics (for relationships, empathy, vulnerability)\n"
           << "- Rawlsian justice (for fairness, equality, rights)\n\n"
           << "Respond in JSON format: {\"philosophies\": [\"kant\", \"utilitarian\", ...], "
           << "\"reasoning\": \"explanation of detected implications\"}\n";
    
    // Create LLM request
    InferenceEngineEnhanced::EnhancedInferenceRequest request;
    request.base_request.prompt = prompt.str();
    request.base_request.max_tokens = 500;
    request.base_request.temperature = 0.3f;  // Lower temperature for more focused analysis
    request.base_request.top_p = 0.9f;
    request.priority = 5;  // Medium priority
    request.allow_caching = true;  // Cache similar scenario analyses
    
    try {
        // Submit request and wait for response
        auto handle = llm_engine_->submit(request);
        auto response = handle.get();
        
        if (response.text.empty()) {
            return {Status::Error("LLM inference failed: empty response"), {}};
        }
        
        // Parse JSON response
        try {
            json response_json = json::parse(response.text);
            
            if (!response_json.contains("philosophies")) {
                return {Status::Error("LLM response missing 'philosophies' field"), {}};
            }
            
            std::vector<std::string> philosophies;
            for (const auto& phil : response_json["philosophies"]) {
                std::string phil_str = phil.get<std::string>();
                
                // Validate philosophy names
                if (phil_str == "kant" || phil_str == "utilitarian" || 
                    phil_str == "virtue" || phil_str == "care_ethics" || 
                    phil_str == "rawls") {
                    philosophies.push_back(phil_str);
                }
            }
            
            // Log reasoning for audit trail
            if (response_json.contains("reasoning") && decision_auditor_) {
                std::string reasoning = response_json["reasoning"].get<std::string>();
                // NOTE: In production, log this to audit trail for transparency
                // decision_auditor_->logReasoning(scenario.id, "llm_detection", reasoning);
            }
            
            return {Status::OK(), philosophies};
            
        } catch (const json::exception& e) {
            // If JSON parsing fails, try simple text extraction
            std::vector<std::string> fallback_philosophies;
            std::string text = response.text;
            std::transform(text.begin(), text.end(), text.begin(), ::tolower);
            
            // Simple keyword extraction from LLM response
            if (text.find("kant") != std::string::npos || 
                text.find("deontolog") != std::string::npos) {
                fallback_philosophies.push_back("kant");
            }
            if (text.find("utilitarian") != std::string::npos || 
                text.find("consequential") != std::string::npos) {
                fallback_philosophies.push_back("utilitarian");
            }
            if (text.find("virtue") != std::string::npos) {
                fallback_philosophies.push_back("virtue");
            }
            if (text.find("care") != std::string::npos) {
                fallback_philosophies.push_back("care_ethics");
            }
            if (text.find("rawls") != std::string::npos || 
                text.find("justice") != std::string::npos) {
                fallback_philosophies.push_back("rawls");
            }
            
            if (!fallback_philosophies.empty()) {
                return {Status::OK(), fallback_philosophies};
            }
            
            return {Status::Error("Failed to parse LLM response: " + std::string(e.what())), {}};
        }
        
    } catch (const std::exception& e) {
        return {Status::Error("LLM request exception: " + std::string(e.what())), {}};
    }
}

} // namespace llm
} // namespace themis

