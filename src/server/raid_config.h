#pragma once

#include "sharding/redundancy_strategy.h"
#include <nlohmann/json.hpp>

namespace themis::server::detail {

inline sharding::RedundancyConfig parseRaidConfig(const nlohmann::json& input) {
    sharding::RedundancyConfig config;
    const auto mode = input.value("mode", std::string("MIRROR"));
    if (mode == "NONE") config.mode = sharding::RedundancyMode::NONE;
    else if (mode == "MIRROR") config.mode = sharding::RedundancyMode::MIRROR;
    else if (mode == "STRIPE") config.mode = sharding::RedundancyMode::STRIPE;
    else if (mode == "STRIPE_MIRROR") config.mode = sharding::RedundancyMode::STRIPE_MIRROR;
    else if (mode == "PARITY") config.mode = sharding::RedundancyMode::PARITY;
    else if (mode == "RAID6") config.mode = sharding::RedundancyMode::RAID6;
    else if (mode == "GEO_MIRROR") config.mode = sharding::RedundancyMode::GEO_MIRROR;

    config.replication_factor = input.value("replication_factor", 3);
    const auto concern = input.value("write_concern", std::string("MAJORITY"));
    if (concern == "ONE") config.write_concern = sharding::WriteConcern::ONE;
    else if (concern == "MAJORITY") config.write_concern = sharding::WriteConcern::MAJORITY;
    else if (concern == "ALL") config.write_concern = sharding::WriteConcern::ALL;
    else if (concern == "QUORUM") config.write_concern = sharding::WriteConcern::QUORUM;
    config.enable_raft_consensus = input.value("enable_raft_consensus", false);
    return config;
}

} // namespace themis::server::detail
