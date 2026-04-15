/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            discourse_engine.cpp                               ║
  Version:         0.0.9                                              ║
  Last Modified:   2026-04-15 04:16:29                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     231                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 87778519a4  2026-04-12  feat(ethics_ai): remove stubs — computed scoring, YAML fi... ║
    • 9ab72c5089  2026-03-12  refactor: flatten plugin hierarchy to src/<name>/ and inc... ║
    • acdb250dbf  2026-03-12  feat: migrate plugins to src/include with CMake switches ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "discourse_engine.h"
#include "ethics_evaluator.h"
#include <sstream>
#include <random>
#include <iomanip>

namespace themis {
namespace plugins {
namespace ethics {

EthicalDiscourseEngine::EthicalDiscourseEngine(
    std::shared_ptr<PhilosophyLoader> philosophy_loader,
    std::shared_ptr<ArgumentStore> store,
    std::shared_ptr<RAGContextEngine> rag_engine)
    : philosophy_loader_(philosophy_loader)
    , store_(store)
    , rag_engine_(rag_engine) {
}

std::variant<DebateInitialization, Status> EthicalDiscourseEngine::initializeDebate(
    const std::string& dilemma_description,
    const std::vector<std::string>& philosophy_schools,
    const std::string& category) {
    
    // Validate philosophy schools
    for (const auto& school : philosophy_schools) {
        if (!philosophy_loader_->hasProfile(school)) {
            return Status::Error("Philosophy profile not found: " + school);
        }
    }
    
    // Generate debate ID
    std::stringstream ss;
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    ss << "debate_" << time_t;
    
    DebateInitialization debate;
    debate.debate_id = ss.str();
    debate.dilemma_description = dilemma_description;
    debate.philosophy_schools = philosophy_schools;
    debate.category = category;
    debate.created_at = now;
    
    return debate;
}

std::variant<EthicalDecision, Status> EthicalDiscourseEngine::makeDecision(
    const std::string& dilemma_description,
    const std::vector<std::string>& philosophy_schools,
    const std::string& category,
    bool use_rag) {
    
    // Validate inputs
    if (philosophy_schools.empty()) {
        return Status::Error("At least one philosophy school required");
    }
    
    // Get RAG context if requested
    RAGContext rag_context;
    if (use_rag) {
        auto rag_result = rag_engine_->buildContext(
            dilemma_description, 
            philosophy_schools, 
            category
        );
        if (auto* context = std::get_if<RAGContext>(&rag_result)) {
            rag_context = *context;
        }
    }
    
    // Generate arguments from each philosophy
    std::vector<EthicalArgument> arguments;
    for (const auto& school : philosophy_schools) {
        auto profile_result = philosophy_loader_->getProfile(school);
        if (auto* profile = std::get_if<PhilosophyProfile>(&profile_result)) {
            // Generate pro argument
            auto arg = generateArgument(*profile, dilemma_description, ArgumentType::PRO);
            store_->storeArgument(arg, true);
            arguments.push_back(arg);
        }
    }
    
    // Synthesize decision
    std::string primary_philosophy = philosophy_schools[0];
    std::string decision_text = synthesizeDecision(arguments, primary_philosophy);
    
    // Create decision object
    std::stringstream ss;
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    ss << "decision_" << time_t;
    
    EthicalDecision decision;
    decision.decision_id = ss.str();
    decision.dilemma_id = ""; // Would be set if part of a debate
    decision.decision_text = decision_text;
    decision.primary_philosophy = primary_philosophy;
    decision.supporting_philosophies = philosophy_schools;
    decision.confidence = EthicsEvaluator::computeConfidence(arguments);
    decision.consensus_level = EthicsEvaluator::computeConsensus(arguments);
    decision.created_at = now;
    
    // Store decision
    store_->storeDecision(decision);
    
    return decision;
}

EthicalArgument EthicalDiscourseEngine::generateArgument(
    const PhilosophyProfile& profile,
    const std::string& dilemma,
    ArgumentType type) {
    
    // Generate argument ID
    std::stringstream ss;
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::random_device rd;
    ss << "arg_" << time_t << "_" << (rd() % 1000);
    
    EthicalArgument argument;
    argument.id = ss.str();
    argument.philosophy_school = profile.school_id;
    argument.argument_type = type;
    argument.created_at = now;

    // Derive strength from the richness of the profile: more theses → stronger argument.
    // Heuristic: 0 theses → WEAK (no principled basis); 1-2 → MODERATE (minimal support);
    // 3-5 → STRONG (well-grounded); 6+ → DECISIVE (comprehensive philosophical basis).
    // This feeds directly into EthicsEvaluator::computeConfidence() via ArgumentStrength.
    const size_t total_theses = profile.main_theses.size() + profile.secondary_theses.size();
    if (total_theses == 0) {
        argument.strength = ArgumentStrength::WEAK;
    } else if (total_theses <= 2) {
        argument.strength = ArgumentStrength::MODERATE;
    } else if (total_theses <= 5) {
        argument.strength = ArgumentStrength::STRONG;
    } else {
        argument.strength = ArgumentStrength::DECISIVE;
    }
    
    // Build content from all available profile data.
    std::stringstream content;
    content << "From the perspective of " << profile.name << ":\n";

    // Incorporate all main theses.
    for (const auto& thesis : profile.main_theses) {
        content << "  • " << thesis << "\n";
    }

    // Incorporate secondary theses if present.
    if (!profile.secondary_theses.empty()) {
        content << "Supporting principles:\n";
        for (const auto& thesis : profile.secondary_theses) {
            content << "  – " << thesis << "\n";
        }
    }

    // Reference the decision framework if available.
    auto fw_it = profile.decision_framework.find("primary");
    if (fw_it != profile.decision_framework.end()) {
        content << "Decision framework: " << fw_it->second << "\n";
    }

    // Apply to the specific dilemma.
    content << "Applied to: \"" << dilemma << "\"\n";
    if (type == ArgumentType::PRO) {
        content << "This framework supports proceeding, as the core principles "
                   "justify the action when all dimensions are weighed.";
    } else {
        content << "This framework raises concerns: the principles cited above "
                   "indicate caution or constraint is warranted.";
    }
    
    argument.content = content.str();
    argument.principle_basis = profile.main_theses;
    
    return argument;
}

std::string EthicalDiscourseEngine::synthesizeDecision(
    const std::vector<EthicalArgument>& arguments,
    const std::string& primary_philosophy) {
    
    std::stringstream ss;
    ss << "After considering perspectives from ";
    
    for (size_t i = 0; i < arguments.size(); ++i) {
        if (i > 0 && i == arguments.size() - 1) {
            ss << " and ";
        } else if (i > 0) {
            ss << ", ";
        }
        ss << arguments[i].philosophy_school;
    }
    
    ss << ", the primary recommendation based on " << primary_philosophy 
       << " is to proceed with careful consideration of all ethical dimensions.";
    
    return ss.str();
}

} // namespace ethics
} // namespace plugins
} // namespace themis
