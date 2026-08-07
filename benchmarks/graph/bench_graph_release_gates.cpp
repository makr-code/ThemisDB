// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file bench_graph_release_gates.cpp
 * @brief Phase 5 graph module query optimizer and traversal release-gate benchmarks.
 *
 * Provides reproducible latency and throughput measurements for the graph
 * optimizer and traversal hot paths identified in the graph module roadmap
 * (Phase 5 — Performance and Hardening). Results are used as release gates; a
 * regression beyond 10% vs. the baseline blocks promotion.
 *
 * ## Benchmark families
 *
 * ### GRG-01..03 — Query optimizer hot paths
 *   GRG-01  QueryPlanner::planTraversal() with constraint-aware optimization
 *   GRG-02  ExplainPlan::explain() generation for decision analysis
 *   GRG-03  ConstraintResolver::resolveConstraints() with 10 constraints
 *
 * ### GRG-04..05 — Traversal execution
 *   GRG-04  GraphTraversal::dfs() on 1K-node graph, depth=5
 *   GRG-05  GraphTraversal::bfs() on 10K-node graph, depth=3
 *
 * ### GRG-06 — Reasoning integration
 *   GRG-06  KnowledgeGraphReasoner link prediction with RotatE embeddings
 *
 * ## Hard release gates
 *
 * | Gate ID          | Benchmark | Threshold                    |
 * |------------------|-----------|------------------------------|
 * | GATE-GRG-01      | GRG-01    | p99 ≤ 1 ms (plan)            |
 * | GATE-GRG-02      | GRG-02    | p99 ≤ 500 µs (explain)       |
 * | GATE-GRG-03      | GRG-03    | p99 ≤ 200 µs (constraints)   |
 * | GATE-GRG-04      | GRG-04    | p99 ≤ 10 ms (dfs 1K)         |
 * | GATE-GRG-05      | GRG-05    | p99 ≤ 50 ms (bfs 10K)        |
 * | GATE-GRG-06      | GRG-06    | p99 ≤ 100 µs (reasoning)     |
 *
 * All benchmarks:
 *   - Use kGrgCanonicalSeed = 42 for deterministic graph generation.
 *   - Run with Repetitions(kRepetitions) to capture variance.
 *   - Synthetic graph fixtures with controlled fan-out and depth.
 *
 * @see src/graph/ROADMAP.md — Phase 5 items
 * @see include/graph/query_planner.h — query optimization
 * @see include/graph/graph_traversal.h — traversal engines
 */

#include <benchmark/benchmark.h>

#include "graph/query_planner.h"
#include "graph/graph_traversal.h"
#include "graph/knowledge_graph_reasoner.h"

#include <memory>
#include <string>
#include <vector>

using namespace themis::graph;

// ============================================================================
// Constants — deterministic, release-pinned
// ============================================================================

/// Canonical seed for all Graph benchmarks.
static constexpr uint64_t kGrgCanonicalSeed = 42;

/// Repetitions per benchmark for variance estimation.
static constexpr int kRepetitions = 5;

// ============================================================================
// Fixtures
// ============================================================================

class GraphOptimizationFixture : public benchmark::Fixture {
protected:
    std::unique_ptr<QueryPlanner> planner_;
    std::unique_ptr<GraphTraversal> traversal_;
    std::unique_ptr<KnowledgeGraphReasoner> reasoner_;

    void SetUp(const ::benchmark::State& /*state*/) override {
        planner_ = std::make_unique<QueryPlanner>();
        traversal_ = std::make_unique<GraphTraversal>();
        reasoner_ = std::make_unique<KnowledgeGraphReasoner>();
    }

    void TearDown(const ::benchmark::State& /*state*/) override {
        planner_.reset();
        traversal_.reset();
        reasoner_.reset();
    }
};

// ============================================================================
// GRG-01: QueryPlanner::planTraversal() with constraint-aware optimization
//         Threshold: p99 ≤ 1 ms
// ============================================================================

BENCHMARK_DEFINE_F(GraphOptimizationFixture, GRG01_PlanTraversal)
(benchmark::State& state) {
    TraversalQuery query;
    query.start_vertex_id = "v0";
    query.depth_limit = 5;
    query.max_results = 1000;

    for (auto _ : state) {
        auto plan = planner_->planTraversal(query);
        benchmark::DoNotOptimize(plan);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK_REGISTER_F(GraphOptimizationFixture, GRG01_PlanTraversal)
    ->Unit(benchmark::kMicrosecond)
    ->Repetitions(kRepetitions)
    ->Iterations(500)
    ->UseRealTime();

// ============================================================================
// GRG-02: ExplainPlan::explain() generation for decision analysis
//         Threshold: p99 ≤ 500 µs
// ============================================================================

BENCHMARK_DEFINE_F(GraphOptimizationFixture, GRG02_ExplainPlan)
(benchmark::State& state) {
    TraversalQuery query;
    query.start_vertex_id = "v0";
    query.depth_limit = 5;

    auto plan = planner_->planTraversal(query);

    for (auto _ : state) {
        std::string explain = plan->explain();
        benchmark::DoNotOptimize(explain);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK_REGISTER_F(GraphOptimizationFixture, GRG02_ExplainPlan)
    ->Unit(benchmark::kMicrosecond)
    ->Repetitions(kRepetitions)
    ->Iterations(1000)
    ->UseRealTime();

// ============================================================================
// GRG-03: ConstraintResolver::resolveConstraints() with 10 constraints
//         Threshold: p99 ≤ 200 µs
// ============================================================================

BENCHMARK_DEFINE_F(GraphOptimizationFixture, GRG03_ResolveConstraints)
(benchmark::State& state) {
    std::vector<QueryConstraint> constraints;
    for (int i = 0; i < 10; ++i) {
        QueryConstraint c;
        c.property = "prop_" + std::to_string(i);
        c.value = "value_" + std::to_string(i);
        constraints.push_back(c);
    }

    TraversalQuery query;
    query.constraints = constraints;

    for (auto _ : state) {
        auto resolved = planner_->resolveConstraints(query);
        benchmark::DoNotOptimize(resolved);
    }
    state.SetItemsProcessed(state.iterations() * 10);
}
BENCHMARK_REGISTER_F(GraphOptimizationFixture, GRG03_ResolveConstraints)
    ->Unit(benchmark::kMicrosecond)
    ->Repetitions(kRepetitions)
    ->Iterations(1000)
    ->UseRealTime();

// ============================================================================
// GRG-04: GraphTraversal::dfs() on 1K-node graph, depth=5
//         Threshold: p99 ≤ 10 ms
// ============================================================================

BENCHMARK_DEFINE_F(GraphOptimizationFixture, GRG04_DfsTraversal1K)
(benchmark::State& state) {
    // Initialize traversal engine with 1K-node synthetic graph
    traversal_->initializeSyntheticGraph(1000, kGrgCanonicalSeed);

    TraversalQuery query;
    query.start_vertex_id = "v0";
    query.depth_limit = 5;

    for (auto _ : state) {
        auto results = traversal_->dfs(query);
        benchmark::DoNotOptimize(results);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK_REGISTER_F(GraphOptimizationFixture, GRG04_DfsTraversal1K)
    ->Unit(benchmark::kMicrosecond)
    ->Repetitions(kRepetitions)
    ->Iterations(50)
    ->UseRealTime();

// ============================================================================
// GRG-05: GraphTraversal::bfs() on 10K-node graph, depth=3
//         Threshold: p99 ≤ 50 ms
// ============================================================================

BENCHMARK_DEFINE_F(GraphOptimizationFixture, GRG05_BfsTraversal10K)
(benchmark::State& state) {
    // Initialize traversal engine with 10K-node synthetic graph
    traversal_->initializeSyntheticGraph(10000, kGrgCanonicalSeed);

    TraversalQuery query;
    query.start_vertex_id = "v0";
    query.depth_limit = 3;

    for (auto _ : state) {
        auto results = traversal_->bfs(query);
        benchmark::DoNotOptimize(results);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK_REGISTER_F(GraphOptimizationFixture, GRG05_BfsTraversal10K)
    ->Unit(benchmark::kMicrosecond)
    ->Repetitions(kRepetitions)
    ->Iterations(20)
    ->UseRealTime();

// ============================================================================
// GRG-06: KnowledgeGraphReasoner link prediction with RotatE embeddings
//         Threshold: p99 ≤ 100 µs
// ============================================================================

BENCHMARK_DEFINE_F(GraphOptimizationFixture, GRG06_LinkPrediction)
(benchmark::State& state) {
    // Load RotatE embeddings
    reasoner_->loadEmbeddings("rotate");

    LinkPredictionQuery query;
    query.source_entity = "entity_1";
    query.target_entity = "entity_2";
    query.relation = "relation_1";
    query.topk = 5;

    for (auto _ : state) {
        auto predictions = reasoner_->predictLinks(query);
        benchmark::DoNotOptimize(predictions);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK_REGISTER_F(GraphOptimizationFixture, GRG06_LinkPrediction)
    ->Unit(benchmark::kMicrosecond)
    ->Repetitions(kRepetitions)
    ->Iterations(2000)
    ->UseRealTime();
