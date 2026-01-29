#pragma once

#include "plugins/ethics_ai/ethics_ai_types.h"
#include <memory>
#include <mutex>

namespace themis {
namespace plugins {
namespace ethics {

/**
 * @brief Argument Storage Manager
 * 
 * Manages storage and retrieval of ethical arguments across multiple
 * storage models (Graph, Relational, Vector, Timeline).
 * 
 * This is a placeholder interface that will be integrated with
 * ThemisDB's actual storage managers when building with the main project.
 */
class ArgumentStore {
public:
    ArgumentStore() = default;
    ~ArgumentStore() = default;
    
    /**
     * @brief Initialize the argument store
     * @param config Configuration options
     * @return Status indicating success/failure
     */
    Status initialize(const std::map<std::string, std::string>& config);
    
    /**
     * @brief Store an ethical argument
     * @param argument The argument to store
     * @param store_vector Whether to generate and store vector embedding
     * @return Status indicating success/failure
     */
    Status storeArgument(const EthicalArgument& argument, bool store_vector = true);
    
    /**
     * @brief Retrieve argument by ID
     * @param argument_id Argument identifier
     * @return Argument or error
     */
    std::variant<EthicalArgument, Status> getArgument(const std::string& argument_id);
    
    /**
     * @brief Get arguments by philosophy school
     * @param philosophy_school School identifier
     * @param argument_types Filter by types (empty = all)
     * @param limit Maximum results
     * @return List of arguments or error
     */
    std::variant<std::vector<EthicalArgument>, Status> getArgumentsByPhilosophy(
        const std::string& philosophy_school,
        const std::vector<ArgumentType>& argument_types,
        size_t limit
    );
    
    /**
     * @brief Store an argument chain
     * @param chain The chain to store
     * @return Status indicating success/failure
     */
    Status storeChain(const ArgumentChain& chain);
    
    /**
     * @brief Retrieve argument chain by ID
     * @param chain_id Chain identifier
     * @return Chain or error
     */
    std::variant<ArgumentChain, Status> getChain(const std::string& chain_id);
    
    /**
     * @brief Store a decision
     * @param decision The decision to store
     * @return Status indicating success/failure
     */
    Status storeDecision(const EthicalDecision& decision);
    
    /**
     * @brief Retrieve decision by ID
     * @param decision_id Decision identifier
     * @return Decision or error
     */
    std::variant<EthicalDecision, Status> getDecision(const std::string& decision_id);
    
    /**
     * @brief Shutdown the store
     */
    void shutdown();
    
private:
    std::mutex mutex_;
    bool initialized_ = false;
    
    // TODO: Add actual storage manager references when integrating
    // std::shared_ptr<GraphManager> graph_mgr_;
    // std::shared_ptr<RelationalManager> relational_mgr_;
    // std::shared_ptr<VectorIndexManager> vector_mgr_;
    // std::shared_ptr<TimelineManager> timeline_mgr_;
    
    // In-memory storage for now (placeholder)
    std::map<std::string, EthicalArgument> arguments_;
    std::map<std::string, ArgumentChain> chains_;
    std::map<std::string, EthicalDecision> decisions_;
};

} // namespace ethics
} // namespace plugins
} // namespace themis
