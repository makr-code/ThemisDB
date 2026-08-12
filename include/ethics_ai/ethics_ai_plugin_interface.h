/**
 * @file ethics_ai_plugin_interface.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/**
 * @brief Ethics AI Plugin Interface
 * 
 * Provides native C++ interface for ethical AI decision-making framework.
 * This is a complete native implementation without Python dependencies.
 * 
 * Features:
 * - Multi-philosophy ethical debates
 * - RAG-based context retrieval (7 AQL patterns)
 * - Argument chain management
 * - Decision synthesis
 * - 5-dimension evaluation metrics
 * - Multi-model storage integration
 */
class IEthicsAIPlugin : public IThemisPlugin {
public:
    virtual ~IEthicsAIPlugin() = default;
    
    // ========== Debate Initialization ==========
    
    /**
     * @brief Initialize an ethical debate session
     * @param dilemma_description Description of the ethical dilemma
     * @param philosophy_schools List of philosophy schools to participate
     * @param category Category of the dilemma (e.g., "bioethics", "autonomous_systems")
     * @return Debate ID or error status
     */
    [[nodiscard]] virtual std::variant<DebateInitialization, Status> initializeDebate(
        const std::string& dilemma_description,
        const std::vector<std::string>& philosophy_schools,
        const std::string& category = "general"
    ) = 0;
    
    // ========== Argument Management ==========
    
    /**
     * @brief Store an ethical argument in multi-model storage
     * @param argument The argument to store
     * @param store_vector Whether to generate and store vector embedding
     * @return Status indicating success/failure
     */
    [[nodiscard]] virtual Status storeArgument(
        const EthicalArgument& argument,
        bool store_vector = true
    ) = 0;
    
    /**
     * @brief Retrieve arguments by philosophy school
     * @param philosophy_school School identifier (e.g., "kant", "utilitarianism")
     * @param argument_types Filter by types (empty = all types)
     * @param limit Maximum number of results
     * @return List of arguments or error
     */
    [[nodiscard]] virtual std::variant<std::vector<EthicalArgument>, Status> getArgumentsByPhilosophy(
        const std::string& philosophy_school,
        const std::vector<ArgumentType>& argument_types = {},
        size_t limit = 20
    ) = 0;
    
    /**
     * @brief Retrieve argument by ID
     * @param argument_id Argument identifier
     * @return Argument or error
     */
    [[nodiscard]] virtual std::variant<EthicalArgument, Status> getArgumentById(
        const std::string& argument_id
    ) = 0;
    
    /**
     * @brief Store an argument chain
     * @param chain The argument chain to store
     * @return Status indicating success/failure
     */
    [[nodiscard]] virtual Status storeArgumentChain(const ArgumentChain& chain) = 0;
    
    /**
     * @brief Retrieve argument chain by ID
     * @param chain_id Chain identifier
     * @return Argument chain or error
     */
    [[nodiscard]] virtual std::variant<ArgumentChain, Status> getArgumentChain(
        const std::string& chain_id
    ) = 0;
    
    // ========== RAG Context Retrieval ==========
    
    /**
     * @brief Build RAG context for ethical decision-making
     * @param dilemma_description Description of current dilemma
     * @param philosophy_schools Participating philosophy schools
     * @param category Dilemma category
     * @return RAG context or error
     */
    [[nodiscard]] virtual std::variant<RAGContext, Status> buildRAGContext(
        const std::string& dilemma_description,
        const std::vector<std::string>& philosophy_schools,
        const std::string& category = "general"
    ) = 0;
    
    /**
     * @brief Find similar dilemmas (AQL Pattern 1)
     * @param query_text Query text for similarity search
     * @param threshold Similarity threshold (default: 0.65)
     * @param limit Maximum results
     * @return List of similar dilemma IDs
     */
    [[nodiscard]] virtual std::variant<std::vector<std::string>, Status> findSimilarDilemmas(
        const std::string& query_text,
        double threshold = 0.65,
        size_t limit = 10
    ) = 0;
    
    /**
     * @brief Get best practices for a category (AQL Pattern 3)
     * @param category Dilemma category
     * @param min_satisfaction Minimum satisfaction score
     * @param limit Maximum results
     * @return List of best practice decision IDs
     */
    [[nodiscard]] virtual std::variant<std::vector<std::string>, Status> getBestPractices(
        const std::string& category,
        double min_satisfaction = 0.8,
        size_t limit = 10
    ) = 0;
    
    /**
     * @brief Vector semantic search for arguments (AQL Pattern 4)
     * @param query_embedding Query vector embedding
     * @param philosophy_school Optional philosophy filter
     * @param limit Maximum results
     * @return List of argument IDs with similarity scores
     */
    [[nodiscard]] virtual std::variant<std::vector<std::pair<std::string, double>>, Status> 
    vectorSemanticSearch(
        const std::vector<float>& query_embedding,
        const std::string& philosophy_school = "",
        size_t limit = 20
    ) = 0;
    
    /**
     * @brief Traverse argument chains (AQL Pattern 5)
     * @param start_argument_id Starting argument ID
     * @param max_depth Maximum traversal depth
     * @param direction "supports" or "counters"
     * @return List of connected argument IDs
     */
    [[nodiscard]] virtual std::variant<std::vector<std::string>, Status> traverseArgumentChain(
        const std::string& start_argument_id,
        size_t max_depth = 5,
        const std::string& direction = "both"
    ) = 0;
    
    // ========== Decision Making ==========
    
    /**
     * @brief Make ethical decision using RAG and multi-philosophy debate
     * @param dilemma_description Ethical dilemma to analyze
     * @param philosophy_schools Philosophy schools to consult
     * @param category Dilemma category
     * @param use_rag Whether to use RAG context
     * @return Ethical decision or error
     */
    [[nodiscard]] virtual std::variant<EthicalDecision, Status> makeDecision(
        const std::string& dilemma_description,
        const std::vector<std::string>& philosophy_schools,
        const std::string& category = "general",
        bool use_rag = true
    ) = 0;
    
    /**
     * @brief Store a decision
     * @param decision The decision to store
     * @return Status indicating success/failure
     */
    [[nodiscard]] virtual Status storeDecision(const EthicalDecision& decision) = 0;
    
    /**
     * @brief Retrieve decision by ID
     * @param decision_id Decision identifier
     * @return Decision or error
     */
    [[nodiscard]] virtual std::variant<EthicalDecision, Status> getDecision(
        const std::string& decision_id
    ) = 0;
    
    // ========== Evaluation ==========
    
    /**
     * @brief Evaluate ethical decision quality (5 dimensions)
     * @param decision The decision to evaluate
     * @param arguments Arguments used in decision (optional)
     * @return Evaluation result or error
     */
    [[nodiscard]] virtual std::variant<EthicsEvaluationResult, Status> evaluateDecision(
        const EthicalDecision& decision,
        const std::vector<EthicalArgument>& arguments = {}
    ) = 0;
    
    // ========== Philosophy Profile Management ==========
    
    /**
     * @brief Load philosophy profiles from directory
     * @param philosophy_dir Directory containing YAML files
     * @return Number of profiles loaded or error
     */
    [[nodiscard]] virtual std::variant<size_t, Status> loadPhilosophyProfiles(
        const std::string& philosophy_dir
    ) = 0;
    
    /**
     * @brief Get philosophy profile by ID
     * @param school_id School identifier
     * @return Philosophy profile or error
     */
    [[nodiscard]] virtual std::variant<PhilosophyProfile, Status> getPhilosophyProfile(
        const std::string& school_id
    ) = 0;
    
    /**
     * @brief List all loaded philosophy schools
     * @return List of school IDs
     */
    [[nodiscard]] virtual std::vector<std::string> listPhilosophySchools() const = 0;
    
    // ========== Monitoring ==========
    
    /**
     * @brief Get current metrics in Prometheus format
     * @return Prometheus metrics string
     */
    [[nodiscard]] virtual std::string getPrometheusMetrics() const = 0;
    
    /**
     * @brief Get dashboard data in JSON format
     * @return JSON string with dashboard data
     */
    [[nodiscard]] virtual std::string getDashboardJSON() const = 0;
    
    /**
     * @brief Get plugin statistics
     * @return Statistics as key-value map
     */
    [[nodiscard]] virtual std::map<std::string, double> getStatistics() const = 0;
    
    // ========== Configuration ==========
    
    /**
     * @brief Set configuration option
     * @param key Configuration key
     * @param value Configuration value
     * @return Status indicating success/failure
     */
    [[nodiscard]] virtual Status setConfig(const std::string& key, const std::string& value) = 0;
    
    /**
     * @brief Get configuration option
     * @param key Configuration key
     * @return Configuration value or nullopt if not found
     */
    [[nodiscard]] virtual std::optional<std::string> getConfig(const std::string& key) const = 0;
    
    // ========== Integration with Core System ==========
    
    /**
     * @brief Set reference to EthicalGuidelinesManager for plugin integration
     * @param manager Pointer to EthicalGuidelinesManager
     * 
     * This method is called by the core system to establish the integration
     * between the plugin and the base ethical guidelines system. The plugin
     * can then register its philosophy profiles with the manager during
     * initialization.
     */
    virtual void setEthicalGuidelinesManager(void* manager) = 0;
};

} // namespace ethics
} // namespace plugins
} // namespace themis
