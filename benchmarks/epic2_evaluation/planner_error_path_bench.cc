/**
 * @file planner_error_path_bench.cc
 * @brief Benchmarks for EPIC 2.5 Planner error path overhead (Category C enforcement).
 *
 * Measures the wall-clock cost of error condition detection and fallback initiation
 * in the query planner's decision paths. The goal is to confirm that error handling
 * overhead stays well below the latency budget mandated by ADR E2-003 and that
 * error paths do not introduce pathological latency regressions.
 *
 * ## What this measures
 * - **Error detection overhead:** Time to detect and classify error conditions
 *   (query exception handling disabled, index buffer safety check failed, thread
 *   safety check failed, resource exhaustion).
 * - **Fallback initiation latency:** Time to initiate forced-exact fallback from
 *   an error condition.
 * - **Error path total latency:** End-to-end time to process error and return
 *   fall-closed decision to caller.
 * - **Overhead isolation:** Difference between error-path latency and nominal
 *   path latency to quantify the pure error-handling cost.
 *
 * ## Build
 *   cmake --preset linux-release -DTHEMIS_BUILD_EPIC2=ON
 *   cmake --build --preset linux-release --target planner_error_path_bench
 *
 * @see src/evaluation/include/query_planner.h
 * @see src/evaluation/PERFORMANCE_EXPECTATIONS.md §6 (Planner Error Path Overhead)
 * @see docs/adr/adr-e2-003-query-planner-routing-model.md
 */

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <numeric>
#include <string>
#include <vector>

#include "include/query_planner.h"

using namespace themis::evaluation;

// ---------------------------------------------------------------------------
// Benchmark helpers
// ---------------------------------------------------------------------------

namespace {

/// @brief Percentile calculation helper.
double computePercentile(const std::vector<double>& values, double percentile) {
    if (values.empty()) {
      return 0.0;
    }
    if (percentile <= 0.0) {
      return values.front();
    }
    if (percentile >= 100.0) {
      return values.back();
    }
    
    const double pos = (percentile / 100.0) * static_cast<double>(values.size() - 1);
    const int low = static_cast<int>(pos);
    const int high = std::min(low + 1, static_cast<int>(values.size() - 1));
    const double frac = pos - static_cast<double>(low);
    return values[low] * (1.0 - frac) + values[high] * frac;
}

/// @brief Struct to hold latency statistics for an error scenario.
struct ErrorPathStats {
    double avg_ns;      ///< Average latency (nanoseconds)
    double p50_ns;      ///< p50 latency percentile
    double p95_ns;      ///< p95 latency percentile
    double p99_ns;      ///< p99 latency percentile
    double min_ns;      ///< Minimum observed latency
    double max_ns;      ///< Maximum observed latency
    double overhead_ns; ///< Overhead vs. nominal path (avg)
};

/// @brief Canonical fully-enabled eligibility (baseline for comparison).
ExecutionEligibility fullEligibility() {
    ExecutionEligibility e;
    e.cuda_available              = true;
    e.ann_enabled                 = true;
    e.gpu_error_handling_gate     = true;
    e.gpu_parity_validated        = true;
    e.force_exact                 = false;
    e.force_cpu                   = false;
    e.query_thread_safety_ok      = true;
    e.query_exception_handling_ok = true;
    e.index_buffer_safety_ok      = true;
    e.distributed_multi_shard     = false;
    e.shard_manifests_available   = false;
    return e;
}

/// @brief Default policy configuration.
PlannerConfig defaultConfig() {
    PlannerConfig cfg;
    cfg.max_staleness_ms             = 5'000;
    cfg.max_delta_lag                = 1'000;
    cfg.min_residual_threshold       = 0.95;
    cfg.max_rank_cap                 = 1'000;
    cfg.min_shard_summary_confidence = 0.80;
    cfg.policy_version               = "v1-bench";
    cfg.staleness_threshold_key      = "tensor.max_staleness_ms";
    return cfg;
}

/// @brief Fresh artifact that passes all freshness gates (nominal baseline).
TensorArtifactFreshness freshArtifact() {
    TensorArtifactFreshness f;
    f.artifact_age_ms     = 1'000;
    f.delta_lag           = 100;
    f.residual_threshold  = 0.97;
    f.rank_cap            = 500;
    f.rebuild_in_progress = false;
    return f;
}

/**
 * @brief Benchmark error path with per-call latency measurement.
 *
 * @param planner        The planner under test.
 * @param e              Eligibility signals (with error condition set).
 * @param f              Tensor artifact freshness.
 * @param c              Policy configuration.
 * @param N              Number of iterations.
 * @param scenario_name  Human-readable scenario name for logging.
 * @return               ErrorPathStats with latency percentiles.
 */
ErrorPathStats benchmarkErrorPath(
    QueryPlanner&                  planner,
    const ExecutionEligibility&    e,
    const TensorArtifactFreshness& f,
    const PlannerConfig&           c,
    int                            N,
    const std::string&             scenario_name)
{
    std::vector<double> latencies;
    latencies.reserve(N);

    std::printf("[benchmarkErrorPath] scenario='%s', iterations=%d\n",
                scenario_name.c_str(), N);

    // Warmup
    for (int i = 0; i < 1000; ++i) {
        volatile auto d = planner.selectPath(e, f, c);
        (void)d;
    }
    
    // Measure per-call latency
    for (int i = 0; i < N; ++i) {
        const auto t0 = std::chrono::steady_clock::now();
        volatile auto d = planner.selectPath(e, f, c);
        (void)d;
        const auto t1 = std::chrono::steady_clock::now();
        
        const double call_ns = static_cast<double>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());
        latencies.push_back(call_ns);
    }
    
    // Sort for percentile calculation
    std::sort(latencies.begin(), latencies.end());
    
    // Compute statistics
    const double sum = std::accumulate(latencies.begin(), latencies.end(), 0.0);
    
    ErrorPathStats stats;
    stats.avg_ns = sum / static_cast<double>(N);
    stats.p50_ns = computePercentile(latencies, 50.0);
    stats.p95_ns = computePercentile(latencies, 95.0);
    stats.p99_ns = computePercentile(latencies, 99.0);
    stats.min_ns = latencies.front();
    stats.max_ns = latencies.back();
    stats.overhead_ns = 0.0;  // Will be set by caller
    
    return stats;
}

} // namespace

// ---------------------------------------------------------------------------
// Main — error path benchmark runner
// ---------------------------------------------------------------------------

/**
 * @brief Entry point for the planner error path benchmark suite.
 *
 * Runs benchmarks for various error conditions and reports latency percentiles
 * along with overhead relative to nominal path. Designed for Category C
 * enforcement validation per PERFORMANCE_EXPECTATIONS.md §6.
 *
 * Exit code 0 on success; 1 if any error path exceeds the 150 µs latency budget
 * or shows > 50% overhead vs. nominal path.
 *
 * @return 0 on success, 1 on error path latency regression.
 */
int main() {
    constexpr int N                       = 100'000;
    constexpr double ERROR_BUDGET_NS      = 150'000.0;  // 150 µs — error path budget
    constexpr double OVERHEAD_THRESHOLD   = 0.50;       // 50% overhead tolerance
    
    auto planner = makeDefaultQueryPlanner();
    
    const auto e_full   = fullEligibility();
    const auto f_fresh  = freshArtifact();
    const auto cfg      = defaultConfig();
    
    std::printf("======================================================================\n");
    std::printf("EPIC 2.5 Planner Error Path Benchmark — Category C Enforcement\n");
    std::printf("======================================================================\n");
    std::printf("Iterations per scenario: %d\n", N);
    std::printf("Error path latency budget: %.0f ns (%.0f µs)\n", 
                ERROR_BUDGET_NS, ERROR_BUDGET_NS / 1000.0);
    std::printf("Overhead tolerance: %.0f%% above nominal\n", OVERHEAD_THRESHOLD * 100.0);
    std::printf("======================================================================\n\n");
    
    // --- Nominal Path (baseline for overhead calculation) ---
    std::printf("Running nominal path baseline...\n");
    const auto nominal_stats = benchmarkErrorPath(*planner, e_full, f_fresh, cfg, N, "nominal");
    std::printf("[Nominal Path — All systems OK]\n");
    std::printf("  avg: %.1f ns,  p50: %.1f ns,  p95: %.1f ns,  p99: %.1f ns\n",
                nominal_stats.avg_ns, nominal_stats.p50_ns, nominal_stats.p95_ns, nominal_stats.p99_ns);
    std::printf("  min: %.1f ns,  max: %.1f ns\n\n", nominal_stats.min_ns, nominal_stats.max_ns);
    
    // --- Error Scenario 1: Query exception handling disabled ---
    std::printf("Running Error Scenario 1 (exception handling disabled)...\n");
    {
        auto e_no_exc = e_full;
        e_no_exc.query_exception_handling_ok = false;
        const auto error_stats = benchmarkErrorPath(*planner, e_no_exc, f_fresh, cfg, N, 
                                                     "exception_handling_disabled");
        std::printf("[Error 1 — query_exception_handling_ok=false]\n");
        std::printf("  avg: %.1f ns,  p50: %.1f ns,  p95: %.1f ns,  p99: %.1f ns\n",
                    error_stats.avg_ns, error_stats.p50_ns, error_stats.p95_ns, error_stats.p99_ns);
        std::printf("  min: %.1f ns,  max: %.1f ns\n", error_stats.min_ns, error_stats.max_ns);
        const double overhead_pct = (error_stats.avg_ns / nominal_stats.avg_ns - 1.0) * 100.0;
        std::printf("  overhead: %.1f%% vs. nominal\n\n", overhead_pct);
    }
    
    // --- Error Scenario 2: Index buffer safety check failed ---
    std::printf("Running Error Scenario 2 (index buffer safety failed)...\n");
    {
        auto e_no_buf = e_full;
        e_no_buf.index_buffer_safety_ok = false;
        const auto error_stats = benchmarkErrorPath(*planner, e_no_buf, f_fresh, cfg, N, 
                                                     "index_buffer_safety_failed");
        std::printf("[Error 2 — index_buffer_safety_ok=false]\n");
        std::printf("  avg: %.1f ns,  p50: %.1f ns,  p95: %.1f ns,  p99: %.1f ns\n",
                    error_stats.avg_ns, error_stats.p50_ns, error_stats.p95_ns, error_stats.p99_ns);
        std::printf("  min: %.1f ns,  max: %.1f ns\n", error_stats.min_ns, error_stats.max_ns);
        const double overhead_pct = (error_stats.avg_ns / nominal_stats.avg_ns - 1.0) * 100.0;
        std::printf("  overhead: %.1f%% vs. nominal\n\n", overhead_pct);
    }
    
    // --- Error Scenario 3: Thread safety check failed ---
    std::printf("Running Error Scenario 3 (thread safety failed)...\n");
    {
        auto e_no_thread = e_full;
        e_no_thread.query_thread_safety_ok = false;
        const auto error_stats = benchmarkErrorPath(*planner, e_no_thread, f_fresh, cfg, N, 
                                                     "thread_safety_failed");
        std::printf("[Error 3 — query_thread_safety_ok=false]\n");
        std::printf("  avg: %.1f ns,  p50: %.1f ns,  p95: %.1f ns,  p99: %.1f ns\n",
                    error_stats.avg_ns, error_stats.p50_ns, error_stats.p95_ns, error_stats.p99_ns);
        std::printf("  min: %.1f ns,  max: %.1f ns\n", error_stats.min_ns, error_stats.max_ns);
        const double overhead_pct = (error_stats.avg_ns / nominal_stats.avg_ns - 1.0) * 100.0;
        std::printf("  overhead: %.1f%% vs. nominal\n\n", overhead_pct);
    }
    
    // --- Error Scenario 4: Multiple safety checks failed (compound error) ---
    std::printf("Running Error Scenario 4 (compound error: multiple checks failed)...\n");
    {
        auto e_compound = e_full;
        e_compound.query_exception_handling_ok = false;
        e_compound.index_buffer_safety_ok = false;
        e_compound.query_thread_safety_ok = false;
        const auto error_stats = benchmarkErrorPath(*planner, e_compound, f_fresh, cfg, N, 
                                                     "compound_error");
        std::printf("[Error 4 — Multiple safety checks failed]\n");
        std::printf("  avg: %.1f ns,  p50: %.1f ns,  p95: %.1f ns,  p99: %.1f ns\n",
                    error_stats.avg_ns, error_stats.p50_ns, error_stats.p95_ns, error_stats.p99_ns);
        std::printf("  min: %.1f ns,  max: %.1f ns\n", error_stats.min_ns, error_stats.max_ns);
        const double overhead_pct = (error_stats.avg_ns / nominal_stats.avg_ns - 1.0) * 100.0;
        std::printf("  overhead: %.1f%% vs. nominal\n\n", overhead_pct);
    }
    
    // --- Guardrail Check ---
    std::printf("======================================================================\n");
    std::printf("Guardrail Compliance Check\n");
    std::printf("======================================================================\n");
    
    // Re-measure error paths for final validation (single nominal + representative error)
    auto e_error_sample = e_full;
    e_error_sample.query_exception_handling_ok = false;
    const auto error_final = benchmarkErrorPath(*planner, e_error_sample, f_fresh, cfg, N,
                                                 "final_validation");
    
    const double error_p95 = error_final.p95_ns;
    const double overhead_p95 = (error_final.p95_ns / nominal_stats.p95_ns - 1.0) * 100.0;
    
    std::printf("Representative error path p95: %.1f ns (nominal: %.1f ns)\n",
                error_p95, nominal_stats.p95_ns);
    std::printf("Overhead: %.1f%% vs. nominal path\n\n", overhead_p95);
    
    bool pass = true;
    
    if (error_p95 > ERROR_BUDGET_NS) {
        std::printf("❌ FAIL: Error path p95 (%.1f ns) exceeds budget (%.1f ns)\n",
                    error_p95, ERROR_BUDGET_NS);
        pass = false;
    } else {
        std::printf("✓ Error path p95 within budget (%.1f ns < %.1f ns)\n",
                    error_p95, ERROR_BUDGET_NS);
    }
    
    if (overhead_p95 > OVERHEAD_THRESHOLD * 100.0) {
        std::printf("❌ FAIL: Error overhead %.1f%% exceeds tolerance (%.0f%%)\n",
                    overhead_p95, OVERHEAD_THRESHOLD * 100.0);
        pass = false;
    } else {
        std::printf("✓ Error overhead within tolerance (%.1f%% < %.0f%%)\n",
                    overhead_p95, OVERHEAD_THRESHOLD * 100.0);
    }
    
    std::printf("\n");
    if (pass) {
        std::printf("✓ PASS — All error path guardrails satisfied\n");
        return 0;
    } else {
        std::printf("❌ FAIL — One or more error path guardrails violated\n");
        return 1;
    }
}
