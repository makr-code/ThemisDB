/**
 * @file bench_fts_phase_b.cpp
 * @brief Release-gate benchmark for Query/RAG FTS phrase and proximity queries.
 *
 * Measures p50/p95/p99 query latency on deterministic 10K and 100K corpora
 * using the production `FtsExecutor` and its on-disk index path. The 100K
 * scenario is the binding release gate for the remaining Query FTS acceptance
 * item (`p95 <= 100 ms`).
 */

#include <benchmark/benchmark.h>

#include "query/fts_executor.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <memory>
#include <mutex>
#include <numeric>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#ifdef _WIN32
  #include <windows.h>
  #define GET_PID() static_cast<int>(GetCurrentProcessId())
#else
  #include <unistd.h>
  #define GET_PID() ::getpid()
#endif

namespace themis::query::fts {
namespace {

constexpr std::size_t kExactPhraseStride = 512U;
constexpr std::size_t kNearPhraseStride = 341U;
constexpr int kBenchmarkIterations = 4;
constexpr double kGateP95Ms = 100.0;

int warmupQueriesPerIteration(std::size_t document_count) {
  return document_count >= 100000U ? 1 : 2;
}

int measuredQueriesPerIteration(std::size_t document_count) {
  return document_count >= 100000U ? 5 : 12;
}

std::filesystem::path makeBenchIndexPath(std::size_t document_count) {
  const auto base = std::filesystem::temp_directory_path();
  return base / ("themis_fts_phase_b_bench_" + std::to_string(GET_PID()) + "_" +
                 std::to_string(document_count));
}

double percentileMs(std::vector<double> samples, double percentile) {
  if (samples.empty()) {
    return 0.0;
  }
  std::sort(samples.begin(), samples.end());
  const auto clamped = std::clamp(percentile, 0.0, 1.0);
  const auto index = static_cast<std::size_t>(
      std::ceil(clamped * static_cast<double>(samples.size() - 1U)));
  return samples[std::min(index, samples.size() - 1U)];
}

struct LatencySummary {
  double p50_ms = 0.0;
  double p95_ms = 0.0;
  double p99_ms = 0.0;
  double avg_ms = 0.0;
  std::size_t result_count = 0;
};

LatencySummary summarizeLatenciesMs(const std::vector<double>& latencies_ms,
                                    std::size_t result_count) {
  LatencySummary summary;
  summary.result_count = result_count;
  if (latencies_ms.empty()) {
    return summary;
  }

  summary.p50_ms = percentileMs(latencies_ms, 0.50);
  summary.p95_ms = percentileMs(latencies_ms, 0.95);
  summary.p99_ms = percentileMs(latencies_ms, 0.99);
  summary.avg_ms =
      std::accumulate(latencies_ms.begin(), latencies_ms.end(), 0.0) /
      static_cast<double>(latencies_ms.size());
  return summary;
}

class FtsBenchmarkCorpus {
 public:
  static FtsBenchmarkCorpus& instance(std::size_t document_count) {
    static std::mutex mutex;
    static std::unordered_map<std::size_t, std::unique_ptr<FtsBenchmarkCorpus>> corpora;

    std::lock_guard<std::mutex> lock(mutex);
    auto& corpus = corpora[document_count];
    if (!corpus) {
      corpus = std::unique_ptr<FtsBenchmarkCorpus>(new FtsBenchmarkCorpus(document_count));
    }
    return *corpus;
  }

  FtsExecutor& executor() { return *executor_; }

  ~FtsBenchmarkCorpus() {
    executor_.reset();
    std::filesystem::remove_all(index_path_);
  }

 private:
  explicit FtsBenchmarkCorpus(std::size_t document_count)
      : index_path_(makeBenchIndexPath(document_count)) {
    std::filesystem::remove_all(index_path_);
    std::filesystem::create_directories(index_path_);
    executor_ = std::make_unique<FtsExecutor>(index_path_.string());

    IndexUpdateBatch batch;
    batch.additions.reserve(document_count);
    for (std::size_t index = 0; index < document_count; ++index) {
      std::string text = "document " + std::to_string(index) +
                         " retrieval cache service telemetry ";
      if (index % kExactPhraseStride == 0U) {
        text += "graph database engine query planner ranking";
      } else if (index % kNearPhraseStride == 0U) {
        text += "graph scalable analytical engine query planner ranking";
      } else {
        text += "distributed baseline shard metrics storage compaction";
      }
      batch.additions.push_back({static_cast<uint64_t>(index + 1U), std::move(text)});
    }

    auto status = executor_->updateIndex(batch);
    if (!status.has_value()) {
      throw std::runtime_error("failed to prepare FTS benchmark corpus");
    }
  }

  std::filesystem::path index_path_;
  std::unique_ptr<FtsExecutor> executor_;
};

ExecutionOptions benchmarkOptions() {
  ExecutionOptions options;
  options.limit = 25;
  options.timeout = std::chrono::seconds(5);
  options.include_snippets = false;
  options.parallel_merge = false;
  return options;
}

template <typename QueryFactory>
void runLatencyBenchmark(benchmark::State& state, QueryFactory&& query_factory,
                         const char* error_message_prefix) {
  const auto document_count = static_cast<std::size_t>(state.range(0));
  auto& executor = FtsBenchmarkCorpus::instance(document_count).executor();
  const auto options = benchmarkOptions();
  const int warmup_queries = warmupQueriesPerIteration(document_count);
  const int measured_queries = measuredQueriesPerIteration(document_count);

  std::vector<double> latencies_ms;
  latencies_ms.reserve(static_cast<std::size_t>(kBenchmarkIterations) *
                       static_cast<std::size_t>(measured_queries));
  std::size_t last_result_count = 0;

  for (auto _ : state) {
    state.PauseTiming();
    for (int warmup = 0; warmup < warmup_queries; ++warmup) {
      auto warmup_result = executor.execute(query_factory(), options);
      if (!warmup_result.has_value()) {
        const std::string error =
            std::string(error_message_prefix) + " warmup failed";
        state.SkipWithError(error.c_str());
        return;
      }
      benchmark::DoNotOptimize(warmup_result->size());
    }
    state.ResumeTiming();

    for (int sample = 0; sample < measured_queries; ++sample) {
      const auto start = std::chrono::steady_clock::now();
      auto result = executor.execute(query_factory(), options);
      const auto end = std::chrono::steady_clock::now();
      if (!result.has_value()) {
        const std::string error =
            std::string(error_message_prefix) + " measured query failed";
        state.SkipWithError(error.c_str());
        return;
      }

      last_result_count = result->size();
      benchmark::DoNotOptimize(last_result_count);
      latencies_ms.push_back(
          std::chrono::duration<double, std::milli>(end - start).count());
    }
  }

  const auto summary = summarizeLatenciesMs(latencies_ms, last_result_count);
  state.SetItemsProcessed(
      state.iterations() * static_cast<int64_t>(measured_queries));
  state.counters["dataset_docs"] = static_cast<double>(document_count);
  state.counters["hits"] = static_cast<double>(summary.result_count);
  state.counters["p50_ms"] = summary.p50_ms;
  state.counters["p95_ms"] = summary.p95_ms;
  state.counters["p99_ms"] = summary.p99_ms;
  state.counters["avg_ms"] = summary.avg_ms;
  state.counters["gate_target_ms"] = kGateP95Ms;
  state.counters["gate_pass"] = (summary.p95_ms <= kGateP95Ms) ? 1.0 : 0.0;
}

void BM_FtsPhraseQuery(benchmark::State& state) {
  runLatencyBenchmark(
      state,
      []() { return SearchNode::makePhrase("graph database engine"); },
      "phrase query");
}

void BM_FtsProximityQuery(benchmark::State& state) {
  runLatencyBenchmark(state,
                      []() {
                        SearchNode query = SearchNode::makePhrase("graph engine");
                        query.proximity_distance = 3U;
                        return query;
                      },
                      "proximity query");
}

BENCHMARK(BM_FtsPhraseQuery)
    ->Arg(10000)
    ->Arg(100000)
    ->Iterations(kBenchmarkIterations)
    ->UseRealTime()
    ->Unit(benchmark::kMillisecond);
BENCHMARK(BM_FtsProximityQuery)
    ->Arg(10000)
    ->Arg(100000)
    ->Iterations(kBenchmarkIterations)
    ->UseRealTime()
    ->Unit(benchmark::kMillisecond);

}  // namespace
}  // namespace themis::query::fts
