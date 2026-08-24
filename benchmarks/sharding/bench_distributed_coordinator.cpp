#include <benchmark/benchmark.h>
#include "sharding/distributed_coordinator.h"

using namespace themis::sharding;

static void BM_Coordinator_StartElection(benchmark::State& state) {
    for (auto _ : state) {
        state.PauseTiming();
        
        // Setup topology (not measured)
        auto topology = std::make_shared<ShardTopology>();
        for (int i = 0; i < state.range(0); ++i) {
            ShardInfo shard;
            shard.shard_id = "shard" + std::to_string(i);
            shard.primary_endpoint = "localhost:" + std::to_string(8000 + i);
            shard.is_healthy = true;
            topology->addShard(shard);
        }
        
        GossipConfigManagerConfig gossip_config;
        gossip_config.local_shard_id = "shard0";
        gossip_config.local_endpoint = "localhost:8000";
        gossip_config.enabled = false;
        
        auto gossip = std::make_shared<GossipConfigManager>(gossip_config, topology);
        
        DistributedCoordinator::Config config;
        config.election_timeout_ms = 100;  // Faster for benchmarking
        
        DistributedCoordinator coordinator("shard0", topology, gossip, config);
        
        state.ResumeTiming();
        
        // Measured operations
        coordinator.startElection();
        coordinator.stepDown();  // Reset for next iteration
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Coordinator_StartElection)
    ->Arg(3)->Arg(10)->Arg(50)
    ->Unit(benchmark::kMillisecond);

static void BM_Coordinator_ScheduleTask(benchmark::State& state) {
    // Setup (one-time, not measured)
    auto topology = std::make_shared<ShardTopology>();
    
    GossipConfigManagerConfig gossip_config;
    gossip_config.local_shard_id = "shard1";
    gossip_config.local_endpoint = "localhost:8001";
    gossip_config.enabled = false;
    
    auto gossip = std::make_shared<GossipConfigManager>(gossip_config, topology);
    
    DistributedCoordinator coordinator("shard1", topology, gossip);
    coordinator.becomeLeader();  // Manually become leader
    
    int counter = 0;
    for (auto _ : state) {
        DistributedCoordinator::CoordinatorTask task;
        task.task_id = "task-" + std::to_string(counter++);
        task.type = DistributedCoordinator::TaskType::MAINTENANCE;
        task.created_at = std::chrono::system_clock::now();
        task.assigned_leader = "shard1";
        
        coordinator.scheduleTask(task);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Coordinator_ScheduleTask)->Unit(benchmark::kMicrosecond);

static void BM_Coordinator_GetLeaderInfo(benchmark::State& state) {
    auto topology = std::make_shared<ShardTopology>();
    
    GossipConfigManagerConfig gossip_config;
    gossip_config.local_shard_id = "shard1";
    gossip_config.local_endpoint = "localhost:8001";
    gossip_config.enabled = false;
    
    auto gossip = std::make_shared<GossipConfigManager>(gossip_config, topology);
    
    DistributedCoordinator coordinator("shard1", topology, gossip);
    coordinator.becomeLeader();
    
    for (auto _ : state) {
        auto info = coordinator.getLeaderInfo();
        benchmark::DoNotOptimize(info);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Coordinator_GetLeaderInfo)->Unit(benchmark::kNanosecond);

static void BM_Coordinator_GetStatistics(benchmark::State& state) {
    auto topology = std::make_shared<ShardTopology>();
    
    GossipConfigManagerConfig gossip_config;
    gossip_config.local_shard_id = "shard1";
    gossip_config.local_endpoint = "localhost:8001";
    gossip_config.enabled = false;
    
    auto gossip = std::make_shared<GossipConfigManager>(gossip_config, topology);
    
    DistributedCoordinator coordinator("shard1", topology, gossip);
    
    for (auto _ : state) {
        auto stats = coordinator.getStatistics();
        benchmark::DoNotOptimize(stats);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Coordinator_GetStatistics)->Unit(benchmark::kNanosecond);

static void BM_Coordinator_StepDown(benchmark::State& state) {
    auto topology = std::make_shared<ShardTopology>();
    
    GossipConfigManagerConfig gossip_config;
    gossip_config.local_shard_id = "shard1";
    gossip_config.local_endpoint = "localhost:8001";
    gossip_config.enabled = false;
    
    auto gossip = std::make_shared<GossipConfigManager>(gossip_config, topology);
    
    DistributedCoordinator coordinator("shard1", topology, gossip);
    
    for (auto _ : state) {
        coordinator.becomeLeader();
        coordinator.stepDown();
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Coordinator_StepDown)->Unit(benchmark::kMicrosecond);

static void BM_Coordinator_TaskJsonSerialization(benchmark::State& state) {
    DistributedCoordinator::CoordinatorTask task;
    task.task_id = "benchmark-task-001";
    task.type = DistributedCoordinator::TaskType::REBALANCE;
    task.payload = {
        {"source_shard", "shard1"},
        {"target_shard", "shard2"},
        {"collection", "users"},
        {"token_range_start", 0},
        {"token_range_end", 1000000}
    };
    task.ttl = std::chrono::seconds(600);
    task.created_at = std::chrono::system_clock::now();
    task.started_at = std::chrono::system_clock::now();
    task.assigned_leader = "shard1";
    
    for (auto _ : state) {
        auto json = task.toJson();
        benchmark::DoNotOptimize(json);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Coordinator_TaskJsonSerialization)->Unit(benchmark::kMicrosecond);

static void BM_Coordinator_TaskJsonDeserialization(benchmark::State& state) {
    DistributedCoordinator::CoordinatorTask task;
    task.task_id = "benchmark-task-001";
    task.type = DistributedCoordinator::TaskType::REBALANCE;
    task.payload = {
        {"source_shard", "shard1"},
        {"target_shard", "shard2"}
    };
    task.ttl = std::chrono::seconds(600);
    task.created_at = std::chrono::system_clock::now();
    task.started_at = std::chrono::system_clock::now();
    task.assigned_leader = "shard1";
    
    auto json = task.toJson();
    
    for (auto _ : state) {
        auto deserialized = DistributedCoordinator::CoordinatorTask::fromJson(json);
        benchmark::DoNotOptimize(deserialized);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Coordinator_TaskJsonDeserialization)->Unit(benchmark::kMicrosecond);

static void BM_Coordinator_GetPendingTasks(benchmark::State& state) {
    // Setup (one-time, not measured)
    auto topology = std::make_shared<ShardTopology>();
    
    GossipConfigManagerConfig gossip_config;
    gossip_config.local_shard_id = "shard1";
    gossip_config.local_endpoint = "localhost:8001";
    gossip_config.enabled = false;
    
    auto gossip = std::make_shared<GossipConfigManager>(gossip_config, topology);
    
    DistributedCoordinator coordinator("shard1", topology, gossip);
    coordinator.becomeLeader();
    
    // Add some tasks
    for (int i = 0; i < state.range(0); ++i) {
        DistributedCoordinator::CoordinatorTask task;
        task.task_id = "task-" + std::to_string(i);
        task.type = DistributedCoordinator::TaskType::MAINTENANCE;
        task.created_at = std::chrono::system_clock::now();
        task.assigned_leader = "shard1";
        coordinator.scheduleTask(task);
    }
    
    for (auto _ : state) {
        auto tasks = coordinator.getPendingTasks();
        benchmark::DoNotOptimize(tasks);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Coordinator_GetPendingTasks)
    ->Arg(10)->Arg(100)->Arg(1000)
    ->Unit(benchmark::kMicrosecond);

static void BM_Coordinator_CancelTask(benchmark::State& state) {
    auto topology = std::make_shared<ShardTopology>();
    
    GossipConfigManagerConfig gossip_config;
    gossip_config.local_shard_id = "shard1";
    gossip_config.local_endpoint = "localhost:8001";
    gossip_config.enabled = false;
    
    auto gossip = std::make_shared<GossipConfigManager>(gossip_config, topology);
    
    DistributedCoordinator coordinator("shard1", topology, gossip);
    coordinator.becomeLeader();
    
    int counter = 0;
    for (auto _ : state) {
        // Schedule a task
        DistributedCoordinator::CoordinatorTask task;
        task.task_id = "task-" + std::to_string(counter);
        task.type = DistributedCoordinator::TaskType::MAINTENANCE;
        task.created_at = std::chrono::system_clock::now();
        task.assigned_leader = "shard1";
        coordinator.scheduleTask(task);
        
        // Cancel it
        bool cancelled = coordinator.cancelTask(task.task_id);
        benchmark::DoNotOptimize(cancelled);
        
        counter++;
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Coordinator_CancelTask)->Unit(benchmark::kMicrosecond);

static void BM_Coordinator_BecomeLeader(benchmark::State& state) {
    auto topology = std::make_shared<ShardTopology>();
    
    GossipConfigManagerConfig gossip_config;
    gossip_config.local_shard_id = "shard1";
    gossip_config.local_endpoint = "localhost:8001";
    gossip_config.enabled = false;
    
    auto gossip = std::make_shared<GossipConfigManager>(gossip_config, topology);
    
    DistributedCoordinator coordinator("shard1", topology, gossip);
    
    for (auto _ : state) {
        coordinator.becomeLeader();
        coordinator.stepDown();
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Coordinator_BecomeLeader)->Unit(benchmark::kMicrosecond);

BENCHMARK_MAIN();
