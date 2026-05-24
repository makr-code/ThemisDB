/*
 * ThemisDB | File: training_service_registry.cpp | Version: 0.0.47 | Last Modified: 2026-04-15 18:58:58
 * Author: ThemisDB Version Bot | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 73
 * Open Issues: TODOs=1, Stubs=1, Gaps=3, Unimpl=0, Mock=1, Sim=0, Debt=0
 * Gap Correlation: internal=3 | external_v3=21 | delta=18 | status=divergent
 * External Severity (v3): C=6, H=14, M=1
 * PR: #745 Integrate ShardRouter and ShardTopology for inter-shard RPC communi... (2026-03-23T21:52:05Z)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
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
