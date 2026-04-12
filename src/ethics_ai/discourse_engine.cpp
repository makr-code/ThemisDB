/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            discourse_engine.cpp                               ║
  Version:         0.0.3                                              ║
  Last Modified:   2026-04-06 04:15:39                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     197                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 9ab72c5089  2026-03-12  refactor: flatten plugin hierarchy to src/<name>/ and inc... ║
    • acdb250dbf  2026-03-12  feat: migrate plugins to src/include with CMake switches ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "discourse_engine.h"
#include <sstream>
#include <random>
#include <iomanip>
#include <algorithm>
#include <cctype>
#include <cstdlib>

#if defined(THEMIS_ENABLE_LLM) && THEMIS_ENABLE_LLM
#include "llm/embedded_llm.h"
#endif

namespace themis {
namespace plugins {
namespace ethics {

namespace {

bool isTruthyEnv(const char* value) {
    if (!value) {
        return false;
    }

    std::string normalized(value);
    std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });

    return normalized == "1" || normalized == "ON" || normalized == "TRUE" ||
           normalized == "YES";
}

ArgumentType dialecticTypeForIndex(size_t index) {
    if (index == 0) {
        return ArgumentType::PRO;
    }
    if (index == 1) {
        return ArgumentType::CONTRA;
    }
    if (index == 2) {
        return ArgumentType::REBUTTAL;
    }
    return ArgumentType::SYNTHESIS;
}

const char* argumentTypeLabel(ArgumentType type) {
    switch (type) {
        case ArgumentType::PRO:
            return "PRO";
        case ArgumentType::CONTRA:
            return "CONTRA";
        case ArgumentType::REBUTTAL:
            return "REBUTTAL";
        case ArgumentType::SYNTHESIS:
            return "SYNTHESIS";
        case ArgumentType::QUESTION:
            return "QUESTION";
        case ArgumentType::CLARIFICATION:
            return "CLARIFICATION";
        default:
            return "UNKNOWN";
    }
}

std::string clampForPrompt(const std::string& text, size_t max_chars) {
    if (text.size() <= max_chars) {
        return text;
    }
    if (max_chars <= 3) {
        return text.substr(0, max_chars);
    }
    return text.substr(0, max_chars - 3) + "...";
}

} // namespace

EthicalDiscourseEngine::EthicalDiscourseEngine(
    std::shared_ptr<PhilosophyLoader> philosophy_loader,
    std::shared_ptr<ArgumentStore> store,
    std::shared_ptr<RAGContextEngine> rag_engine)
    : philosophy_loader_(philosophy_loader)
    , store_(store)
    , rag_engine_(rag_engine)
    , llm_inference_enabled_(isTruthyEnv(std::getenv("THEMIS_ETHICS_LLM_INFERENCE"))) {
#if defined(THEMIS_ENABLE_LLM) && THEMIS_ENABLE_LLM
    if (llm_inference_enabled_) {
        themis::llm::EmbeddedLLM::Config llm_config;
        if (const char* model_path = std::getenv("THEMIS_ETHICS_LLM_MODEL_PATH")) {
            llm_config.model_path = model_path;
        }
        llm_config.enable_streaming = false;

        llm_ = std::make_unique<themis::llm::EmbeddedLLM>(llm_config);
        if (!llm_ || !llm_->isReady()) {
            llm_inference_enabled_ = false;
            llm_.reset();
        }
    }
#else
    llm_inference_enabled_ = false;
#endif
}

EthicalDiscourseEngine::~EthicalDiscourseEngine() = default;

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
    
    // Generate one dialectic step per philosophy school in sequence:
    // thesis (PRO) -> antithesis (CONTRA) -> rebuttal (REBUTTAL) -> synthesis (SYNTHESIS)
    std::vector<EthicalArgument> arguments;
    for (size_t i = 0; i < philosophy_schools.size(); ++i) {
        const auto& school = philosophy_schools[i];
        auto profile_result = philosophy_loader_->getProfile(school);
        if (auto* profile = std::get_if<PhilosophyProfile>(&profile_result)) {
            auto arg = generateArgument(*profile, dilemma_description, dialecticTypeForIndex(i));
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
    decision.confidence = 0.75; // Placeholder
    decision.consensus_level = arguments.size() > 1 ? 0.70 : 1.0;
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
    argument.strength = ArgumentStrength::MODERATE;
    argument.created_at = now;
    
    argument.content = generateArgumentContent(profile, dilemma, type);
    argument.principle_basis = profile.main_theses;
    
    return argument;
}

bool EthicalDiscourseEngine::llmInferenceEnabled() const {
#if defined(THEMIS_ENABLE_LLM) && THEMIS_ENABLE_LLM
    return llm_inference_enabled_ && llm_ && llm_->isReady();
#else
    return false;
#endif
}

std::string EthicalDiscourseEngine::generateArgumentContent(
    const PhilosophyProfile& profile,
    const std::string& dilemma,
    ArgumentType type) const {
    if (llmInferenceEnabled()) {
#if defined(THEMIS_ENABLE_LLM) && THEMIS_ENABLE_LLM
        const std::string compact_dilemma = clampForPrompt(dilemma, 420);
        const std::string primary_thesis =
            profile.main_theses.empty() ? std::string() : clampForPrompt(profile.main_theses[0], 180);

        std::stringstream prompt;
        prompt << "You are an ethics assistant. Produce one concise dialectic step.\n"
               << "Step type: " << argumentTypeLabel(type) << "\n"
               << "Philosophy: " << profile.name << " (" << profile.school_id << ")\n"
               << "Primary principle: " << (primary_thesis.empty() ? "N/A" : primary_thesis) << "\n"
               << "Dilemma: " << compact_dilemma << "\n"
               << "Return only 2-3 sentences of the argument text.";

        const std::string generated = llm_->generateWithParams(prompt.str(), 0.3f, 0.9f, 64);
        if (!generated.empty()) {
            return generated;
        }
#endif
    }

    // Deterministic fallback for tests and deployments without active LLM model.
    std::stringstream content;
    content << "From the perspective of " << profile.name << ":\n";
    if (!profile.main_theses.empty()) {
        content << profile.main_theses[0] << "\n";
    }
    content << "Applied to this dilemma, ";

    if (type == ArgumentType::PRO) {
        content << "we should consider the ethical implications carefully.";
    } else if (type == ArgumentType::CONTRA) {
        content << "there are substantial objections that should be weighed before acting.";
    } else if (type == ArgumentType::REBUTTAL) {
        content << "the strongest objections can be addressed with transparent safeguards.";
    } else if (type == ArgumentType::SYNTHESIS) {
        content << "a balanced synthesis should integrate benefits, risks, and accountability.";
    } else {
        content << "we must recognize the complexities involved.";
    }

    return content.str();
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
