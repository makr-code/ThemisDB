// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file benchmark_runner.cpp
 * @brief Google Benchmark cases for the public utils API contract value types.
 *
 * These narrow microbenchmarks do not measure end-to-end utility services.
 */

#include "utils/utils_api_contract.h"

#include <benchmark/benchmark.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace {

void BM_BloomFilterConfigConstruction(benchmark::State& state) {
    for (auto _ : state) {
        themis::utils::BloomFilterConfig config;
        config.expectedItems = 1'000'000;
        config.targetFalsePositiveRate = 0.01;
        benchmark::DoNotOptimize(config);
    }
}
BENCHMARK(BM_BloomFilterConfigConstruction)->Repetitions(5);

void BM_RetryPolicyCopy(benchmark::State& state) {
    const themis::utils::RetryPolicy original{};
    for (auto _ : state) {
        const auto copy = original;
        benchmark::DoNotOptimize(copy);
    }
}
BENCHMARK(BM_RetryPolicyCopy)->Repetitions(5);

void BM_UtilsErrorCast(benchmark::State& state) {
    constexpr std::array kErrors{
        themis::utils::UtilsError::kAuditOverflow,
        themis::utils::UtilsError::kBatchRollback,
        themis::utils::UtilsError::kBatchSizeExceeded,
        themis::utils::UtilsError::kRetryExhausted,
        themis::utils::UtilsError::kDeserInvalid,
        themis::utils::UtilsError::kPoolExhausted,
    };
    std::size_t index = 0;
    for (auto _ : state) {
        const auto value =
            static_cast<std::int32_t>(kErrors[index++ % kErrors.size()]);
        benchmark::DoNotOptimize(value);
    }
}
BENCHMARK(BM_UtilsErrorCast)->Repetitions(5);

void BM_RetryPolicyBatchConstruction(benchmark::State& state) {
    const auto count = static_cast<std::size_t>(state.range(0));
    for (auto _ : state) {
        std::vector<themis::utils::RetryPolicy> policies(count);
        for (auto& policy : policies) {
            policy.maxAttempts = 3;
        }
        benchmark::DoNotOptimize(policies.data());
    }
    state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(count));
}
BENCHMARK(BM_RetryPolicyBatchConstruction)->Arg(1000)->Repetitions(5);

void BM_BloomFilterConfigBatchConstruction(benchmark::State& state) {
    const auto count = static_cast<std::size_t>(state.range(0));
    for (auto _ : state) {
        std::vector<themis::utils::BloomFilterConfig> configs(count);
        for (auto& config : configs) {
            config.expectedItems = 100;
            config.targetFalsePositiveRate = 0.01;
        }
        benchmark::DoNotOptimize(configs.data());
    }
    state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(count));
}
BENCHMARK(BM_BloomFilterConfigBatchConstruction)->Arg(1000)->Repetitions(5);

void BM_UtilsErrorSwitchDispatch(benchmark::State& state) {
    constexpr std::array kErrors{
        themis::utils::UtilsError::kAuditOverflow,
        themis::utils::UtilsError::kBatchRollback,
        themis::utils::UtilsError::kBatchSizeExceeded,
        themis::utils::UtilsError::kRetryExhausted,
        themis::utils::UtilsError::kDeserInvalid,
        themis::utils::UtilsError::kPoolExhausted,
    };
    std::size_t index = 0;
    for (auto _ : state) {
        const auto error = kErrors[index++ % kErrors.size()];
        std::int32_t result = 0;
        switch (error) {
            case themis::utils::UtilsError::kAuditOverflow: result = 1; break;
            case themis::utils::UtilsError::kBatchRollback: result = 2; break;
            case themis::utils::UtilsError::kBatchSizeExceeded: result = 3; break;
            case themis::utils::UtilsError::kRetryExhausted: result = 4; break;
            case themis::utils::UtilsError::kDeserInvalid: result = 5; break;
            case themis::utils::UtilsError::kPoolExhausted: result = 6; break;
        }
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_UtilsErrorSwitchDispatch)->Repetitions(5);

} // namespace

BENCHMARK_MAIN();
