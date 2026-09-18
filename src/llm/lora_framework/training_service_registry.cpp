/**
 * @file training_service_registry.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "llm/lora_framework/training_service_registry.h"
#include "sharding/shard_router.h"
#include "sharding/shard_topology.h"
#include <spdlog/spdlog.h>

namespace themis {
namespace llm {
namespace lora {

/**
 * @brief Get Instance.
 * @return Return value.
 * @details Implements getInstance without additional internal calls.
 */
TrainingServiceRegistry& TrainingServiceRegistry::getInstance() {
    static TrainingServiceRegistry instance;
    return instance;
}

/**
 * @brief Register Shard Router.
 * @param[in] router Input parameter.
 * @details Calls: lock(), spdlog::info().
 */
void TrainingServiceRegistry::registerShardRouter(
    std::shared_ptr<themis::sharding::ShardRouter> router
) {
    std::lock_guard<std::mutex> lock(mutex_);
    shard_router_ = router;
    if (router) {
        spdlog::info("ShardRouter registered in TrainingServiceRegistry");
    } else {
        spdlog::info("ShardRouter unregistered from TrainingServiceRegistry");
    }
}

/**
 * @brief Register Shard Topology.
 * @param[in] topology Input parameter.
 * @details Calls: lock(), spdlog::info().
 */
void TrainingServiceRegistry::registerShardTopology(
    std::shared_ptr<themis::sharding::ShardTopology> topology
) {
    std::lock_guard<std::mutex> lock(mutex_);
    shard_topology_ = topology;
    if (topology) {
        spdlog::info("ShardTopology registered in TrainingServiceRegistry");
    } else {
        spdlog::info("ShardTopology unregistered from TrainingServiceRegistry");
    }
}

std::shared_ptr<themis::sharding::ShardRouter> 
TrainingServiceRegistry::getShardRouter() const {
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lock(mutex_);
    return shard_router_;
}

std::shared_ptr<themis::sharding::ShardTopology> 
TrainingServiceRegistry::getShardTopology() const {
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lock(mutex_);
    return shard_topology_;
}

bool TrainingServiceRegistry::hasShardInfrastructure() const {
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lock(mutex_);
    return shard_router_ != nullptr && shard_topology_ != nullptr;
}

/**
 * @brief Clear.
 * @details Calls: lock(), spdlog::info().
 */
void TrainingServiceRegistry::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    shard_router_ = nullptr;
    shard_topology_ = nullptr;
    spdlog::info("TrainingServiceRegistry cleared");
}

} // namespace lora
} // namespace llm
} // namespace themis
