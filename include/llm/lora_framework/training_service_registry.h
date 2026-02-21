/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            training_service_registry.h                        ║
  Version:         0.0.4                                              ║
  Last Modified:   2026-02-21 08:33:50                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     107                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 65ea7343c  2026-01-20  Integrate ShardRouter and ShardTopology for inter-shard R... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <memory>
#include <mutex>
#include <string>

// Forward declarations
namespace themis {
namespace sharding {
    class ShardRouter;
    class ShardTopology;
}
}

namespace themis {
namespace llm {
namespace lora {

/**
 * @brief Service Registry for Distributed Training Dependencies
 * 
 * Provides dependency injection for ShardRouter and ShardTopology.
 * Enables LoRATrainingService to access shard infrastructure without
 * tight coupling to specific implementations.
 * 
 * Thread-safe singleton pattern.
 */
class TrainingServiceRegistry {
public:
    /**
     * @brief Get singleton instance
     * @return Reference to the singleton registry
     */
    static TrainingServiceRegistry& getInstance();
    
    /**
     * @brief Register ShardRouter instance
     * @param router Shared pointer to ShardRouter
     */
    void registerShardRouter(std::shared_ptr<themis::sharding::ShardRouter> router);
    
    /**
     * @brief Register ShardTopology instance
     * @param topology Shared pointer to ShardTopology
     */
    void registerShardTopology(std::shared_ptr<themis::sharding::ShardTopology> topology);
    
    /**
     * @brief Get registered ShardRouter
     * @return Shared pointer to ShardRouter, or nullptr if not registered
     */
    std::shared_ptr<themis::sharding::ShardRouter> getShardRouter() const;
    
    /**
     * @brief Get registered ShardTopology
     * @return Shared pointer to ShardTopology, or nullptr if not registered
     */
    std::shared_ptr<themis::sharding::ShardTopology> getShardTopology() const;
    
    /**
     * @brief Check if shard infrastructure is available
     * @return true if both ShardRouter and ShardTopology are registered
     */
    bool hasShardInfrastructure() const;
    
    /**
     * @brief Clear all registered instances (for testing)
     */
    void clear();
    
    // Disable copy and move
    TrainingServiceRegistry(const TrainingServiceRegistry&) = delete;
    TrainingServiceRegistry& operator=(const TrainingServiceRegistry&) = delete;
    TrainingServiceRegistry(TrainingServiceRegistry&&) = delete;
    TrainingServiceRegistry& operator=(TrainingServiceRegistry&&) = delete;
    
private:
    TrainingServiceRegistry() = default;
    ~TrainingServiceRegistry() = default;
    
    mutable std::mutex mutex_;
    std::shared_ptr<themis::sharding::ShardRouter> shard_router_;
    std::shared_ptr<themis::sharding::ShardTopology> shard_topology_;
};

} // namespace lora
} // namespace llm
} // namespace themis
