/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            bench_olap_analytics.cpp                           ║
  Version:         0.0.37                                             ║
  Last Modified:   2026-04-06 04:03:45                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     34                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • a629043ab2  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include <benchmark/benchmark.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <numeric>
#include <random>
#include <unordered_map>
#include <utility>
#include <vector>

#if defined(_WIN32)
#include <windows.h>
#include <psapi.h>
#pragma comment(lib, "psapi.lib")
#endif

namespace {

double current_process_rss_mb() {
#if defined(_WIN32)
  PROCESS_MEMORY_COUNTERS_EX pmc{};
  if (GetProcessMemoryInfo(GetCurrentProcess(),
                           reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&pmc),
                           sizeof(pmc)) != 0) {
    return static_cast<double>(pmc.WorkingSetSize) / (1024.0 * 1024.0);
  }
#endif
  return 0.0;
}

std::vector<double> make_series(size_t count, uint32_t seed) {
  std::vector<double> values(count);
  std::mt19937 rng(seed);
  std::uniform_real_distribution<double> noise(-0.5, 0.5);
  for (size_t i = 0; i < count; ++i) {
    values[i] = 100.0 + 0.01 * static_cast<double>(i) + noise(rng);
  }
  return values;
}

// AN-2 proxy: apply batched deltas to a materialized aggregate state.
static void BM_OLAP_IVM_DeltaApply_10k(benchmark::State& state) {
  const int64_t rows = state.range(0);
  std::vector<double> base(static_cast<size_t>(rows), 100.0);
  std::vector<double> delta(static_cast<size_t>(rows), 0.0);

  std::mt19937 rng(42);
  std::uniform_real_distribution<double> upd(-5.0, 5.0);
  for (double& d : delta) {
    d = upd(rng);
  }

  for (auto _ : state) {
    double sum = 0.0;
    for (int64_t i = 0; i < rows; ++i) {
      base[static_cast<size_t>(i)] += delta[static_cast<size_t>(i)];
      sum += base[static_cast<size_t>(i)];
    }
    benchmark::DoNotOptimize(sum);
    benchmark::ClobberMemory();
  }

  state.SetItemsProcessed(state.iterations() * rows);
  state.SetLabel("AN-2_ivm_delta_apply");
}
BENCHMARK(BM_OLAP_IVM_DeltaApply_10k)->Arg(10000);

// AN-8 proxy: predictBatch for 1k series with horizon 30.
static void BM_OLAP_PredictBatch_1k30(benchmark::State& state) {
  const int64_t series_count = state.range(0);
  const int horizon = 30;
  const size_t history = 64;

  std::vector<std::vector<double>> series;
  series.reserve(static_cast<size_t>(series_count));
  for (int64_t i = 0; i < series_count; ++i) {
    series.push_back(make_series(history, static_cast<uint32_t>(100 + i)));
  }

  for (auto _ : state) {
    double checksum = 0.0;
    for (int64_t s = 0; s < series_count; ++s) {
      const auto& cur = series[static_cast<size_t>(s)];
      const double mean =
        std::accumulate(cur.begin(), cur.end(), 0.0) / static_cast<double>(cur.size());
      for (int h = 1; h <= horizon; ++h) {
        checksum += mean + 0.02 * static_cast<double>(h);
      }
    }
    benchmark::DoNotOptimize(checksum);
    benchmark::ClobberMemory();
  }

  state.SetItemsProcessed(state.iterations() * series_count * horizon);
  state.SetLabel("AN-8_predict_batch_1k_x_30");
}
BENCHMARK(BM_OLAP_PredictBatch_1k30)->Arg(1000);

// AN-9 proxy: evaluate a 3x3 grid and keep the best score.
static void BM_OLAP_AutoTune_Grid9(benchmark::State& state) {
  const int64_t sample_count = state.range(0);
  std::vector<double> samples = make_series(static_cast<size_t>(sample_count), 77);

  std::vector<std::pair<double, double>> grid;
  grid.reserve(9);
  for (double alpha : {0.10, 0.25, 0.50}) {
    for (double beta : {0.05, 0.15, 0.30}) {
      grid.emplace_back(alpha, beta);
    }
  }

  for (auto _ : state) {
    double best = std::numeric_limits<double>::infinity();
    for (const auto& cfg : grid) {
      const double alpha = cfg.first;
      const double beta = cfg.second;
      double error = 0.0;
      for (size_t i = 1; i < samples.size(); ++i) {
        const double pred = alpha * samples[i - 1] + beta * static_cast<double>(i % 7);
        const double diff = samples[i] - pred;
        error += diff * diff;
      }
      best = std::min(best, error);
    }
    benchmark::DoNotOptimize(best);
    benchmark::ClobberMemory();
  }

  state.SetItemsProcessed(state.iterations() * sample_count * static_cast<int64_t>(grid.size()));
  state.SetLabel("AN-9_autotune_grid_3x3");
}
BENCHMARK(BM_OLAP_AutoTune_Grid9)->Arg(500);

// AN-1 proxy: streaming window aggregation with bounded memory footprint.
static void BM_OLAP_StreamingWindow_Aggregation(benchmark::State& state) {
  const int64_t rows = state.range(0);
  const size_t window = static_cast<size_t>(std::min<int64_t>(rows, 2048));
  std::vector<double> values = make_series(static_cast<size_t>(rows), 314);
  std::vector<double> ring(window, 0.0);
  double peak_rss_mb = current_process_rss_mb();

  for (auto _ : state) {
    double running_sum = 0.0;
    double sink = 0.0;
    size_t idx = 0;
    for (int64_t i = 0; i < rows; ++i) {
      running_sum -= ring[idx];
      ring[idx] = values[static_cast<size_t>(i)];
      running_sum += ring[idx];
      idx = (idx + 1) % window;
      sink += running_sum / static_cast<double>(window);
    }
    benchmark::DoNotOptimize(sink);
    benchmark::ClobberMemory();

    const double rss_now = current_process_rss_mb();
    if (rss_now > peak_rss_mb) {
      peak_rss_mb = rss_now;
    }
  }

  state.SetItemsProcessed(state.iterations() * rows);
  state.SetLabel("AN-1_streaming_window_memory");
  state.counters["peak_rss_mb"] = peak_rss_mb;
}
BENCHMARK(BM_OLAP_StreamingWindow_Aggregation)
  ->Arg(1000)
  ->Arg(10000)
  ->Arg(100000);

// AN-5 proxy: CEP stop lifecycle latency under loaded state.
static void BM_OLAP_CEP_Stop_Lifecycle(benchmark::State& state) {
  const int64_t pending_events = state.range(0);
  std::vector<double> event_buffer(static_cast<size_t>(pending_events), 0.0);
  std::iota(event_buffer.begin(), event_buffer.end(), 1.0);

  for (auto _ : state) {
    // Simulate CEP shutdown drain/flush pass over pending state.
    double flushed = 0.0;
    for (double& e : event_buffer) {
      flushed += e;
      e = 0.0;
    }
    benchmark::DoNotOptimize(flushed);
    benchmark::ClobberMemory();

    // Re-arm buffer so each iteration has comparable stop work.
    std::iota(event_buffer.begin(), event_buffer.end(), 1.0);
  }

  state.SetItemsProcessed(state.iterations() * pending_events);
  state.SetLabel("AN-5_cep_stop_lifecycle");
}
BENCHMARK(BM_OLAP_CEP_Stop_Lifecycle)->Arg(10000);

// AN-7 proxy: IsolationForest-like training over a 1k-point window.
static void BM_OLAP_IsolationForest_Training_1k(benchmark::State& state) {
  const int64_t points = state.range(0);
  std::vector<double> values = make_series(static_cast<size_t>(points), 991);

  for (auto _ : state) {
    std::mt19937 rng(123);
    std::uniform_int_distribution<int64_t> split_dist(1, points - 2);

    // Lightweight tree ensemble construction approximation.
    double model_score = 0.0;
    constexpr int trees = 64;
    for (int t = 0; t < trees; ++t) {
      const int64_t split = split_dist(rng);
      double left_mean = 0.0;
      double right_mean = 0.0;
      for (int64_t i = 0; i < split; ++i) {
        left_mean += values[static_cast<size_t>(i)];
      }
      for (int64_t i = split; i < points; ++i) {
        right_mean += values[static_cast<size_t>(i)];
      }
      left_mean /= static_cast<double>(split);
      right_mean /= static_cast<double>(points - split);
      model_score += std::abs(left_mean - right_mean);
    }

    benchmark::DoNotOptimize(model_score);
    benchmark::ClobberMemory();
  }

  state.SetItemsProcessed(state.iterations() * points);
  state.SetLabel("AN-7_isolation_forest_training_1k");
}
BENCHMARK(BM_OLAP_IsolationForest_Training_1k)->Arg(1000);

} // namespace

BENCHMARK_MAIN();
