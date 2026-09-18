/**
 * @file ethics_ai_plugin_interface.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "plugins/plugin_interface.h"
#include "ethics_ai/ethics_ai_types.h"
#include <variant>
#include <functional>

namespace themis {
namespace plugins {
namespace ethics {

class IEthicsAIPlugin : public IThemisPlugin {
public:
    /**
     * @brief IEthics AIPlugin.
     * @return Return value.
     */
    virtual ~IEthicsAIPlugin() = default;
    
    // ========== Debate Initialization ==========
    
    [[nodiscard]] virtual std::variant<DebateInitialization, Status> initializeDebate(
        const std::string& dilemma_description,
        const std::vector<std::string>& philosophy_schools,
        const std::string& category = "general"
    ) = 0;
    
    // ========== Argument Management ==========
    
    [[nodiscard]] virtual Status storeArgument(
        const EthicalArgument& argument,
        bool store_vector = true
    ) = 0;
    
    [[nodiscard]] virtual std::variant<std::vector<EthicalArgument>, Status> getArgumentsByPhilosophy(
        const std::string& philosophy_school,
        const std::vector<ArgumentType>& argument_types = {},
        size_t limit = 20
    ) = 0;
    
    [[nodiscard]] virtual std::variant<EthicalArgument, Status> getArgumentById(
        const std::string& argument_id
    ) = 0;
    
    [[nodiscard]] virtual Status storeArgumentChain(const ArgumentChain& chain) = 0;
    
    [[nodiscard]] virtual std::variant<ArgumentChain, Status> getArgumentChain(
        const std::string& chain_id
    ) = 0;
    
    // ========== RAG Context Retrieval ==========
    
    [[nodiscard]] virtual std::variant<RAGContext, Status> buildRAGContext(
        const std::string& dilemma_description,
        const std::vector<std::string>& philosophy_schools,
        const std::string& category = "general"
    ) = 0;
    
    [[nodiscard]] virtual std::variant<std::vector<std::string>, Status> findSimilarDilemmas(
        const std::string& query_text,
        double threshold = 0.65,
        size_t limit = 10
    ) = 0;
    
    [[nodiscard]] virtual std::variant<std::vector<std::string>, Status> getBestPractices(
        const std::string& category,
        double min_satisfaction = 0.8,
        size_t limit = 10
    ) = 0;
    
    [[nodiscard]] virtual std::variant<std::vector<std::pair<std::string, double>>, Status> 
    vectorSemanticSearch(
        const std::vector<float>& query_embedding,
        const std::string& philosophy_school = "",
        size_t limit = 20
    ) = 0;
    
    [[nodiscard]] virtual std::variant<std::vector<std::string>, Status> traverseArgumentChain(
        const std::string& start_argument_id,
        size_t max_depth = 5,
        const std::string& direction = "both"
    ) = 0;
    
    // ========== Decision Making ==========
    
    [[nodiscard]] virtual std::variant<EthicalDecision, Status> makeDecision(
        const std::string& dilemma_description,
        const std::vector<std::string>& philosophy_schools,
        const std::string& category = "general",
        bool use_rag = true
    ) = 0;
    
    [[nodiscard]] virtual Status storeDecision(const EthicalDecision& decision) = 0;
    
    [[nodiscard]] virtual std::variant<EthicalDecision, Status> getDecision(
        const std::string& decision_id
    ) = 0;
    
    // ========== Evaluation ==========
    
    [[nodiscard]] virtual std::variant<EthicsEvaluationResult, Status> evaluateDecision(
        const EthicalDecision& decision,
        const std::vector<EthicalArgument>& arguments = {}
    ) = 0;
    
    // ========== Philosophy Profile Management ==========
    
    [[nodiscard]] virtual std::variant<size_t, Status> loadPhilosophyProfiles(
        const std::string& philosophy_dir
    ) = 0;
    
    [[nodiscard]] virtual std::variant<PhilosophyProfile, Status> getPhilosophyProfile(
        const std::string& school_id
    ) = 0;
    
    [[nodiscard]] virtual std::vector<std::string> listPhilosophySchools() const = 0;
    
    // ========== Monitoring ==========
    
    [[nodiscard]] virtual std::string getPrometheusMetrics() const = 0;
    
    [[nodiscard]] virtual std::string getDashboardJSON() const = 0;
    
    [[nodiscard]] virtual std::map<std::string, double> getStatistics() const = 0;
    
    // ========== Configuration ==========
    
    [[nodiscard]] virtual Status setConfig(const std::string& key, const std::string& value) = 0;
    
    [[nodiscard]] virtual std::optional<std::string> getConfig(const std::string& key) const = 0;
    
    
    /**
     * @brief Set Ethical Guidelines Manager.
     * @param[in,out] manager Input/output parameter.
     */
    virtual void setEthicalGuidelinesManager(void* manager) = 0;
};

} // namespace ethics
} // namespace plugins
} // namespace themis
