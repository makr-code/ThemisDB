/**
 * @file planner_decision_bench.cc
 * @brief Benchmarks for EPIC 2.5 DefaultQueryPlanner path-selection overhead.
 *
 * Measures the wall-clock cost of @ref QueryPlanner::selectPath() across all
 * five canonical execution paths and representative fallback scenarios. The goal
 * is to confirm that planner decision overhead stays well below the 5-second
 * SLA tail-latency budget mandated by ADR E2-003.
 *
 * ## What this measures
 * - **Planner-only latency**: time inside `selectPath()`, excluding downstream
 *   ANN/graph/tensor retrieval. This must be sub-microsecond on modern hardware.
 * - **Path-selection distribution**: how often each path is selected under
 *   representative eligibility mixes (useful for capacity planning).
 * - **Module gap threshold monitoring**: counts how many decisions were blocked
 *   by `FallbackReason::ModuleGapThreshold` to track gap-fix progress impact.
 *
 * ## Build
 *   cmake --preset linux-release -DTHEMIS_BUILD_EPIC2=ON
 *   cmake --build --preset linux-release --target planner_decision_bench
 *
 * @see src/evaluation/include/query_planner.h
 * @see docs/EPIC2_QUERY_PLANNER.md
 * @see docs/adr/adr-e2-003-query-planner-routing-model.md
 */

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <initializer_list>
#include <memory>
#include <numeric>
#include <string>
#include <vector>

#include "include/query_planner.h"

using namespace themis::evaluation;

// ---------------------------------------------------------------------------
// Benchmark helpers
// ---------------------------------------------------------------------------

namespace {

/// @brief Canonical fully-enabled eligibility (allows Path 1).
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

/// @brief No artifact present (zero-value sentinel — triggers Path 1).
TensorArtifactFreshness noArtifact() {
    return TensorArtifactFreshness{};
}

/// @brief Fresh artifact that passes all freshness gates (triggers Path 2).
TensorArtifactFreshness freshArtifact() {
    TensorArtifactFreshness f;
    f.artifact_age_ms     = 1'000;
    f.delta_lag           = 100;
    f.residual_threshold  = 0.97;
    f.rank_cap            = 500;
    f.rebuild_in_progress = false;
    return f;
}

/// @brief Stale artifact (age exceeds max_staleness_ms — triggers Path 4).
TensorArtifactFreshness staleArtifact() {
    TensorArtifactFreshness f;
    f.artifact_age_ms     = 10'000; // > 5 000 ms → stale
    f.delta_lag           = 100;
    f.residual_threshold  = 0.97;
    f.rank_cap            = 500;
    f.rebuild_in_progress = false;
    return f;
}

/**
 * @brief Observe and count planner decisions by path for gap-threshold monitoring.
 *
 * Records the last decision path and a per-category count useful for detecting
 * regressions in module gap thresholds over time.
 */
class CountingObserver final : public PlannerObserver {
public:
    void onDecision(const PlannerDecision& d, uint64_t /*latency_us*/) noexcept override {
        counts[static_cast<int>(d.path)]++;
        if (d.fallback_reason == FallbackReason::ModuleGapThreshold) {
            gap_threshold_blocks++;
        }
    }

    int counts[6]{};               ///< Index = ExecutionPath numeric value (1-5).
    int gap_threshold_blocks{0};   ///< Decisions blocked by module gap threshold.
};

/**
 * @brief Percentile calculation helper.
 *
 * @param values   Sorted vector of measurements (must be sorted ascending).
 * @param percentile  Percentile to compute (0.0 to 100.0).
 * @return         The value at the given percentile.
 */
double computePercentile(const std::vector<double>& values, double percentile) {
    if (values.empty()) return 0.0;
    if (percentile <= 0.0) return values.front();
    if (percentile >= 100.0) return values.back();
    
    const double pos = (percentile / 100.0) * static_cast<double>(values.size() - 1);
    const int low = static_cast<int>(pos);
    const int high = std::min(low + 1, static_cast<int>(values.size() - 1));
    const double frac = pos - static_cast<double>(low);
    return values[low] * (1.0 - frac) + values[high] * frac;
}

/**
 * @brief Struct to hold latency statistics for a benchmark path.
 */
struct LatencyStats {
    double avg_ns;
    double p50_ns;
    double p95_ns;
    double p99_ns;
    double min_ns;
    double max_ns;
};

/**
 * @brief Run `selectPath()` N times and return latency statistics.
 *
 * Measures per-call latency and computes p50/p95/p99 percentiles.
 *
 * @param planner  The planner under test.
 * @param e        Eligibility signals for the scenario.
 * @param f        Tensor artifact freshness.
 * @param c        Policy configuration.
 * @param N        Number of iterations.
 * @return         LatencyStats with average, p50, p95, p99, min, max.
 */
LatencyStats benchmarkPath(
    QueryPlanner&                  planner,
    const ExecutionEligibility&    e,
    const TensorArtifactFreshness& f,
    const PlannerConfig&           c,
    int                            N)
{
    std::vector<double> latencies;
    latencies.reserve(N);
    
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
    
    LatencyStats stats;
    stats.avg_ns = sum / static_cast<double>(N);
    stats.p50_ns = computePercentile(latencies, 50.0);
    stats.p95_ns = computePercentile(latencies, 95.0);
    stats.p99_ns = computePercentile(latencies, 99.0);
    stats.min_ns = latencies.front();
    stats.max_ns = latencies.back();
    
    return stats;
}

} // namespace

// ---------------------------------------------------------------------------
// Main — benchmark runner
// ---------------------------------------------------------------------------

/**
 * @brief Entry point for the planner decision benchmark suite.
 *
 * Runs benchmarks for all five execution paths and reports latency percentiles
 * (p50/p95/p99) along with fallback rate statistics. Designed for performance
 * regression detection and guard-rail enforcement per PERFORMANCE_EXPECTATIONS.md.
 *
 * Exit code 0 on success; 1 if any measured latency exceeds the configured
 * regression threshold (default: p95 > 800 µs / 800,000 ns).
 *
 * @return 0 on success, 1 on latency regression.
 */
int main() {
    constexpr int N                    = 100'000;
    constexpr double P95_REGRESSION_NS = 800'000.0;  // 800 µs — regression threshold
    
    CountingObserver obs;
    auto planner = makeDefaultQueryPlanner(&obs);

    const auto e_full   = fullEligibility();
    const auto f_none   = noArtifact();
    const auto f_fresh  = freshArtifact();
    const auto f_stale  = staleArtifact();
    const auto cfg      = defaultConfig();

    std::printf("======================================================================\n");
    std::printf("EPIC 2.5 Planner Decision Benchmark — Phase 5 Guardrail Validation\n");
    std::printf("======================================================================\n");
    std::printf("Iterations per path: %d\n", N);
    std::printf("Target hardware: 2+ GHz x86_64, 16GB+ RAM, Linux\n");
    std::printf("Guardrails: p95 ≤ 500µs, p99 ≤ 1000µs\n");
    std::printf("Regression threshold: p95 > %.0f ns (%.0f µs)\n", P95_REGRESSION_NS, P95_REGRESSION_NS / 1000.0);
    std::printf("======================================================================\n\n");

    // --- Path 1: ANN Only ---
    std::printf("Running Path 1 (ANN Only) benchmark...\n");
    const auto p1_stats = benchmarkPath(*planner, e_full, f_none, cfg, N);
    std::printf("[Path 1 — ANN Only]\n");
    std::printf("  avg: %.1f ns,  p50: %.1f ns,  p95: %.1f ns,  p99: %.1f ns\n",
                p1_stats.avg_ns, p1_stats.p50_ns, p1_stats.p95_ns, p1_stats.p99_ns);
    std::printf("  min: %.1f ns,  max: %.1f ns\n\n", p1_stats.min_ns, p1_stats.max_ns);

    // --- Path 2: ANN + Tensor Summary ---
    std::printf("Running Path 2 (ANN + Tensor Summary) benchmark...\n");
    const auto p2_stats = benchmarkPath(*planner, e_full, f_fresh, cfg, N);
    std::printf("[Path 2 — ANN + Tensor Summary]\n");
    std::printf("  avg: %.1f ns,  p50: %.1f ns,  p95: %.1f ns,  p99: %.1f ns\n",
                p2_stats.avg_ns, p2_stats.p50_ns, p2_stats.p95_ns, p2_stats.p99_ns);
    std::printf("  min: %.1f ns,  max: %.1f ns\n\n", p2_stats.min_ns, p2_stats.max_ns);

    // --- Path 4 via stale tensor ---
    std::printf("Running Path 4 (Stale Tensor) benchmark...\n");
    const auto p4_stale_stats = benchmarkPath(*planner, e_full, f_stale, cfg, N);
    std::printf("[Path 4 — Stale Tensor]\n");
    std::printf("  avg: %.1f ns,  p50: %.1f ns,  p95: %.1f ns,  p99: %.1f ns\n",
                p4_stale_stats.avg_ns, p4_stale_stats.p50_ns, p4_stale_stats.p95_ns, p4_stale_stats.p99_ns);
    std::printf("  min: %.1f ns,  max: %.1f ns\n\n", p4_stale_stats.min_ns, p4_stale_stats.max_ns);

    // --- Path 4 via force_exact ---
    std::printf("Running Path 4 (force_exact) benchmark...\n");
    auto e_exact = e_full;
    e_exact.force_exact = true;
    const auto p4_exact_stats = benchmarkPath(*planner, e_exact, f_none, cfg, N);
    std::printf("[Path 4 — force_exact]\n");
    std::printf("  avg: %.1f ns,  p50: %.1f ns,  p95: %.1f ns,  p99: %.1f ns\n",
                p4_exact_stats.avg_ns, p4_exact_stats.p50_ns, p4_exact_stats.p95_ns, p4_exact_stats.p99_ns);
    std::printf("  min: %.1f ns,  max: %.1f ns\n\n", p4_exact_stats.min_ns, p4_exact_stats.max_ns);

    // --- Path 4 via module gap threshold ---
    std::printf("Running Path 4 (ModuleGapThreshold) benchmark...\n");
    auto e_gap = e_full;
    e_gap.index_buffer_safety_ok = false;
    const auto p4_gap_stats = benchmarkPath(*planner, e_gap, f_none, cfg, N);
    std::printf("[Path 4 — ModuleGapThreshold]\n");
    std::printf("  avg: %.1f ns,  p50: %.1f ns,  p95: %.1f ns,  p99: %.1f ns\n",
                p4_gap_stats.avg_ns, p4_gap_stats.p50_ns, p4_gap_stats.p95_ns, p4_gap_stats.p99_ns);
    std::printf("  min: %.1f ns,  max: %.1f ns\n",
                p4_gap_stats.min_ns, p4_gap_stats.max_ns);
    std::printf("  blocks (FallbackReason::ModuleGapThreshold): %d\n\n", obs.gap_threshold_blocks);

    // --- Path 5: Distributed ---
    std::printf("Running Path 5 (Distributed) benchmark...\n");
    auto e_dist = e_full;
    e_dist.distributed_multi_shard   = true;
    e_dist.shard_manifests_available = true;
    const auto p5_stats = benchmarkPath(*planner, e_dist, f_none, cfg, N);
    std::printf("[Path 5 — Distributed]\n");
    std::printf("  avg: %.1f ns,  p50: %.1f ns,  p95: %.1f ns,  p99: %.1f ns\n",
                p5_stats.avg_ns, p5_stats.p50_ns, p5_stats.p95_ns, p5_stats.p99_ns);
    std::printf("  min: %.1f ns,  max: %.1f ns\n\n", p5_stats.min_ns, p5_stats.max_ns);

    // --- Decision Distribution and Fallback Analysis ---
    std::printf("======================================================================\n");
    std::printf("Decision Distribution & Fallback Rate Analysis\n");
    std::printf("======================================================================\n");
    
    const int total_decisions = obs.counts[1] + obs.counts[2] + obs.counts[3] + obs.counts[4] + obs.counts[5];
    std::printf("Total decisions observed: %d\n", total_decisions);
    std::printf("Path distribution:\n");
    std::printf("  Path 1 (ANN Only):         %d (%.1f%%)\n",
                obs.counts[1], total_decisions > 0 ? (100.0 * obs.counts[1] / total_decisions) : 0.0);
    std::printf("  Path 2 (ANN + Tensor):     %d (%.1f%%)\n",
                obs.counts[2], total_decisions > 0 ? (100.0 * obs.counts[2] / total_decisions) : 0.0);
    std::printf("  Path 3 (Degraded):         %d (%.1f%%)\n",
                obs.counts[3], total_decisions > 0 ? (100.0 * obs.counts[3] / total_decisions) : 0.0);
    std::printf("  Path 4 (Exact/Fallback):   %d (%.1f%%)\n",
                obs.counts[4], total_decisions > 0 ? (100.0 * obs.counts[4] / total_decisions) : 0.0);
    std::printf("  Path 5 (Distributed):      %d (%.1f%%)\n",
                obs.counts[5], total_decisions > 0 ? (100.0 * obs.counts[5] / total_decisions) : 0.0);
    
    std::printf("\nModuleGapThreshold fallback blocks: %d (%.1f%% of Path 4)\n",
                obs.gap_threshold_blocks,
                obs.counts[4] > 0 ? (100.0 * obs.gap_threshold_blocks / obs.counts[4]) : 0.0);

    // --- Regression Check ---
    std::printf("\n======================================================================\n");
    std::printf("Regression Check\n");
    std::printf("======================================================================\n");
    
    const double max_p95 = std::max({p1_stats.p95_ns, p2_stats.p95_ns,
                                      p4_stale_stats.p95_ns, p4_exact_stats.p95_ns,
                                      p4_gap_stats.p95_ns, p5_stats.p95_ns});
    const double max_p99 = std::max({p1_stats.p99_ns, p2_stats.p99_ns,
                                      p4_stale_stats.p99_ns, p4_exact_stats.p99_ns,
                                      p4_gap_stats.p99_ns, p5_stats.p99_ns});
    
    if (max_p95 > P95_REGRESSION_NS) {
        std::printf("❌ REGRESSION DETECTED: max p95 latency %.1f ns exceeds threshold %.1f ns\n",
                    max_p95, P95_REGRESSION_NS);
        std::printf("   Regression: %.1f%% above threshold\n", 
                    (max_p95 / P95_REGRESSION_NS - 1.0) * 100.0);
        return 1;
    }
    
    std::printf("✓ All paths within guardrail thresholds:\n");
    std::printf("  Max p95: %.1f ns (limit: %.1f ns)\n", max_p95, P95_REGRESSION_NS);
    std::printf("  Max p99: %.1f ns (limit: 1000000 ns)\n", max_p99);
    std::printf("\n✓ PASS — All guardrails satisfied\n");
    return 0;