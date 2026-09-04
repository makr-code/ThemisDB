// Benchmark: Content Version Management Performance
// Tests version creation, diff computation, and retrieval performance

#include <benchmark/benchmark.h>
#include <string>
#include <vector>
#include <random>
#include <chrono>
#include "content/version_manager.h"

using namespace themis::content;

// Generate random content of specified size
std::string generate_content(size_t size) {
    static std::mt19937 gen(42);
    static std::uniform_int_distribution<> dis(32, 126);
    
    std::string content = {};
    content.reserve(size);
    for (size_t i = 0; i < size; ++i) {
        content += static_cast<char>(dis(gen));
    }
    return content;
}

// Benchmark: Version creation latency for different file sizes
static void BM_VersionCreation(benchmark::State& state) {
    size_t file_size = state.range(0);
    VersionManager vm;
    std::string content = generate_content(file_size);

    for (auto _ : state) {
        int v = vm.createVersionWithContent("bench_doc", content);
        benchmark::DoNotOptimize(v);
    }

    state.SetBytesProcessed(state.iterations() * file_size);
}
BENCHMARK(BM_VersionCreation)->Arg(1024)->Arg(10*1024)->Arg(100*1024)->Arg(1024*1024)->Arg(10*1024*1024);

// Benchmark: Diff computation performance
static void BM_DiffComputation(benchmark::State& state) {
    size_t file_size = state.range(0);
    std::string old_content = generate_content(file_size);
    std::string new_content = generate_content(file_size);

    for (auto _ : state) {
        std::string diff = VersionManager::computeDelta(old_content, new_content);
        benchmark::DoNotOptimize(diff);
    }

    state.SetBytesProcessed(state.iterations() * file_size * 2);
}
BENCHMARK(BM_DiffComputation)->Arg(1024)->Arg(10*1024)->Arg(100*1024)->Arg(1024*1024);

// Benchmark: Version retrieval latency
static void BM_VersionRetrieval(benchmark::State& state) {
    VersionManager vm;
    const int num_versions = 100;

    // Create 100 versions
    for (int i = 0; i < num_versions; ++i) {
        std::string content = generate_content(1024);
        vm.createVersionWithContent("bench_doc", content);
    }

    int idx = 1;
    for (auto _ : state) {
        auto version = vm.getContent("bench_doc", (idx % num_versions) + 1);
        benchmark::DoNotOptimize(version);
        idx++;
    }
}
BENCHMARK(BM_VersionRetrieval);

// Benchmark: Storage overhead analysis
static void BM_StorageOverhead(benchmark::State& state) {
    size_t num_versions = state.range(0);

    for (auto _ : state) {
        VersionManager vm;
        size_t total = 0;
        for (size_t i = 0; i < num_versions; ++i) {
            std::string content = generate_content(1024);
            vm.createVersionWithContent("bench_doc", content);
            auto v = vm.getVersion("bench_doc", static_cast<int>(i + 1));
            if (v) {
                total += v->size_bytes + v->delta.size();
            }
        }
        benchmark::DoNotOptimize(total);
    }
}
BENCHMARK(BM_StorageOverhead)->Arg(10)->Arg(50)->Arg(100)->Arg(500);

// Benchmark: Concurrent version operations
static void BM_ConcurrentVersioning(benchmark::State& state) {
    VersionManager vm;
    std::string content = generate_content(10 * 1024);

    for (auto _ : state) {
        int v = vm.createVersionWithContent("bench_doc", content);
        benchmark::DoNotOptimize(v);
    }

    state.SetBytesProcessed(state.iterations() * content.size());
}
BENCHMARK(BM_ConcurrentVersioning)->Threads(1)->Threads(2)->Threads(4)->Threads(8);

BENCHMARK_MAIN();
