// Benchmark: Gossip Config Manager Performance
// Measures gossip protocol performance including broadcast latency,
// gossip rounds, and conflict resolution overhead

#include "sharding/gossip_config_manager.h"
#include "sharding/shard_topology.h"
#include "shard_rpc.pb.h"
#include <benchmark/benchmark.h>
#include <memory>
#include <random>
#include <string>
#include <chrono>

using namespace themis::sharding;

// ============================================================================
// Benchmark Fixtures
// ============================================================================

class GossipConfigBenchmarkFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& state) override {
        num_shards_ = state.range(0);
        
        // Create topology with shards
        topology_ = std::make_shared<ShardTopology>();
        
        static constexpr int BASE_PORT = 8000;
        for (int i = 0; i < num_shards_; i++) {
            ShardInfo shard;
            shard.shard_id = "shard-" + std::to_string(i);
            shard.primary_endpoint = "localhost:" + std::to_string(BASE_PORT + i);
            shard.is_healthy = true;
            topology_->addShard(shard);
        }
        
        // Create gossip config manager
        GossipConfigManagerConfig config;
        config.enabled = false;  // Controlled manually for benchmarks
        config.local_shard_id = "shard-0";
        config.local_endpoint = "localhost:8000";
        config.gossip_interval_ms = 1000;
        config.fanout = 3;
        
        manager_ = std::make_unique<GossipConfigManager>(config, topology_);
    }
    
    void TearDown(const ::benchmark::State& state) override {
        if (manager_) {
            manager_->stop();
        }
    }
    
protected:
    int num_shards_;
    std::shared_ptr<ShardTopology> topology_;
    std::unique_ptr<GossipConfigManager> manager_;
};

// ============================================================================
// Broadcast Latency Benchmarks
// ============================================================================

BENCHMARK_DEFINE_F(GossipConfigBenchmarkFixture, BroadcastLatency)(benchmark::State& state) {
    // Benchmark: Measure time to create and serialize a config update message
    // Target: <20 μs as per acceptance criteria
    
    std::string config_key = "benchmark.config.key";
    std::string config_value = "benchmark_value";
    
    for (auto _ : state) {
        auto start = std::chrono::high_resolution_clock::now();
        
        // Publish config update (creates message, updates vector clock, stores locally)
        std::string update_id = manager_->publishConfigUpdate(config_key, config_value);
        
        auto end = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        
        state.SetIterationTime(elapsed.count() / 1e9);  // Convert to seconds
        
        benchmark::DoNotOptimize(update_id);
    }
    
    // Report in microseconds for easier comparison with 20 μs target
    state.SetLabel("Broadcast time (μs)");
    state.counters["broadcast_us"] = benchmark::Counter(
        state.iterations(), 
        benchmark::Counter::kIsRate | benchmark::Counter::kInvert
    );
}

BENCHMARK_REGISTER_F(GossipConfigBenchmarkFixture, BroadcastLatency)
    ->Arg(10)   // 10 shards
    ->Arg(50)   // 50 shards
    ->Arg(100)  // 100 shards
    ->Arg(500)  // 500 shards
    ->UseManualTime()
    ->Unit(benchmark::kMicrosecond);

// ============================================================================
// Vector Clock Operations
// ============================================================================

static void BM_VectorClockIncrement(benchmark::State& state) {
    VectorClock clock;
    std::string shard_id = "shard-0";
    
    for (auto _ : state) {
        clock.increment(shard_id);
        benchmark::DoNotOptimize(clock);
    }
    
    state.SetLabel("Vector clock increment");
}

BENCHMARK(BM_VectorClockIncrement)->Unit(benchmark::kNanosecond);

static void BM_VectorClockMerge(benchmark::State& state) {
    int num_shards = state.range(0);
    
    VectorClock clock1;
    VectorClock clock2;
    
    // Initialize clocks with different values
    for (int i = 0; i < num_shards; ++i) {
        std::string shard_id = "shard-" + std::to_string(i);
        clock1.set(shard_id, i * 2);
        clock2.set(shard_id, i * 3);
    }
    
    for (auto _ : state) {
        VectorClock temp = clock1;
        temp.merge(clock2);
        benchmark::DoNotOptimize(temp);
    }
    
    state.SetLabel("Vector clock merge");
}

BENCHMARK(BM_VectorClockMerge)
    ->Arg(10)
    ->Arg(50)
    ->Arg(100)
    ->Arg(500)
    ->Unit(benchmark::kNanosecond);

static void BM_VectorClockCompare(benchmark::State& state) {
    int num_shards = state.range(0);
    
    VectorClock clock1;
    VectorClock clock2;
    
    for (int i = 0; i < num_shards; ++i) {
        std::string shard_id = "shard-" + std::to_string(i);
        clock1.set(shard_id, i * 2);
        clock2.set(shard_id, i * 3);
    }
    
    for (auto _ : state) {
        auto result = clock1.compare(clock2);
        benchmark::DoNotOptimize(result);
    }
    
    state.SetLabel("Vector clock compare");
}

BENCHMARK(BM_VectorClockCompare)
    ->Arg(10)
    ->Arg(50)
    ->Arg(100)
    ->Arg(500)
    ->Unit(benchmark::kNanosecond);

// ============================================================================
// Conflict Resolution Benchmarks
// ============================================================================

BENCHMARK_DEFINE_F(GossipConfigBenchmarkFixture, ConflictResolution)(benchmark::State& state) {
    // Benchmark: Measure conflict resolution overhead
    
    std::string config_key = "conflict.key";
    int conflict_count = 0;
    
    for (auto _ : state) {
        // Create two concurrent updates
        ConfigUpdate update1;
        update1.update_id = "update-1-" + std::to_string(conflict_count);
        update1.config_key = config_key;
        update1.config_value = "value1";
        update1.timestamp_ns = 1000 + conflict_count;
        update1.originator_shard_id = "shard-1";
        update1.ttl = 10;
        update1.vector_clock.set("shard-1", 5);
        update1.vector_clock.set("shard-2", 3);
        
        ConfigUpdate update2;
        update2.update_id = "update-2-" + std::to_string(conflict_count);
        update2.config_key = config_key;
        update2.config_value = "value2";
        update2.timestamp_ns = 2000 + conflict_count;
        update2.originator_shard_id = "shard-2";
        update2.ttl = 10;
        update2.vector_clock.set("shard-1", 3);
        update2.vector_clock.set("shard-2", 7);
        
        // Create gossip messages
        proto::GossipMessage msg1;
        msg1.set_sender_shard_id("shard-1");
        msg1.set_timestamp_ns(1000 + conflict_count);
        msg1.set_message_type("config_update");
        *msg1.mutable_config_update() = update1.toProto();
        *msg1.mutable_vector_clock() = update1.vector_clock.toProto();
        
        proto::GossipMessage msg2;
        msg2.set_sender_shard_id("shard-2");
        msg2.set_timestamp_ns(2000 + conflict_count);
        msg2.set_message_type("config_update");
        *msg2.mutable_config_update() = update2.toProto();
        *msg2.mutable_vector_clock() = update2.vector_clock.toProto();
        
        auto start = std::chrono::high_resolution_clock::now();
        
        // Apply both updates (conflict resolution happens here)
        manager_->handleGossipMessage(msg1);
        manager_->handleGossipMessage(msg2);
        
        auto end = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        
        state.SetIterationTime(elapsed.count() / 1e9);
        
        conflict_count++;
        benchmark::DoNotOptimize(msg1);
        benchmark::DoNotOptimize(msg2);
    }
    
    state.SetLabel("Conflict resolution overhead");
}

BENCHMARK_REGISTER_F(GossipConfigBenchmarkFixture, ConflictResolution)
    ->Arg(10)
    ->Arg(50)
    ->Arg(100)
    ->UseManualTime()
    ->Unit(benchmark::kMicrosecond);

// ============================================================================
// Message Handling Benchmarks
// ============================================================================

BENCHMARK_DEFINE_F(GossipConfigBenchmarkFixture, MessageHandling)(benchmark::State& state) {
    // Benchmark: Measure time to handle incoming gossip messages
    
    int message_count = 0;
    
    for (auto _ : state) {
        // Create a config update message
        ConfigUpdate update;
        update.update_id = "msg-" + std::to_string(message_count++);
        update.config_key = "msg.test.key";
        update.config_value = "msg_value";
        update.timestamp_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
        update.originator_shard_id = "shard-5";
        update.ttl = 10;
        update.vector_clock.set("shard-5", message_count);
        
        proto::GossipMessage msg;
        msg.set_sender_shard_id("shard-5");
        msg.set_timestamp_ns(update.timestamp_ns);
        msg.set_message_type("config_update");
        *msg.mutable_config_update() = update.toProto();
        *msg.mutable_vector_clock() = update.vector_clock.toProto();
        
        auto start = std::chrono::high_resolution_clock::now();
        
        auto response = manager_->handleGossipMessage(msg);
        
        auto end = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        
        state.SetIterationTime(elapsed.count() / 1e9);
        
        benchmark::DoNotOptimize(response);
    }
    
    state.SetLabel("Message handling time");
}

BENCHMARK_REGISTER_F(GossipConfigBenchmarkFixture, MessageHandling)
    ->Arg(10)
    ->Arg(50)
    ->Arg(100)
    ->UseManualTime()
    ->Unit(benchmark::kMicrosecond);

// ============================================================================
// Serialization Benchmarks
// ============================================================================

static void BM_ConfigUpdateSerialization(benchmark::State& state) {
    ConfigUpdate update;
    update.update_id = "bench-update-123";
    update.config_key = "bench.config.key";
    update.config_value = "benchmark_value_with_some_data";
    update.timestamp_ns = 1234567890ULL;
    update.originator_shard_id = "shard-1";
    update.ttl = 10;
    update.vector_clock.set("shard-1", 5);
    update.vector_clock.set("shard-2", 3);
    update.vector_clock.set("shard-3", 7);
    
    for (auto _ : state) {
        auto proto = update.toProto();
        auto deserialized = ConfigUpdate::fromProto(proto);
        benchmark::DoNotOptimize(deserialized);
    }
    
    state.SetLabel("ConfigUpdate proto serialization");
}

BENCHMARK(BM_ConfigUpdateSerialization)->Unit(benchmark::kNanosecond);

static void BM_ResourceSnapshotSerialization(benchmark::State& state) {
    ResourceSnapshot snapshot;
    snapshot.shard_id = "shard-1";
    snapshot.timestamp_ns = 9876543210ULL;
    snapshot.available_memory_bytes = 1024 * 1024 * 1024;
    snapshot.total_memory_bytes = 4ULL * 1024 * 1024 * 1024;
    snapshot.available_cpu_cores = 4;
    snapshot.total_cpu_cores = 8;
    snapshot.cpu_usage_percent = 45.5;
    snapshot.memory_usage_percent = 75.0;
    snapshot.is_healthy = true;
    snapshot.status = "healthy";
    snapshot.warnings.push_back("High CPU usage");
    snapshot.warnings.push_back("Disk approaching capacity");
    
    for (auto _ : state) {
        auto proto = snapshot.toProto();
        auto deserialized = ResourceSnapshot::fromProto(proto);
        benchmark::DoNotOptimize(deserialized);
    }
    
    state.SetLabel("ResourceSnapshot proto serialization");
}

BENCHMARK(BM_ResourceSnapshotSerialization)->Unit(benchmark::kNanosecond);

// ============================================================================
// Scalability Benchmarks
// ============================================================================

BENCHMARK_DEFINE_F(GossipConfigBenchmarkFixture, ScalabilityTest)(benchmark::State& state) {
    // Benchmark: Test performance with varying number of shards
    int num_updates = state.range(0);
    
    for (auto _ : state) {
        auto start = std::chrono::high_resolution_clock::now();
        
        // Publish multiple config updates
        for (int i = 0; i < num_updates; ++i) {
            std::string key = "scale.test.key." + std::to_string(i);
            std::string value = "value_" + std::to_string(i);
            manager_->publishConfigUpdate(key, value);
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        state.SetIterationTime(elapsed.count() / 1e6);
    }
    
    state.SetLabel("Scalability: updates=" + std::to_string(state.range(0)));
    state.counters["updates_per_sec"] = benchmark::Counter(
        state.range(0) * state.iterations(),
        benchmark::Counter::kIsRate
    );
}

BENCHMARK_REGISTER_F(GossipConfigBenchmarkFixture, ScalabilityTest)
    ->Arg(10)
    ->Arg(100)
    ->Arg(1000)
    ->Arg(10000)
    ->UseManualTime()
    ->Unit(benchmark::kMillisecond);

// ============================================================================
// Resource Snapshot Benchmarks
// ============================================================================

BENCHMARK_DEFINE_F(GossipConfigBenchmarkFixture, ResourceSnapshotPublish)(benchmark::State& state) {
    // Benchmark: Measure time to publish resource snapshots
    
    int snapshot_count = 0;
    
    for (auto _ : state) {
        ResourceSnapshot snapshot;
        snapshot.shard_id = "shard-0";
        snapshot.timestamp_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
        snapshot.available_memory_bytes = 1024 * 1024 * 1024 - snapshot_count;
        snapshot.total_memory_bytes = 4ULL * 1024 * 1024 * 1024;
        snapshot.cpu_usage_percent = 50.0 + (snapshot_count % 50);
        snapshot.is_healthy = true;
        
        auto start = std::chrono::high_resolution_clock::now();
        
        manager_->publishResourceSnapshot(snapshot);
        
        auto end = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        
        state.SetIterationTime(elapsed.count() / 1e9);
        
        snapshot_count++;
        benchmark::DoNotOptimize(snapshot);
    }
    
    state.SetLabel("Resource snapshot publish time");
}

BENCHMARK_REGISTER_F(GossipConfigBenchmarkFixture, ResourceSnapshotPublish)
    ->Arg(10)
    ->Arg(50)
    ->Arg(100)
    ->UseManualTime()
    ->Unit(benchmark::kMicrosecond);

// ============================================================================
// Main
// ============================================================================

BENCHMARK_MAIN();
