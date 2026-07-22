#include <benchmark/benchmark.h>

#include "tensor/tensor_compression_routing_accelerator.h"

#include <vector>

namespace {

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

void BM_CompressCpu(benchmark::State& state) {
    themis::tensor::TensorCompressionRoutingAccelerator accelerator;
    const auto input = makeInput(static_cast<std::size_t>(state.range(0)));
    for (auto _ : state) {
        benchmark::DoNotOptimize(accelerator.compressToInt8(input, 0.125f, true));
    }
}

void BM_CompressDispatch(benchmark::State& state) {
    themis::tensor::TensorCompressionRoutingAccelerator accelerator;
    const auto input = makeInput(static_cast<std::size_t>(state.range(0)));
    for (auto _ : state) {
        benchmark::DoNotOptimize(accelerator.compressToInt8(input, 0.125f, false));
    }
}

void BM_RoutingCpu(benchmark::State& state) {
    themis::tensor::TensorCompressionRoutingAccelerator accelerator;
    const std::size_t dim = static_cast<std::size_t>(state.range(0));
    const auto input = makeInput(dim);
    const auto routes = makeRoutes(32, dim);
    for (auto _ : state) {
        benchmark::DoNotOptimize(accelerator.computeRoutingScores(input, routes, true));
    }
}

void BM_RoutingDispatch(benchmark::State& state) {
    themis::tensor::TensorCompressionRoutingAccelerator accelerator;
    const std::size_t dim = static_cast<std::size_t>(state.range(0));
    const auto input = makeInput(dim);
    const auto routes = makeRoutes(32, dim);
    for (auto _ : state) {
        benchmark::DoNotOptimize(accelerator.computeRoutingScores(input, routes, false));
    }
}

BENCHMARK(BM_CompressCpu)->Arg(256)->Arg(4096)->Arg(65536);
BENCHMARK(BM_CompressDispatch)->Arg(256)->Arg(4096)->Arg(65536);
BENCHMARK(BM_RoutingCpu)->Arg(64)->Arg(256)->Arg(1024);
BENCHMARK(BM_RoutingDispatch)->Arg(64)->Arg(256)->Arg(1024);

}  // namespace
