/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            example_basic_usage.cpp                            ║
  Version:         0.0.39                                             ║
  Last Modified:   2026-04-13 20:28:40                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     273                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * Ethics AI Plugin - Basic Usage Example
 * 
 * This example demonstrates:
 * 1. Loading philosophy profiles
 * 2. Storing and retrieving arguments
 * 3. Making ethical decisions
 * 4. Evaluating decisions
 * 5. Accessing metrics
 */

#include "plugins/ethics_ai/ethics_ai_types.h"
#include "plugins/ethics_ai/ethics_ai_plugin_interface.h"
#include "plugins/ethics_ai/philosophy_loader.h"
#include "plugins/ethics_ai/argument_store.h"
#include "plugins/ethics_ai/rag_context_engine.h"
#include "plugins/ethics_ai/discourse_engine.h"
#include "plugins/ethics_ai/ethics_evaluator.h"
#include <iostream>
#include <iomanip>
#include <memory>

using namespace themis::plugins::ethics;

void printSeparator(const std::string& title = "") {
    std::cout << "\n";
    std::cout << "═══════════════════════════════════════════════════════════════" << std::endl;
    if (!title.empty()) {
        std::cout << "  " << title << std::endl;
        std::cout << "═══════════════════════════════════════════════════════════════" << std::endl;
    }
}

void demonstratePhilosophyLoader() {
    printSeparator("1. Philosophy Profile Loading");
    
    PhilosophyLoader loader;
    
    // Load from actual philosophies directory
    std::string phil_dir = "plugins/ethics_ai/philosophies";
    auto result = loader.loadFromDirectory(phil_dir);
    
    if (auto* count = std::get_if<size_t>(&result)) {
        std::cout << "✓ Loaded " << *count << " philosophy profiles" << std::endl;
        
        // List all loaded schools
        auto schools = loader.getSchoolIds();
        std::cout << "\nAvailable philosophy schools:" << std::endl;
        for (const auto& school : schools) {
            auto profile_result = loader.getProfile(school);
            if (auto* profile = std::get_if<PhilosophyProfile>(&profile_result)) {
                std::cout << "  • " << school << ": " << profile->name;
                if (!profile->main_theses.empty()) {
                    std::cout << " (" << profile->main_theses.size() << " main theses)";
                }
                std::cout << std::endl;
            }
        }
        
        // Show details of Kantian ethics
        auto kant_result = loader.getProfile("kant");
        if (auto* kant = std::get_if<PhilosophyProfile>(&kant_result)) {
            std::cout << "\nKantian Ethics Details:" << std::endl;
            std::cout << "  Name: " << kant->name << std::endl;
            std::cout << "  Main Theses: " << kant->main_theses.size() << std::endl;
            if (!kant->main_theses.empty()) {
                std::cout << "    - " << kant->main_theses[0] << std::endl;
            }
        }
    } else {
        auto status = std::get<Status>(result);
        std::cout << "✗ Failed to load: " << status.message << std::endl;
    }
}

void demonstrateArgumentStore() {
    printSeparator("2. Argument Storage and Retrieval");
    
    ArgumentStore store;
    std::map<std::string, std::string> config;
    
    if (!store.initialize(config).isOK()) {
        std::cout << "✗ Failed to initialize argument store" << std::endl;
        return;
    }
    
    // Create and store arguments
    EthicalArgument kant_arg;
    kant_arg.id = "kant_001";
    kant_arg.philosophy_school = "kant";
    kant_arg.argument_type = ArgumentType::PRO;
    kant_arg.content = "All persons have inherent dignity and must be treated as ends in themselves, never merely as means.";
    kant_arg.principle_basis = {"categorical_imperative", "human_dignity"};
    kant_arg.strength = ArgumentStrength::STRONG;
    
    EthicalArgument util_arg;
    util_arg.id = "util_001";
    util_arg.philosophy_school = "utilitarianism";
    util_arg.argument_type = ArgumentType::PRO;
    util_arg.content = "Actions should maximize overall happiness and minimize suffering for all affected parties.";
    util_arg.principle_basis = {"greatest_happiness_principle"};
    util_arg.strength = ArgumentStrength::STRONG;
    
    store.storeArgument(kant_arg, false);
    store.storeArgument(util_arg, false);
    
    std::cout << "✓ Stored 2 arguments" << std::endl;
    
    // Retrieve Kantian arguments
    auto kant_result = store.getArgumentsByPhilosophy("kant", {}, 10);
    if (auto* args = std::get_if<std::vector<EthicalArgument>>(&kant_result)) {
        std::cout << "✓ Retrieved " << args->size() << " Kantian argument(s)" << std::endl;
        for (const auto& arg : *args) {
            std::cout << "  • " << arg.id << ": \"" 
                     << arg.content.substr(0, 60) << "...\"" << std::endl;
        }
    }
    
    store.shutdown();
}

void demonstrateDiscourseEngine() {
    printSeparator("3. Ethical Decision Making");
    
    // Create components
    auto loader = std::make_shared<PhilosophyLoader>();
    auto store = std::make_shared<ArgumentStore>();
    auto rag_engine = std::make_shared<RAGContextEngine>(store);
    
    // Initialize
    std::map<std::string, std::string> config;
    store->initialize(config);
    loader->loadFromDirectory("plugins/ethics_ai/philosophies");
    
    EthicalDiscourseEngine engine(loader, store, rag_engine);
    
    // Define ethical dilemma
    std::string dilemma = "Should an autonomous vehicle prioritize passenger safety over pedestrian safety in an unavoidable accident scenario?";
    std::vector<std::string> schools = {"kant", "utilitarianism"};
    
    std::cout << "Dilemma: " << dilemma << std::endl;
    std::cout << "\nPhilosophies consulted: ";
    for (size_t i = 0; i < schools.size(); ++i) {
        std::cout << schools[i];
        if (i < schools.size() - 1) std::cout << ", ";
    }
    std::cout << std::endl;
    
    // Initialize debate
    auto debate_result = engine.initializeDebate(dilemma, schools, "autonomous_systems");
    if (auto* debate = std::get_if<DebateInitialization>(&debate_result)) {
        std::cout << "\n✓ Debate initialized: " << debate->debate_id << std::endl;
    }
    
    // Make decision
    auto decision_result = engine.makeDecision(dilemma, schools, "autonomous_systems", true);
    if (auto* decision = std::get_if<EthicalDecision>(&decision_result)) {
        std::cout << "\n✓ Decision made: " << decision->decision_id << std::endl;
        std::cout << "\nDecision Text:" << std::endl;
        std::cout << "  " << decision->decision_text << std::endl;
        std::cout << "\nMetrics:" << std::endl;
        std::cout << "  Confidence:      " << std::fixed << std::setprecision(2) 
                  << (decision->confidence * 100) << "%" << std::endl;
        std::cout << "  Consensus Level: " << std::fixed << std::setprecision(2) 
                  << (decision->consensus_level * 100) << "%" << std::endl;
    }
    
    store->shutdown();
}

void demonstrateEvaluation() {
    printSeparator("4. Decision Evaluation (5 Dimensions)");
    
    // Create a test decision
    EthicalDecision decision;
    decision.decision_id = "dec_example";
    decision.dilemma_id = "dilemma_example";
    decision.decision_text = "Comprehensive decision with detailed ethical reasoning based on multiple philosophical frameworks.";
    decision.primary_philosophy = "kant";
    decision.supporting_philosophies = {"kant", "utilitarianism", "virtue_ethics"};
    decision.argument_chain_ids = {"chain_1", "chain_2"};
    decision.confidence = 0.85;
    decision.consensus_level = 0.78;
    
    // Create supporting arguments
    std::vector<EthicalArgument> arguments;
    
    EthicalArgument arg1;
    arg1.philosophy_school = "kant";
    arg1.strength = ArgumentStrength::STRONG;
    arguments.push_back(arg1);
    
    EthicalArgument arg2;
    arg2.philosophy_school = "utilitarianism";
    arg2.strength = ArgumentStrength::MODERATE;
    arguments.push_back(arg2);
    
    // Evaluate
    EthicsEvaluator evaluator;
    auto eval_result = evaluator.evaluateDecision(decision, arguments);
    
    if (auto* eval = std::get_if<EthicsEvaluationResult>(&eval_result)) {
        std::cout << "Evaluation Results:" << std::endl;
        std::cout << "  Overall Score:         " << std::fixed << std::setprecision(3) 
                  << eval->overall_score << " (" 
                  << (int)(eval->overall_score * 100) << "%)" << std::endl;
        std::cout << "\nDimension Breakdown:" << std::endl;
        std::cout << "  1. Decision Quality:   " << eval->decision_quality_score 
                  << " (" << (int)(eval->decision_quality_score * 100) << "%)" << std::endl;
        std::cout << "  2. Consistency:        " << eval->consistency_score 
                  << " (" << (int)(eval->consistency_score * 100) << "%)" << std::endl;
        std::cout << "  3. Fairness:           " << eval->fairness_score 
                  << " (" << (int)(eval->fairness_score * 100) << "%)" << std::endl;
        std::cout << "  4. Alignment:          " << eval->alignment_score 
                  << " (" << (int)(eval->alignment_score * 100) << "%)" << std::endl;
        std::cout << "  5. Transparency:       " << eval->transparency_score 
                  << " (" << (int)(eval->transparency_score * 100) << "%)" << std::endl;
        
        std::cout << "\nDetailed Metrics (" << eval->detailed_metrics.size() << " total):" << std::endl;
        for (const auto& [key, value] : eval->detailed_metrics) {
            std::cout << "  • " << key << ": " << value << std::endl;
        }
    }
}

int main() {
    std::cout << "╔════════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║         ETHICS AI PLUGIN - USAGE DEMONSTRATION                 ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════════════════════╝" << std::endl;
    
    try {
        demonstratePhilosophyLoader();
        demonstrateArgumentStore();
        demonstrateDiscourseEngine();
        demonstrateEvaluation();
        
        printSeparator("Demonstration Complete");
        std::cout << "\n✓ All demonstrations completed successfully!" << std::endl;
        std::cout << "\nNext steps:" << std::endl;
        std::cout << "  • Integrate with ThemisDB storage managers" << std::endl;
        std::cout << "  • Implement AQL queries for RAG patterns" << std::endl;
        std::cout << "  • Add vector search capabilities" << std::endl;
        std::cout << "  • Enable graph traversal for argument chains" << std::endl;
        std::cout << std::endl;
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\n✗ Error: " << e.what() << std::endl;
        return 1;
    }
}
