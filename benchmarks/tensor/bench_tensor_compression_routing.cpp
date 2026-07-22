#include <benchmark/benchmark.h>

#include "tensor/tensor_compression_routing_accelerator.h"

#include <vector>

namespace {

constexpr int kWarmupCold = 50;
constexpr int kWarmupWarm = 100;
constexpr int kWarmupHot = 200;

std::vector<float> makeInput(std::size_t n) {
    std::vector<float> values(n);
    for (std::size_t i = 0; i < n; ++i) {
        values[i] = static_cast<float>((i % 17) - 8) * 0.125f;
    }
    return values;
}

std::vector<std::vector<float>> makeRoutes(std::size_t routes, std::size_t dim) {
    std::vector<std::vector<float>> out(routes, std::vector<float>(dim));
    for (std::size_t r = 0; r < routes; ++r) {
        for (std::size_t i = 0; i < dim; ++i) {
            out[r][i] = static_cast<float>(((r + i) % 11) - 5) * 0.1f;
        }
    }
    return out;
}

void runCompressionWarmup(themis::tensor::TensorCompressionRoutingAccelerator& accelerator,
                          const std::vector<float>& input,
                          bool force_cpu) {
    for (int i = 0; i < kWarmupCold; ++i) {
        benchmark::DoNotOptimize(accelerator.compressToInt8(input, 0.125f, force_cpu));
    }
    for (int i = 0; i < kWarmupWarm; ++i) {
        benchmark::DoNotOptimize(accelerator.compressToInt8(input, 0.125f, force_cpu));
    }
    for (int i = 0; i < kWarmupHot; ++i) {
        benchmark::DoNotOptimize(accelerator.compressToInt8(input, 0.125f, force_cpu));
    }
}

void runRoutingWarmup(themis::tensor::TensorCompressionRoutingAccelerator& accelerator,
                      const std::vector<float>& input,
                      const std::vector<std::vector<float>>& routes,
                      bool force_cpu) {
    for (int i = 0; i < kWarmupCold; ++i) {
        benchmark::DoNotOptimize(accelerator.computeRoutingScores(input, routes, force_cpu));
    }
    for (int i = 0; i < kWarmupWarm; ++i) {
        benchmark::DoNotOptimize(accelerator.computeRoutingScores(input, routes, force_cpu));
    }
    for (int i = 0; i < kWarmupHot; ++i) {
        benchmark::DoNotOptimize(accelerator.computeRoutingScores(input, routes, force_cpu));
    }
}

void BM_CompressCpu(benchmark::State& state) {
    themis::tensor::TensorCompressionRoutingAccelerator accelerator;
    const auto input = makeInput(static_cast<std::size_t>(state.range(0)));
    runCompressionWarmup(accelerator, input, true);
    for (auto _ : state) {
        benchmark::DoNotOptimize(accelerator.compressToInt8(input, 0.125f, true));
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(input.size()));
}

void BM_CompressDispatch(benchmark::State& state) {
    themis::tensor::TensorCompressionRoutingAccelerator accelerator;
    const auto input = makeInput(static_cast<std::size_t>(state.range(0)));
    runCompressionWarmup(accelerator, input, false);
    for (auto _ : state) {
        benchmark::DoNotOptimize(accelerator.compressToInt8(input, 0.125f, false));
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(input.size()));
}

void BM_RoutingCpu(benchmark::State& state) {
    themis::tensor::TensorCompressionRoutingAccelerator accelerator;
    const std::size_t dim = static_cast<std::size_t>(state.range(0));
    const auto input = makeInput(dim);
    const auto routes = makeRoutes(32, dim);
    runRoutingWarmup(accelerator, input, routes, true);
    for (auto _ : state) {
        benchmark::DoNotOptimize(accelerator.computeRoutingScores(input, routes, true));
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(routes.size()));
}

void BM_RoutingDispatch(benchmark::State& state) {
    themis::tensor::TensorCompressionRoutingAccelerator accelerator;
    const std::size_t dim = static_cast<std::size_t>(state.range(0));
    const auto input = makeInput(dim);
    const auto routes = makeRoutes(32, dim);
    runRoutingWarmup(accelerator, input, routes, false);
    for (auto _ : state) {
        benchmark::DoNotOptimize(accelerator.computeRoutingScores(input, routes, false));
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(routes.size()));
}

BENCHMARK(BM_CompressCpu)->Arg(256)->Arg(4096)->Arg(65536)->Iterations(50000)->Unit(benchmark::kNanosecond);
BENCHMARK(BM_CompressDispatch)->Arg(256)->Arg(4096)->Arg(65536)->Iterations(50000)->Unit(benchmark::kNanosecond);
BENCHMARK(BM_RoutingCpu)->Arg(64)->Arg(256)->Arg(1024)->Iterations(5000)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_RoutingDispatch)->Arg(64)->Arg(256)->Arg(1024)->Iterations(5000)->Unit(benchmark::kMicrosecond);

}  // namespace
