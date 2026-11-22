#include <benchmark/benchmark.h>

#include "utils/zstd_codec.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace {

std::vector<uint8_t> makePayload(std::size_t size) {
    std::vector<uint8_t> payload(size);
    for (std::size_t i = 0; i < size; ++i) {
        payload[i] = static_cast<uint8_t>((i / 64) % 251);
    }
    return payload;
}

void BM_ZstdCompress(benchmark::State& state) {
#ifndef THEMIS_HAS_ZSTD
    state.SkipWithMessage("Zstandard support is disabled in this build");
    return;
#else
    const auto input = makePayload(static_cast<std::size_t>(state.range(0)));
    for (auto _ : state) {
        auto compressed = themis::utils::zstd_compress_safe(input.data(), input.size());
        if (!compressed) {
            const std::string message = compressed.error().message();
            state.SkipWithError(message.c_str());
            return;
        }
        benchmark::DoNotOptimize(compressed->data());
        benchmark::DoNotOptimize(compressed->size());
        benchmark::ClobberMemory();
    }
    state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(input.size()));
#endif
}

void BM_ZstdDecompress(benchmark::State& state) {
#ifndef THEMIS_HAS_ZSTD
    state.SkipWithMessage("Zstandard support is disabled in this build");
    return;
#else
    const auto input = makePayload(static_cast<std::size_t>(state.range(0)));
    auto compressed = themis::utils::zstd_compress_safe(input.data(), input.size());
    if (!compressed) {
        const std::string message = compressed.error().message();
        state.SkipWithError(message.c_str());
        return;
    }

    auto verification = themis::utils::zstd_decompress_safe(*compressed);
    if (!verification || *verification != input) {
        state.SkipWithError("Zstandard decompression failed its setup round-trip check");
        return;
    }

    for (auto _ : state) {
        auto decompressed = themis::utils::zstd_decompress_safe(*compressed);
        if (!decompressed) {
            const std::string message = decompressed.error().message();
            state.SkipWithError(message.c_str());
            return;
        }
        benchmark::DoNotOptimize(decompressed->data());
        benchmark::DoNotOptimize(decompressed->size());
        benchmark::ClobberMemory();
    }
    state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(input.size()));
#endif
}

}

BENCHMARK(BM_ZstdCompress)->Arg(4 * 1024)->Arg(64 * 1024)->Arg(1024 * 1024);
BENCHMARK(BM_ZstdDecompress)->Arg(4 * 1024)->Arg(64 * 1024)->Arg(1024 * 1024);
