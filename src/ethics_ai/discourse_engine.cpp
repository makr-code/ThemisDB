/**
 * @file discourse_engine.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "discourse_engine.h"
#include "chain_visualizer.h"

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <random>
#include <sstream>

#include "ethics_evaluator.h"

namespace themis {
namespace plugins {
namespace ethics {

EthicalDiscourseEngine::EthicalDiscourseEngine(std::shared_ptr<PhilosophyLoader> philosophy_loader,
                                               std::shared_ptr<ArgumentStore> store,
                                               std::shared_ptr<RAGContextEngine> rag_engine)
    : philosophy_loader_(philosophy_loader), store_(store), rag_engine_(rag_engine) {}

void EthicalDiscourseEngine::setChainVisualizerOutputPath(const std::string& output_path) {
    chain_visualizer_output_path_ = output_path;
}

std::variant<DebateInitialization, Status>
EthicalDiscourseEngine::initializeDebate(const std::string &dilemma_description,
                                         const std::vector<std::string> &philosophy_schools,
                                         const std::string &category) {
    // Validate philosophy schools
    for (const auto &school : philosophy_schools) {
        if (!philosophy_loader_->hasProfile(school)) {
            return Status::Error("Philosophy profile not found: " + school);
        }
    }

    // Generate debate ID
    std::stringstream ss = {};
    auto now    = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    ss << "debate_" << time_t;

    DebateInitialization debate;
    debate.debate_id           = ss.str();
    debate.dilemma_description = dilemma_description;
    debate.philosophy_schools  = philosophy_schools;
    debate.category            = category;
    debate.created_at          = now;

    // Register the debate so continueDebate() can find it later.
    {
        std::lock_guard<std::mutex> lock(debates_mutex_);
        active_debates_[debate.debate_id] = debate;
    }

    return debate;
}

std::variant<EthicalDecision, Status>
EthicalDiscourseEngine::makeDecision(const std::string &dilemma_description,
                                     const std::vector<std::string> &philosophy_schools, const std::string &category,
                                     bool use_rag) {
    // Validate inputs
    if (philosophy_schools.empty()) {
        return Status::Error("At least one philosophy school required");
    }

    // Get RAG context if requested
    RAGContext rag_context = {};
    if (use_rag) {
        auto rag_result = rag_engine_->buildContext(dilemma_description, philosophy_schools, category);
        if (auto *context = std::get_if<RAGContext>(&rag_result)) {
            rag_context = *context;
        }
    }

    // Generate arguments from each philosophy
    std::vector<EthicalArgument> arguments = {};

    for (const auto &school : philosophy_schools) {
        auto profile_result = philosophy_loader_->getProfile(school);
        if (auto *profile = std::get_if<PhilosophyProfile>(&profile_result)) {
            // Generate pro argument
            auto arg = generateArgument(*profile, dilemma_description, ArgumentType::PRO);
            store_->storeArgument(arg, true);
            arguments.push_back(arg);
        }
    }

    // Synthesize decision
    std::string primary_philosophy = philosophy_schools[0];
    std::string decision_text      = synthesizeDecision(arguments, primary_philosophy);

    // Create decision object
    std::stringstream ss = {};
    auto now    = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    ss << "decision_" << time_t;

    EthicalDecision decision;
    decision.decision_id             = ss.str();
    decision.dilemma_id              = ""; // Would be set if part of a debate
    decision.decision_text           = decision_text;
    decision.primary_philosophy      = primary_philosophy;
    decision.supporting_philosophies = philosophy_schools;
    decision.confidence              = EthicsEvaluator::computeConfidence(arguments);
    decision.consensus_level         = EthicsEvaluator::computeConsensus(arguments);
    decision.created_at              = now;

    if (rag_engine_) {
        const auto legal_grounding = rag_engine_->retrieveLegalGrounding(dilemma_description);
        decision.metadata["legal_db_unavailable"] = legal_grounding.legal_db_unavailable ? "true" : "false";
        decision.metadata["legal_grounding_available"] = legal_grounding.grounding_available ? "true" : "false";
        decision.metadata["legal_grounding_retrieved_at_utc"] = legal_grounding.retrieval_timestamp_utc;
        std::stringstream norm_refs_csv = {};
        for (size_t i = 0; i <static_cast<int>(legal_grounding.norm_refs.size()); ++i) {
            if (i > 0) {
              norm_refs_csv << ",";
            }
            norm_refs_csv << legal_grounding.norm_refs[i];
        }
        decision.metadata["norm_refs"] = norm_refs_csv.str();
    }

    if (!chain_visualizer_output_path_.empty()) {
        std::vector<std::string> argument_ids = {};

        argument_ids.reserve(arguments.size());
        for (const auto& arg : arguments) {
            argument_ids.push_back(arg.id);
        }

        const std::string dot = ChainVisualizer::exportDot(argument_ids, *store_, decision.decision_id);
        const std::string mermaid = ChainVisualizer::exportMermaid(argument_ids, *store_);

        std::error_code ec = {};
        std::filesystem::create_directories(chain_visualizer_output_path_, ec);
        if (ec) {
            return Status::Error("Failed to create ChainVisualizer artifact directory: "
                                 + chain_visualizer_output_path_);
        }

        const std::filesystem::path base_path(chain_visualizer_output_path_);
        const std::filesystem::path dot_path = base_path / (decision.decision_id + ".dot");
        const std::filesystem::path mermaid_path = base_path / (decision.decision_id + ".mmd");

        {
            std::ofstream out(dot_path);
            if (!out) {
                return Status::Error("Failed to write DOT artifact: " + dot_path.string());
            }
            out << dot;
        }
        {
            std::ofstream out(mermaid_path);
            if (!out) {
                return Status::Error("Failed to write Mermaid artifact: " + mermaid_path.string());
            }
            out << mermaid;
        }

        decision.metadata["chain_visualizer_dot_path"] = dot_path.string();
        decision.metadata["chain_visualizer_mermaid_path"] = mermaid_path.string();
    }

    // Store decision
    store_->storeDecision(decision);

    return decision;
}

EthicalArgument EthicalDiscourseEngine::generateArgument(const PhilosophyProfile &profile, const std::string &dilemma,
                                                         ArgumentType type) {
    // Generate argument ID
    std::stringstream ss = {};
    auto now    = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::random_device rd = {};
    ss << "arg_" << time_t << "_" << (rd() % 1000);

    EthicalArgument argument;
    argument.id                = ss.str();
    argument.philosophy_school = profile.school_id;
    argument.argument_type     = type;
    argument.created_at        = now;

    // Derive strength from the richness of the profile: more theses → stronger argument.
    // Heuristic: 0 theses → WEAK (no principled basis); 1-2 → MODERATE (minimal support);
    // 3-5 → STRONG (well-grounded); 6+ → DECISIVE (comprehensive philosophical basis).
    // This feeds directly into EthicsEvaluator::computeConfidence() via ArgumentStrength.
    const size_t total_theses = static_cast<int>(profile.main_theses.size()) + static_cast<int>(profile.secondary_theses.size()) ;
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
    std::stringstream content = {};
    content << "From the perspective of " << profile.name << ":\n";

    // Incorporate all main theses.
    for (const auto &thesis : profile.main_theses) {
        content << "  • " << thesis << "\n";
    }

    // Incorporate secondary theses if present.
    if (!profile.secondary_theses.empty()) {
        content << "Supporting principles:\n";
        for (const auto &thesis : profile.secondary_theses) {
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

    argument.content         = content.str();
    argument.principle_basis = profile.main_theses;

    return argument;
}

std::string EthicalDiscourseEngine::synthesizeDecision(const std::vector<EthicalArgument> &arguments,
                                                       const std::string &primary_philosophy) {
    std::stringstream ss = {};
    ss << "After considering perspectives from ";

    for (size_t i = 0; i < arguments.size(); ++i) {
        if (i > 0 && i == static_cast<int>(arguments.size()) - 1) {
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

// ============================================================================
// v0.2.0 — Multi-Round Debates
// ============================================================================

std::variant<DebateRound, Status> EthicalDiscourseEngine::continueDebate(const std::string &debate_id,
                                                                         int round_number) {
    // Cap rounds at 3 to bound computation cost.
    const int capped_round = (round_number > 3) ? 3 : (round_number < 1 ? 1 : round_number);

    // Look up the active debate.
    DebateInitialization init;
    {
        std::lock_guard<std::mutex> lock(debates_mutex_);
        auto it = active_debates_.find(debate_id);
        if (it == active_debates_.end()) {
            return Status::Error("Debate not found: " + debate_id);
        }
        init = it->second;
    }

    if (init.philosophy_schools.empty()) {
        return Status::Error("No philosophy schools in debate: " + debate_id);
    }

    // Collect previous-round argument IDs for cross-referencing.
    std::vector<std::string> prev_arg_ids;
    {
        std::lock_guard<std::mutex> lock(debates_mutex_);
        auto it = debate_arguments_.find(debate_id);
        if (it != debate_arguments_.end()) {
            prev_arg_ids.reserve(it->second.size());
            for (const auto &arg : it->second) {
                prev_arg_ids.push_back(arg.id);
            }
        }
    }

    DebateRound round;
    round.debate_id    = debate_id;
    round.round_number = capped_round;

    for (const auto &school : init.philosophy_schools) {
        auto profile_result = philosophy_loader_->getProfile(school);
        if (!std::holds_alternative<PhilosophyProfile>(profile_result)) {
            continue;
        }
        const auto &profile = std::get<PhilosophyProfile>(profile_result);

        // Round 1 → PRO, Round 2 → CONTRA, Round 3 → SYNTHESIS
        ArgumentType arg_type = ArgumentType::PRO;
        if (capped_round == 2) {
            arg_type = ArgumentType::REBUTTAL;
        } else if (capped_round == 3) {
            arg_type = ArgumentType::SYNTHESIS;
        }

        EthicalArgument arg = generateArgument(profile, init.dilemma_description, arg_type);
        // Link counter-arguments to previous round.
        arg.counterarguments = prev_arg_ids;

        // Store in ArgumentStore.
        if (store_) {
            store_->storeArgument(arg, /*store_vector=*/false);
        }
        round.arguments.push_back(arg);
    }

    // Accumulate all arguments for next-round context.
    {
        std::lock_guard<std::mutex> lock(debates_mutex_);
        auto &all = debate_arguments_[debate_id];
        for (const auto &a : round.arguments) {
            all.push_back(a);
        }
    }

    // Persist the round.
    if (store_) {
        store_->storeDebateRound(round);
    }

    return round;
}

} // namespace ethics
} // namespace plugins
} // namespace themis
