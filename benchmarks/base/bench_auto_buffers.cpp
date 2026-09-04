/**
 * @file bench_auto_buffers.cpp
 * @brief Performance benchmarks for buffer operations (v1.3.0)
 * 
 * Simplified benchmarks for v1.3.0 with available APIs.
 */

#include <benchmark/benchmark.h>
#include <memory>
#include <random>

// ===== Basic Performance Benchmarks =====

static void BM_VectorOperations_RandomAccess(benchmark::State& state) {
    std::vector<float> vec(state.range(0));
    std::random_device rd = {};
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, state.range(0) - 1);
    
    for (auto _ : state) {
        for (int i = 0; i < 1000; ++i) {
            int idx = dis(gen);
            benchmark::DoNotOptimize(vec[idx]);
        }
    }
    
    state.SetComplexityN(state.range(0));
}
BENCHMARK(BM_VectorOperations_RandomAccess)
    ->RangeMultiplier(2)
    ->Range(1024, 1024*1024)
    ->Complexity();

static void BM_VectorOperations_SequentialRead(benchmark::State& state) {
    std::vector<float> vec(state.range(0));
    std::fill(vec.begin(), vec.end(), 1.0f);
    
    for (auto _ : state) {
        float sum = 0;
        for (auto& v : vec) {
            sum += v;
        }
        benchmark::DoNotOptimize(sum);
    }
    
    state.SetBytesProcessed(state.iterations() * state.range(0) * sizeof(float));
}
BENCHMARK(BM_VectorOperations_SequentialRead)
    ->RangeMultiplier(2)
    ->Range(1024, 1024*1024);

static void BM_VectorOperations_Write(benchmark::State& state) {
    std::vector<float> vec(state.range(0));
    float val = 1.0f;
    
    for (auto _ : state) {
        for (auto& v : vec) {
            v = val;
        }
    }
    
    state.SetBytesProcessed(state.iterations() * state.range(0) * sizeof(float));
}
BENCHMARK(BM_VectorOperations_Write)
    ->RangeMultiplier(2)
    ->Range(1024, 1024*1024);

static void BM_StringOperations_Concatenation(benchmark::State& state) {
    std::string base = "benchmark_entity_";
    
    for (auto _ : state) {
        std::string result = {};
        for (int i = 0; i < state.range(0); ++i) {
            result = base + std::to_string(i);
            benchmark::DoNotOptimize(result);
        }
    }
}
BENCHMARK(BM_StringOperations_Concatenation)
    ->Range(1, 10000);

static void BM_MapOperations_Insert(benchmark::State& state) {
    std::map<std::string, double> data;
    
    for (auto _ : state) {
        data.clear();
        for (int i = 0; i < state.range(0); ++i) {
            data["key_" + std::to_string(i)] = i * 1.5;
        }
    }
}
BENCHMARK(BM_MapOperations_Insert)
    ->Range(100, 10000);

static void BM_MapOperations_Lookup(benchmark::State& state) {
    std::map<std::string, double> data = {};

    for (int i = 0; i < 1000; ++i) {
        data["key_" + std::to_string(i)] = i * 1.5;
    }
    
    std::random_device rd = {};
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 999);
    
    for (auto _ : state) {
        for (int i = 0; i < state.range(0); ++i) {
            int idx = dis(gen);
            benchmark::DoNotOptimize(data.at("key_" + std::to_string(idx)));
        }
    }
}
BENCHMARK(BM_MapOperations_Lookup)
    ->Range(100, 10000);

// ===== Main =====

int main(int argc, char** argv) {
    ::benchmark::Initialize(&argc, argv);
    if (::benchmark::ReportUnrecognizedArguments(argc, argv)) {
        return 1;
    }
    
    ::benchmark::RunSpecifiedBenchmarks();
    ::benchmark::Shutdown();
    
    return 0;
}

