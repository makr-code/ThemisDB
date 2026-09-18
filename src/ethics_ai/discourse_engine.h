/**
 * @file discourse_engine.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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

class EthicalDiscourseEngine {
public:
    EthicalDiscourseEngine(
        std::shared_ptr<PhilosophyLoader> philosophy_loader,
        std::shared_ptr<ArgumentStore> store,
        std::shared_ptr<RAGContextEngine> rag_engine
    );
    ~EthicalDiscourseEngine() = default;
    
    std::variant<DebateInitialization, Status> initializeDebate(
        const std::string& dilemma_description,
        const std::vector<std::string>& philosophy_schools,
        const std::string& category
    );
    
    std::variant<EthicalDecision, Status> makeDecision(
        const std::string& dilemma_description,
        const std::vector<std::string>& philosophy_schools,
        const std::string& category,
        bool use_rag
    );

    /**
     * @brief Set Chain Visualizer Output Path.
     * @param[in] output_path Path to the output.
     */
    void setChainVisualizerOutputPath(const std::string& output_path);

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
    std::map<std::string, DebateInitialization> active_debates_;
    std::map<std::string, std::vector<EthicalArgument>> debate_arguments_;
    
    // Helper methods
    /**
     * @brief Generate Argument.
     * @param[in] profile Input parameter.
     * @param[in] dilemma Input parameter.
     * @param[in] type Input parameter.
     * @return Return value.
     */
    EthicalArgument generateArgument(
        const PhilosophyProfile& profile,
        const std::string& dilemma,
        ArgumentType type
    );
    
    /**
     * @brief Synthesize Decision.
     * @param[in] arguments Input parameter.
     * @param[in] primary_philosophy Input parameter.
     * @return Return value.
     */
    std::string synthesizeDecision(
        const std::vector<EthicalArgument>& arguments,
        const std::string& primary_philosophy
    );
};

} // namespace ethics
} // namespace plugins
} // namespace themis
