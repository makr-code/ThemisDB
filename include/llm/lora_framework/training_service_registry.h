/**
 * @file training_service_registry.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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
