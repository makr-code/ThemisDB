/**
 * @file discourse_engine.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: discourse_engine.h | Version: 0.0.13 | Last Modified: 2026-05-31 12:17:24
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 114
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * PR History (last 5): none
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#pragma once

#include "ethics_ai/ethics_ai_types.h"
#include "philosophy_loader.h"
#include "argument_store.h"
#include "rag_context_engine.h"
#include <map>
#include <memory>
#include <mutex>

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
    ~EthicalDiscourseEngine() = default;
    
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

    /**
     * @brief Configure output directory for ChainVisualizer artifacts.
     * @param output_path Directory for DOT/Mermaid decision artifacts.
     */
    void setChainVisualizerOutputPath(const std::string& output_path);

    /**
     * @brief Continue a debate for one additional round.
     *
     * Each philosophy school generates a counter-argument to arguments produced
     * in the previous round.  The new arguments are stored in the `ArgumentStore`
     * with `argument_type = REBUTTAL` and their `counterarguments` field populated
     * with the IDs of the previous round's arguments.
     *
     * Maximum 3 rounds (round_number 1..3) to bound computation cost.
     *
     * @param debate_id   The debate to continue (from `DebateInitialization::debate_id`).
     * @param round_number  1-based round number (capped at 3 internally).
     * @return `DebateRound` with all new arguments, or `Status::Error` when
     *         the debate is not found or the school list is empty.
     */
    std::variant<DebateRound, Status> continueDebate(
        const std::string& debate_id,
        int round_number
    );
    
private:
    std::shared_ptr<PhilosophyLoader> philosophy_loader_;
    std::shared_ptr<ArgumentStore> store_;
    std::shared_ptr<RAGContextEngine> rag_engine_;
    std::string chain_visualizer_output_path_;

    mutable std::mutex debates_mutex_;
    /// Active debates indexed by debate_id → DebateInitialization
    std::map<std::string, DebateInitialization> active_debates_;
    /// All arguments generated per debate_id (for round context)
    std::map<std::string, std::vector<EthicalArgument>> debate_arguments_;
    
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
};

} // namespace ethics
} // namespace plugins
} // namespace themis
