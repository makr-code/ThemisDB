/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            moral_analyzer_example.cpp                         ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:43:55                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     395                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file moral_analyzer_example.cpp
 * @brief Example demonstrating the MoralAnalyzer for graph-based ethical reasoning
 * 
 * This example shows how to use the MoralAnalyzer class to:
 * 1. Create ethical scenarios (Trolley Problem, Autonomous Vehicle)
 * 2. Build decision graphs in ThemisDB
 * 3. Analyze scenarios with different philosophical frameworks
 * 4. Use multi-philosophy ensemble reasoning
 * 5. Store and visualize ethical decisions
 * 
 * Compile and run:
 *   mkdir -p build && cd build
 *   cmake ..
 *   make moral_analyzer_example
 *   ./moral_analyzer_example
 */

#include "llm/moral_analyzer.h"
#include "llm/ethical_guidelines_manager.h"
#include "storage/rocksdb_wrapper.h"
#include <iostream>
#include <memory>

using namespace themis;
using namespace themis::llm;

void printSeparator(const std::string& title = "") {
    std::cout << "\n" << std::string(70, '=') << "\n";
    if (!title.empty()) {
        std::cout << title << "\n";
        std::cout << std::string(70, '=') << "\n";
    }
}

void printDecision(const MoralAnalyzer::EthicalDecision& decision) {
    std::cout << "Decision ID: " << decision.decision_id << "\n";
    std::cout << "Philosophy: " << decision.philosophy << "\n";
    std::cout << "Recommended Action: " << decision.recommended_action << "\n";
    std::cout << "Confidence: " << (decision.confidence * 100) << "%\n";
    std::cout << "\nReasoning:\n" << decision.reasoning << "\n";
    
    if (!decision.principle_citations.empty()) {
        std::cout << "\nPrinciple Citations:\n";
        for (const auto& principle : decision.principle_citations) {
            std::cout << "  - " << principle << "\n";
        }
    }
    
    std::cout << "\nMetrics:\n";
    std::cout << "  Consistency: " << (decision.metrics.consistency * 100) << "%\n";
    std::cout << "  Fairness: " << (decision.metrics.fairness * 100) << "%\n";
    std::cout << "  Transparency: " << (decision.metrics.transparency * 100) << "%\n";
    
    if (!decision.alternative_perspectives.empty()) {
        std::cout << "\nAlternative Perspectives:\n";
        for (const auto& [phil, action] : decision.alternative_perspectives) {
            std::cout << "  " << phil << ": " << action << "\n";
        }
    }
}

/**
 * Example 1: Classic Trolley Problem
 */
void example1_trolleyProblem(MoralAnalyzer& analyzer) {
    printSeparator("Example 1: Classic Trolley Problem");
    
    // Create the trolley problem scenario
    MoralAnalyzer::EthicalScenario scenario;
    scenario.id = "trolley_001";
    scenario.description = 
        "A runaway trolley is heading towards five people tied to the tracks. "
        "You are standing next to a lever that can divert the trolley to a side track, "
        "where only one person is tied. Do you pull the lever?";
    scenario.domain = "classic_dilemma";
    scenario.stakeholders = {
        {"people_on_main_track", 5},
        {"person_on_side_track", 1},
        {"decision_maker", 1}
    };
    scenario.possible_actions = {
        "pull_lever",
        "do_nothing"
    };
    scenario.relevant_principles = {
        "minimize_harm",
        "do_not_kill",
        "respect_autonomy"
    };
    
    std::cout << "Scenario: " << scenario.description << "\n\n";
    
    // Analyze with Kantian ethics
    std::cout << "--- Analyzing with Kantian Ethics ---\n";
    auto [kant_status, kant_decision] = analyzer.analyzeWithPhilosophy(
        scenario,
        "kant"
    );
    
    if (kant_status.ok) {
        printDecision(kant_decision);
    } else {
        std::cerr << "Error: " << kant_status.message << "\n";
    }
    
    std::cout << "\n--- Analyzing with Utilitarian Ethics ---\n";
    auto [util_status, util_decision] = analyzer.analyzeWithPhilosophy(
        scenario,
        "utilitarian"
    );
    
    if (util_status.ok) {
        printDecision(util_decision);
    } else {
        std::cerr << "Error: " << util_status.message << "\n";
    }
}

/**
 * Example 2: Autonomous Vehicle Dilemma
 */
void example2_autonomousVehicle(MoralAnalyzer& analyzer) {
    printSeparator("Example 2: Autonomous Vehicle Dilemma");
    
    MoralAnalyzer::EthicalScenario scenario;
    scenario.id = "av_001";
    scenario.description = 
        "An autonomous vehicle's brakes fail. It must choose between hitting "
        "a barrier (risking passenger life) or swerving into pedestrians "
        "(risking pedestrian lives). What should the vehicle do?";
    scenario.domain = "autonomous_systems";
    scenario.stakeholders = {
        {"passenger", 1},
        {"pedestrians", 3},
        {"other_drivers", 2}
    };
    scenario.possible_actions = {
        "hit_barrier",
        "swerve_into_pedestrians",
        "emergency_brake"
    };
    scenario.relevant_principles = {
        "protect_passengers",
        "minimize_harm",
        "fairness",
        "responsibility"
    };
    
    std::cout << "Scenario: " << scenario.description << "\n\n";
    
    // Use multi-philosophy analysis
    std::cout << "--- Multi-Philosophy Analysis ---\n";
    auto [status, decision] = analyzer.analyzeMultiPhilosophy(
        scenario,
        {"kant", "utilitarian", "virtue"}
    );
    
    if (status.ok) {
        printDecision(decision);
        
        // Store the decision
        auto store_status = analyzer.storeDecision(decision);
        if (store_status.ok) {
            std::cout << "\n✓ Decision stored in ThemisDB\n";
        }
    } else {
        std::cerr << "Error: " << status.message << "\n";
    }
}

/**
 * Example 3: Medical Ethics - Organ Transplant
 */
void example3_organTransplant(MoralAnalyzer& analyzer) {
    printSeparator("Example 3: Medical Ethics - Organ Transplant");
    
    MoralAnalyzer::EthicalScenario scenario;
    scenario.id = "medical_001";
    scenario.description = 
        "A doctor has five patients who will die without organ transplants. "
        "A healthy patient comes in for a routine checkup. The doctor could "
        "harvest the healthy patient's organs to save the five. Should they?";
    scenario.domain = "medical_ethics";
    scenario.stakeholders = {
        {"healthy_patient", 1},
        {"patients_needing_organs", 5},
        {"doctor", 1},
        {"hospital", 1}
    };
    scenario.possible_actions = {
        "harvest_organs",
        "do_not_harvest",
        "seek_alternative_donors"
    };
    scenario.relevant_principles = {
        "do_no_harm",
        "respect_autonomy",
        "beneficence",
        "justice"
    };
    scenario.context_weights = {
        {"urgency", 0.9},
        {"certainty", 0.8},
        {"reversibility", 0.0}
    };
    
    std::cout << "Scenario: " << scenario.description << "\n\n";
    
    // Analyze with all three major frameworks
    std::vector<std::string> philosophies = {"kant", "utilitarian", "virtue"};
    
    for (const auto& philosophy : philosophies) {
        std::cout << "\n--- " << philosophy << " perspective ---\n";
        auto [status, decision] = analyzer.analyzeWithPhilosophy(scenario, philosophy);
        
        if (status.ok) {
            std::cout << "Recommended: " << decision.recommended_action << "\n";
            std::cout << "Confidence: " << (decision.confidence * 100) << "%\n";
            std::cout << "Score: " << decision.reasoning_path.total_score << "\n";
        }
    }
    
    // Get synthesis
    std::cout << "\n--- Synthesized Multi-Philosophy Decision ---\n";
    auto [synth_status, synth_decision] = analyzer.analyzeMultiPhilosophy(
        scenario,
        philosophies
    );
    
    if (synth_status.ok) {
        printDecision(synth_decision);
    }
}

/**
 * Example 4: Export Decision Graph
 */
void example4_exportGraph(MoralAnalyzer& analyzer) {
    printSeparator("Example 4: Visualize Decision Graph");
    
    // Create a simple scenario
    MoralAnalyzer::EthicalScenario scenario;
    scenario.id = "export_001";
    scenario.description = "Should AI be allowed to make life-or-death decisions?";
    scenario.domain = "ai_ethics";
    scenario.stakeholders = {
        {"humans_affected", 10},
        {"ai_developers", 5}
    };
    scenario.possible_actions = {
        "allow_with_oversight",
        "prohibit_completely",
        "allow_in_limited_contexts"
    };
    
    std::cout << "Building decision graph...\n";
    auto build_status = analyzer.buildDecisionGraph(scenario);
    
    if (build_status.ok) {
        std::cout << "✓ Decision graph built successfully\n\n";
        
        // Export to DOT format
        std::string dot = analyzer.exportDecisionGraphDOT(scenario.id);
        std::cout << "Decision Graph (DOT format):\n";
        std::cout << dot << "\n";
        
        std::cout << "\nTo visualize, save to file and run:\n";
        std::cout << "  dot -Tpng decision_graph.dot -o decision_graph.png\n";
    } else {
        std::cerr << "Error: " << build_status.message << "\n";
    }
}

/**
 * Example 5: Reasoning Explanation
 */
void example5_reasoningExplanation(MoralAnalyzer& analyzer) {
    printSeparator("Example 5: Detailed Reasoning Explanation");
    
    MoralAnalyzer::EthicalScenario scenario;
    scenario.id = "privacy_001";
    scenario.description = 
        "A company has user data that could help law enforcement catch a criminal. "
        "Should they provide the data without a warrant?";
    scenario.domain = "privacy_ethics";
    scenario.stakeholders = {
        {"users", 1000000},
        {"law_enforcement", 10},
        {"potential_victims", 5},
        {"criminal", 1}
    };
    scenario.possible_actions = {
        "provide_data_without_warrant",
        "require_warrant",
        "refuse_all_requests"
    };
    scenario.relevant_principles = {
        "privacy",
        "public_safety",
        "rule_of_law",
        "transparency"
    };
    
    std::cout << "Scenario: " << scenario.description << "\n\n";
    
    // Analyze with Kantian ethics
    auto [status, decision] = analyzer.analyzeWithPhilosophy(scenario, "kant");
    
    if (status.ok) {
        // Get detailed explanation
        std::string explanation = analyzer.getReasoningExplanation(decision);
        std::cout << explanation << "\n";
    }
}

/**
 * Main function - Run all examples
 */
int main(int argc, char* argv[]) {
    try {
        printSeparator("ThemisDB Moral Analyzer - Examples");
        
        std::cout << "\nInitializing ThemisDB...\n";
        
        // Initialize RocksDB
        RocksDBWrapper db = {};
        if (!db.open("/tmp/themisdb_moral_analyzer_example")) {
            std::cerr << "Failed to open database\n";
            return 1;
        }
        
        std::cout << "✓ Database initialized\n";
        
        // Create ethical guidelines manager
        auto ethical_guidelines = std::make_shared<EthicalGuidelinesManager>();
        std::cout << "✓ Ethical guidelines manager initialized\n";
        
        // Create moral analyzer
        MoralAnalyzer analyzer(db, ethical_guidelines);
        std::cout << "✓ Moral analyzer initialized\n";
        
        // Run examples
        example1_trolleyProblem(analyzer);
        example2_autonomousVehicle(analyzer);
        example3_organTransplant(analyzer);
        example4_exportGraph(analyzer);
        example5_reasoningExplanation(analyzer);
        
        printSeparator("All Examples Completed");
        std::cout << "\nThe MoralAnalyzer has demonstrated:\n";
        std::cout << "  ✓ Graph-based ethical scenario representation\n";
        std::cout << "  ✓ Multi-philosophy reasoning (Kant, Utilitarian, Virtue)\n";
        std::cout << "  ✓ Decision synthesis from multiple perspectives\n";
        std::cout << "  ✓ Transparent reasoning chains\n";
        std::cout << "  ✓ Storage in ThemisDB multi-model architecture\n";
        std::cout << "  ✓ Graph visualization export\n";
        std::cout << "\nFor production use:\n";
        std::cout << "  - Integrate with LLM for sophisticated reasoning\n";
        std::cout << "  - Add vector search for similar scenario retrieval\n";
        std::cout << "  - Implement bias detection and mitigation\n";
        std::cout << "  - Connect to ethics evaluation metrics\n";
        std::cout << "  - Enable real-time monitoring and dashboards\n";
        
        db.close();
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
        return 1;
    }
}
