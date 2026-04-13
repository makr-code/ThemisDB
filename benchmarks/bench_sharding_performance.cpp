/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            bench_sharding_performance.cpp                     ║
  Version:         0.0.38                                             ║
  Last Modified:   2026-04-13 04:12:12                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   83.0/100                                       ║
    • Total Lines:     790                                            ║
    • Open Issues:     TODOs: 1, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • a629043ab2  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// Benchmark: Comprehensive Sharding Performance
// Measures scatter-gather latency, cross-shard joins, rebalancing, and P2P gossip overhead
//
// Created: December 2025
// Purpose: TODO-BENCH-001 - Complete sharding performance benchmarks

#include "sharding/shard_router.h"
#include "sharding/shard_topology.h"
#include "sharding/consistent_hash.h"
#include "sharding/urn_resolver.h"
#include "sharding/remote_executor.h"
#include "sharding/data_migrator.h"
#include "sharding/gossip_protocol.h"
#include "sharding/cloud_agent.h"
#include "sharding/urn.h"
#include <benchmark/benchmark.h>
#include <memory>
#include <random>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <atomic>
#include <mutex>

using namespace themis::sharding;

// ============================================================================
// Helper Functions
// ============================================================================

namespace {

std::string generateUUID(std::mt19937& rng) {
    auto to_hex = [](uint32_t x) {
        static const char* hex = "0123456789abcdef";
        std::string s(8, '0');
        for (int i = 7; i >= 0; --i) { s[i] = hex[x & 0xF]; x >>= 4; }
        return s;
    };
    std::uniform_int_distribution<uint32_t> dist(0, UINT32_MAX);
    uint32_t a = dist(rng);
    uint32_t b = dist(rng);
    uint32_t c = dist(rng);
    uint32_t d = dist(rng);
    return to_hex(a).substr(0,8) + "-" + to_hex(b).substr(0,4) + "-4" + 
           to_hex(c).substr(0,3) + "-" + to_hex(d).substr(0,4) + "-" + 
           to_hex(a^b^c^d).substr(0,12);
}

} // anonymous namespace

// ============================================================================
// Scatter-Gather Benchmark Fixture
// ============================================================================

class ScatterGatherFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& state) override {
        num_shards_ = state.range(0);
        
        // Create consistent hash ring and topology
        hash_ring_ = std::make_shared<ConsistentHashRing>();
        ShardTopology::Config topo_cfg{
            /*metadata_endpoint=*/"",
            /*cluster_name=*/"bench_scatter",
            /*refresh_interval_sec=*/0,
            /*enable_health_checks=*/false
        };
        topology_ = std::make_shared<ShardTopology>(topo_cfg);

        // Add shards with different datacenters
        for (int i = 0; i < num_shards_; i++) {
            std::string shard_id = "shard_" + std::to_string(i);
            hash_ring_->addShard(shard_id, 150);

            ShardInfo info;
            info.shard_id = shard_id;
            info.primary_endpoint = "http://localhost:" + std::to_string(8080 + i);
            info.datacenter = "dc" + std::to_string(i % 3); // 3 datacenters
            info.rack = "rack" + std::to_string(i % 5);
            info.token_start = 0;
            info.token_end = 0;
            info.is_healthy = true;
            info.capabilities = {"read", "write"};
            topology_->addShard(info);
        }

        local_shard_id_ = "shard_0";
        resolver_ = std::make_shared<URNResolver>(topology_, hash_ring_, local_shard_id_);

        RemoteExecutor::Config rexec_cfg{};
        rexec_cfg.local_shard_id = local_shard_id_;
        executor_ = std::make_shared<RemoteExecutor>(rexec_cfg);

        ShardRouter::Config config;
        config.local_shard_id = local_shard_id_;
        config.scatter_timeout_ms = 5000;
        config.max_concurrent_shards = 16;
        router_ = std::make_unique<ShardRouter>(resolver_, executor_, config);
        
        // Pre-generate URNs
        std::mt19937 rng(42);
        for (int i = 0; i < 10000; i++) {
            URN urn{"document", "bench", "entities", generateUUID(rng)};
            test_urns_.push_back(urn);
        }
    }
    
    void TearDown(const ::benchmark::State&) override {
        router_.reset();
        executor_.reset();
        resolver_.reset();
        hash_ring_.reset();
        topology_.reset();
        test_urns_.clear();
    }
    
protected:
    int num_shards_;
    std::shared_ptr<ConsistentHashRing> hash_ring_;
    std::shared_ptr<ShardTopology> topology_;
    std::shared_ptr<URNResolver> resolver_;
    std::shared_ptr<RemoteExecutor> executor_;
    std::unique_ptr<ShardRouter> router_;
    std::vector<URN> test_urns_;
    std::string local_shard_id_;
};

// ============================================================================
// Benchmark: Scatter-Gather Latency
// ============================================================================

BENCHMARK_DEFINE_F(ScatterGatherFixture, ScatterGatherLatency)(benchmark::State& state) {
    const int query_complexity = state.range(1); // 1=simple, 2=medium, 3=complex
    size_t urn_index = 0;
    
    for (auto _ : state) {
        // Simulate scatter-gather query that touches multiple shards
        std::vector<URN> query_urns;
        const int batch_size = query_complexity * 10;
        
        for (int i = 0; i < batch_size; i++) {
            query_urns.push_back(test_urns_[(urn_index + i) % test_urns_.size()]);
        }
        
        // Route each URN (simulates scatter)
        std::vector<std::string> target_shards;
        for (const auto& urn : query_urns) {
            auto result = resolver_->resolvePrimary(urn);
            if (result.has_value()) {
                const auto& shard_info = result.value();
                target_shards.push_back(shard_info.shard_id);
            }
        }
        
        benchmark::DoNotOptimize(target_shards);
        urn_index += batch_size;
    }
    
    state.SetItemsProcessed(state.iterations());
    state.counters["shards"] = num_shards_;
    state.counters["complexity"] = query_complexity;
    state.counters["ops_per_sec"] = benchmark::Counter(
        state.iterations(), benchmark::Counter::kIsRate);
}

BENCHMARK_REGISTER_F(ScatterGatherFixture, ScatterGatherLatency)
    ->Args({10, 1})    // 10 shards, simple query
    ->Args({10, 2})    // 10 shards, medium query
    ->Args({10, 3})    // 10 shards, complex query
    ->Args({50, 1})    // 50 shards, simple
    ->Args({50, 2})    // 50 shards, medium
    ->Args({100, 1})   // 100 shards, simple
    ->Args({100, 2})   // 100 shards, medium
    ->Unit(benchmark::kMicrosecond);

// ============================================================================
// Benchmark: Cross-Shard Join Performance
// ============================================================================

class CrossShardJoinFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& state) override {
        num_shards_ = state.range(0);
        left_table_size_ = state.range(1);
        right_table_size_ = state.range(2);
        
        // Setup topology
        hash_ring_ = std::make_shared<ConsistentHashRing>();
        ShardTopology::Config topo_cfg{"", "bench_join", 0, false};
        topology_ = std::make_shared<ShardTopology>(topo_cfg);

        for (int i = 0; i < num_shards_; i++) {
            std::string shard_id = "shard_" + std::to_string(i);
            hash_ring_->addShard(shard_id, 150);

            ShardInfo info;
            info.shard_id = shard_id;
            info.primary_endpoint = "http://localhost:" + std::to_string(8080 + i);
            info.datacenter = "dc1";
            info.is_healthy = true;
            topology_->addShard(info);
        }
        
        // Generate test data for join
        std::mt19937 rng(42);
        
        for (int i = 0; i < left_table_size_; i++) {
            left_keys_.push_back("key_" + std::to_string(i));
            left_values_.push_back(rng() % 10000);
        }
        
        for (int i = 0; i < right_table_size_; i++) {
            // 50% overlap with left table
            int key_id = (i < right_table_size_ / 2) ? i : (rng() % left_table_size_);
            right_keys_.push_back("key_" + std::to_string(key_id));
            right_values_.push_back(rng() % 10000);
        }
    }
    
    void TearDown(const ::benchmark::State&) override {
        hash_ring_.reset();
        topology_.reset();
        left_keys_.clear();
        left_values_.clear();
        right_keys_.clear();
        right_values_.clear();
    }
    
protected:
    int num_shards_;
    int left_table_size_;
    int right_table_size_;
    std::shared_ptr<ConsistentHashRing> hash_ring_;
    std::shared_ptr<ShardTopology> topology_;
    std::vector<std::string> left_keys_;
    std::vector<int> left_values_;
    std::vector<std::string> right_keys_;
    std::vector<int> right_values_;
};

BENCHMARK_DEFINE_F(CrossShardJoinFixture, BroadcastHashJoin)(benchmark::State& state) {
    for (auto _ : state) {
        // Build hash table from smaller (left) table
        std::unordered_map<std::string, int> hash_table;
        hash_table.reserve(left_keys_.size());
        
        for (size_t i = 0; i < left_keys_.size(); i++) {
            hash_table[left_keys_[i]] = left_values_[i];
        }
        
        // Probe with right table
        std::vector<std::pair<int, int>> join_results;
        join_results.reserve(right_keys_.size() / 2); // Estimate
        
        for (size_t i = 0; i < right_keys_.size(); i++) {
            auto it = hash_table.find(right_keys_[i]);
            if (it != hash_table.end()) {
                join_results.emplace_back(it->second, right_values_[i]);
            }
        }
        
        benchmark::DoNotOptimize(join_results);
    }
    
    state.SetItemsProcessed(state.iterations() * (left_table_size_ + right_table_size_));
    state.counters["left_size"] = left_table_size_;
    state.counters["right_size"] = right_table_size_;
    state.counters["shards"] = num_shards_;
}

BENCHMARK_REGISTER_F(CrossShardJoinFixture, BroadcastHashJoin)
    ->Args({10, 1000, 1000})      // 10 shards, 1K x 1K
    ->Args({10, 10000, 10000})    // 10 shards, 10K x 10K
    ->Args({10, 100000, 100000})  // 10 shards, 100K x 100K
    ->Args({50, 10000, 10000})    // 50 shards, 10K x 10K
    ->Args({100, 10000, 10000})   // 100 shards, 10K x 10K
    ->Unit(benchmark::kMillisecond);

BENCHMARK_DEFINE_F(CrossShardJoinFixture, CoLocatedJoinSimulation)(benchmark::State& state) {
    // Simulates co-located join where data is partitioned by join key
    for (auto _ : state) {
        // Partition data by shard
        std::vector<std::vector<std::pair<std::string, int>>> left_partitions(num_shards_);
        std::vector<std::vector<std::pair<std::string, int>>> right_partitions(num_shards_);
        
        // Partition left table
        for (size_t i = 0; i < left_keys_.size(); i++) {
            size_t hash = std::hash<std::string>{}(left_keys_[i]);
            int shard = hash % num_shards_;
            left_partitions[shard].emplace_back(left_keys_[i], left_values_[i]);
        }
        
        // Partition right table
        for (size_t i = 0; i < right_keys_.size(); i++) {
            size_t hash = std::hash<std::string>{}(right_keys_[i]);
            int shard = hash % num_shards_;
            right_partitions[shard].emplace_back(right_keys_[i], right_values_[i]);
        }
        
        // Local joins per shard
        std::vector<std::pair<int, int>> all_results;
        
        for (int s = 0; s < num_shards_; s++) {
            std::unordered_map<std::string, int> local_hash;
            for (const auto& [key, val] : left_partitions[s]) {
                local_hash[key] = val;
            }
            
            for (const auto& [key, val] : right_partitions[s]) {
                auto it = local_hash.find(key);
                if (it != local_hash.end()) {
                    all_results.emplace_back(it->second, val);
                }
            }
        }
        
        benchmark::DoNotOptimize(all_results);
    }
    
    state.SetItemsProcessed(state.iterations() * (left_table_size_ + right_table_size_));
    state.counters["left_size"] = left_table_size_;
    state.counters["right_size"] = right_table_size_;
    state.counters["shards"] = num_shards_;
}

BENCHMARK_REGISTER_F(CrossShardJoinFixture, CoLocatedJoinSimulation)
    ->Args({10, 10000, 10000})
    ->Args({50, 10000, 10000})
    ->Args({100, 10000, 10000})
    ->Unit(benchmark::kMillisecond);

// ============================================================================
// Benchmark: Rebalancing Throughput
// ============================================================================

class RebalancingFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& state) override {
        num_entities_ = state.range(0);
        batch_size_ = state.range(1);
        
        // Generate test entities
        std::mt19937 rng(42);
        for (int i = 0; i < num_entities_; i++) {
            std::string entity_data = R"({"id":")" + std::to_string(i) + 
                R"(","name":"Entity_)" + std::to_string(i) + 
                R"(","value":)" + std::to_string(rng() % 10000) + 
                R"(,"metadata":{"created":"2025-12-01","tags":["bench","test"]}})";
            entities_.push_back(entity_data);
        }
    }
    
    void TearDown(const ::benchmark::State&) override {
        entities_.clear();
    }
    
protected:
    int num_entities_;
    int batch_size_;
    std::vector<std::string> entities_;
};

BENCHMARK_DEFINE_F(RebalancingFixture, BatchSerializationThroughput)(benchmark::State& state) {
    for (auto _ : state) {
        // Simulate batch serialization for migration
        std::string batch_payload = "[";
        
        size_t max_items = std::min(static_cast<size_t>(batch_size_), entities_.size());
        for (size_t i = 0; i < max_items; i++) {
            if (i > 0) batch_payload += ",";
            batch_payload += entities_[i];
        }
        batch_payload += "]";
        
        benchmark::DoNotOptimize(batch_payload);
        benchmark::ClobberMemory();
    }
    
    state.SetBytesProcessed(state.iterations() * batch_size_ * 200); // ~200 bytes per entity
    state.counters["batch_size"] = batch_size_;
    state.counters["entities_per_sec"] = benchmark::Counter(
        state.iterations() * batch_size_, benchmark::Counter::kIsRate);
}

BENCHMARK_REGISTER_F(RebalancingFixture, BatchSerializationThroughput)
    ->Args({10000, 100})    // 10K entities, batch 100
    ->Args({10000, 500})    // 10K entities, batch 500
    ->Args({10000, 1000})   // 10K entities, batch 1000
    ->Args({100000, 500})   // 100K entities, batch 500
    ->Args({100000, 1000})  // 100K entities, batch 1000
    ->Unit(benchmark::kMicrosecond);

BENCHMARK_DEFINE_F(RebalancingFixture, BatchDeserializationThroughput)(benchmark::State& state) {
    // Pre-create batch payload
    std::string batch_payload = "[";
    size_t max_items = std::min(static_cast<size_t>(batch_size_), entities_.size());
    for (size_t i = 0; i < max_items; i++) {
        if (i > 0) batch_payload += ",";
        batch_payload += entities_[i];
    }
    batch_payload += "]";
    
    for (auto _ : state) {
        // Simple parse simulation (count entities)
        int entity_count = 0;
        int depth = 0;
        
        for (char c : batch_payload) {
            if (c == '{') {
                if (depth == 1) entity_count++;
                depth++;
            } else if (c == '}') {
                depth--;
            }
        }
        
        benchmark::DoNotOptimize(entity_count);
    }
    
    state.SetBytesProcessed(state.iterations() * batch_payload.size());
    state.counters["batch_size"] = batch_size_;
}

BENCHMARK_REGISTER_F(RebalancingFixture, BatchDeserializationThroughput)
    ->Args({10000, 100})
    ->Args({10000, 500})
    ->Args({10000, 1000})
    ->Unit(benchmark::kMicrosecond);

// ============================================================================
// Benchmark: P2P Gossip Overhead
// ============================================================================

class GossipOverheadFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& state) override {
        num_peers_ = state.range(0);
        
        // Generate peer list
        for (int i = 0; i < num_peers_; i++) {
            peers_.push_back("peer_" + std::to_string(i) + ".example.com:8080");
        }
        
        // Generate gossip message
        gossip_message_ = R"({"type":"heartbeat","sender":"peer_0","timestamp":)" + 
            std::to_string(std::chrono::system_clock::now().time_since_epoch().count()) +
            R"(,"peers":[)";
        
        for (int i = 0; i < num_peers_; i++) {
            if (i > 0) gossip_message_ += ",";
            gossip_message_ += R"({"id":"peer_)" + std::to_string(i) + 
                R"(","endpoint":")" + peers_[i] + 
                R"(","dc":"dc)" + std::to_string(i % 3) + 
                R"(","healthy":true,"version":)" + std::to_string(i * 100) + "}";
        }
        gossip_message_ += "]}";
    }
    
    void TearDown(const ::benchmark::State&) override {
        peers_.clear();
        gossip_message_.clear();
    }
    
protected:
    int num_peers_;
    std::vector<std::string> peers_;
    std::string gossip_message_;
};

BENCHMARK_DEFINE_F(GossipOverheadFixture, MessageSerialization)(benchmark::State& state) {
    for (auto _ : state) {
        // Simulate gossip message creation
        std::string msg = R"({"type":"heartbeat","sender":"peer_0","timestamp":)" + 
            std::to_string(std::chrono::system_clock::now().time_since_epoch().count()) +
            R"(,"peer_count":)" + std::to_string(num_peers_) + "}";
        
        benchmark::DoNotOptimize(msg);
    }
    
    state.SetItemsProcessed(state.iterations());
    state.counters["peers"] = num_peers_;
    state.counters["msg_size"] = gossip_message_.size();
}

BENCHMARK_REGISTER_F(GossipOverheadFixture, MessageSerialization)
    ->Arg(10)
    ->Arg(50)
    ->Arg(100)
    ->Arg(500)
    ->Unit(benchmark::kNanosecond);

BENCHMARK_DEFINE_F(GossipOverheadFixture, FanoutSelection)(benchmark::State& state) {
    const int fanout = 3; // Typical SWIM fanout
    std::mt19937 rng(42);
    
    for (auto _ : state) {
        // Select random peers for fanout
        std::vector<int> selected;
        selected.reserve(fanout);
        
        std::uniform_int_distribution<int> dist(0, num_peers_ - 1);
        
        while (selected.size() < static_cast<size_t>(fanout) && 
               selected.size() < static_cast<size_t>(num_peers_)) {
            int idx = dist(rng);
            
            // Check for duplicates (simple linear scan for small fanout)
            bool duplicate = false;
            for (int s : selected) {
                if (s == idx) {
                    duplicate = true;
                    break;
                }
            }
            
            if (!duplicate) {
                selected.push_back(idx);
            }
        }
        
        benchmark::DoNotOptimize(selected);
    }
    
    state.SetItemsProcessed(state.iterations());
    state.counters["peers"] = num_peers_;
    state.counters["fanout"] = fanout;
}

BENCHMARK_REGISTER_F(GossipOverheadFixture, FanoutSelection)
    ->Arg(10)
    ->Arg(50)
    ->Arg(100)
    ->Arg(500)
    ->Unit(benchmark::kNanosecond);

BENCHMARK_DEFINE_F(GossipOverheadFixture, VersionVectorMerge)(benchmark::State& state) {
    // Create version vectors
    std::map<std::string, uint64_t> local_version;
    std::map<std::string, uint64_t> remote_version;
    
    std::mt19937 rng(42);
    for (int i = 0; i < num_peers_; i++) {
        std::string peer = "peer_" + std::to_string(i);
        local_version[peer] = rng() % 10000;
        remote_version[peer] = rng() % 10000;
    }
    
    for (auto _ : state) {
        // Merge version vectors (take max)
        std::map<std::string, uint64_t> merged;
        
        for (const auto& [peer, ver] : local_version) {
            merged[peer] = ver;
        }
        
        for (const auto& [peer, ver] : remote_version) {
            auto it = merged.find(peer);
            if (it == merged.end() || ver > it->second) {
                merged[peer] = ver;
            }
        }
        
        benchmark::DoNotOptimize(merged);
    }
    
    state.SetItemsProcessed(state.iterations() * num_peers_ * 2);
    state.counters["peers"] = num_peers_;
}

BENCHMARK_REGISTER_F(GossipOverheadFixture, VersionVectorMerge)
    ->Arg(10)
    ->Arg(50)
    ->Arg(100)
    ->Arg(500)
    ->Unit(benchmark::kMicrosecond);

// ============================================================================
// Benchmark: Multi-DC Routing Overhead
// ============================================================================

class MultiDCRoutingFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& state) override {
        num_dcs_ = state.range(0);
        shards_per_dc_ = state.range(1);
        
        hash_ring_ = std::make_shared<ConsistentHashRing>();
        ShardTopology::Config topo_cfg{"", "bench_multidc", 0, false};
        topology_ = std::make_shared<ShardTopology>(topo_cfg);

        int shard_idx = 0;
        for (int dc = 0; dc < num_dcs_; dc++) {
            for (int s = 0; s < shards_per_dc_; s++) {
                std::string shard_id = "shard_dc" + std::to_string(dc) + "_" + std::to_string(s);
                hash_ring_->addShard(shard_id, 150);

                ShardInfo info;
                info.shard_id = shard_id;
                info.primary_endpoint = "http://dc" + std::to_string(dc) + "-node" + std::to_string(s) + ":8080";
                info.datacenter = "dc" + std::to_string(dc);
                info.rack = "rack" + std::to_string(s % 3);
                info.is_healthy = true;
                info.capabilities = {"read", "write"};
                topology_->addShard(info);
                
                shard_idx++;
            }
        }
        
        local_dc_ = "dc0";
        
        // Generate URNs
        std::mt19937 rng(42);
        for (int i = 0; i < 10000; i++) {
            URN urn{"document", "multidc", "data", generateUUID(rng)};
            test_urns_.push_back(urn);
        }
    }
    
    void TearDown(const ::benchmark::State&) override {
        hash_ring_.reset();
        topology_.reset();
        test_urns_.clear();
    }
    
protected:
    int num_dcs_;
    int shards_per_dc_;
    std::string local_dc_;
    std::shared_ptr<ConsistentHashRing> hash_ring_;
    std::shared_ptr<ShardTopology> topology_;
    std::vector<URN> test_urns_;
};

BENCHMARK_DEFINE_F(MultiDCRoutingFixture, DCProximityRouting)(benchmark::State& state) {
    size_t urn_index = 0;
    
    for (auto _ : state) {
        const URN& urn = test_urns_[urn_index % test_urns_.size()];
        
        // Get shard for URN
        auto shard_id = hash_ring_->getShardForURN(urn);
        
        // Get shard info
        auto shard_info = topology_->getShard(shard_id);
        
        // Check if local DC
        bool is_local_dc = shard_info && shard_info->datacenter == local_dc_;
        
        benchmark::DoNotOptimize(is_local_dc);
        urn_index++;
    }
    
    state.SetItemsProcessed(state.iterations());
    state.counters["dcs"] = num_dcs_;
    state.counters["shards_per_dc"] = shards_per_dc_;
    state.counters["total_shards"] = num_dcs_ * shards_per_dc_;
}

BENCHMARK_REGISTER_F(MultiDCRoutingFixture, DCProximityRouting)
    ->Args({3, 10})     // 3 DCs, 10 shards each (30 total)
    ->Args({3, 50})     // 3 DCs, 50 shards each (150 total)
    ->Args({5, 20})     // 5 DCs, 20 shards each (100 total)
    ->Args({10, 10})    // 10 DCs, 10 shards each (100 total)
    ->Unit(benchmark::kNanosecond);

BENCHMARK_DEFINE_F(MultiDCRoutingFixture, CrossDCLatencySimulation)(benchmark::State& state) {
    // Simulate cross-DC latency overhead
    const std::map<std::string, int> dc_latencies = {
        {"dc0", 1},    // Local DC - 1ms
        {"dc1", 50},   // Same region - 50ms
        {"dc2", 100},  // Different region - 100ms
    };
    
    size_t urn_index = 0;
    int total_latency = 0;
    int request_count = 0;
    
    for (auto _ : state) {
        const URN& urn = test_urns_[urn_index % test_urns_.size()];
        
        auto shard_id = hash_ring_->getShardForURN(urn);
        auto shard_info = topology_->getShard(shard_id);
        
        if (shard_info) {
            auto it = dc_latencies.find(shard_info->datacenter);
            int latency = (it != dc_latencies.end()) ? it->second : 100;
            total_latency += latency;
            request_count++;
        }
        
        urn_index++;
    }
    
    state.counters["avg_latency_ms"] = (request_count > 0) ? 
        static_cast<double>(total_latency) / request_count : 0;
    state.counters["dcs"] = num_dcs_;
}

BENCHMARK_REGISTER_F(MultiDCRoutingFixture, CrossDCLatencySimulation)
    ->Args({3, 10})
    ->Args({3, 50})
    ->Args({5, 20})
    ->Unit(benchmark::kNanosecond);

// ============================================================================
// Benchmark: Concurrent Shard Operations
// ============================================================================

static void BM_ConcurrentShardAccess(benchmark::State& state) {
    const int num_shards = 100;
    const int num_threads = state.range(0);
    
    auto hash_ring = std::make_shared<ConsistentHashRing>();
    for (int i = 0; i < num_shards; i++) {
        hash_ring->addShard("shard_" + std::to_string(i), 150);
    }
    
    // Generate URNs per thread
    std::vector<std::vector<URN>> thread_urns(num_threads);
    std::mt19937 rng(42);
    
    for (int t = 0; t < num_threads; t++) {
        for (int i = 0; i < 1000; i++) {
            thread_urns[t].emplace_back("document", "bench", "concurrent", generateUUID(rng));
        }
    }
    
    std::atomic<int64_t> total_ops{0};
    
    for (auto _ : state) {
        std::vector<std::thread> threads;
        threads.reserve(num_threads);
        
        for (int t = 0; t < num_threads; t++) {
            threads.emplace_back([&, t]() {
                for (const auto& urn : thread_urns[t]) {
                    auto shard = hash_ring->getShardForURN(urn);
                    benchmark::DoNotOptimize(shard);
                }
                total_ops.fetch_add(thread_urns[t].size(), std::memory_order_relaxed);
            });
        }
        
        for (auto& thread : threads) {
            thread.join();
        }
    }
    
    state.SetItemsProcessed(total_ops.load());
    state.counters["threads"] = num_threads;
    state.counters["shards"] = num_shards;
    state.counters["ops_per_thread"] = 1000;
}

BENCHMARK(BM_ConcurrentShardAccess)
    ->Arg(1)
    ->Arg(2)
    ->Arg(4)
    ->Arg(8)
    ->Arg(16)
    ->Unit(benchmark::kMillisecond)
    ->UseRealTime();

// benchmark_main is linked via CMake
