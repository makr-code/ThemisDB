/**
 * @file training_service_registry.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

class TrainingServiceRegistry {
public:
    /**
     * @brief Get Instance.
     * @return Return value.
     */
    static TrainingServiceRegistry& getInstance();
    
    /**
     * @brief Register Shard Router.
     * @param[in] router Input parameter.
     */
    void registerShardRouter(std::shared_ptr<themis::sharding::ShardRouter> router);
    
    /**
     * @brief Register Shard Topology.
     * @param[in] topology Input parameter.
     */
    void registerShardTopology(std::shared_ptr<themis::sharding::ShardTopology> topology);
    
    /**
     * @brief Get Shard Router.
     * @return Return value.
     */
    std::shared_ptr<themis::sharding::ShardRouter> getShardRouter() const;
    
    /**
     * @brief Get Shard Topology.
     * @return Return value.
     */
    std::shared_ptr<themis::sharding::ShardTopology> getShardTopology() const;
    
    /**
     * @brief Has Shard Infrastructure.
     * @return True when the operation succeeds.
     */
    bool hasShardInfrastructure() const;
    
    /**
     * @brief Clear.
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
