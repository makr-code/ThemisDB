/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            argument_store.h                                   ║
  Version:         0.0.3                                              ║
  Last Modified:   2026-04-06 04:15:39                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     153                                            ║
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

#pragma once

#include "plugins/ethics_ai/ethics_ai_types.h"
#include "storage/base_entity.h"
#include <memory>
#include <mutex>

// Forward declarations for ThemisDB components
namespace themis {
class RocksDBWrapper;
namespace query { class QueryEngine; }
}

namespace themis {
namespace plugins {
namespace ethics {

/**
 * @brief Argument Storage using ThemisDB BaseEntity and AQL
 * 
 * Stores ethical arguments directly as BaseEntity instances.
 * No duplicate storage structures - uses ThemisDB's unified storage.
 * 
 * Storage Collections:
 * - ethics_arguments: Argument entities
 * - ethics_decisions: Decision entities
 * - ethics_debates: Debate session entities
 * - ethics_profiles: Philosophy profile entities
 * 
 * Keys follow ThemisDB pattern:
 * - entity:ethics_arguments:{id}
 * - entity:ethics_decisions:{id}
 * etc.
 */
class ArgumentStore {
public:
    ArgumentStore() = default;
    ~ArgumentStore() = default;
    
    /**
     * @brief Initialize the argument store with ThemisDB storage
     * @param storage RocksDB storage backend
     * @param query_engine Query engine for AQL execution (optional for standalone mode)
     * @return Status indicating success/failure
     */
    Status initialize(
        std::shared_ptr<RocksDBWrapper> storage,
        std::shared_ptr<query::QueryEngine> query_engine = nullptr
    );
    
    /**
     * @brief Store an ethical argument as BaseEntity
     * @param argument The argument to store
     * @param store_vector Whether to generate and store vector embedding
     * @return Status indicating success/failure
     */
    Status storeArgument(const EthicalArgument& argument, bool store_vector = true);
    
    /**
     * @brief Retrieve argument by ID (uses BaseEntity)
     * @param argument_id Argument identifier
     * @return Argument or error
     */
    std::variant<EthicalArgument, Status> getArgument(const std::string& argument_id);
    
    /**
     * @brief Get arguments by philosophy school (uses AQL if available, else scan)
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
     * @brief Store a decision as BaseEntity
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
     * @brief Store a philosophy profile as BaseEntity
     * @param profile The profile to store
     * @return Status indicating success/failure
     */
    Status storePhilosophyProfile(const PhilosophyProfile& profile);
    
    /**
     * @brief Retrieve philosophy profile by school
     * @param school School identifier
     * @return Profile or error
     */
    std::variant<PhilosophyProfile, Status> getPhilosophyProfile(const std::string& school);

    /**
     * @brief Store an argument chain
     * @param chain The argument chain to store
     * @return Status indicating success/failure
     */
    Status storeChain(const ArgumentChain& chain);

    /**
     * @brief Retrieve an argument chain by ID
     * @param chain_id Chain identifier
     * @return Chain or error
     */
    std::variant<ArgumentChain, Status> getChain(const std::string& chain_id);

    /**
     * @brief Shutdown the store
     */
    void shutdown();
    
private:
    std::mutex mutex_;
    bool initialized_ = false;
    
    // Direct ThemisDB storage - no wrappers
    std::shared_ptr<RocksDBWrapper> storage_;
    std::shared_ptr<query::QueryEngine> query_engine_;
    
    // Fallback in-memory storage for standalone mode (testing)
    bool standalone_mode_ = false;
    std::map<std::string, EthicalArgument> arguments_;
    std::map<std::string, EthicalDecision> decisions_;
    std::map<std::string, PhilosophyProfile> profiles_;
    std::map<std::string, ArgumentChain> chains_; ///< In-memory chain cache
};

} // namespace ethics
} // namespace plugins
} // namespace themis
