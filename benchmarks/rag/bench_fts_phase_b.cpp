#include <benchmark/benchmark.h>

#include "query/fts_executor.h"

#include <chrono>
#include <filesystem>
#include <memory>
#include <mutex>
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

std::filesystem::path makeBenchIndexPath(std::size_t document_count) {
  const auto base = std::filesystem::temp_directory_path();
  return base / ("themis_fts_phase_b_bench_" + std::to_string(GET_PID()) + "_" +
                 std::to_string(document_count));
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
      if (index % 50U == 0U) {
        text += "graph database engine query planner ranking";
      } else if (index % 40U == 0U) {
        text += "graph scalable analytical engine query planner ranking";
      } else {
        text += "distributed baseline shard metrics";
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

void BM_FtsPhraseQuery(benchmark::State& state) {
  const auto document_count = static_cast<std::size_t>(state.range(0));
  auto& executor = FtsBenchmarkCorpus::instance(document_count).executor();
  const auto options = benchmarkOptions();
  const SearchNode query = SearchNode::makePhrase("graph database engine");

  for (auto _ : state) {
    auto result = executor.execute(query, options);
    if (!result.has_value()) {
      state.SkipWithError("phrase query execution failed");
      break;
    }
    benchmark::DoNotOptimize(result->size());
  }

  state.SetItemsProcessed(
      static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(document_count));
}

void BM_FtsProximityQuery(benchmark::State& state) {
  const auto document_count = static_cast<std::size_t>(state.range(0));
  auto& executor = FtsBenchmarkCorpus::instance(document_count).executor();
  const auto options = benchmarkOptions();

  SearchNode query = SearchNode::makePhrase("graph engine");
  query.proximity_distance = 3U;

  for (auto _ : state) {
    auto result = executor.execute(query, options);
    if (!result.has_value()) {
      state.SkipWithError("proximity query execution failed");
      break;
    }
    benchmark::DoNotOptimize(result->size());
  }

  state.SetItemsProcessed(
      static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(document_count));
}

BENCHMARK(BM_FtsPhraseQuery)->Arg(10000)->Arg(100000)->UseRealTime();
BENCHMARK(BM_FtsProximityQuery)->Arg(10000)->Arg(100000)->UseRealTime();

}  // namespace
}  // namespace themis::query::fts
