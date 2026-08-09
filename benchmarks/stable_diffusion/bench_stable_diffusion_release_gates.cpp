// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file bench_stable_diffusion_release_gates.cpp
 * @brief Release-gate benchmarks for stable_diffusion request paths.
 *
 * Gate coverage:
 * - SD-BENCH-01: Stub backend time-to-PNG latency for 512x512 requests.
 * - SD-BENCH-02: In-memory backend proxy time-to-PNG latency for 512x512 requests.
 * - SD-BENCH-03: Parallel request stability under multi-threaded benchmark execution.
 */

#include <benchmark/benchmark.h>

#include "stable_diffusion/sd_generator.h"
#include "stable_diffusion/sd_plugin.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

using namespace themis::imggen;

namespace {

static constexpr int kWidth = 512;
static constexpr int kHeight = 512;
static constexpr uint64_t kCanonicalSeed = 42u;

[[nodiscard]] SDGenerationConfig makeCfg512() {
    SDGenerationConfig cfg;
    cfg.width = kWidth;
    cfg.height = kHeight;
    cfg.seed = static_cast<int64_t>(kCanonicalSeed);
    cfg.steps = 20;
    cfg.cfg_scale = 7.0f;
    cfg.sampler = "euler_a";
    return cfg;
}

[[nodiscard]] std::unique_ptr<SDPlugin> makeInMemoryPlugin() {
    auto generator = std::make_unique<InMemorySDGenerator>();
    generator->setNextPixels(std::vector<uint8_t>(static_cast<size_t>(kWidth) * kHeight * 3u, 127u),
                             kWidth, kHeight, kCanonicalSeed);
    return std::make_unique<SDPlugin>(std::move(generator), SDPromptSanitizer{});
}

void BM_SD_TimeToPng_Stub512(benchmark::State& state) {
    SDPlugin plugin;
    if (!plugin.initialize("", {})) {
        state.SkipWithError("failed to initialize SDPlugin in stub mode");
        return;
    }

    const auto cfg = makeCfg512();
    for (auto _ : state) {
        const auto img = plugin.generate("benchmark prompt", cfg);
        if (!img.success) {
            state.SkipWithError(img.error_message.c_str());
            return;
        }
        benchmark::DoNotOptimize(img.png_data.data());
        benchmark::DoNotOptimize(img.perceptual_hash);
    }

    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(kWidth) * kHeight * 3);
}

void BM_SD_TimeToPng_InMemoryProxy512(benchmark::State& state) {
    auto plugin = makeInMemoryPlugin();
    if (!plugin->initialize("", {})) {
        state.SkipWithError("failed to initialize SDPlugin with in-memory backend");
        return;
    }

    const auto cfg = makeCfg512();
    for (auto _ : state) {
        const auto img = plugin->generate("benchmark prompt", cfg);
        if (!img.success) {
            state.SkipWithError(img.error_message.c_str());
            return;
        }
        benchmark::DoNotOptimize(img.png_data.data());
        benchmark::DoNotOptimize(img.perceptual_hash);
    }

    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(kWidth) * kHeight * 3);
}

void BM_SD_ParallelGenerate_Stability(benchmark::State& state) {
    auto plugin = makeInMemoryPlugin();
    if (!plugin->initialize("", {})) {
        state.SkipWithError("failed to initialize SDPlugin for parallel stability benchmark");
        return;
    }

    SDGenerationConfig cfg;
    cfg.width = 256;
    cfg.height = 256;
    cfg.seed = static_cast<int64_t>(kCanonicalSeed);

    for (auto _ : state) {
        const auto img = plugin->generate("parallel benchmark prompt", cfg);
        if (!img.success) {
            state.SkipWithError(img.error_message.c_str());
            return;
        }
        benchmark::DoNotOptimize(img.seed_used);
    }

    state.SetItemsProcessed(state.iterations());
}

BENCHMARK(BM_SD_TimeToPng_Stub512)
    ->Unit(benchmark::kMillisecond)
    ->UseRealTime();

BENCHMARK(BM_SD_TimeToPng_InMemoryProxy512)
    ->Unit(benchmark::kMillisecond)
    ->UseRealTime();

BENCHMARK(BM_SD_ParallelGenerate_Stability)
    ->Threads(1)
    ->Threads(4)
    ->Unit(benchmark::kMicrosecond);

} // namespace
