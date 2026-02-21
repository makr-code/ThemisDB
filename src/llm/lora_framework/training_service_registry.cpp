/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            training_service_registry.cpp                      ║
  Version:         0.0.23                                             ║
  Last Modified:   2026-02-21 19:43:05                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     91                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 03329d86d  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31e8b8df0  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 0d722b04c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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
