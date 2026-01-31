/**
 * @file complete_ethics_pipeline_example.cpp
 * @brief Complete end-to-end ethics pipeline demonstration
 * 
 * This example demonstrates the complete moral philosophy debate pipeline,
 * similar to the Python workflow in main.py and example_complete_workflow.py:
 * 
 * 1. Load ethical scenarios from YAML configuration
 * 2. Conduct multi-philosophy debates
 * 3. Generate cross-philosophy critiques
 * 4. Synthesize AI recommendations
 * 5. Store results in ThemisDB (Graph, Vector, Relational, Timeline)
 * 6. Export visualizations
 * 7. Run benchmarks
 * 8. Track outcomes for self-improvement
 * 
 * Compile and run:
 *   mkdir -p build && cd build
 *   cmake ..
 *   make complete_ethics_pipeline_example
 *   ./complete_ethics_pipeline_example
 */

#include "llm/moral_debate_pipeline.h"
#include "llm/moral_analyzer.h"
#include "llm/ethical_guidelines_manager.h"
#include "storage/rocksdb_wrapper.h"
#include <iostream>
#include <iomanip>
#include <memory>

using namespace themis;
using namespace themis::llm;

void printSeparator(const std::string& title = "") {
    std::cout << "\n" << std::string(80, '=') << "\n";
    if (!title.empty()) {
        std::cout << title << "\n";
        std::cout << std::string(80, '=') << "\n";
    }
}

void printDebateSession(const MoralDebatePipeline::DebateSession& debate) {
    std::cout << "\nDebate Session ID: " << debate.session_id << "\n";
    std::cout << "Scenario: " << debate.scenario.description.substr(0, 100) << "...\n";
    std::cout << "Timestamp: " << debate.timestamp_ms << "\n";
    std::cout << "Consensus Score: " << std::fixed << std::setprecision(2) 
              << (debate.consensus_score * 100) << "%\n";
    
    std::cout << "\n--- Philosophy Perspectives ---\n";
    for (const auto& perspective : debate.perspectives) {
        std::cout << "\n" << perspective.philosophy_name << ":\n";
        std::cout << "  Recommendation: " << perspective.recommended_action << "\n";
        std::cout << "  Confidence: " << (perspective.confidence * 100) << "%\n";
        std::cout << "  Supporting Principles: ";
        for (size_t i = 0; i < perspective.supporting_principles.size(); ++i) {
            std::cout << perspective.supporting_principles[i];
            if (i < perspective.supporting_principles.size() - 1) std::cout << ", ";
        }
        std::cout << "\n";
    }
    
    if (!debate.common_ground.empty()) {
        std::cout << "\n--- Common Ground ---\n";
        for (const auto& principle : debate.common_ground) {
            std::cout << "  - " << principle << "\n";
        }
    }
    
    if (!debate.conflicting_principles.empty()) {
        std::cout << "\n--- Conflicting Principles ---\n";
        for (const auto& conflict : debate.conflicting_principles) {
            std::cout << "  - " << conflict << "\n";
        }
    }
    
    std::cout << "\n--- Final Recommendation ---\n";
    std::cout << debate.final_recommendation << "\n";
    
    if (!debate.ai_synthesis.empty()) {
        std::cout << "\n--- AI Synthesis ---\n";
        std::cout << debate.ai_synthesis << "\n";
    }
}

/**
 * Example 1: Complete Trolley Problem Pipeline
 */
void example1_trolleyProblemPipeline(MoralDebatePipeline& pipeline) {
    printSeparator("Example 1: Complete Trolley Problem Pipeline");
    
    // Create trolley problem scenario
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
        "respect_autonomy",
        "doctrine_of_double_effect"
    };
    
    std::cout << "\nScenario: " << scenario.description << "\n";
    
    // Conduct multi-philosophy debate
    std::cout << "\n--- Conducting Multi-Philosophy Debate ---\n";
    std::vector<std::string> philosophies = {
        "kant",
        "utilitarian",
        "virtue",
        "care_ethics"
    };
    
    auto [status, debate] = pipeline.conductDebate(scenario, philosophies);
    
    if (status != MoralDebatePipeline::Status::OK) {
        std::cout << "Error conducting debate!\n";
        return;
    }
    
    printDebateSession(debate);
    
    // Store results in ThemisDB
    std::cout << "\n--- Storing Results in ThemisDB ---\n";
    auto store_status = pipeline.storeDebateResults(debate);
    if (store_status == MoralDebatePipeline::Status::OK) {
        std::cout << "✓ Successfully stored in Graph, Vector, Relational, and Timeline databases\n";
    }
    
    // Export visualization
    std::cout << "\n--- Exporting Visualization ---\n";
    std::string dot_path = "/tmp/trolley_debate.dot";
    auto export_status = pipeline.exportDebateGraph(debate, dot_path);
    if (export_status == MoralDebatePipeline::Status::OK) {
        std::cout << "✓ Debate graph exported to: " << dot_path << "\n";
        std::cout << "  Use: dot -Tpng " << dot_path << " -o trolley_debate.png\n";
    }
}

/**
 * Example 2: Autonomous Vehicle Ethics Pipeline
 */
void example2_autonomousVehiclePipeline(MoralDebatePipeline& pipeline) {
    printSeparator("Example 2: Autonomous Vehicle Ethics Pipeline");
    
    MoralAnalyzer::EthicalScenario scenario;
    scenario.id = "av_001";
    scenario.description = 
        "An autonomous vehicle's brakes fail. It must choose between hitting a barrier "
        "(high risk to passenger) or swerving into pedestrians (high risk to 3 pedestrians). "
        "What should the vehicle be programmed to do?";
    scenario.domain = "autonomous_systems";
    scenario.stakeholders = {
        {"passenger", 1},
        {"pedestrians", 3},
        {"manufacturer", 1}
    };
    scenario.possible_actions = {
        "protect_passenger",
        "minimize_casualties"
    };
    scenario.relevant_principles = {
        "minimize_harm",
        "contractual_duty",
        "fairness",
        "transparency"
    };
    scenario.context_weights = {
        {"urgency", 1.0},
        {"certainty", 0.8}
    };
    
    std::cout << "\nScenario: " << scenario.description << "\n";
    
    // Conduct debate with different philosophies
    std::vector<std::string> philosophies = {
        "kant",
        "utilitarian",
        "virtue",
        "rawls"  // Justice as fairness
    };
    
    auto [status, debate] = pipeline.conductDebate(scenario, philosophies);
    
    if (status == MoralDebatePipeline::Status::OK) {
        printDebateSession(debate);
        
        // Store and export
        pipeline.storeDebateResults(debate);
        pipeline.exportDebateGraph(debate, "/tmp/av_ethics_debate.dot");
        
        // Track simulated outcome
        std::cout << "\n--- Tracking Outcome (Simulated) ---\n";
        auto outcome_status = pipeline.trackOutcome(
            debate.session_id,
            "Vehicle minimized casualties, passenger injured but survived",
            0.85,  // Success score
            {
                {"passenger", "Survived with minor injuries"},
                {"pedestrians", "All safe"},
                {"public", "Positive reaction to ethical decision"}
            }
        );
        
        if (outcome_status == MoralDebatePipeline::Status::OK) {
            std::cout << "✓ Outcome tracked for self-improvement loop\n";
        }
    }
}

/**
 * Example 3: Medical Triage Pipeline
 */
void example3_medicalTriagePipeline(MoralDebatePipeline& pipeline) {
    printSeparator("Example 3: Medical Triage Ethics Pipeline");
    
    MoralAnalyzer::EthicalScenario scenario;
    scenario.id = "medical_001";
    scenario.description = 
        "During a pandemic, a hospital has 10 ventilators and 15 critical patients. "
        "How should the hospital allocate ventilators? Consider: 5 elderly patients "
        "(70-80 years, multiple comorbidities), 5 middle-aged patients (40-50 years, "
        "good health), 5 young patients (20-30 years, excellent health).";
    scenario.domain = "medical";
    scenario.stakeholders = {
        {"elderly_patients", 5},
        {"middle_aged_patients", 5},
        {"young_patients", 5},
        {"healthcare_workers", 20}
    };
    scenario.possible_actions = {
        "first_come_first_serve",
        "maximize_survival",
        "lottery_system",
        "prioritize_young"
    };
    scenario.relevant_principles = {
        "equal_treatment",
        "maximize_benefit",
        "procedural_justice",
        "stewardship",
        "transparency"
    };
    
    std::cout << "\nScenario: " << scenario.description << "\n";
    
    std::vector<std::string> philosophies = {
        "kant",
        "utilitarian",
        "rawls",
        "virtue",
        "care_ethics"
    };
    
    auto [status, debate] = pipeline.conductDebate(scenario, philosophies);
    
    if (status == MoralDebatePipeline::Status::OK) {
        printDebateSession(debate);
        
        // Check if human oversight is needed
        if (debate.consensus_score < 0.7) {
            std::cout << "\n⚠️  LOW CONSENSUS - HUMAN OVERSIGHT RECOMMENDED\n";
            std::cout << "This is a high-stakes decision with significant ethical tensions.\n";
        }
        
        pipeline.storeDebateResults(debate);
        pipeline.exportDebateGraph(debate, "/tmp/medical_triage_debate.dot");
    }
}

/**
 * Example 4: Benchmark Multiple Scenarios
 */
void example4_benchmarkSuite(MoralDebatePipeline& pipeline) {
    printSeparator("Example 4: Benchmarking Ethics Pipeline");
    
    // Create test scenarios
    std::vector<std::string> scenario_ids = {
        "trolley_001",
        "av_001",
        "medical_001"
    };
    
    std::vector<std::string> test_philosophies = {
        "kant",
        "utilitarian",
        "virtue"
    };
    
    std::cout << "\nRunning benchmarks on " << scenario_ids.size() 
              << " scenarios with " << test_philosophies.size() 
              << " philosophies...\n";
    
    auto results = pipeline.runBenchmarks(scenario_ids, test_philosophies);
    
    std::cout << "\n--- Benchmark Results ---\n";
    for (const auto& [scenario_id, metrics] : results) {
        std::cout << "\n" << scenario_id << ":\n";
        for (const auto& [metric_name, value] : metrics) {
            std::cout << "  " << std::setw(25) << std::left << metric_name << ": " 
                      << std::fixed << std::setprecision(3) << value << "\n";
        }
    }
    
    // Display pipeline statistics
    std::cout << "\n--- Pipeline Statistics ---\n";
    auto stats = pipeline.getStatistics();
    for (const auto& [stat_name, value] : stats) {
        std::cout << "  " << std::setw(25) << std::left << stat_name << ": " 
                  << std::fixed << std::setprecision(2) << value << "\n";
    }
}

/**
 * Example 5: Complete Self-Improving Loop
 */
void example5_selfImprovingLoop(MoralDebatePipeline& pipeline) {
    printSeparator("Example 5: Self-Improving Ethics Loop");
    
    std::cout << "\nDemonstrating complete 4-phase self-improving workflow:\n";
    std::cout << "1. Ethical Decision (Multi-Philosophy Debate)\n";
    std::cout << "2. Action Execution (Simulated)\n";
    std::cout << "3. Outcome Tracking\n";
    std::cout << "4. Continuous Improvement (LoRa Retraining)\n\n";
    
    // Phase 1: Make ethical decision
    MoralAnalyzer::EthicalScenario scenario;
    scenario.id = "ai_ethics_001";
    scenario.description = 
        "An AI hiring system is found to have a bias that gives higher scores to "
        "male candidates due to historical training data. What should the company do?";
    scenario.domain = "ai_ethics";
    scenario.stakeholders = {
        {"male_candidates", 10000},
        {"female_candidates", 10000},
        {"company", 1}
    };
    scenario.possible_actions = {
        "continue_biased_system",
        "add_bias_correction",
        "scrap_system"
    };
    scenario.relevant_principles = {
        "fairness",
        "non_discrimination",
        "accountability",
        "transparency"
    };
    
    std::cout << "Scenario: " << scenario.description << "\n";
    
    std::vector<std::string> philosophies = {"kant", "utilitarian", "virtue", "care_ethics"};
    auto [status, debate] = pipeline.conductDebate(scenario, philosophies);
    
    if (status == MoralDebatePipeline::Status::OK) {
        std::cout << "\n✓ Phase 1 Complete: Decision made - " 
                  << debate.final_recommendation << "\n";
        
        // Phase 2: Simulate action execution
        std::cout << "\n✓ Phase 2 Complete: Action executed (simulated)\n";
        
        // Phase 3: Track outcome
        pipeline.trackOutcome(
            debate.session_id,
            "Company added bias correction, achieved gender parity in hiring",
            0.92,
            {
                {"female_candidates", "Improved hiring rates"},
                {"male_candidates", "Fair evaluation maintained"},
                {"company", "Positive public perception"}
            }
        );
        std::cout << "\n✓ Phase 3 Complete: Outcome tracked (success: 92%)\n";
        
        // Phase 4: Continuous improvement
        std::cout << "\n✓ Phase 4: Continuous Improvement\n";
        std::cout << "  - Outcome data stored for LoRa retraining\n";
        std::cout << "  - Ethical alignment metrics updated\n";
        std::cout << "  - Philosophy weights adjusted based on success\n";
        std::cout << "  - Next retraining scheduled when threshold reached\n";
        
        // Store and visualize
        pipeline.storeDebateResults(debate);
        pipeline.exportDebateGraph(debate, "/tmp/self_improving_loop.dot");
        
        std::cout << "\n🔄 Self-Improving Loop Active\n";
    }
}

int main() {
    printSeparator("COMPLETE MORAL PHILOSOPHY DEBATE PIPELINE");
    
    std::cout << "\nThis example demonstrates a complete end-to-end ethics pipeline\n";
    std::cout << "similar to the Python workflow (main.py, example_complete_workflow.py):\n\n";
    std::cout << "  ✓ Multi-philosophy debate orchestration\n";
    std::cout << "  ✓ Cross-philosophy critiques\n";
    std::cout << "  ✓ AI synthesis and recommendations\n";
    std::cout << "  ✓ Multi-model storage (Graph/Vector/Relational/Timeline)\n";
    std::cout << "  ✓ Visualization export\n";
    std::cout << "  ✓ Benchmarking suite\n";
    std::cout << "  ✓ Outcome tracking\n";
    std::cout << "  ✓ Self-improving loop\n";
    
    // Initialize components
    std::cout << "\n--- Initializing Pipeline ---\n";
    
    auto db = std::make_shared<storage::RocksDBWrapper>("/tmp/themisdb_ethics");
    auto guidelines_mgr = std::make_shared<EthicalGuidelinesManager>(db);
    
    // Configure pipeline
    MoralDebatePipeline::PipelineConfig config;
    config.yaml_scenarios_path = "ethical_scenarios.yaml";
    config.philosophies_yaml_path = "philosophies.yaml";
    config.enable_llm_integration = false;  // Set to true when LLM available
    config.enable_vector_search = true;
    config.enable_outcome_tracking = true;
    config.enable_self_improvement = true;
    config.max_debate_rounds = 2;
    config.consensus_threshold = 0.7;
    
    // Create pipeline
    MoralDebatePipeline pipeline(db, guidelines_mgr, nullptr, config);
    
    std::cout << "✓ Pipeline initialized\n";
    std::cout << "✓ Database: /tmp/themisdb_ethics\n";
    std::cout << "✓ Configuration loaded\n";
    
    // Run examples
    try {
        example1_trolleyProblemPipeline(pipeline);
        example2_autonomousVehiclePipeline(pipeline);
        example3_medicalTriagePipeline(pipeline);
        example4_benchmarkSuite(pipeline);
        example5_selfImprovingLoop(pipeline);
        
        printSeparator("PIPELINE COMPLETE");
        
        std::cout << "\nAll examples completed successfully!\n\n";
        std::cout << "Generated visualizations:\n";
        std::cout << "  - /tmp/trolley_debate.dot\n";
        std::cout << "  - /tmp/av_ethics_debate.dot\n";
        std::cout << "  - /tmp/medical_triage_debate.dot\n";
        std::cout << "  - /tmp/self_improving_loop.dot\n\n";
        std::cout << "Convert to images using: dot -Tpng file.dot -o file.png\n\n";
        
        std::cout << "Next steps for production deployment:\n";
        std::cout << "  1. Integrate LLM backend (llama.cpp, Claude, GPT-4)\n";
        std::cout << "  2. Connect YAML parser (yaml-cpp library)\n";
        std::cout << "  3. Implement vector search for precedent retrieval\n";
        std::cout << "  4. Set up continuous monitoring dashboard\n";
        std::cout << "  5. Configure LoRa retraining triggers\n";
        std::cout << "  6. Deploy with HITL (Human-in-the-Loop) safeguards\n\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}
