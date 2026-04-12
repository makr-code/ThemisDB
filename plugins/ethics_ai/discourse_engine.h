/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            discourse_engine.h                                 ║
  Version:         0.0.37                                             ║
  Last Modified:   2026-04-06 04:13:11                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     98                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "plugins/ethics_ai/ethics_ai_types.h"
#include "philosophy_loader.h"
#include "argument_store.h"
#include "rag_context_engine.h"
#include <memory>

namespace themis {
namespace llm {
class EmbeddedLLM;
} // namespace llm
} // namespace themis

namespace themis {
namespace plugins {
namespace ethics {

/**
 * @brief Ethical Discourse Engine
 * 
 * Orchestrates multi-philosophy ethical debates and decision-making.
 * Integrates philosophy profiles, argument generation, and decision synthesis.
 */
class EthicalDiscourseEngine {
public:
    EthicalDiscourseEngine(
        std::shared_ptr<PhilosophyLoader> philosophy_loader,
        std::shared_ptr<ArgumentStore> store,
        std::shared_ptr<RAGContextEngine> rag_engine
    );
    ~EthicalDiscourseEngine();
    
    /**
     * @brief Initialize a debate
     * @param dilemma_description Description of the dilemma
     * @param philosophy_schools Participating philosophies
     * @param category Dilemma category
     * @return Debate initialization or error
     */
    std::variant<DebateInitialization, Status> initializeDebate(
        const std::string& dilemma_description,
        const std::vector<std::string>& philosophy_schools,
        const std::string& category
    );
    
    /**
     * @brief Make an ethical decision
     * @param dilemma_description Description of the dilemma
     * @param philosophy_schools Participating philosophies
     * @param category Dilemma category
     * @param use_rag Whether to use RAG context
     * @return Ethical decision or error
     */
    std::variant<EthicalDecision, Status> makeDecision(
        const std::string& dilemma_description,
        const std::vector<std::string>& philosophy_schools,
        const std::string& category,
        bool use_rag
    );
    
private:
    std::shared_ptr<PhilosophyLoader> philosophy_loader_;
    std::shared_ptr<ArgumentStore> store_;
    std::shared_ptr<RAGContextEngine> rag_engine_;
    
    // Helper methods
    EthicalArgument generateArgument(
        const PhilosophyProfile& profile,
        const std::string& dilemma,
        ArgumentType type
    );
    
    std::string synthesizeDecision(
        const std::vector<EthicalArgument>& arguments,
        const std::string& primary_philosophy
    );

    bool llmInferenceEnabled() const;

    std::string generateArgumentContent(
      const PhilosophyProfile& profile,
      const std::string& dilemma,
      ArgumentType type
    ) const;

    bool llm_inference_enabled_;
  #if defined(THEMIS_ENABLE_LLM) && THEMIS_ENABLE_LLM
    std::unique_ptr<themis::llm::EmbeddedLLM> llm_;
  #endif
};

} // namespace ethics
} // namespace plugins
} // namespace themis
