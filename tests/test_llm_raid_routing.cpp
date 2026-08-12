// Copyright 2026 ThemisDB — Licensed under MIT License

/**
 * @file test_llm_raid_routing.cpp
 * @brief LLM+RAID Phase 2 integration: focused unit tests for LEAST_LOADED
 *        domain routing.  Tests CBS-LLM-01..03 live in
 *        test_continuous_batch_scheduler.cpp and are exercised via themis_tests.
 *
 * Coverage (6 tests):
 *  - DLR-01: routeByDomain() picks shard with highest accuracy_delta
 *  - DLR-02: LEAST_LOADED tie-break — idle shard wins when deltas are equal
 *  - DLR-03: Higher accuracy_delta beats lower pending load
 *  - DLR-04: updateShardLLMLoad() is idempotent (last write wins)
 *  - DLR-05: routeByDomain() returns "" when no domain is registered
 *  - DLR-06: Three-way tie — least pending wins
 */

#include <gtest/gtest.h>

#include "sharding/adaptive_shard_router.h"
#include "sharding/consistent_hash.h"
#include "sharding/shard_topology.h"
#include "sharding/urn_resolver.h"
#include "distributed_knowledge/adapter_capability_announcement.h"

#include <memory>
#include <string>

using namespace themis::distributed_knowledge;
using namespace themis::sharding;

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

namespace {

/// Build a minimal AdaptiveShardRouter that only uses routeByDomain() /
/// updateAdapterCapability() / updateShardLLMLoad().
AdaptiveShardRouter makeRouter() {
    auto topology = std::make_shared<ShardTopology>();
    auto ring     = std::make_shared<ConsistentHashRing>();
    auto resolver = std::make_shared<URNResolver>(topology, ring);
    ShardRouter::Config base_cfg;
    return AdaptiveShardRouter(resolver, nullptr, topology, base_cfg);
}

/// Register a capability for a shard.
void reg(AdaptiveShardRouter& r,
         const std::string& shard_id,
         AdapterDomainType domain,
         double delta)
{
    AdapterCapabilityAnnouncement cap;
    cap.domain_type    = domain;
    cap.accuracy_delta = delta;
    cap.adapter_version = "v1";
    r.updateAdapterCapability(shard_id, cap);
}

} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// Domain-routing tests
// ─────────────────────────────────────────────────────────────────────────────

// DLR-01: Highest accuracy_delta wins without load information.
TEST(LLMRaidRouting, DLR01_HighestAccuracyDeltaWins) {
    auto r = makeRouter();
    reg(r, "shard-low",  AdapterDomainType::TRANSACTION, 0.3);
    reg(r, "shard-high", AdapterDomainType::TRANSACTION, 0.8);
    reg(r, "shard-mid",  AdapterDomainType::TRANSACTION, 0.5);

    EXPECT_EQ(r.routeByDomain(AdapterDomainType::TRANSACTION), "shard-high");
}

// DLR-02: Tied accuracy_delta → idle shard (lower pending) wins.
TEST(LLMRaidRouting, DLR02_TieBreakPicksIdleShard) {
    auto r = makeRouter();
    reg(r, "shard-busy", AdapterDomainType::TRANSACTION, 0.7);
    reg(r, "shard-idle", AdapterDomainType::TRANSACTION, 0.7);

    r.updateShardLLMLoad("shard-busy", /*pending=*/80, /*avg_queue_ms=*/150.0);
    r.updateShardLLMLoad("shard-idle", /*pending=*/1,  /*avg_queue_ms=*/2.0);

    EXPECT_EQ(r.routeByDomain(AdapterDomainType::TRANSACTION), "shard-idle");
}

// DLR-03: Higher accuracy_delta beats low pending load.
TEST(LLMRaidRouting, DLR03_HigherAccuracyBeatsLowerLoad) {
    auto r = makeRouter();
    reg(r, "shard-expert", AdapterDomainType::GEOSPATIAL, 0.95);
    reg(r, "shard-empty",  AdapterDomainType::GEOSPATIAL, 0.40);

    r.updateShardLLMLoad("shard-expert", /*pending=*/200, /*avg_queue_ms=*/500.0);
    r.updateShardLLMLoad("shard-empty",  /*pending=*/0,   /*avg_queue_ms=*/0.0);

    EXPECT_EQ(r.routeByDomain(AdapterDomainType::GEOSPATIAL), "shard-expert");
}

// DLR-04: updateShardLLMLoad() is idempotent — last write wins.
TEST(LLMRaidRouting, DLR04_UpdateLLMLoadLastWriteWins) {
    auto r = makeRouter();
    reg(r, "shard-a", AdapterDomainType::VECTOR_SEARCH, 0.6);
    reg(r, "shard-b", AdapterDomainType::VECTOR_SEARCH, 0.6);

    // First: shard-a has low load → would win
    r.updateShardLLMLoad("shard-a", /*pending=*/2,  /*avg_queue_ms=*/3.0);
    r.updateShardLLMLoad("shard-b", /*pending=*/50, /*avg_queue_ms=*/100.0);
    EXPECT_EQ(r.routeByDomain(AdapterDomainType::VECTOR_SEARCH), "shard-a");

    // Overwrite: shard-b now has low load → must win after update
    r.updateShardLLMLoad("shard-a", /*pending=*/50, /*avg_queue_ms=*/100.0);
    r.updateShardLLMLoad("shard-b", /*pending=*/2,  /*avg_queue_ms=*/3.0);
    EXPECT_EQ(r.routeByDomain(AdapterDomainType::VECTOR_SEARCH), "shard-b");
}

// DLR-05: routeByDomain() returns "" when no shard registered for domain.
TEST(LLMRaidRouting, DLR05_UnknownDomainReturnsEmpty) {
    auto r = makeRouter();
    reg(r, "shard-a", AdapterDomainType::TRANSACTION, 0.5);

    EXPECT_EQ(r.routeByDomain(AdapterDomainType::GEOSPATIAL), "");
}

// DLR-06: Three-way tie → shard with minimum pending wins.
TEST(LLMRaidRouting, DLR06_ThreeWayTiePicksMinPending) {
    auto r = makeRouter();
    reg(r, "s0", AdapterDomainType::SCHEMA_ADVISOR, 0.55);
    reg(r, "s1", AdapterDomainType::SCHEMA_ADVISOR, 0.55);
    reg(r, "s2", AdapterDomainType::SCHEMA_ADVISOR, 0.55);

    r.updateShardLLMLoad("s0", /*pending=*/30, /*avg_queue_ms=*/60.0);
    r.updateShardLLMLoad("s1", /*pending=*/5,  /*avg_queue_ms=*/10.0);
    r.updateShardLLMLoad("s2", /*pending=*/20, /*avg_queue_ms=*/40.0);

    EXPECT_EQ(r.routeByDomain(AdapterDomainType::SCHEMA_ADVISOR), "s1");
}

