// Benchmark: Graph Query Optimizer Performance
// Measures query optimization and execution performance

#include "graph/graph_query_optimizer.h"
#include "graph/parallel_traversal.h"
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
        
        // Create parallel traversal
        parallel_traversal_ = std::make_unique<themis::graph::ParallelTraversal>(*graph_mgr_);
        
        // Build test graph
        graph_size_ = state.range(0);
        buildTestGraph(graph_size_, 4); // 4 average degree
    }
    
    void TearDown(const ::benchmark::State& /*state*/) override {
        parallel_traversal_.reset();
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
                if (to_idx == i) {
                  continue;
                }
                
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
    std::unique_ptr<themis::graph::ParallelTraversal> parallel_traversal_;
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

// ============================================================================
// Parallel Multi-Source BFS Benchmarks
// ============================================================================

// Benchmark: Multi-source BFS with varying source count and graph size.
// state.range(0) = graph size, state.range(1) = number of source vertices.
BENCHMARK_DEFINE_F(GraphQueryOptimizerBenchmarkFixture, MultiSourceBFS)(benchmark::State& state) {
    const int num_sources = static_cast<int>(state.range(1));
    std::vector<std::string> sources;
    sources.reserve(num_sources);
    for (int i = 0; i < num_sources && i < graph_size_; ++i) {
        sources.push_back("node_" + std::to_string(i));
    }

    themis::graph::ParallelTraversal::Config cfg;
    cfg.max_depth = 3;

    for (auto _ : state) {
        auto result = parallel_traversal_->multiSourceBFS(sources, cfg);
        benchmark::DoNotOptimize(result);
        if (result) {
            state.counters["visited"] = result->visited_vertices.size();
        }
    }

    state.SetItemsProcessed(state.iterations());
    state.counters["graph_size"] = graph_size_;
    state.counters["num_sources"] = num_sources;
}

// Benchmark: Multi-source DFS with varying source count and graph size.
BENCHMARK_DEFINE_F(GraphQueryOptimizerBenchmarkFixture, MultiSourceDFS)(benchmark::State& state) {
    const int num_sources = static_cast<int>(state.range(1));
    std::vector<std::string> sources;
    sources.reserve(num_sources);
    for (int i = 0; i < num_sources && i < graph_size_; ++i) {
        sources.push_back("node_" + std::to_string(i));
    }

    themis::graph::ParallelTraversal::Config cfg;
    cfg.max_depth = 3;

    for (auto _ : state) {
        auto result = parallel_traversal_->multiSourceDFS(sources, cfg);
        benchmark::DoNotOptimize(result);
        if (result) {
            state.counters["visited"] = result->visited_vertices.size();
        }
    }

    state.SetItemsProcessed(state.iterations());
    state.counters["graph_size"] = graph_size_;
    state.counters["num_sources"] = num_sources;
}

// Benchmark: Parallel BFS thread scaling – same 4 sources, varying thread count.
// state.range(0) = graph size, state.range(1) = num_threads for parallel traversal.
BENCHMARK_DEFINE_F(GraphQueryOptimizerBenchmarkFixture, MultiSourceBFS_ThreadScaling)(benchmark::State& state) {
    const int num_threads = static_cast<int>(state.range(1));
    const int kSources = 4;
    std::vector<std::string> sources = {};

    for (int i = 0; i < kSources && i < graph_size_; ++i) {
        sources.push_back("node_" + std::to_string(i));
    }

    themis::graph::ParallelTraversal::Config cfg;
    cfg.max_depth = 3;
    cfg.num_threads = static_cast<uint32_t>(num_threads);

    for (auto _ : state) {
        auto result = parallel_traversal_->multiSourceBFS(sources, cfg);
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(state.iterations());
    state.counters["graph_size"] = graph_size_;
    state.counters["num_threads"] = num_threads;
}

// Multi-source BFS: vary graph size (1 source) and number of sources (fixed 100-node graph).
BENCHMARK_REGISTER_F(GraphQueryOptimizerBenchmarkFixture, MultiSourceBFS)
    ->Args({100, 1})
    ->Args({100, 4})
    ->Args({100, 8})
    ->Args({500, 1})
    ->Args({500, 4})
    ->Args({500, 8})
    ->Unit(benchmark::kMillisecond);

// Multi-source DFS: same parameter sweep.
BENCHMARK_REGISTER_F(GraphQueryOptimizerBenchmarkFixture, MultiSourceDFS)
    ->Args({100, 1})
    ->Args({100, 4})
    ->Args({100, 8})
    ->Args({500, 1})
    ->Args({500, 4})
    ->Args({500, 8})
    ->Unit(benchmark::kMillisecond);

// Thread scaling: graph_size=500, num_threads=1/2/4/8.
BENCHMARK_REGISTER_F(GraphQueryOptimizerBenchmarkFixture, MultiSourceBFS_ThreadScaling)
    ->Args({500, 1})
    ->Args({500, 2})
    ->Args({500, 4})
    ->Args({500, 8})
    ->Unit(benchmark::kMillisecond);

// ============================================================================
// Incremental Graph Query Execution Benchmarks
// ============================================================================

// Benchmark: registering and calling onGraphChange with a single incremental query.
// state.range(0) = graph size
// state.range(1) = BFS depth
BENCHMARK_DEFINE_F(GraphQueryOptimizerBenchmarkFixture, IncrementalBFS_OnGraphChange)(benchmark::State& state) {
    const int depth = static_cast<int>(state.range(1));

    GraphQueryOptimizer::QueryConstraints constraints;
    constraints.max_depth = depth;

    // Register a single incremental query; seed the initial snapshot.
    size_t delta_total = 0;
    auto handle = optimizer_->registerIncrementalBFS(
        "node_0", depth, constraints,
        [&delta_total](const GraphQueryOptimizer::IncrementalQueryResult& r) {
            delta_total += r.added.size() + r.removed.size();
        });

    // Build a change set that touches node_1 (always in BFS result for depth>=1).
    GraphQueryOptimizer::GraphChangeSet changes;
    changes.addEdgeAdded("bench_edge_new", "node_1", "node_2");

    for (auto _ : state) {
        auto count = optimizer_->onGraphChange(changes);
        benchmark::DoNotOptimize(count);
    }

    optimizer_->unregisterIncrementalQuery(handle);

    state.SetItemsProcessed(state.iterations());
    state.counters["graph_size"]  = graph_size_;
    state.counters["depth"]       = depth;
    state.counters["delta_total"] = static_cast<double>(delta_total);
}

// Benchmark: onGraphChange with multiple registered queries - measures fan-out cost.
// state.range(0) = graph size, state.range(1) = number of registered queries
BENCHMARK_DEFINE_F(GraphQueryOptimizerBenchmarkFixture, IncrementalBFS_MultiQuery_FanOut)(benchmark::State& state) {
    const int num_queries = static_cast<int>(state.range(1));

    GraphQueryOptimizer::QueryConstraints constraints;
    constraints.max_depth = 2;

    std::vector<GraphQueryOptimizer::IncrementalQueryHandle> handles;
    handles.reserve(num_queries);
    for (int i = 0; i < num_queries; ++i) {
        handles.push_back(optimizer_->registerIncrementalBFS(
            "node_0", 2, constraints,
            [](const GraphQueryOptimizer::IncrementalQueryResult&) {}));
    }

    GraphQueryOptimizer::GraphChangeSet changes;
    changes.addEdgeAdded("bench_edge_fanout", "node_1", "node_2");

    for (auto _ : state) {
        auto count = optimizer_->onGraphChange(changes);
        benchmark::DoNotOptimize(count);
    }

    for (auto h : handles) {
        optimizer_->unregisterIncrementalQuery(h);
    }

    state.SetItemsProcessed(state.iterations());
    state.counters["graph_size"]  = graph_size_;
    state.counters["num_queries"] = num_queries;
}

BENCHMARK_REGISTER_F(GraphQueryOptimizerBenchmarkFixture, IncrementalBFS_OnGraphChange)
    ->Args({100, 2})
    ->Args({100, 3})
    ->Args({500, 2})
    ->Args({500, 3})
    ->Unit(benchmark::kMicrosecond);

BENCHMARK_REGISTER_F(GraphQueryOptimizerBenchmarkFixture, IncrementalBFS_MultiQuery_FanOut)
    ->Args({100, 1})
    ->Args({100, 4})
    ->Args({100, 8})
    ->Args({500, 1})
    ->Args({500, 4})
    ->Unit(benchmark::kMicrosecond);

// Run the benchmarks
BENCHMARK_MAIN();
