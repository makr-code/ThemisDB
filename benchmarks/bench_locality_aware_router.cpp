#include <benchmark/benchmark.h>
#include "sharding/locality_aware_router.h"

using namespace themis::sharding;

// Test configuration constants
namespace {
    constexpr uint32_t TEST_GOSSIP_INTERVAL_MS = 10000; // Disable auto-gossip for benchmarks
    constexpr const char* TEST_LOCAL_ENDPOINT = "localhost:8001";
}

static void BM_LocalityRouter_RouteQuery(benchmark::State& state) {
    auto topology = std::make_shared<ShardTopology>();
    for (int i = 0; i < state.range(0); ++i) {
        ShardInfo shard;
        shard.shard_id = "shard" + std::to_string(i);
        shard.primary_endpoint = "localhost:50000";
        shard.datacenter = "dc1";
        shard.is_healthy = true;
        topology->addShard(shard);
    }
    
    GossipConfigManagerConfig gossip_config;
    gossip_config.gossip_interval_ms = TEST_GOSSIP_INTERVAL_MS;
    gossip_config.local_shard_id = "shard0";
    gossip_config.local_endpoint = TEST_LOCAL_ENDPOINT;
    
    auto gossip = std::make_shared<GossipConfigManager>(gossip_config, topology);
    auto resource_mgr = std::make_shared<ShardResourceManager>("shard0", gossip);
    LocalityAwareRouter router("shard0", topology, resource_mgr);
    
    // Pre-populate placement data
    for (int i = 0; i < 1000; ++i) {
        router.updateDataPlacement("users", "user:" + std::to_string(i),
                                    "shard" + std::to_string(i % state.range(0)));
    }
    
    LocalityAwareRouter::QuerySpec spec;
    spec.accessed_collections = {"users"};
    spec.accessed_keys = {"user:42"};
    
    for (auto _ : state) {
        std::string target = router.routeQuery(spec);
        benchmark::DoNotOptimize(target);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LocalityRouter_RouteQuery)
    ->Arg(10)->Arg(50)->Arg(100)->Arg(500)
    ->Unit(benchmark::kMicrosecond);

static void BM_LocalityRouter_ComputeAffinity(benchmark::State& state) {
    auto topology = std::make_shared<ShardTopology>();
    for (int i = 0; i < 10; ++i) {
        ShardInfo shard;
        shard.shard_id = "shard" + std::to_string(i);
        shard.primary_endpoint = "localhost:50000";
        shard.datacenter = "dc1";
        shard.is_healthy = true;
        topology->addShard(shard);
    }
    
    GossipConfigManagerConfig gossip_config;
    gossip_config.gossip_interval_ms = TEST_GOSSIP_INTERVAL_MS;
    gossip_config.local_shard_id = "shard0";
    gossip_config.local_endpoint = TEST_LOCAL_ENDPOINT;
    
    auto gossip = std::make_shared<GossipConfigManager>(gossip_config, topology);
    auto resource_mgr = std::make_shared<ShardResourceManager>("shard0", gossip);
    LocalityAwareRouter router("shard0", topology, resource_mgr);
    
    LocalityAwareRouter::QuerySpec spec;
    spec.accessed_collections = {"users"};
    for (int i = 0; i < state.range(0); ++i) {
        spec.accessed_keys.push_back("key:" + std::to_string(i));
    }
    
    for (auto _ : state) {
        auto affinities = router.computeAffinity(spec);
        benchmark::DoNotOptimize(affinities);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LocalityRouter_ComputeAffinity)
    ->Arg(10)->Arg(100)->Arg(1000)
    ->Unit(benchmark::kMicrosecond);

static void BM_LocalityRouter_UpdatePlacement(benchmark::State& state) {
    auto topology = std::make_shared<ShardTopology>();
    
    ShardInfo shard;
    shard.shard_id = "shard0";
    shard.primary_endpoint = "localhost:50000";
    shard.datacenter = "dc1";
    shard.is_healthy = true;
    topology->addShard(shard);
    
    GossipConfigManagerConfig gossip_config;
    gossip_config.gossip_interval_ms = TEST_GOSSIP_INTERVAL_MS;
    gossip_config.local_shard_id = "shard0";
    gossip_config.local_endpoint = TEST_LOCAL_ENDPOINT;
    
    auto gossip = std::make_shared<GossipConfigManager>(gossip_config, topology);
    auto resource_mgr = std::make_shared<ShardResourceManager>("shard0", gossip);
    LocalityAwareRouter router("shard0", topology, resource_mgr);
    
    int counter = 0;
    for (auto _ : state) {
        router.updateDataPlacement("collection", 
                                    "key:" + std::to_string(counter++),
                                    "shard0");
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LocalityRouter_UpdatePlacement)
    ->Unit(benchmark::kNanosecond);

static void BM_LocalityRouter_HasData(benchmark::State& state) {
    auto topology = std::make_shared<ShardTopology>();
    
    ShardInfo shard;
    shard.shard_id = "shard0";
    shard.primary_endpoint = "localhost:50000";
    shard.datacenter = "dc1";
    shard.is_healthy = true;
    topology->addShard(shard);
    
    GossipConfigManagerConfig gossip_config;
    gossip_config.gossip_interval_ms = TEST_GOSSIP_INTERVAL_MS;
    gossip_config.local_shard_id = "shard0";
    gossip_config.local_endpoint = TEST_LOCAL_ENDPOINT;
    
    auto gossip = std::make_shared<GossipConfigManager>(gossip_config, topology);
    auto resource_mgr = std::make_shared<ShardResourceManager>("shard0", gossip);
    LocalityAwareRouter router("shard0", topology, resource_mgr);
    
    // Pre-populate some data
    for (int i = 0; i < 1000; ++i) {
        router.updateDataPlacement("collection", "key:" + std::to_string(i), "shard0");
    }
    
    for (auto _ : state) {
        bool result = router.hasData("shard0", "collection", "key:500");
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LocalityRouter_HasData)
    ->Unit(benchmark::kNanosecond);

static void BM_LocalityRouter_MultiShardQuery(benchmark::State& state) {
    auto topology = std::make_shared<ShardTopology>();
    for (int i = 0; i < 10; ++i) {
        ShardInfo shard;
        shard.shard_id = "shard" + std::to_string(i);
        shard.primary_endpoint = "localhost:50000";
        shard.datacenter = "dc1";
        shard.is_healthy = true;
        topology->addShard(shard);
    }
    
    GossipConfigManagerConfig gossip_config;
    gossip_config.gossip_interval_ms = TEST_GOSSIP_INTERVAL_MS;
    gossip_config.local_shard_id = "shard0";
    gossip_config.local_endpoint = TEST_LOCAL_ENDPOINT;
    
    auto gossip = std::make_shared<GossipConfigManager>(gossip_config, topology);
    auto resource_mgr = std::make_shared<ShardResourceManager>("shard0", gossip);
    LocalityAwareRouter router("shard0", topology, resource_mgr);
    
    // Spread data across multiple shards
    for (int i = 0; i < 100; ++i) {
        router.updateDataPlacement("users", "user:" + std::to_string(i),
                                    "shard" + std::to_string(i % 10));
    }
    
    LocalityAwareRouter::QuerySpec spec;
    spec.accessed_collections = {"users"};
    for (int i = 0; i < state.range(0); ++i) {
        spec.accessed_keys.push_back("user:" + std::to_string(i));
    }
    
    for (auto _ : state) {
        auto targets = router.routeMultiShardQuery(spec);
        benchmark::DoNotOptimize(targets);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LocalityRouter_MultiShardQuery)
    ->Arg(10)->Arg(50)->Arg(100)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK_MAIN();
