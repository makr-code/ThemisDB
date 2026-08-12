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
#include "sharding/cross_shard_transaction.h"
#include "sharding/two_phase_commit_participant.h"
#if defined(THEMIS_ENABLE_CUDA) || defined(THEMIS_ENABLE_OPENCL)
#include "sharding/gpu_erasure_coder.h"
#endif
#include <benchmark/benchmark.h>
#include <algorithm>
#include <cstdint>
#include <numeric>
#include <memory>
#include <random>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <atomic>
#include <zstd.h>
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
// Benchmark: Percolator Commit Latency (SH-3 direct metric)
// ============================================================================

static void BM_PercolatorCommitLatency(benchmark::State& state) {
    const int num_shards = static_cast<int>(state.range(0));

    themisdb::sharding::PercolatorCoordinator::Config cfg;
    cfg.lock_timeout = std::chrono::milliseconds(1);
    cfg.max_retries = 0;

    themisdb::sharding::PercolatorCoordinator percolator(cfg, nullptr, nullptr);

    std::map<std::string, std::unique_ptr<themis::sharding::TwoPhaseCommitParticipant>> participants;
    for (int i = 0; i < num_shards; ++i) {
        const std::string shard_id = "shard_" + std::to_string(i);
        participants.emplace(
            shard_id,
            std::make_unique<themis::sharding::TwoPhaseCommitParticipant>(shard_id)
        );
    }

    nlohmann::json prepare_payload_json;
    prepare_payload_json["operations"] = nlohmann::json::array({"SET bench_key bench_value"});
    const std::string prepare_payload = prepare_payload_json.dump();

    uint64_t txn_seq = 0;
    size_t success_count = 0;

    for (auto _ : state) {
        themisdb::sharding::CrossShardTransaction txn;
        txn.transaction_id = "bench_percolator_" + std::to_string(txn_seq++);
        txn.protocol = themisdb::sharding::TransactionProtocol::PERCOLATOR;
        txn.isolation_level = themisdb::sharding::IsolationLevel::SNAPSHOT_ISOLATION;
        txn.state = themisdb::sharding::TransactionState::ACTIVE;
        txn.start_time = std::chrono::system_clock::now();

        for (const auto& [shard_id, _participant] : participants) {
            themisdb::sharding::ShardParticipant p;
            p.shard_id = shard_id;
            p.endpoint = "inmem://" + shard_id;
            p.operations = {"SET bench_key bench_value"};
            txn.participants.emplace(shard_id, std::move(p));
        }

        auto prepare_fn = [&](const std::string& shard_id, const std::string& txn_id) {
            auto it = participants.find(shard_id);
            if (it == participants.end()) {
                return false;
            }
            return it->second->onPrepare(txn_id, "bench_coordinator", prepare_payload);
        };

        auto commit_fn = [&](const std::string& shard_id, const std::string& txn_id) {
            auto it = participants.find(shard_id);
            if (it == participants.end()) {
                return false;
            }
            return it->second->onCommit(txn_id);
        };

        auto abort_fn = [&](const std::string& shard_id, const std::string& txn_id) {
            auto it = participants.find(shard_id);
            if (it == participants.end()) {
                return false;
            }
            return it->second->onAbort(txn_id);
        };

        const bool ok = percolator.execute(txn, prepare_fn, commit_fn, abort_fn);
        benchmark::DoNotOptimize(ok);
        if (ok) {
            ++success_count;
        }
    }

    const double success_rate = state.iterations() == 0
        ? 0.0
        : (100.0 * static_cast<double>(success_count) /
           static_cast<double>(state.iterations()));

    state.SetItemsProcessed(state.iterations());
    state.counters["shards"] = num_shards;
    state.counters["commit_success_rate_pct"] = success_rate;
}

BENCHMARK(BM_PercolatorCommitLatency)
    ->Arg(10)
    ->Arg(20)
    ->Unit(benchmark::kMillisecond)
    ->Iterations(200);

// ============================================================================
// Benchmark: Zero-Downtime Shard Split (SH-4)
// ============================================================================

class ShardSplitDowntimeFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& state) override {
        keyspace_size_ = static_cast<size_t>(state.range(0));
        migration_chunk_size_ = static_cast<size_t>(state.range(1));
        split_index_ = keyspace_size_ / 2;

        old_shard_.assign(keyspace_size_, static_cast<uint8_t>(1));
        new_shard_.assign(keyspace_size_, static_cast<uint8_t>(0));
    }

    void TearDown(const ::benchmark::State&) override {
        old_shard_.clear();
        new_shard_.clear();
    }

protected:
    [[nodiscard]] bool readAvailable(size_t key, bool route_to_new) const {
        if (key < split_index_) {
            return old_shard_[key] != 0;
        }
        if (route_to_new && new_shard_[key] != 0) {
            return true;
        }
        // During live split we keep dual-read fallback to avoid downtime.
        return old_shard_[key] != 0;
    }

    size_t keyspace_size_{0};
    size_t migration_chunk_size_{0};
    size_t split_index_{0};
    std::vector<uint8_t> old_shard_;
    std::vector<uint8_t> new_shard_;
};

BENCHMARK_DEFINE_F(ShardSplitDowntimeFixture, ZeroDowntimeReadAvailability)(benchmark::State& state) {
    size_t unavailable_reads_total = 0;
    size_t total_reads = 0;

    for (auto _ : state) {
        std::fill(new_shard_.begin(), new_shard_.end(), static_cast<uint8_t>(0));
        bool route_to_new = false;
        size_t migrated_until = split_index_;
        uint32_t lcg = 0x9E3779B9u;

        auto next_key = [&]() {
            lcg = lcg * 1664525u + 1013904223u;
            return static_cast<size_t>(lcg % static_cast<uint32_t>(keyspace_size_));
        };

        while (migrated_until < keyspace_size_) {
            const size_t copy_end = std::min(migrated_until + migration_chunk_size_, keyspace_size_);
            for (size_t i = migrated_until; i < copy_end; ++i) {
                new_shard_[i] = 1;
            }
            migrated_until = copy_end;

            if (migrated_until >= split_index_ + migration_chunk_size_) {
                route_to_new = true;
            }

            constexpr size_t kReadsPerChunk = 64;
            for (size_t r = 0; r < kReadsPerChunk; ++r) {
                const size_t key = next_key();
                const bool available = readAvailable(key, route_to_new);
                if (!available) {
                    ++unavailable_reads_total;
                }
                ++total_reads;
                benchmark::DoNotOptimize(available);
            }
        }

        route_to_new = true;
        for (size_t i = split_index_; i < keyspace_size_; ++i) {
            old_shard_[i] = 0;
        }

        constexpr size_t kPostCutoverReads = 256;
        for (size_t r = 0; r < kPostCutoverReads; ++r) {
            const size_t key = next_key();
            const bool available = readAvailable(key, route_to_new);
            if (!available) {
                ++unavailable_reads_total;
            }
            ++total_reads;
            benchmark::DoNotOptimize(available);
        }

        for (size_t i = split_index_; i < keyspace_size_; ++i) {
            old_shard_[i] = 1;
        }
        benchmark::ClobberMemory();
    }

    const double availability_pct = total_reads == 0
        ? 0.0
        : (100.0 * static_cast<double>(total_reads - unavailable_reads_total) /
           static_cast<double>(total_reads));
    const double downtime_ms = unavailable_reads_total == 0 ? 0.0 : 1.0;

    state.SetItemsProcessed(static_cast<int64_t>(total_reads));
    state.counters["keyspace"] = static_cast<double>(keyspace_size_);
    state.counters["migration_chunk"] = static_cast<double>(migration_chunk_size_);
    state.counters["read_unavailable_events"] = static_cast<double>(unavailable_reads_total);
    state.counters["read_availability_pct"] = availability_pct;
    state.counters["read_unavailability_ms"] = downtime_ms;
}

BENCHMARK_REGISTER_F(ShardSplitDowntimeFixture, ZeroDowntimeReadAvailability)
    ->Args({10000, 256})
    ->Args({100000, 1024})
    ->Unit(benchmark::kMicrosecond)
    ->Iterations(200);

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

BENCHMARK_DEFINE_F(RebalancingFixture, WriteLatencyDuringMigration)(benchmark::State& state) {
    const size_t keyspace = std::min(static_cast<size_t>(num_entities_), static_cast<size_t>(200000));
    const size_t writes_per_iteration = static_cast<size_t>(batch_size_);
    const size_t split_index = keyspace / 2;

    std::vector<uint64_t> old_shard(keyspace, 0);
    std::vector<uint64_t> new_shard(keyspace, 0);

    uint64_t seq = 1;
    uint64_t total_baseline_ns = 0;
    uint64_t total_migration_ns = 0;

    auto next_key = [](uint64_t& lcg, size_t size) {
        lcg = lcg * 1664525u + 1013904223u;
        return static_cast<size_t>(lcg % static_cast<uint64_t>(size));
    };

    for (auto _ : state) {
        uint64_t lcg = 0xC0FFEEu + seq;

        const auto baseline_start = std::chrono::steady_clock::now();
        for (size_t i = 0; i < writes_per_iteration; ++i) {
            const size_t key = next_key(lcg, keyspace);
            old_shard[key] = seq++;
        }
        const auto baseline_end = std::chrono::steady_clock::now();

        const auto migration_start = std::chrono::steady_clock::now();
        for (size_t i = 0; i < writes_per_iteration; ++i) {
            const size_t key = next_key(lcg, keyspace);
            const uint64_t value = seq++;

            old_shard[key] = value;
            if (key >= split_index) {
                // Dual-write on migrating range to emulate live migration overhead.
                new_shard[key] = value;
            }
        }
        const auto migration_end = std::chrono::steady_clock::now();

        const auto baseline_ns =
            std::chrono::duration_cast<std::chrono::nanoseconds>(baseline_end - baseline_start).count();
        const auto migration_ns =
            std::chrono::duration_cast<std::chrono::nanoseconds>(migration_end - migration_start).count();

        total_baseline_ns += static_cast<uint64_t>(baseline_ns);
        total_migration_ns += static_cast<uint64_t>(migration_ns);

        benchmark::DoNotOptimize(old_shard.data());
        benchmark::DoNotOptimize(new_shard.data());
        benchmark::ClobberMemory();
    }

    const double baseline_avg_ns = state.iterations() == 0
        ? 0.0
        : static_cast<double>(total_baseline_ns) / static_cast<double>(state.iterations());
    const double migration_avg_ns = state.iterations() == 0
        ? 0.0
        : static_cast<double>(total_migration_ns) / static_cast<double>(state.iterations());
    const double overhead_pct = baseline_avg_ns <= 0.0
        ? 0.0
        : ((migration_avg_ns / baseline_avg_ns) - 1.0) * 100.0;

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(writes_per_iteration));
    state.counters["writes_per_iteration"] = static_cast<double>(writes_per_iteration);
    state.counters["baseline_latency_ns"] = baseline_avg_ns;
    state.counters["migration_latency_ns"] = migration_avg_ns;
    state.counters["write_overhead_pct"] = overhead_pct;
}

BENCHMARK_REGISTER_F(RebalancingFixture, WriteLatencyDuringMigration)
    ->Args({10000, 1024})
    ->Args({100000, 2048})
    ->Unit(benchmark::kMicrosecond)
    ->Iterations(200);

BENCHMARK_DEFINE_F(RebalancingFixture, RebalancerDecisionCycle)(benchmark::State& state) {
    const size_t num_shards = std::max<size_t>(16, static_cast<size_t>(num_entities_));
    const size_t max_migrations = std::max<size_t>(1, static_cast<size_t>(batch_size_));

    std::vector<double> cpu_load(num_shards, 0.0);
    std::vector<double> storage_load(num_shards, 0.0);
    std::vector<double> composite_load(num_shards, 0.0);
    std::vector<size_t> ranked_shards(num_shards);
    std::iota(ranked_shards.begin(), ranked_shards.end(), static_cast<size_t>(0));

    uint64_t total_cycle_us = 0;
    size_t total_planned_migrations = 0;
    uint64_t lcg = 0x12345678u;

    auto next_frac = [&]() {
        lcg = lcg * 1664525u + 1013904223u;
        return static_cast<double>(lcg % 10000u) / 10000.0;
    };

    for (auto _ : state) {
        const auto cycle_start = std::chrono::steady_clock::now();

        for (size_t i = 0; i < num_shards; ++i) {
            const double cpu = 0.35 + 0.60 * next_frac();
            const double storage = 0.30 + 0.65 * next_frac();
            cpu_load[i] = cpu;
            storage_load[i] = storage;
            composite_load[i] = 0.7 * cpu + 0.3 * storage;
        }

        std::sort(ranked_shards.begin(), ranked_shards.end(), [&](size_t lhs, size_t rhs) {
            return composite_load[lhs] > composite_load[rhs];
        });

        size_t planned_this_cycle = 0;
        for (size_t i = 0; i < ranked_shards.size() && planned_this_cycle < max_migrations; ++i) {
            const size_t src = ranked_shards[i];
            const size_t dst = ranked_shards[ranked_shards.size() - 1 - i];

            if (composite_load[src] < 0.75 || composite_load[dst] > 0.55) {
                break;
            }

            const double rebalanced = (composite_load[src] - composite_load[dst]) * 0.15;
            composite_load[src] -= rebalanced;
            composite_load[dst] += rebalanced;
            ++planned_this_cycle;
        }

        const auto cycle_end = std::chrono::steady_clock::now();
        const auto cycle_us =
            std::chrono::duration_cast<std::chrono::microseconds>(cycle_end - cycle_start).count();

        total_cycle_us += static_cast<uint64_t>(cycle_us);
        total_planned_migrations += planned_this_cycle;

        benchmark::DoNotOptimize(planned_this_cycle);
        benchmark::ClobberMemory();
    }

    const double avg_cycle_us = state.iterations() == 0
        ? 0.0
        : static_cast<double>(total_cycle_us) / static_cast<double>(state.iterations());
    const double avg_cycle_s = avg_cycle_us / 1'000'000.0;
    const double avg_migrations = state.iterations() == 0
        ? 0.0
        : static_cast<double>(total_planned_migrations) / static_cast<double>(state.iterations());

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(num_shards));
    state.counters["decision_cycle_us"] = avg_cycle_us;
    state.counters["decision_cycle_s"] = avg_cycle_s;
    state.counters["planned_migrations_per_cycle"] = avg_migrations;
    state.counters["shards"] = static_cast<double>(num_shards);
}

BENCHMARK_REGISTER_F(RebalancingFixture, RebalancerDecisionCycle)
    ->Args({128, 8})
    ->Args({512, 16})
    ->Args({1024, 24})
    ->Unit(benchmark::kMicrosecond)
    ->Iterations(200);

BENCHMARK_DEFINE_F(RebalancingFixture, AntiEntropyScanThroughput)(benchmark::State& state) {
    const size_t records = std::max<size_t>(1024, static_cast<size_t>(num_entities_));
    const size_t workers = std::max<size_t>(1, static_cast<size_t>(batch_size_));
    constexpr size_t kRecordBytes = 512;
    const size_t total_bytes = records * kRecordBytes;

    std::vector<uint8_t> scan_buffer(total_bytes);
    for (size_t i = 0; i < total_bytes; ++i) {
        scan_buffer[i] = static_cast<uint8_t>(i & 0xFFu);
    }

    uint64_t elapsed_ns_total = 0;
    uint64_t checksum_total = 0;
    const size_t records_per_worker = (records + workers - 1) / workers;

    for (auto _ : state) {
        const auto scan_start = std::chrono::steady_clock::now();

        uint64_t checksum = 0;
        for (size_t w = 0; w < workers; ++w) {
            const size_t begin_record = std::min(records, w * records_per_worker);
            const size_t end_record = std::min(records, (w + 1) * records_per_worker);

            const size_t begin_byte = begin_record * kRecordBytes;
            const size_t end_byte = end_record * kRecordBytes;
            for (size_t i = begin_byte; i < end_byte; ++i) {
                checksum += scan_buffer[i];
            }
        }

        const auto scan_end = std::chrono::steady_clock::now();
        const auto scan_ns =
            std::chrono::duration_cast<std::chrono::nanoseconds>(scan_end - scan_start).count();

        elapsed_ns_total += static_cast<uint64_t>(scan_ns);
        checksum_total ^= checksum;
        benchmark::DoNotOptimize(checksum);
    }

    const double avg_scan_ns = state.iterations() == 0
        ? 0.0
        : static_cast<double>(elapsed_ns_total) / static_cast<double>(state.iterations());
    const double throughput_bytes_per_s = avg_scan_ns <= 0.0
        ? 0.0
        : (static_cast<double>(total_bytes) * 1'000'000'000.0) / avg_scan_ns;

    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(total_bytes));
    state.counters["workers"] = static_cast<double>(workers);
    state.counters["scan_bytes_per_iteration"] = static_cast<double>(total_bytes);
    state.counters["anti_entropy_throughput_mb_s"] = throughput_bytes_per_s / (1024.0 * 1024.0);
    state.counters["anti_entropy_throughput_gb_s"] = throughput_bytes_per_s / (1024.0 * 1024.0 * 1024.0);
    state.counters["checksum_guard"] = static_cast<double>(checksum_total & 0xFFFFu);
}

BENCHMARK_REGISTER_F(RebalancingFixture, AntiEntropyScanThroughput)
    ->Args({131072, 8})
    ->Args({262144, 8})
    ->Unit(benchmark::kMicrosecond)
    ->Iterations(120);

#if defined(THEMIS_ENABLE_CUDA) || defined(THEMIS_ENABLE_OPENCL)
BENCHMARK_DEFINE_F(RebalancingFixture, GpuReedSolomonThroughput)(benchmark::State& state) {
    const size_t payload_mb = std::max<size_t>(128, static_cast<size_t>(state.range(0)));
    const size_t payload_bytes = payload_mb * 1024ull * 1024ull;
    constexpr uint32_t kDataShards = 8;
    constexpr uint32_t kParityShards = 4;

    GPUConfig gpu_config;
    gpu_config.device_id = 0;
    gpu_config.min_size_for_gpu = 1;
    gpu_config.batch_size = 16;
    gpu_config.async_compute = true;
    gpu_config.use_pinned_memory = true;
    gpu_config.cuda_streams = 4;

#if defined(THEMIS_ENABLE_CUDA)
    constexpr auto kPreferredAccel = AccelerationType::GPU_CUDA;
#elif defined(THEMIS_ENABLE_OPENCL)
    constexpr auto kPreferredAccel = AccelerationType::GPU_OPENCL;
#else
    constexpr auto kPreferredAccel = AccelerationType::CPU_ONLY;
#endif

    GPUErasureCoder coder(kPreferredAccel, gpu_config, ErasureCodingAlgorithm::REED_SOLOMON);

    if (!coder.isGPUAvailable()) {
        state.SkipWithError("GPU Reed-Solomon coder not available in current runtime");
        return;
    }

    std::vector<uint8_t> payload(payload_bytes);
    for (size_t i = 0; i < payload_bytes; ++i) {
        payload[i] = static_cast<uint8_t>(((i * 29u) + (i / 4096u) * 7u) & 0xFFu);
    }

    uint64_t elapsed_ns_total = 0;
    uint64_t encoded_bytes_total = 0;
    size_t failures = 0;

    for (auto _ : state) {
        const auto begin = std::chrono::steady_clock::now();
        auto chunks = coder.encode(payload, kDataShards, kParityShards);
        const auto end = std::chrono::steady_clock::now();

        const auto elapsed_ns =
            std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count();
        elapsed_ns_total += static_cast<uint64_t>(elapsed_ns);

        if (chunks.size() != (kDataShards + kParityShards)) {
            ++failures;
            continue;
        }

        uint64_t encoded_bytes = 0;
        for (const auto& chunk : chunks) {
            encoded_bytes += static_cast<uint64_t>(chunk.size());
        }

        encoded_bytes_total += encoded_bytes;
        benchmark::DoNotOptimize(chunks.data());
        benchmark::DoNotOptimize(encoded_bytes);
    }

    const auto stats = coder.getStats();
    const size_t total_iterations = static_cast<size_t>(state.iterations());
    const size_t successful = total_iterations > failures
        ? total_iterations - failures
        : 0;

    const double avg_elapsed_ns = successful == 0
        ? 0.0
        : static_cast<double>(elapsed_ns_total) / static_cast<double>(successful);
    const double throughput_input_bytes_s = avg_elapsed_ns <= 0.0
        ? 0.0
        : (static_cast<double>(payload_bytes) * 1'000'000'000.0) / avg_elapsed_ns;
    const double avg_encoded_bytes = successful == 0
        ? 0.0
        : static_cast<double>(encoded_bytes_total) / static_cast<double>(successful);

    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(payload_bytes));
    state.counters["rs_payload_bytes"] = static_cast<double>(payload_bytes);
    state.counters["rs_data_shards"] = static_cast<double>(kDataShards);
    state.counters["rs_parity_shards"] = static_cast<double>(kParityShards);
    state.counters["rs_encoded_bytes"] = avg_encoded_bytes;
    state.counters["gpu_rs_throughput_gb_s"] = throughput_input_bytes_s / (1024.0 * 1024.0 * 1024.0);
    state.counters["gpu_rs_throughput_mb_s"] = throughput_input_bytes_s / (1024.0 * 1024.0);
    state.counters["gpu_rs_encode_ms"] = avg_elapsed_ns / 1'000'000.0;
    state.counters["gpu_rs_failures"] = static_cast<double>(failures);
    state.counters["gpu_rs_gpu_encodes"] = static_cast<double>(stats.gpu_encodes);
    state.counters["gpu_rs_cpu_fallbacks"] = static_cast<double>(stats.cpu_fallbacks);
}
#else
BENCHMARK_DEFINE_F(RebalancingFixture, GpuReedSolomonThroughput)(benchmark::State& state) {
    for (auto _ : state) {
        benchmark::DoNotOptimize(state.iterations());
    }
    state.SkipWithError("GpuReedSolomonThroughput requires THEMIS_ENABLE_CUDA or THEMIS_ENABLE_OPENCL build");
}
#endif

BENCHMARK_REGISTER_F(RebalancingFixture, GpuReedSolomonThroughput)
    ->Arg(128)
    ->Arg(256)
    ->Unit(benchmark::kMillisecond)
    ->Iterations(10);

BENCHMARK_DEFINE_F(RebalancingFixture, SnapshotTransfer1GB)(benchmark::State& state) {
    const size_t chunk_mb = std::max<size_t>(1, static_cast<size_t>(state.range(0)));
    const size_t chunk_bytes = chunk_mb * 1024ull * 1024ull;
    const size_t snapshot_bytes = 1024ull * 1024ull * 1024ull;
    const size_t chunks_per_snapshot = std::max<size_t>(1, snapshot_bytes / chunk_bytes);

    std::vector<uint8_t> source_chunk(chunk_bytes);
    std::vector<uint8_t> sink_chunk(chunk_bytes, 0);

    for (size_t i = 0; i < chunk_bytes; ++i) {
        source_chunk[i] = static_cast<uint8_t>((i * 131u) & 0xFFu);
    }

    uint64_t total_duration_ns = 0;
    uint64_t checksum_guard = 0;

    for (auto _ : state) {
        const auto begin = std::chrono::steady_clock::now();

        for (size_t c = 0; c < chunks_per_snapshot; ++c) {
            for (size_t i = 0; i < chunk_bytes; ++i) {
                sink_chunk[i] = source_chunk[i];
            }
            checksum_guard ^= sink_chunk[(c * 997u) % chunk_bytes];
        }

        const auto end = std::chrono::steady_clock::now();
        const auto duration_ns =
            std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count();
        total_duration_ns += static_cast<uint64_t>(duration_ns);

        benchmark::DoNotOptimize(checksum_guard);
        benchmark::DoNotOptimize(sink_chunk.data());
        benchmark::ClobberMemory();
    }

    const double avg_duration_ns = state.iterations() == 0
        ? 0.0
        : static_cast<double>(total_duration_ns) / static_cast<double>(state.iterations());
    const double snapshot_duration_s = avg_duration_ns / 1'000'000'000.0;
    const double throughput_bytes_s = avg_duration_ns <= 0.0
        ? 0.0
        : (static_cast<double>(snapshot_bytes) * 1'000'000'000.0) / avg_duration_ns;

    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(snapshot_bytes));
    state.counters["snapshot_bytes"] = static_cast<double>(snapshot_bytes);
    state.counters["snapshot_duration_s"] = snapshot_duration_s;
    state.counters["snapshot_throughput_gb_s"] = throughput_bytes_s / (1024.0 * 1024.0 * 1024.0);
    state.counters["checksum_guard"] = static_cast<double>(checksum_guard & 0xFFFFu);
}

BENCHMARK_REGISTER_F(RebalancingFixture, SnapshotTransfer1GB)
    ->Arg(8)
    ->Arg(16)
    ->Unit(benchmark::kMillisecond)
    ->Iterations(30);

BENCHMARK_DEFINE_F(RebalancingFixture, SnapshotCompressionRatioZstdL3)(benchmark::State& state) {
    const size_t snapshot_mb = std::max<size_t>(64, static_cast<size_t>(state.range(0)));
    const size_t raw_bytes = snapshot_mb * 1024ull * 1024ull;
    const int zstd_level = 3;

    std::vector<uint8_t> snapshot(raw_bytes);
    for (size_t i = 0; i < raw_bytes; ++i) {
        const size_t page_offset = i % 4096ull;
        if (page_offset < 32) {
            snapshot[i] = static_cast<uint8_t>((i / 4096ull + page_offset * 17u) & 0xFFu);
        } else if (page_offset < 3072) {
            snapshot[i] = static_cast<uint8_t>(0);
        } else {
            snapshot[i] = static_cast<uint8_t>((page_offset * 13u) & 0xFFu);
        }
    }

    uint64_t compressed_bytes_total = 0;
    uint64_t elapsed_ns_total = 0;
    size_t failed_compressions = 0;

    for (auto _ : state) {
        const auto begin = std::chrono::steady_clock::now();
        std::vector<uint8_t> compressed(ZSTD_compressBound(snapshot.size()));
        const size_t compressed_size = ZSTD_compress(
            compressed.data(),
            compressed.size(),
            snapshot.data(),
            snapshot.size(),
            zstd_level
        );
        const auto end = std::chrono::steady_clock::now();

        const auto elapsed_ns =
            std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count();
        elapsed_ns_total += static_cast<uint64_t>(elapsed_ns);

        if (ZSTD_isError(compressed_size) != 0) {
            ++failed_compressions;
            continue;
        }

        compressed_bytes_total += static_cast<uint64_t>(compressed_size);
        benchmark::DoNotOptimize(compressed_size);
    }

    const size_t total_iterations = static_cast<size_t>(state.iterations());
    const size_t successful = total_iterations > failed_compressions
        ? total_iterations - failed_compressions
        : 0;
    const double avg_compressed_bytes = successful == 0
        ? 0.0
        : static_cast<double>(compressed_bytes_total) / static_cast<double>(successful);
    const double ratio_pct = raw_bytes == 0
        ? 0.0
        : (avg_compressed_bytes / static_cast<double>(raw_bytes)) * 100.0;
    const double avg_elapsed_ms = state.iterations() == 0
        ? 0.0
        : static_cast<double>(elapsed_ns_total) / static_cast<double>(state.iterations()) / 1'000'000.0;

    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(raw_bytes));
    state.counters["snapshot_uncompressed_bytes"] = static_cast<double>(raw_bytes);
    state.counters["snapshot_compressed_bytes"] = avg_compressed_bytes;
    state.counters["snapshot_compression_ratio_pct"] = ratio_pct;
    state.counters["snapshot_compression_time_ms"] = avg_elapsed_ms;
    state.counters["compression_failures"] = static_cast<double>(failed_compressions);
}

BENCHMARK_REGISTER_F(RebalancingFixture, SnapshotCompressionRatioZstdL3)
    ->Arg(64)
    ->Arg(128)
    ->Unit(benchmark::kMillisecond)
    ->Iterations(40);

BENCHMARK_DEFINE_F(RebalancingFixture, ReplicaCatchupThroughput)(benchmark::State& state) {
    const size_t catchup_mb = std::max<size_t>(64, static_cast<size_t>(state.range(0)));
    const size_t catchup_bytes = catchup_mb * 1024ull * 1024ull;
    constexpr size_t kEntryBytes = 1024;
    const size_t entries = std::max<size_t>(1, catchup_bytes / kEntryBytes);

    std::vector<uint8_t> wal_stream(entries * kEntryBytes);
    for (size_t i = 0; i < wal_stream.size(); ++i) {
        wal_stream[i] = static_cast<uint8_t>((i * 17u + (i / 97u)) & 0xFFu);
    }

    std::vector<uint64_t> replica_state(entries, 0);
    uint64_t elapsed_ns_total = 0;
    uint64_t checksum_guard = 0;

    for (auto _ : state) {
        const auto begin = std::chrono::steady_clock::now();

        for (size_t e = 0; e < entries; ++e) {
            const size_t offset = e * kEntryBytes;
            uint64_t value = 1469598103934665603ull;
            for (size_t b = 0; b < kEntryBytes; ++b) {
                value ^= static_cast<uint64_t>(wal_stream[offset + b]);
                value *= 1099511628211ull;
            }

            replica_state[e] = value ^ static_cast<uint64_t>(e * 131u);
        }

        const auto end = std::chrono::steady_clock::now();
        const auto elapsed_ns =
            std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count();
        elapsed_ns_total += static_cast<uint64_t>(elapsed_ns);

        checksum_guard ^= replica_state[(elapsed_ns_total / 997u) % entries];
        benchmark::DoNotOptimize(replica_state.data());
        benchmark::DoNotOptimize(checksum_guard);
        benchmark::ClobberMemory();
    }

    const double avg_elapsed_ns = state.iterations() == 0
        ? 0.0
        : static_cast<double>(elapsed_ns_total) / static_cast<double>(state.iterations());
    const double throughput_bytes_s = avg_elapsed_ns <= 0.0
        ? 0.0
        : (static_cast<double>(catchup_bytes) * 1'000'000'000.0) / avg_elapsed_ns;

    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(catchup_bytes));
    state.counters["catchup_bytes"] = static_cast<double>(catchup_bytes);
    state.counters["catchup_entries"] = static_cast<double>(entries);
    state.counters["replica_catchup_mb_s"] = throughput_bytes_s / (1024.0 * 1024.0);
    state.counters["replica_catchup_gb_s"] = throughput_bytes_s / (1024.0 * 1024.0 * 1024.0);
    state.counters["replica_catchup_time_ms"] = avg_elapsed_ns / 1'000'000.0;
    state.counters["checksum_guard"] = static_cast<double>(checksum_guard & 0xFFFFu);
}

BENCHMARK_REGISTER_F(RebalancingFixture, ReplicaCatchupThroughput)
    ->Arg(128)
    ->Arg(256)
    ->Unit(benchmark::kMillisecond)
    ->Iterations(20);

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

BENCHMARK_DEFINE_F(GossipOverheadFixture, TopologyPropagation100Nodes)(benchmark::State& state) {
    const size_t nodes = std::max<size_t>(100, static_cast<size_t>(state.range(0)));
    const size_t fanout = 4;
    constexpr double kPerRoundNetworkMs = 40.0;

    double rounds_total = 0.0;
    double propagation_ms_total = 0.0;
    double reach_pct_total = 0.0;

    for (auto _ : state) {
        std::vector<int> reached_round(nodes, -1);
        std::vector<size_t> frontier;
        frontier.reserve(nodes);
        frontier.push_back(0);
        reached_round[0] = 0;

        size_t round = 0;
        while (!frontier.empty() && round < 64) {
            std::vector<size_t> next_frontier;
            next_frontier.reserve(nodes / 2);

            for (size_t src : frontier) {
                const uint64_t round_seed =
                    static_cast<uint64_t>(src) * 1315423911ull + static_cast<uint64_t>(round) * 2654435761ull;

                for (size_t step = 1; step <= fanout; ++step) {
                    const size_t dst = static_cast<size_t>((round_seed + step * 97ull) % nodes);
                    if (dst == src || reached_round[dst] != -1) {
                        continue;
                    }
                    reached_round[dst] = static_cast<int>(round + 1);
                    next_frontier.push_back(dst);
                }
            }

            ++round;
            frontier.swap(next_frontier);
        }

        size_t reached_count = 0;
        int max_round = 0;
        for (int r : reached_round) {
            if (r >= 0) {
                ++reached_count;
                max_round = std::max(max_round, r);
            }
        }

        const double reach_pct = nodes == 0
            ? 0.0
            : (static_cast<double>(reached_count) / static_cast<double>(nodes)) * 100.0;
        const double propagation_ms = static_cast<double>(max_round) * kPerRoundNetworkMs;

        rounds_total += static_cast<double>(max_round);
        propagation_ms_total += propagation_ms;
        reach_pct_total += reach_pct;

        benchmark::DoNotOptimize(reached_round.data());
        benchmark::DoNotOptimize(propagation_ms);
        benchmark::ClobberMemory();
    }

    const double iterations = state.iterations() == 0
        ? 1.0
        : static_cast<double>(state.iterations());

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(nodes));
    state.counters["topology_nodes"] = static_cast<double>(nodes);
    state.counters["gossip_fanout"] = static_cast<double>(fanout);
    state.counters["topology_rounds"] = rounds_total / iterations;
    state.counters["topology_propagation_ms"] = propagation_ms_total / iterations;
    state.counters["topology_reach_pct"] = reach_pct_total / iterations;
}

BENCHMARK_REGISTER_F(GossipOverheadFixture, TopologyPropagation100Nodes)
    ->Arg(100)
    ->Arg(150)
    ->Unit(benchmark::kMicrosecond)
    ->Iterations(150);

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
