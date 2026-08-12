/**
 * @file training_service_registry.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

TrainingServiceRegistry& TrainingServiceRegistry::getInstance() {
    static TrainingServiceRegistry instance;
    return instance;
}

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
    std::lock_guard<std::mutex> lock(mutex_);
    return shard_router_;
}

std::shared_ptr<themis::sharding::ShardTopology> 
TrainingServiceRegistry::getShardTopology() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return shard_topology_;
}

bool TrainingServiceRegistry::hasShardInfrastructure() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return shard_router_ != nullptr && shard_topology_ != nullptr;
}

void TrainingServiceRegistry::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    shard_router_ = nullptr;
    shard_topology_ = nullptr;
    spdlog::info("TrainingServiceRegistry cleared");
}

} // namespace lora
} // namespace llm
} // namespace themis
