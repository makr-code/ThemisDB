/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            bench_graph_traversal.cpp                          ║
  Version:         0.0.9                                              ║
  Last Modified:   2026-02-21 13:48:08                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     725                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// Benchmark: Graph Traversal Performance
// Measures BFS/DFS and graph traversal algorithm performance

#include "index/graph_index.h"
#include "index/graph_analytics.h"
#include "storage/rocksdb_wrapper.h"
#include "storage/base_entity.h"
#include <benchmark/benchmark.h>
#include <filesystem>
#include <random>
#include <queue>
#include <stack>
#include <set>

using namespace themis;

// ============================================================================
// Test Setup
// ============================================================================

class GraphTraversalBenchmarkFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& state) override {
        // Clean up any existing test database
        test_db_path_ = "./data/bench_graph_traversal_tmp";
        if (std::filesystem::exists(test_db_path_)) {
            std::filesystem::remove_all(test_db_path_);
        }
        
        // Create RocksDB wrapper
        RocksDBWrapper::Config config;
        config.db_path = test_db_path_;
        config.memtable_size_mb = 256;
        config.block_cache_size_mb = 512;
        
        db_ = std::make_unique<RocksDBWrapper>(config);
       if (!db_->open()) { throw std::runtime_error("Failed to open RocksDB in benchmark"); }
        if (!db_->open()) {
            throw std::runtime_error("Failed to open database");
        }
        
        // Create graph index manager
        graph_mgr_ = std::make_unique<GraphIndexManager>(*db_);
        
        // Create graph analytics
        analytics_ = std::make_unique<GraphAnalytics>(*graph_mgr_);
        
        // Build test graph
        graph_size_ = state.range(0);
        buildTestGraph(graph_size_, state.range(1)); // size and connectivity
    }
    
    void TearDown(const ::benchmark::State& /*state*/) override {
        analytics_.reset();
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
        
        // Create node ids (nodes are implicit via edges)
        for (int i = 0; i < num_nodes; i++) {
            std::string node_id = "node_" + std::to_string(i);
            node_ids_.push_back(node_id);
        }
        
        // Create edges (directed graph)
        std::uniform_int_distribution<int> node_dist(0, num_nodes - 1);
        
        for (int i = 0; i < num_nodes; i++) {
            int edges_to_add = avg_degree / 2 + (rng() % (avg_degree / 2 + 1));
            
            for (int j = 0; j < edges_to_add; j++) {
                int target = node_dist(rng);
                if (target != i) { // No self-loops
                    std::string edge_id = "edge_" + std::to_string(i) + "_" + std::to_string(target);
                    
                    BaseEntity edge(edge_id);
                    edge.setField("id", edge_id);
                    edge.setField("_from", node_ids_[i]);
                    edge.setField("_to", node_ids_[target]);
                    edge.setField("_graph", "test_graph");
                    edge.setField("_weight", 1.0 + (rng() % 10));

                    auto st = graph_mgr_->addEdge(edge);
                    if (!st.ok) {
                        throw std::runtime_error("Failed to add edge: " + st.message);
                    }
                }
            }
        }
    }
    
protected:
    std::string test_db_path_;
    std::unique_ptr<RocksDBWrapper> db_;
    std::unique_ptr<GraphIndexManager> graph_mgr_;
    std::unique_ptr<GraphAnalytics> analytics_;
    std::vector<std::string> node_ids_;
    int graph_size_;
};

// ============================================================================
// Benchmark: BFS Traversal
// ============================================================================

BENCHMARK_DEFINE_F(GraphTraversalBenchmarkFixture, BFSTraversal)(benchmark::State& state) {
    if (node_ids_.empty()) {
        state.SkipWithError("No nodes in graph");
        return;
    }
    
    std::string start_node = node_ids_[0];
    
    for (auto _ : state) {
        std::queue<std::string> queue;
        std::set<std::string> visited;
        
        queue.push(start_node);
        visited.insert(start_node);
        
        int nodes_visited = 0;
        
        while (!queue.empty() && nodes_visited < graph_size_) {
            std::string current = queue.front();
            queue.pop();
            nodes_visited++;
            
            auto [status, neighbors] = graph_mgr_->outNeighbors(current);
            if (status.ok) {
                for (const auto& neighbor : neighbors) {
                    if (visited.find(neighbor) == visited.end()) {
                        visited.insert(neighbor);
                        queue.push(neighbor);
                    }
                }
            }
        }
        
        benchmark::DoNotOptimize(nodes_visited);
    }
    
    state.SetItemsProcessed(state.iterations());
    state.counters["graph_size"] = graph_size_;
    state.counters["nodes_per_sec"] = benchmark::Counter(
        state.iterations() * graph_size_, benchmark::Counter::kIsRate);
}

BENCHMARK_REGISTER_F(GraphTraversalBenchmarkFixture, BFSTraversal)
    ->Args({100, 4})     // 100 nodes, avg degree 4 (sparse)
    ->Args({1000, 4})    // 1K nodes, sparse
    ->Args({10000, 4})   // 10K nodes, sparse
    ->Args({100, 20})    // 100 nodes, avg degree 20 (dense)
    ->Args({1000, 20})   // 1K nodes, dense
    ->Unit(benchmark::kMillisecond);

// ============================================================================
// Benchmark: DFS Traversal
// ============================================================================

BENCHMARK_DEFINE_F(GraphTraversalBenchmarkFixture, DFSTraversal)(benchmark::State& state) {
    if (node_ids_.empty()) {
        state.SkipWithError("No nodes in graph");
        return;
    }
    
    std::string start_node = node_ids_[0];
    
    for (auto _ : state) {
        std::stack<std::string> stack;
        std::set<std::string> visited;
        
        stack.push(start_node);
        visited.insert(start_node);
        
        int nodes_visited = 0;
        
        while (!stack.empty() && nodes_visited < graph_size_) {
            std::string current = stack.top();
            stack.pop();
            nodes_visited++;
            
            auto [status, neighbors] = graph_mgr_->outNeighbors(current);
            if (status.ok) {
                for (const auto& neighbor : neighbors) {
                    if (visited.find(neighbor) == visited.end()) {
                        visited.insert(neighbor);
                        stack.push(neighbor);
                    }
                }
            }
        }
        
        benchmark::DoNotOptimize(nodes_visited);
    }
    
    state.SetItemsProcessed(state.iterations());
    state.counters["graph_size"] = graph_size_;
}

BENCHMARK_REGISTER_F(GraphTraversalBenchmarkFixture, DFSTraversal)
    ->Args({100, 4})
    ->Args({1000, 4})
    ->Args({10000, 4})
    ->Args({100, 20})
    ->Args({1000, 20})
    ->Unit(benchmark::kMillisecond);

// ============================================================================
// Benchmark: Shortest Path (Dijkstra-like)
// ============================================================================

BENCHMARK_DEFINE_F(GraphTraversalBenchmarkFixture, ShortestPathTraversal)(benchmark::State& state) {
    if (node_ids_.size() < 2) {
        state.SkipWithError("Need at least 2 nodes");
        return;
    }
    
    std::string start_node = node_ids_[0];
    std::string end_node = node_ids_[node_ids_.size() - 1];
    
    for (auto _ : state) {
        using DistNode = std::pair<double, std::string>;
        std::priority_queue<DistNode, std::vector<DistNode>, std::greater<DistNode>> pq;
        std::map<std::string, double> distances;
        std::set<std::string> visited;
        
        pq.push({0.0, start_node});
        distances[start_node] = 0.0;
        
        bool found = false;
        int nodes_explored = 0;
        
        while (!pq.empty() && !found) {
            auto [dist, current] = pq.top();
            pq.pop();
            
            if (visited.find(current) != visited.end()) {
                continue;
            }
            
            visited.insert(current);
            nodes_explored++;
            
            if (current == end_node) {
                found = true;
                break;
            }
            
            auto [status, neighbors] = graph_mgr_->outNeighbors(current);
            if (status.ok) {
                for (const auto& neighbor : neighbors) {
                    double edge_weight = 1.0; // Simplified
                    double new_dist = dist + edge_weight;
                    
                    if (distances.find(neighbor) == distances.end() || 
                        new_dist < distances[neighbor]) {
                        distances[neighbor] = new_dist;
                        pq.push({new_dist, neighbor});
                    }
                }
            }
        }
        
        benchmark::DoNotOptimize(found);
        benchmark::DoNotOptimize(nodes_explored);
    }
    
    state.SetItemsProcessed(state.iterations());
    state.counters["graph_size"] = graph_size_;
}

BENCHMARK_REGISTER_F(GraphTraversalBenchmarkFixture, ShortestPathTraversal)
    ->Args({100, 4})
    ->Args({1000, 4})
    ->Args({10000, 4})
    ->Unit(benchmark::kMillisecond);

// ============================================================================
// Benchmark: Degree Centrality
// ============================================================================

BENCHMARK_DEFINE_F(GraphTraversalBenchmarkFixture, DegreeCentrality)(benchmark::State& state) {
    for (auto _ : state) {
        auto [status, results] = analytics_->degreeCentrality(node_ids_);
        
        if (!status.ok) {
            state.SkipWithError("Degree centrality failed");
        }
        
        benchmark::DoNotOptimize(results);
    }
    
    state.SetItemsProcessed(state.iterations() * node_ids_.size());
    state.counters["graph_size"] = graph_size_;
}

BENCHMARK_REGISTER_F(GraphTraversalBenchmarkFixture, DegreeCentrality)
    ->Args({100, 4})
    ->Args({1000, 4})
    ->Args({10000, 4})
    ->Unit(benchmark::kMillisecond);

// ============================================================================
// Benchmark: Connected Components
// ============================================================================

BENCHMARK_DEFINE_F(GraphTraversalBenchmarkFixture, ConnectedComponents)(benchmark::State& state) {
    for (auto _ : state) {
        std::map<std::string, int> component_id;
        int current_component = 0;
        
        for (const auto& node : node_ids_) {
            if (component_id.find(node) != component_id.end()) {
                continue; // Already assigned
            }
            
            // BFS to find component
            std::queue<std::string> queue;
            queue.push(node);
            component_id[node] = current_component;
            
            while (!queue.empty()) {
                std::string current = queue.front();
                queue.pop();
                
                auto [status, neighbors] = graph_mgr_->outNeighbors(current);
                if (status.ok) {
                    for (const auto& neighbor : neighbors) {
                        if (component_id.find(neighbor) == component_id.end()) {
                            component_id[neighbor] = current_component;
                            queue.push(neighbor);
                        }
                    }
                }
            }
            
            current_component++;
        }
        
        benchmark::DoNotOptimize(current_component);
    }
    
    state.SetItemsProcessed(state.iterations());
    state.counters["graph_size"] = graph_size_;
}

BENCHMARK_REGISTER_F(GraphTraversalBenchmarkFixture, ConnectedComponents)
    ->Args({100, 4})
    ->Args({1000, 4})
    ->Args({10000, 4})
    ->Unit(benchmark::kMillisecond);

// ============================================================================
// Benchmark: Graph Diameter Estimation
// ============================================================================

BENCHMARK_DEFINE_F(GraphTraversalBenchmarkFixture, DiameterEstimation)(benchmark::State& state) {
    // Sample a few nodes and compute max distance
    const int sample_size = std::min(10, static_cast<int>(node_ids_.size()));
    
    for (auto _ : state) {
        int max_distance = 0;
        
        for (int i = 0; i < sample_size; i++) {
            std::string start = node_ids_[i];
            
            std::queue<std::pair<std::string, int>> queue;
            std::set<std::string> visited;
            
            queue.push({start, 0});
            visited.insert(start);
            
            while (!queue.empty()) {
                auto [current, dist] = queue.front();
                queue.pop();
                
                max_distance = std::max(max_distance, dist);
                
                auto [status, neighbors] = graph_mgr_->outNeighbors(current);
                if (status.ok) {
                    for (const auto& neighbor : neighbors) {
                        if (visited.find(neighbor) == visited.end()) {
                            visited.insert(neighbor);
                            queue.push({neighbor, dist + 1});
                        }
                    }
                }
            }
        }
        
        benchmark::DoNotOptimize(max_distance);
    }
    
    state.SetItemsProcessed(state.iterations());
    state.counters["graph_size"] = graph_size_;
    state.counters["sample_size"] = sample_size;
}

BENCHMARK_REGISTER_F(GraphTraversalBenchmarkFixture, DiameterEstimation)
    ->Args({100, 4})
    ->Args({1000, 4})
    ->Unit(benchmark::kMillisecond);

// ============================================================================
// Benchmark: General Graph Traversal (New Feature)
// Tests the QueryEngine::executeGeneralTraversal implementation
// ============================================================================

#include "query/query_engine.h"
#include "index/secondary_index.h"

class GeneralTraversalBenchmarkFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& state) override {
        test_db_path_ = "./data/bench_general_traversal_tmp";
        if (std::filesystem::exists(test_db_path_)) {
            std::filesystem::remove_all(test_db_path_);
        }
        
        RocksDBWrapper::Config config;
        config.db_path = test_db_path_;
        config.memtable_size_mb = 256;
        config.block_cache_size_mb = 512;
        
        db_ = std::make_unique<RocksDBWrapper>(config);
        if (!db_->open()) {
            throw std::runtime_error("Failed to open database");
        }
        
        sec_idx_ = std::make_unique<SecondaryIndexManager>(*db_);
        graph_mgr_ = std::make_unique<GraphIndexManager>(*db_);
        query_engine_ = std::make_unique<QueryEngine>(*db_, *sec_idx_, *graph_mgr_);
        
        graph_size_ = state.range(0);
        avg_degree_ = state.range(1);
        buildTestGraph(graph_size_, avg_degree_);
    }
    
    void TearDown(const ::benchmark::State& /*state*/) override {
        query_engine_.reset();
        graph_mgr_.reset();
        sec_idx_.reset();
        db_->close();
        db_.reset();
        
        if (std::filesystem::exists(test_db_path_)) {
            std::filesystem::remove_all(test_db_path_);
        }
    }
    
    void buildTestGraph(int num_nodes, int avg_degree) {
        std::mt19937 rng(42);
        
        for (int i = 0; i < num_nodes; i++) {
            node_ids_.push_back("node_" + std::to_string(i));
        }
        
        std::uniform_int_distribution<int> node_dist(0, num_nodes - 1);
        
        for (int i = 0; i < num_nodes; i++) {
            int edges_to_add = avg_degree / 2 + (rng() % (avg_degree / 2 + 1));
            
            for (int j = 0; j < edges_to_add; j++) {
                int target = node_dist(rng);
                if (target != i) {
                    std::string edge_id = "edge_" + std::to_string(i) + "_" + std::to_string(target);
                    
                    BaseEntity edge(edge_id);
                    edge.setField("id", edge_id);
                    edge.setField("_from", node_ids_[i]);
                    edge.setField("_to", node_ids_[target]);
                    edge.setField("_graph", "default");
                    edge.setField("_weight", 1.0 + (rng() % 10));
                    
                    auto st = graph_mgr_->addEdge(edge);
                    if (!st.ok) {
                        throw std::runtime_error("Failed to add edge: " + st.message);
                    }
                }
            }
        }
    }
    
protected:
    std::string test_db_path_;
    std::unique_ptr<RocksDBWrapper> db_;
    std::unique_ptr<SecondaryIndexManager> sec_idx_;
    std::unique_ptr<GraphIndexManager> graph_mgr_;
    std::unique_ptr<QueryEngine> query_engine_;
    std::vector<std::string> node_ids_;
    int graph_size_;
    int avg_degree_;
};

// Benchmark: General Traversal - OUTBOUND direction
BENCHMARK_DEFINE_F(GeneralTraversalBenchmarkFixture, GeneralTraversalOutbound)(benchmark::State& state) {
    if (node_ids_.empty()) {
        state.SkipWithError("No nodes in graph");
        return;
    }
    
    std::string start_node = node_ids_[0];
    int depth = state.range(2);
    
    for (auto _ : state) {
        auto result = query_engine_->executeGeneralTraversal(
            start_node,
            1,        // minDepth
            depth,    // maxDepth
            TraversalDirection::OUTBOUND,
            "default"
        );
        
        if (!result) {
            state.SkipWithError("Traversal failed: " + result.error().message());
            return;
        }
        
        auto results = std::move(*result);
        benchmark::DoNotOptimize(results);
    }
    
    state.SetItemsProcessed(state.iterations());
    state.counters["graph_size"] = graph_size_;
    state.counters["avg_degree"] = avg_degree_;
    state.counters["max_depth"] = depth;
}

BENCHMARK_REGISTER_F(GeneralTraversalBenchmarkFixture, GeneralTraversalOutbound)
    ->Args({100, 4, 2})      // 100 nodes, degree 4, depth 2
    ->Args({100, 4, 3})      // 100 nodes, degree 4, depth 3
    ->Args({1000, 4, 2})     // 1K nodes, depth 2
    ->Args({1000, 4, 3})     // 1K nodes, depth 3
    ->Args({10000, 4, 2})    // 10K nodes, depth 2
    ->Args({10000, 4, 3})    // 10K nodes, depth 3 (target from spec)
    ->Args({1000, 20, 2})    // Dense graph, depth 2
    ->Args({1000, 20, 3})    // Dense graph, depth 3
    ->Unit(benchmark::kMillisecond);

// Benchmark: General Traversal - INBOUND direction
BENCHMARK_DEFINE_F(GeneralTraversalBenchmarkFixture, GeneralTraversalInbound)(benchmark::State& state) {
    if (node_ids_.empty()) {
        state.SkipWithError("No nodes in graph");
        return;
    }
    
    std::string start_node = node_ids_[node_ids_.size() / 2];
    int depth = state.range(2);
    
    for (auto _ : state) {
        auto result = query_engine_->executeGeneralTraversal(
            start_node,
            1,
            depth,
            TraversalDirection::INBOUND,
            "default"
        );
        
        if (!result) {
            state.SkipWithError("Traversal failed: " + result.error().message());
            return;
        }
        
        auto results = std::move(*result);
        benchmark::DoNotOptimize(results);
    }
    
    state.SetItemsProcessed(state.iterations());
    state.counters["graph_size"] = graph_size_;
    state.counters["max_depth"] = depth;
}

BENCHMARK_REGISTER_F(GeneralTraversalBenchmarkFixture, GeneralTraversalInbound)
    ->Args({100, 4, 2})
    ->Args({1000, 4, 2})
    ->Args({10000, 4, 2})
    ->Unit(benchmark::kMillisecond);

// Benchmark: General Traversal - ANY direction (bidirectional)
BENCHMARK_DEFINE_F(GeneralTraversalBenchmarkFixture, GeneralTraversalAny)(benchmark::State& state) {
    if (node_ids_.empty()) {
        state.SkipWithError("No nodes in graph");
        return;
    }
    
    std::string start_node = node_ids_[node_ids_.size() / 2];
    int depth = state.range(2);
    
    for (auto _ : state) {
        auto result = query_engine_->executeGeneralTraversal(
            start_node,
            1,
            depth,
            TraversalDirection::ANY,
            "default"
        );
        
        if (!result) {
            state.SkipWithError("Traversal failed: " + result.error().message());
            return;
        }
        
        auto results = std::move(*result);
        benchmark::DoNotOptimize(results);
    }
    
    state.SetItemsProcessed(state.iterations());
    state.counters["graph_size"] = graph_size_;
    state.counters["max_depth"] = depth;
}

BENCHMARK_REGISTER_F(GeneralTraversalBenchmarkFixture, GeneralTraversalAny)
    ->Args({100, 4, 2})
    ->Args({1000, 4, 2})
    ->Args({10000, 4, 2})
    ->Unit(benchmark::kMillisecond);

// Benchmark: General Traversal with Depth Filtering (minDepth > 0)
BENCHMARK_DEFINE_F(GeneralTraversalBenchmarkFixture, GeneralTraversalDepthFilter)(benchmark::State& state) {
    if (node_ids_.empty()) {
        state.SkipWithError("No nodes in graph");
        return;
    }
    
    std::string start_node = node_ids_[0];
    
    for (auto _ : state) {
        auto result = query_engine_->executeGeneralTraversal(
            start_node,
            2,        // minDepth = 2 (filter out depth 0 and 1)
            3,        // maxDepth = 3
            TraversalDirection::OUTBOUND,
            "default"
        );
        
        if (!result) {
            state.SkipWithError("Traversal failed: " + result.error().message());
            return;
        }
        
        auto results = std::move(*result);
        benchmark::DoNotOptimize(results);
    }
    
    state.SetItemsProcessed(state.iterations());
    state.counters["graph_size"] = graph_size_;
}

BENCHMARK_REGISTER_F(GeneralTraversalBenchmarkFixture, GeneralTraversalDepthFilter)
    ->Args({100, 4, 0})
    ->Args({1000, 4, 0})
    ->Args({10000, 4, 0})
    ->Unit(benchmark::kMillisecond);

// Benchmark: General Traversal - Large Result Sets
BENCHMARK_DEFINE_F(GeneralTraversalBenchmarkFixture, GeneralTraversalLargeResults)(benchmark::State& state) {
    if (node_ids_.empty()) {
        state.SkipWithError("No nodes in graph");
        return;
    }
    
    std::string start_node = node_ids_[0];
    
    for (auto _ : state) {
        auto result = query_engine_->executeGeneralTraversal(
            start_node,
            0,        // Include start vertex
            4,        // Deeper traversal
            TraversalDirection::OUTBOUND,
            "default"
        );
        
        if (!result) {
            state.SkipWithError("Traversal failed: " + result.error().message());
            return;
        }
        
        auto results = std::move(*result);
        state.counters["result_count"] = results.size();
        benchmark::DoNotOptimize(results);
    }
    
    state.SetItemsProcessed(state.iterations());
    state.counters["graph_size"] = graph_size_;
}

BENCHMARK_REGISTER_F(GeneralTraversalBenchmarkFixture, GeneralTraversalLargeResults)
    ->Args({1000, 20, 0})    // Dense graph, large result set
    ->Unit(benchmark::kMillisecond);

BENCHMARK_MAIN();
