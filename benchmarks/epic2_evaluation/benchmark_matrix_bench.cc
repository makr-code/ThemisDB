/**
 * @file benchmark_matrix_bench.cc
 * @brief Google Benchmark scenarios for HNSW, DiskANN, Tensor Mid-Layer,
 *        Graph validation, and LLM/LoRA architecture paths (EPIC 2.2).
 *
 * Each benchmark simulates the measurable overhead for a specific architecture
 * path using synthetic data.  The benchmarks are designed to be runnable in
 * CI without GPU or large dataset infrastructure.  Their primary purpose is:
 *
 * 1. Validate that the `BenchmarkMatrix` data structure has negligible
 *    overhead when used as a result collector.
 * 2. Establish stable timing baselines for the matrix-fill and query paths
 *    that the planner and lifecycle modules depend on.
 * 3. Serve as smoke-tests that the benchmark binary compiles and runs cleanly
 *    (Phase 7 CI integration, Issue #5438).
 *
 * Benchmark groups
 * ----------------
 * - BM_Matrix_Record_*        : single record() call latency per scenario
 * - BM_Matrix_Lookup_*        : single lookup() call latency per scenario
 * - BM_Matrix_FillAll_*       : bulk-fill the entire matrix
 * - BM_Matrix_ScenarioSlice   : slice all dimensions for one scenario
 * - BM_Matrix_DimensionSlice  : slice all scenarios for one dimension
 * - BM_Matrix_BestScenario    : rank scenarios on a dimension
 * - BM_Matrix_Compare         : compare two scenarios on a dimension
 * - BM_Matrix_Serialise       : entries() full serialisation
 * - BM_Matrix_Invalidate      : invalidateScenario() cost
 * - BM_ANN_HNSW_Simulation    : synthetic HNSW query simulation
 * - BM_ANN_DiskANN_Simulation : synthetic DiskANN query simulation
 * - BM_Tensor_Update_Worker   : dynamic tensor update overhead simulation
 * - BM_Snapshot_Rebuild       : snapshot rebuild latency simulation
 * - BM_Commit_Overhead        : commit overhead simulation
 * - BM_Query_Routing          : summary-first routing vs. exact load
 * - BM_LLM_LoRA_Inference     : LoRA inference overhead simulation
 * - BM_GPU_BreakEven          : CPU/GPU break-even simulation
 */

#ifndef THEMIS_BENCHMARK_BUILD
#define THEMIS_BENCHMARK_BUILD 1
#endif

#include <benchmark/benchmark.h>

#include "benchmark_matrix.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <numeric>
#include <random>
#include <string>
#include <vector>

using namespace themis::evaluation;

// ============================================================================
// Shared helpers
// ============================================================================

namespace {

/// Build a clean result for use in benchmark helpers.
inline BenchmarkResult cleanResult(double value,
                                   uint32_t samples = 100,
                                   double stddev   = 0.0) noexcept {
    BenchmarkResult r;
    r.value        = value;
    r.stddev       = stddev;
    r.sample_count = samples;
    r.edge_flags   = BenchmarkEdgeCase::NONE;
    return r;
}

/// Populate the full scenario × dimension matrix with synthetic clean results.
void fillFullMatrix(BenchmarkMatrix& m) {
    constexpr auto sCount = static_cast<uint8_t>(BenchmarkScenario::_COUNT);
    constexpr auto dCount = static_cast<uint8_t>(BenchmarkDimension::_COUNT);
    for (uint8_t si = 0; si < sCount; ++si) {
        for (uint8_t di = 0; di < dCount; ++di) {
            m.record(static_cast<BenchmarkScenario>(si),
                     static_cast<BenchmarkDimension>(di),
                     cleanResult(static_cast<double>(si * dCount + di), 10));
        }
    }
}

/// Simple synthetic HNSW search: linear scan over a small in-memory dataset.
/// Returns a candidate list of size k.
std::vector<int>
syntheticHnswSearch(const std::vector<float>& query,
                    const std::vector<std::vector<float>>& dataset,
                    int k) {
    // Compute dot-products and return top-k indices.
    std::vector<std::pair<float, int>> scores;
    scores.reserve(dataset.size());
    for (int i = 0; i < static_cast<int>(dataset.size()); ++i) {
        float dot = 0.f;
        for (std::size_t d = 0; d < query.size(); ++d) {
            dot += query[d] * dataset[i][d];
        }
        scores.emplace_back(dot, i);
    }
    std::partial_sort(scores.begin(),
                      scores.begin() + std::min(k, static_cast<int>(scores.size())),
                      scores.end(),
                      [](const auto& a, const auto& b){ return a.first > b.first; });
    std::vector<int> result;
    result.reserve(k);
    for (int i = 0; i < std::min(k, static_cast<int>(scores.size())); ++i) {
        result.push_back(scores[i].second);
    }
    return result;
}

/// Simulate a tensor compression step: compute mean of a float vector.
float syntheticTensorSummary(const std::vector<float>& v) {
    if (v.empty()) {
      return 0.f;
    }
    float sum = std::accumulate(v.begin(), v.end(), 0.f);
    return sum / static_cast<float>(v.size());
}

/// Simulate a graph validation step: count edges in a small adjacency list.
int syntheticGraphValidate(const std::vector<std::vector<int>>& adj,
                           const std::vector<int>& candidates) {
    int edge_count = 0;
    for (int c : candidates) {
        if (c < static_cast<int>(adj.size())) {
            edge_count += static_cast<int>(adj[c].size());
        }
    }
    return edge_count;
}

/// Simulate commit overhead: string concatenation representing WAL write.
std::string syntheticCommit(int tx_id) {
    return "txn:" + std::to_string(tx_id) + ":commit";
}

/// Generate a synthetic float vector of given dimension.
std::vector<float> makeVector(int dim, float seed_val) {
    std::vector<float> v(static_cast<std::size_t>(dim));
    float x = seed_val;
    for (auto& e : v) {
        e = std::sin(x);
        x += 0.1f;
    }
    // Normalise.
    float norm = 0.f;
    for (auto e : v) {
      norm += e * e;
    }
    norm = std::sqrt(std::max(norm, 1e-6f));
    for (auto& e : v) {
      e /= norm;
    }
    return v;
}

} // namespace

// ============================================================================
// BenchmarkMatrix operation benchmarks
// ============================================================================

/// Record a single clean result into a fresh matrix.
static void BM_Matrix_Record(benchmark::State& state) {
    for (auto _ : state) {
        BenchmarkMatrix m;
        m.record(BenchmarkScenario::HNSW_ANN_ONLY,
                 BenchmarkDimension::RECALL_AT_K,
                 cleanResult(0.95));
        benchmark::DoNotOptimize(m);
    }
}
BENCHMARK(BM_Matrix_Record);

/// Lookup a cell that is present.
static void BM_Matrix_LookupHit(benchmark::State& state) {
    BenchmarkMatrix m;
    m.record(BenchmarkScenario::ANN_TENSOR,
             BenchmarkDimension::QUERY_LATENCY_MS,
             cleanResult(8.5));
    for (auto _ : state) {
        auto r = m.lookup(BenchmarkScenario::ANN_TENSOR,
                          BenchmarkDimension::QUERY_LATENCY_MS);
        benchmark::DoNotOptimize(r);
    }
}
BENCHMARK(BM_Matrix_LookupHit);

/// Lookup a cell that is absent (miss path).
static void BM_Matrix_LookupMiss(benchmark::State& state) {
    BenchmarkMatrix m;
    for (auto _ : state) {
        auto r = m.lookup(BenchmarkScenario::DISKANN_ANN_ONLY,
                          BenchmarkDimension::GPU_SPEEDUP_FACTOR);
        benchmark::DoNotOptimize(r);
    }
}
BENCHMARK(BM_Matrix_LookupMiss);

/// Fill the entire scenario × dimension matrix with synthetic results.
static void BM_Matrix_FillAll(benchmark::State& state) {
    for (auto _ : state) {
        BenchmarkMatrix m;
        fillFullMatrix(m);
        benchmark::DoNotOptimize(m);
    }
}
BENCHMARK(BM_Matrix_FillAll);

/// Retrieve all dimensions for one scenario.
static void BM_Matrix_ScenarioSlice(benchmark::State& state) {
    BenchmarkMatrix m;
    fillFullMatrix(m);
    for (auto _ : state) {
        auto slice = m.scenarioSlice(BenchmarkScenario::ANN_TENSOR_GRAPH);
        benchmark::DoNotOptimize(slice);
    }
}
BENCHMARK(BM_Matrix_ScenarioSlice);

/// Retrieve all scenarios for one dimension.
static void BM_Matrix_DimensionSlice(benchmark::State& state) {
    BenchmarkMatrix m;
    fillFullMatrix(m);
    for (auto _ : state) {
        auto slice = m.dimensionSlice(BenchmarkDimension::QUERY_LATENCY_MS);
        benchmark::DoNotOptimize(slice);
    }
}
BENCHMARK(BM_Matrix_DimensionSlice);

/// Rank scenarios by a single dimension.
static void BM_Matrix_BestScenario(benchmark::State& state) {
    BenchmarkMatrix m;
    fillFullMatrix(m);
    for (auto _ : state) {
        auto best = m.bestScenario(BenchmarkDimension::RECALL_AT_K, true);
        benchmark::DoNotOptimize(best);
    }
}
BENCHMARK(BM_Matrix_BestScenario);

/// Compare two scenarios on a dimension.
static void BM_Matrix_Compare(benchmark::State& state) {
    BenchmarkMatrix m;
    m.record(BenchmarkScenario::HNSW_ANN_ONLY,
             BenchmarkDimension::QPS, cleanResult(1500.0));
    m.record(BenchmarkScenario::DISKANN_ANN_ONLY,
             BenchmarkDimension::QPS, cleanResult(900.0));
    for (auto _ : state) {
        auto ratio = m.compareScenarios(BenchmarkScenario::HNSW_ANN_ONLY,
                                        BenchmarkScenario::DISKANN_ANN_ONLY,
                                        BenchmarkDimension::QPS);
        benchmark::DoNotOptimize(ratio);
    }
}
BENCHMARK(BM_Matrix_Compare);

/// Full serialisation of the matrix to a vector of BenchmarkEntry.
static void BM_Matrix_Serialise(benchmark::State& state) {
    BenchmarkMatrix m;
    fillFullMatrix(m);
    for (auto _ : state) {
        auto entries = m.entries("msmarco-1M", "cpu-avx2", "v1.0");
        benchmark::DoNotOptimize(entries);
    }
}
BENCHMARK(BM_Matrix_Serialise);

/// Invalidate a single scenario (removes all its cells).
static void BM_Matrix_InvalidateScenario(benchmark::State& state) {
    for (auto _ : state) {
        BenchmarkMatrix m;
        fillFullMatrix(m);
        m.invalidateScenario(BenchmarkScenario::ANN_TENSOR_GRAPH);
        benchmark::DoNotOptimize(m);
    }
}
BENCHMARK(BM_Matrix_InvalidateScenario);

// ============================================================================
// HNSW simulation
// ============================================================================

/**
 * @brief Synthetic HNSW ANN query benchmark.
 *
 * Simulates a linear-scan ANN search (stand-in for hnswlib) over a small
 * in-memory dataset to measure the relative query overhead that would be
 * recorded as `HNSW_ANN_ONLY / QUERY_LATENCY_MS` in the matrix.
 *
 * @note This is a simulation to keep the benchmark self-contained.
 *       Production measurements require a real hnswlib index.
 */
// STUB/SIMULATION NOTE:
// Purpose: Simulate HNSW query path overhead for matrix calibration in CI.
// Activation: Always active; no GPU or hnswlib dependency required.
// Production Delta: Uses linear scan instead of HNSW graph traversal.
//                   Actual speedup of HNSW vs. linear varies by dataset size.
// Removal Plan: Replace with real hnswlib index when CI runners have vcpkg
//               benchmark datasets available (Target: v2.0.0 / Q4 2026).
static void BM_ANN_HNSW_Simulation(benchmark::State& state) {
    constexpr int DIM   = 128;
    constexpr int N     = 1024;
    constexpr int K     = 10;

    // Build synthetic dataset.
    std::vector<std::vector<float>> dataset;
    dataset.reserve(N);
    for (int i = 0; i < N; ++i) {
        dataset.push_back(makeVector(DIM, static_cast<float>(i) * 0.01f));
    }
    auto query = makeVector(DIM, 0.5f);

    for (auto _ : state) {
        auto candidates = syntheticHnswSearch(query, dataset, K);
        benchmark::DoNotOptimize(candidates);
    }

    state.SetItemsProcessed(state.iterations());
    state.SetLabel("HNSW_ANN_ONLY | dim=128 | n=1024 | k=10");
}
BENCHMARK(BM_ANN_HNSW_Simulation)->Unit(benchmark::kMicrosecond);

// ============================================================================
// DiskANN simulation
// ============================================================================

/**
 * @brief Synthetic DiskANN query benchmark.
 *
 * Simulates disk-resident graph search with a small additional I/O cost
 * modelled as extra iterations.  The ratio between HNSW and DiskANN latency
 * benchmarks establishes the break-even point for index choice.
 *
 * @note Simulation: disk I/O is modelled by additional computation, not
 *       actual SSD reads.  Requires replacement with real DiskANN bindings.
 */
// STUB/SIMULATION NOTE:
// Purpose: Model DiskANN I/O overhead for matrix comparison in CI.
// Activation: Always active.
// Production Delta: Real DiskANN incurs actual NVMe/SSD seek latency.
// Removal Plan: Wire real DiskANN index once storage benchmarks are available
//               (Target: v2.0.0 / Q4 2026).
static void BM_ANN_DiskANN_Simulation(benchmark::State& state) {
    constexpr int DIM       = 128;
    constexpr int N         = 1024;
    constexpr int K         = 10;
    constexpr int IO_FACTOR = 2; // DiskANN revisits more nodes than HNSW.

    std::vector<std::vector<float>> dataset;
    dataset.reserve(N);
    for (int i = 0; i < N; ++i) {
        dataset.push_back(makeVector(DIM, static_cast<float>(i) * 0.01f));
    }
    auto query = makeVector(DIM, 0.5f);

    for (auto _ : state) {
        // Simulate extra I/O by running the search IO_FACTOR times and merging.
        std::vector<int> merged = {};

        for (int pass = 0; pass < IO_FACTOR; ++pass) {
            auto c = syntheticHnswSearch(query, dataset, K);
            merged.insert(merged.end(), c.begin(), c.end());
        }
        std::sort(merged.begin(), merged.end());
        merged.erase(std::unique(merged.begin(), merged.end()), merged.end());
        if (merged.size() > static_cast<std::size_t>(K)) {
          merged.resize(K);
        }
        benchmark::DoNotOptimize(merged);
    }

    state.SetItemsProcessed(state.iterations());
    state.SetLabel("DISKANN_ANN_ONLY | dim=128 | n=1024 | k=10 | io_factor=2");
}
BENCHMARK(BM_ANN_DiskANN_Simulation)->Unit(benchmark::kMicrosecond);

// ============================================================================
// Tensor Mid-Layer simulation
// ============================================================================

/**
 * @brief Dynamic tensor update worker overhead benchmark.
 *
 * Simulates the incremental tensor-update path: compute a running summary
 * (mean of embedding blocks) for an incoming batch of vectors.  This models
 * the `ANN_TENSOR_DYNAMIC_UPDATE` scenario overhead.
 */
static void BM_Tensor_Update_Worker(benchmark::State& state) {
    constexpr int BATCH_SIZE  = 64;
    constexpr int BLOCK_DIM   = 128;

    std::vector<std::vector<float>> batch;
    batch.reserve(BATCH_SIZE);
    for (int i = 0; i < BATCH_SIZE; ++i) {
        batch.push_back(makeVector(BLOCK_DIM, static_cast<float>(i)));
    }

    for (auto _ : state) {
        float running_mean = 0.f;
        for (const auto& vec : batch) {
            float summary = syntheticTensorSummary(vec);
            running_mean  = 0.9f * running_mean + 0.1f * summary;
        }
        benchmark::DoNotOptimize(running_mean);
    }

    state.SetItemsProcessed(state.iterations() * BATCH_SIZE);
    state.SetLabel("ANN_TENSOR_DYNAMIC_UPDATE | batch=64 | block_dim=128");
}
BENCHMARK(BM_Tensor_Update_Worker)->Unit(benchmark::kMicrosecond);

// ============================================================================
// Snapshot rebuild simulation
// ============================================================================

/**
 * @brief Snapshot rebuild latency simulation.
 *
 * Models the `ANN_TENSOR_SNAPSHOT_REBUILT` scenario: iterating over a
 * simulated tensor store and recomputing compressed summaries for all blocks.
 */
static void BM_Snapshot_Rebuild(benchmark::State& state) {
    const int num_blocks = static_cast<int>(state.range(0));
    constexpr int BLOCK_DIM = 128;

    std::vector<std::vector<float>> blocks;
    blocks.reserve(static_cast<std::size_t>(num_blocks));
    for (int i = 0; i < num_blocks; ++i) {
        blocks.push_back(makeVector(BLOCK_DIM, static_cast<float>(i) * 0.1f));
    }

    for (auto _ : state) {
        std::vector<float> summaries;
        summaries.reserve(static_cast<std::size_t>(num_blocks));
        for (const auto& block : blocks) {
            summaries.push_back(syntheticTensorSummary(block));
        }
        benchmark::DoNotOptimize(summaries);
    }

    state.SetItemsProcessed(state.iterations() *
                             static_cast<int64_t>(num_blocks));
    state.SetLabel("ANN_TENSOR_SNAPSHOT_REBUILT | blocks=" +
                   std::to_string(num_blocks) + " | block_dim=128");
}
BENCHMARK(BM_Snapshot_Rebuild)->RangeMultiplier(4)->Range(64, 4096)
    ->Unit(benchmark::kMicrosecond);

// ============================================================================
// Commit overhead simulation
// ============================================================================

/**
 * @brief Commit overhead benchmark (COMMIT_OVERHEAD scenario).
 *
 * Models the extra latency incurred by a transactional commit compared to a
 * pure read path.  Used to fill `COMMIT_OVERHEAD / COMMIT_OVERHEAD_MS` in the
 * benchmark matrix.
 */
static void BM_Commit_Overhead(benchmark::State& state) {
    int tx_id = 0;
    for (auto _ : state) {
        auto wal_entry = syntheticCommit(tx_id++);
        benchmark::DoNotOptimize(wal_entry);
    }
    state.SetItemsProcessed(state.iterations());
    state.SetLabel("COMMIT_OVERHEAD");
}
BENCHMARK(BM_Commit_Overhead)->Unit(benchmark::kNanosecond);

// ============================================================================
// Query routing quality simulation
// ============================================================================

/**
 * @brief Summary-first routing vs. direct exact load benchmark.
 *
 * Compares two routing strategies:
 * - Summary-first: filter shards using compressed summary; fewer shards loaded.
 * - Direct exact: load all candidate shards unconditionally.
 *
 * The benchmark measures the candidate reduction cost (shard fan-out proxy).
 */
static void BM_Query_Routing_SummaryFirst(benchmark::State& state) {
    constexpr int NUM_SHARDS = 32;
    constexpr int K          = 10;

    // Simulate per-shard summary scores.
    std::vector<float> summaries(NUM_SHARDS);
    for (int i = 0; i < NUM_SHARDS; ++i) {
        summaries[i] = std::sin(static_cast<float>(i));
    }

    for (auto _ : state) {
        // Summary-first: only contact top-4 shards.
        std::vector<std::pair<float, int>> ranked;
        ranked.reserve(NUM_SHARDS);
        for (int i = 0; i < NUM_SHARDS; ++i) {
            ranked.emplace_back(summaries[i], i);
        }
        std::partial_sort(ranked.begin(), ranked.begin() + K / 2,
                          ranked.end(),
                          [](const auto& a, const auto& b){ return a.first > b.first; });
        int contacted = K / 2;
        benchmark::DoNotOptimize(contacted);
    }

    state.SetItemsProcessed(state.iterations());
    state.SetLabel("SUMMARY_FIRST_DISTRIBUTED | shards=32 | k=10");
}
BENCHMARK(BM_Query_Routing_SummaryFirst)->Unit(benchmark::kNanosecond);

static void BM_Query_Routing_ExactLoad(benchmark::State& state) {
    constexpr int NUM_SHARDS = 32;

    for (auto _ : state) {
        // Exact load: contact all shards.
        int contacted = NUM_SHARDS;
        benchmark::DoNotOptimize(contacted);
    }

    state.SetItemsProcessed(state.iterations());
    state.SetLabel("DISTRIBUTED_EXACT_LOAD | shards=32");
}
BENCHMARK(BM_Query_Routing_ExactLoad)->Unit(benchmark::kNanosecond);

// ============================================================================
// Graph validation simulation
// ============================================================================

/**
 * @brief ANN + Tensor + Graph validation benchmark.
 *
 * Simulates adding a graph-based evidence validation step after ANN+Tensor
 * retrieval.  Measures the extra overhead that `ANN_TENSOR_GRAPH` incurs
 * compared to `ANN_TENSOR`.
 */
static void BM_Graph_Validation(benchmark::State& state) {
    constexpr int N           = 512;
    constexpr int DEGREE      = 4;  // Average adjacency-list length.
    constexpr int K           = 10;

    // Build a random adjacency list.
    std::vector<std::vector<int>> adj(N);
    for (int i = 0; i < N; ++i) {
        for (int d = 0; d < DEGREE; ++d) {
            adj[i].push_back((i + d + 1) % N);
        }
    }

    // Fake candidates from a prior ANN step.
    std::vector<int> candidates(K);
    std::iota(candidates.begin(), candidates.end(), 0);

    for (auto _ : state) {
        int edge_count = syntheticGraphValidate(adj, candidates);
        benchmark::DoNotOptimize(edge_count);
    }

    state.SetItemsProcessed(state.iterations());
    state.SetLabel("ANN_TENSOR_GRAPH | n=512 | degree=4 | k=10");
}
BENCHMARK(BM_Graph_Validation)->Unit(benchmark::kNanosecond);

// ============================================================================
// LLM / LoRA simulation
// ============================================================================

/**
 * @brief LoRA inference overhead simulation (LORA_INFERENCE scenario).
 *
 * Models the adapter-application step: a simple vector-vector multiply of
 * rank-r weight matrices, representing the core LoRA forward computation.
 *
 * @note Simulation of LoRA adapter application without real model weights.
 *       Measures matrix-multiply overhead as a proxy for adapter cost.
 */
// STUB/SIMULATION NOTE:
// Purpose: Model LoRA adapter forward-pass overhead for CI benchmarks.
// Activation: Always active; no LLM runtime required.
// Production Delta: Real LoRA inference uses CUDA kernels or CPU BLAS.
// Removal Plan: Replace when LLM plugin infrastructure is available
//               (Target: v2.0.0 / Q4 2026).
static void BM_LLM_LoRA_Inference(benchmark::State& state) {
    constexpr int HIDDEN_DIM = 768;
    constexpr int LORA_RANK  = 8;

    // LoRA A: [hidden × rank], B: [rank × hidden]
    std::vector<float> A(HIDDEN_DIM * LORA_RANK);
    std::vector<float> B(LORA_RANK  * HIDDEN_DIM);
    std::vector<float> x(HIDDEN_DIM);
    std::vector<float> tmp(LORA_RANK);
    std::vector<float> out(HIDDEN_DIM);

    // Initialise with small values.
    for (int i = 0; i < HIDDEN_DIM * LORA_RANK; ++i) {
        A[i] = 0.01f * static_cast<float>(i % 17);
    }
    for (int i = 0; i < LORA_RANK * HIDDEN_DIM; ++i) {
        B[i] = 0.01f * static_cast<float>(i % 13);
    }
    for (int i = 0; i < HIDDEN_DIM; ++i) {
        x[i] = static_cast<float>(i) / HIDDEN_DIM;
    }

    for (auto _ : state) {
        // tmp = A^T * x  (LORA_RANK outputs)
        for (int r = 0; r < LORA_RANK; ++r) {
            float v = 0.f;
            for (int h = 0; h < HIDDEN_DIM; ++h) {
                v += A[static_cast<std::size_t>(h) * LORA_RANK + r] * x[h];
            }
            tmp[r] = v;
        }
        // out = B^T * tmp  (HIDDEN_DIM outputs)
        for (int h = 0; h < HIDDEN_DIM; ++h) {
            float v = 0.f;
            for (int r = 0; r < LORA_RANK; ++r) {
                v += B[static_cast<std::size_t>(r) * HIDDEN_DIM + h] * tmp[r];
            }
            out[h] = v;
        }
        benchmark::DoNotOptimize(out);
    }

    state.SetItemsProcessed(state.iterations());
    state.SetLabel("LORA_INFERENCE | hidden=768 | rank=8");
}
BENCHMARK(BM_LLM_LoRA_Inference)->Unit(benchmark::kMicrosecond);

// ============================================================================
// CPU/GPU break-even simulation
// ============================================================================

/**
 * @brief CPU baseline for GPU break-even comparison (CPU_ONLY scenario).
 *
 * Performs a batched dot-product computation on CPU.  The GPU_ACCELERATED
 * counterpart would run the same workload on CUDA and report the speedup
 * factor recorded in `BenchmarkDimension::GPU_SPEEDUP_FACTOR`.
 */
static void BM_BreakEven_CPU(benchmark::State& state) {
    const int batch_size = static_cast<int>(state.range(0));
    constexpr int DIM    = 256;

    std::vector<float> a(static_cast<std::size_t>(batch_size * DIM), 0.5f);
    std::vector<float> b(static_cast<std::size_t>(batch_size * DIM), 0.3f);
    std::vector<float> result(static_cast<std::size_t>(batch_size), 0.f);

    for (auto _ : state) {
        for (int i = 0; i < batch_size; ++i) {
            float dot = 0.f;
            for (int d = 0; d < DIM; ++d) {
                dot += a[static_cast<std::size_t>(i * DIM + d)] *
                       b[static_cast<std::size_t>(i * DIM + d)];
            }
            result[i] = dot;
        }
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(state.iterations() *
                             static_cast<int64_t>(batch_size));
    state.SetLabel("CPU_ONLY | batch=" + std::to_string(batch_size) +
                   " | dim=256");
}
BENCHMARK(BM_BreakEven_CPU)->RangeMultiplier(8)->Range(8, 2048)
    ->Unit(benchmark::kMicrosecond);

// ============================================================================
// Matrix population + query integrated smoke-test
// ============================================================================

/**
 * @brief End-to-end matrix smoke-test benchmark.
 *
 * Fills the full scenario × dimension matrix from synthetic measurements and
 * then exercises the planner-facing query paths (bestScenario,
 * compareScenarios, scenariosWithFullCoverage).  Ensures overall overhead is
 * negligible relative to the actual benchmark workloads.
 */
static void BM_Matrix_EndToEnd(benchmark::State& state) {
    for (auto _ : state) {
        BenchmarkMatrix m;
        fillFullMatrix(m);

        auto best = m.bestScenario(BenchmarkDimension::RECALL_AT_K, true);
        benchmark::DoNotOptimize(best);

        auto ratio = m.compareScenarios(BenchmarkScenario::HNSW_ANN_ONLY,
                                        BenchmarkScenario::DISKANN_ANN_ONLY,
                                        BenchmarkDimension::QPS);
        benchmark::DoNotOptimize(ratio);

        std::vector<BenchmarkDimension> required = {
            BenchmarkDimension::RECALL_AT_K,
            BenchmarkDimension::QUERY_LATENCY_MS,
        };
        auto covered = m.scenariosWithFullCoverage(required);
        benchmark::DoNotOptimize(covered);
    }
}
BENCHMARK(BM_Matrix_EndToEnd)->Unit(benchmark::kMicrosecond);
