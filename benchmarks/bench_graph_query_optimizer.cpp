/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            bench_graph_query_optimizer.cpp                    ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-02-21 10:34:09                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     303                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 4299bafc6  2026-01-22  Implement Graph Query Engine Optimization (#801) ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// Benchmark: Graph Query Optimizer Performance
// Measures query optimization and execution performance

#include "graph/graph_query_optimizer.h"
#include "index/graph_index.h"
#include "storage/rocksdb_wrapper.h"
#include "storage/base_entity.h"
#include <benchmark/benchmark.h>
#include <filesystem>
#include <random>

using namespace themis;
using namespace themis::graph;

// ============================================================================
// Test Setup
// ============================================================================

class GraphQueryOptimizerBenchmarkFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& state) override {
        // Clean up any existing test database
        test_db_path_ = "./data/bench_graph_optimizer_tmp";
        if (std::filesystem::exists(test_db_path_)) {
            std::filesystem::remove_all(test_db_path_);
        }
        
        // Create RocksDB wrapper
        themis::RocksDBWrapper::Config config;
        config.db_path = test_db_path_;
        config.memtable_size_mb = 256;
        config.block_cache_size_mb = 512;
        
        db_ = std::make_unique<themis::RocksDBWrapper>(config);
        if (!db_->open()) {
            throw std::runtime_error("Failed to open database");
        }
        
        // Create graph index manager
        graph_mgr_ = std::make_unique<themis::GraphIndexManager>(*db_);
        
        // Create optimizer
        optimizer_ = std::make_unique<GraphQueryOptimizer>(*graph_mgr_);
        
        // Build test graph
        graph_size_ = state.range(0);
        buildTestGraph(graph_size_, 4); // 4 average degree
    }
    
    void TearDown(const ::benchmark::State& /*state*/) override {
        optimizer_.reset();
        graph_mgr_.reset();
        db_->close();
        db_.reset();
        
        // Clean up test database
        if (std::filesystem::exists(test_db_path_)) {
            std::filesystem::remove_all(test_db_path_);
        }
    }
    
    void buildTestGraph(int num_nodes, int avg_degree) {
        std::mt19937 rng(42);
        
        // Create edges (directed graph)
        std::uniform_int_distribution<int> node_dist(0, num_nodes - 1);
        
        int edge_id = 0;
        for (int i = 0; i < num_nodes; i++) {
            std::string from_id = "node_" + std::to_string(i);
            
            int edges_to_add = avg_degree / 2 + (rng() % (avg_degree / 2 + 1));
            
            for (int j = 0; j < edges_to_add; j++) {
                int to_idx = node_dist(rng);
                std::string to_id = "node_" + std::to_string(to_idx);
                
                // Skip self-loops
                if (to_idx == i) continue;
                
                themis::BaseEntity edge("edge_" + std::to_string(edge_id++));
                edge.setField("id", "edge_" + std::to_string(edge_id));
                edge.setField("_from", from_id);
                edge.setField("_to", to_id);
                edge.setField("_weight", "1.0");
                
                graph_mgr_->addEdge(edge);
            }
        }
        
        // Rebuild topology for fast lookups
        graph_mgr_->rebuildTopology();
        
        // Collect statistics
        optimizer_->collectStatistics();
    }
    
protected:
    std::string test_db_path_;
    std::unique_ptr<themis::RocksDBWrapper> db_;
    std::unique_ptr<themis::GraphIndexManager> graph_mgr_;
    std::unique_ptr<GraphQueryOptimizer> optimizer_;
    int graph_size_ = 0;
};

// ============================================================================
// Plan Generation Benchmarks
// ============================================================================

BENCHMARK_DEFINE_F(GraphQueryOptimizerBenchmarkFixture, PlanGeneration_ShortestPath)(benchmark::State& state) {
    for (auto _ : state) {
        auto result = optimizer_->optimizeShortestPath("node_0", "node_50");
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK_DEFINE_F(GraphQueryOptimizerBenchmarkFixture, PlanGeneration_KHopNeighborhood)(benchmark::State& state) {
    for (auto _ : state) {
        auto result = optimizer_->optimizeKHopNeighborhood("node_0", 3);
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK_DEFINE_F(GraphQueryOptimizerBenchmarkFixture, PlanGeneration_WithCache)(benchmark::State& state) {
    optimizer_->setPlanCachingEnabled(true);
    
    // Prime the cache
    optimizer_->optimizeShortestPath("node_0", "node_50");
    
    for (auto _ : state) {
        auto result = optimizer_->optimizeShortestPath("node_0", "node_50");
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations());
}

// ============================================================================
// BFS Benchmarks
// ============================================================================

BENCHMARK_DEFINE_F(GraphQueryOptimizerBenchmarkFixture, BFS_Execution)(benchmark::State& state) {
    int max_depth = state.range(1);
    
    for (auto _ : state) {
        GraphQueryOptimizer::ExecutionStats stats;
        auto result = optimizer_->executeBFS("node_0", max_depth, {}, &stats);
        benchmark::DoNotOptimize(result);
        
        if (result) {
            state.counters["nodes_explored"] = stats.nodes_explored;
            state.counters["edges_traversed"] = stats.edges_traversed;
        }
    }
    
    state.SetItemsProcessed(state.iterations());
}

// ============================================================================
// DFS Benchmarks
// ============================================================================

BENCHMARK_DEFINE_F(GraphQueryOptimizerBenchmarkFixture, DFS_Execution)(benchmark::State& state) {
    int max_depth = state.range(1);
    
    for (auto _ : state) {
        GraphQueryOptimizer::ExecutionStats stats;
        auto result = optimizer_->executeDFS("node_0", max_depth, {}, &stats);
        benchmark::DoNotOptimize(result);
        
        if (result) {
            state.counters["nodes_explored"] = stats.nodes_explored;
            state.counters["edges_traversed"] = stats.edges_traversed;
        }
    }
    
    state.SetItemsProcessed(state.iterations());
}

// ============================================================================
// Dijkstra Benchmarks
// ============================================================================

BENCHMARK_DEFINE_F(GraphQueryOptimizerBenchmarkFixture, Dijkstra_Execution)(benchmark::State& state) {
    for (auto _ : state) {
        GraphQueryOptimizer::ExecutionStats stats;
        auto result = optimizer_->executeDijkstra("node_0", "node_50", {}, &stats);
        benchmark::DoNotOptimize(result);
        
        if (result) {
            state.counters["nodes_explored"] = stats.nodes_explored;
        }
    }
    
    state.SetItemsProcessed(state.iterations());
}

// ============================================================================
// Bidirectional Search Benchmarks
// ============================================================================

BENCHMARK_DEFINE_F(GraphQueryOptimizerBenchmarkFixture, Bidirectional_Execution)(benchmark::State& state) {
    for (auto _ : state) {
        GraphQueryOptimizer::ExecutionStats stats;
        auto result = optimizer_->executeBidirectional("node_0", "node_50", {}, &stats);
        benchmark::DoNotOptimize(result);
        
        if (result) {
            state.counters["nodes_explored"] = stats.nodes_explored;
        }
    }
    
    state.SetItemsProcessed(state.iterations());
}

// ============================================================================
// Statistics Collection Benchmarks
// ============================================================================

BENCHMARK_DEFINE_F(GraphQueryOptimizerBenchmarkFixture, Statistics_Collection)(benchmark::State& state) {
    for (auto _ : state) {
        auto result = optimizer_->collectStatistics();
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations());
}

// ============================================================================
// Register Benchmarks
// ============================================================================

// Plan generation benchmarks
BENCHMARK_REGISTER_F(GraphQueryOptimizerBenchmarkFixture, PlanGeneration_ShortestPath)
    ->Arg(100);

BENCHMARK_REGISTER_F(GraphQueryOptimizerBenchmarkFixture, PlanGeneration_KHopNeighborhood)
    ->Arg(100);

BENCHMARK_REGISTER_F(GraphQueryOptimizerBenchmarkFixture, PlanGeneration_WithCache)
    ->Arg(100);

// BFS benchmarks - vary graph size and depth
BENCHMARK_REGISTER_F(GraphQueryOptimizerBenchmarkFixture, BFS_Execution)
    ->Args({100, 2})
    ->Args({100, 3})
    ->Args({100, 4})
    ->Args({500, 2})
    ->Args({500, 3});

// DFS benchmarks
BENCHMARK_REGISTER_F(GraphQueryOptimizerBenchmarkFixture, DFS_Execution)
    ->Args({100, 2})
    ->Args({100, 3})
    ->Args({100, 4})
    ->Args({500, 2})
    ->Args({500, 3});

// Dijkstra benchmarks
BENCHMARK_REGISTER_F(GraphQueryOptimizerBenchmarkFixture, Dijkstra_Execution)
    ->Arg(100)
    ->Arg(500);

// Bidirectional benchmarks
BENCHMARK_REGISTER_F(GraphQueryOptimizerBenchmarkFixture, Bidirectional_Execution)
    ->Arg(100)
    ->Arg(500);

// Statistics benchmarks
BENCHMARK_REGISTER_F(GraphQueryOptimizerBenchmarkFixture, Statistics_Collection)
    ->Arg(100)
    ->Arg(500);

// Run the benchmarks
BENCHMARK_MAIN();
