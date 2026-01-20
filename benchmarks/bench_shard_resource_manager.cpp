#include <benchmark/benchmark.h>
#include "sharding/shard_resource_manager.h"
#include "sharding/shard_topology.h"
#include "sharding/gossip_config_manager.h"

using namespace themis::sharding;

static void BM_ResourceManager_GetSnapshot(benchmark::State& state) {
    auto topology = std::make_shared<ShardTopology>();
    GossipConfigManagerConfig gossip_config;
    gossip_config.gossip_interval_ms = 60000; // Disable auto-gossip
    gossip_config.local_shard_id = "shard1";
    gossip_config.local_endpoint = "localhost:8001";
    auto gossip = std::make_shared<GossipConfigManager>(gossip_config, topology);
    
    ShardResourceManager::Config config;
    config.snapshot_interval_ms = 60000; // Disable auto-update
    
    ShardResourceManager manager("shard1", gossip, config);
    manager.start();
    
    for (auto _ : state) {
        auto snapshot = manager.getCurrentSnapshot();
        benchmark::DoNotOptimize(snapshot);
    }
    
    manager.stop();
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ResourceManager_GetSnapshot)->Unit(benchmark::kMicrosecond);

static void BM_ResourceManager_CanAcceptQuery(benchmark::State& state) {
    auto topology = std::make_shared<ShardTopology>();
    GossipConfigManagerConfig gossip_config;
    gossip_config.gossip_interval_ms = 60000; // Disable auto-gossip
    gossip_config.local_shard_id = "shard1";
    gossip_config.local_endpoint = "localhost:8001";
    auto gossip = std::make_shared<GossipConfigManager>(gossip_config, topology);
    ShardResourceManager manager("shard1", gossip);
    
    ShardResourceManager::QuerySpec query;
    query.estimated_memory_bytes = 100 * 1024 * 1024;
    query.estimated_cpu_percent = 20;
    
    for (auto _ : state) {
        bool result = manager.canAcceptQuery(query);
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ResourceManager_CanAcceptQuery)->Unit(benchmark::kNanosecond);

static void BM_ResourceManager_PeerLookup(benchmark::State& state) {
    auto topology = std::make_shared<ShardTopology>();
    GossipConfigManagerConfig gossip_config;
    gossip_config.gossip_interval_ms = 60000; // Disable auto-gossip
    gossip_config.local_shard_id = "shard1";
    gossip_config.local_endpoint = "localhost:8001";
    auto gossip = std::make_shared<GossipConfigManager>(gossip_config, topology);
    ShardResourceManager manager("shard1", gossip);
    
    // Pre-populate peer cache
    for (int i = 0; i < state.range(0); ++i) {
        ShardResourceManager::ResourceSnapshot snapshot;
        snapshot.health_score = 90.0f;
        snapshot.cpu_usage_percent = 50.0f;
        snapshot.ram_usage_bytes = 2ULL * 1024 * 1024 * 1024;
        snapshot.ram_total_bytes = 8ULL * 1024 * 1024 * 1024;
        manager.receiveResourceUpdate("shard" + std::to_string(i), snapshot);
    }
    
    for (auto _ : state) {
        auto peers = manager.getPeerResources();
        benchmark::DoNotOptimize(peers);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ResourceManager_PeerLookup)
    ->Arg(10)->Arg(50)->Arg(100)->Arg(500)
    ->Unit(benchmark::kMicrosecond);

static void BM_ResourceManager_GetHealthyPeers(benchmark::State& state) {
    auto topology = std::make_shared<ShardTopology>();
    GossipConfigManagerConfig gossip_config;
    gossip_config.gossip_interval_ms = 60000; // Disable auto-gossip
    gossip_config.local_shard_id = "shard1";
    gossip_config.local_endpoint = "localhost:8001";
    auto gossip = std::make_shared<GossipConfigManager>(gossip_config, topology);
    ShardResourceManager manager("shard1", gossip);
    
    // Pre-populate with mix of healthy and unhealthy peers
    for (int i = 0; i < state.range(0); ++i) {
        ShardResourceManager::ResourceSnapshot snapshot;
        snapshot.health_score = (i % 2 == 0) ? 90.0f : 30.0f;
        manager.receiveResourceUpdate("shard" + std::to_string(i), snapshot);
    }
    
    for (auto _ : state) {
        auto healthy = manager.getHealthyPeers();
        benchmark::DoNotOptimize(healthy);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ResourceManager_GetHealthyPeers)
    ->Arg(10)->Arg(50)->Arg(100)
    ->Unit(benchmark::kMicrosecond);

static void BM_ResourceManager_CalculateHealthScore(benchmark::State& state) {
    auto topology = std::make_shared<ShardTopology>();
    GossipConfigManagerConfig gossip_config;
    gossip_config.gossip_interval_ms = 60000; // Disable auto-gossip
    gossip_config.local_shard_id = "shard1";
    gossip_config.local_endpoint = "localhost:8001";
    auto gossip = std::make_shared<GossipConfigManager>(gossip_config, topology);
    ShardResourceManager manager("shard1", gossip);
    
    for (auto _ : state) {
        float score = manager.calculateHealthScore();
        benchmark::DoNotOptimize(score);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ResourceManager_CalculateHealthScore)->Unit(benchmark::kNanosecond);

static void BM_ResourceManager_SnapshotSerialization(benchmark::State& state) {
    ShardResourceManager::ResourceSnapshot snapshot;
    snapshot.cpu_usage_percent = 50.5f;
    snapshot.ram_usage_bytes = 2ULL * 1024 * 1024 * 1024;
    snapshot.ram_total_bytes = 8ULL * 1024 * 1024 * 1024;
    snapshot.health_score = 85.0f;
    snapshot.active_queries = 10;
    snapshot.pending_queries = 5;
    snapshot.timestamp = std::chrono::system_clock::now();
    
    for (auto _ : state) {
        auto json = snapshot.toJson();
        benchmark::DoNotOptimize(json);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ResourceManager_SnapshotSerialization)->Unit(benchmark::kNanosecond);

BENCHMARK_MAIN();
