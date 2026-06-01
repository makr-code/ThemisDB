/**
 * @file benchmark_matrix.h
 * @brief Benchmark matrix — scenario definitions for retrieval and evaluation.
 *
 * Defines the benchmark scenario set for measuring throughput, latency, and
 * recall across ANN backends, tensor mid-layer, graph validation, and the
 * hybrid query planner.
 *
 * Planned in: docs/EPIC2_BENCHMARK_FRAMEWORK.md
 * Sub-issue:   #5438
 */

#pragma once

#include "hardware_profile.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace themis::evaluation {

/// Category of a benchmark scenario.
enum class BenchmarkCategory {
    AnnLatency,       ///< Single-query ANN latency
    AnnThroughput,    ///< Sustained QPS under load
    RecallAtK,        ///< Recall@k accuracy measurement
    TensorRerank,     ///< Tensor mid-layer reranking overhead
    GraphValidation,  ///< Graph truth layer overhead
    EndToEnd,         ///< Full retrieval pipeline latency
    QueryPlanner,     ///< Planner routing decision latency
};

/// A single benchmark scenario definition.
struct BenchmarkScenario {
    std::string       id;
    std::string       description;
    BenchmarkCategory category;
    std::uint64_t     dataset_vectors = 1'000'000;
    std::uint32_t     query_count     = 10'000;
    std::uint32_t     top_k           = 10;
    float             target_recall   = 0.95f;
    double            target_latency_p99_ms = 20.0;
    double            target_qps      = 0.0; ///< 0 = not latency-bound
    std::vector<std::string> hardware_profile_ids; ///< Empty = all profiles
};

/// Results produced by running a benchmark scenario.
struct BenchmarkResult {
    std::string  scenario_id;
    std::string  hardware_profile_id;
    double       latency_p50_ms  = 0.0;
    double       latency_p99_ms  = 0.0;
    double       qps             = 0.0;
    float        recall_at_k     = 0.0f;
    bool         passed          = false;
    std::string  failure_reason;
};

/**
 * @brief Benchmark matrix runner.
 *
 * Manages the full scenario catalogue, executes scenarios against the live
 * retrieval stack, and persists results for regression tracking.
 */
class IBenchmarkMatrix {
public:
    virtual ~IBenchmarkMatrix() = default;

    /// Register a scenario.
    virtual void registerScenario(BenchmarkScenario scenario) = 0;

    /// Run a single scenario and return its result.
    virtual BenchmarkResult run(const std::string& scenario_id,
                                 const HardwareProfile& hw) = 0;

    /// Run all registered scenarios and return the full result set.
    virtual std::vector<BenchmarkResult> runAll(const HardwareProfile& hw) = 0;

    /// List registered scenario IDs.
    virtual std::vector<std::string> listScenarios() const = 0;

    /// Register a result observer called after each scenario completes.
    using ResultObserver = std::function<void(const BenchmarkResult&)>;
    virtual void onResult(ResultObserver obs) = 0;
};

/// Factory: create a BenchmarkMatrix pre-populated with the EPIC 2 scenario set.
std::unique_ptr<IBenchmarkMatrix> makeBenchmarkMatrix();

} // namespace themis::evaluation
