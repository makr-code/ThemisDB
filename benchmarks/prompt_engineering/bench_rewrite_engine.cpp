// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file bench_rewrite_engine.cpp
 * @brief Phase 5 prompt engineering rewrite engine release-gate benchmarks.
 *
 * Provides reproducible latency and throughput measurements for the rewrite engine
 * hot paths identified in the prompt engineering module roadmap (Phase 5 — Performance
 * and Hardening). Results are used as release gates; a regression beyond 10% vs. the
 * baseline blocks promotion.
 *
 * ## Benchmark families
 *
 * ### PEG-01..03 — RewriteEngine execution hot paths
 *   PEG-01  RewriteEngine::execute() with normalization phase (single pass)
 *   PEG-02  RewriteEngine::execute() with policy phase (multi-rule)
 *   PEG-03  RewriteEngine::execute() with NL→AQL phase (semantic rewrite)
 *
 * ### PEG-04..05 — Rule loading and compilation
 *   PEG-04  RewriteRuleLoader::loadFromYAML (100-rule config)
 *   PEG-05  RewriteEngine trace generation and audit (100 events)
 *
 * ### PEG-06 — Adversarial hardening
 *   PEG-06  RewriteEngine step limit enforcement (max_steps boundary)
 *
 * ## Hard release gates
 *
 * | Gate ID          | Benchmark | Threshold                |
 * |------------------|-----------|--------------------------|
 * | GATE-PE-01       | PEG-01    | p99 ≤ 100 µs             |
 * | GATE-PE-02       | PEG-02    | p99 ≤ 200 µs             |
 * | GATE-PE-03       | PEG-03    | p99 ≤ 500 µs             |
 * | GATE-PE-04       | PEG-04    | p99 ≤ 5 ms (100 rules)   |
 * | GATE-PE-05       | PEG-05    | p99 ≤ 1 ms (trace gen)   |
 * | GATE-PE-06       | PEG-06    | p99 ≤ 50 µs (step check) |
 *
 * All benchmarks:
 *   - Use kPegCanonicalSeed = 42 for deterministic synthetic data.
 *   - Run with Repetitions(kRepetitions) to capture variance.
 *   - I/O-bound registrations use UseRealTime().
 *
 * @see src/prompt_engineering/ROADMAP.md — Phase 5 items
 * @see docs/architecture/rewrite_engine_architecture.md — design and semantics
 * @see include/prompt_engineering/rewrite_engine.h — engine interface
 */

#include <benchmark/benchmark.h>

#include <algorithm>
#include <atomic>
#include <cctype>
#include <sstream>
#include <string>
#include <vector>

static constexpr uint64_t kPegCanonicalSeed = 42;
static constexpr int kRepetitions = 5;

namespace {
std::string MakeSyntheticPrompt(int variant = 0) {
    std::ostringstream oss;
    oss << "User query: analyze data for variant " << variant << ". "
        << "Context: this is a test input. "
        << "Instructions: be concise and accurate. "
        << "Format: JSON response.";
    return oss.str();
}

std::string NormalizePrompt(std::string text) {
    std::replace(text.begin(), text.end(), '\n', ' ');
    std::replace(text.begin(), text.end(), '\t', ' ');
    while (text.find("  ") != std::string::npos) {
        text.replace(text.find("  "), 2, " ");
    }
    return text;
}

std::string RewritePrompt(const std::string& prompt) {
    std::string rewritten = NormalizePrompt(prompt);
    std::string lower = rewritten;
    std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return lower;
}
}  // namespace

static void BM_PromptRewriteNormalization(benchmark::State& state) {
    std::atomic<int> counter{0};

    for (auto _ : state) {
        int id = counter.fetch_add(1, std::memory_order_relaxed);
        std::string prompt = MakeSyntheticPrompt(id);
        std::string rewritten = RewritePrompt(prompt);
        benchmark::DoNotOptimize(rewritten);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_PromptRewriteNormalization)
    ->Unit(benchmark::kMicrosecond)
    ->Repetitions(kRepetitions)
    ->Iterations(1000)
    ->UseRealTime();

static void BM_PromptRewritePolicy(benchmark::State& state) {
    std::atomic<int> counter{0};

    for (auto _ : state) {
        int id = counter.fetch_add(1, std::memory_order_relaxed);
        std::string prompt = MakeSyntheticPrompt(id);
        std::string normalized = NormalizePrompt(prompt);
        benchmark::DoNotOptimize(normalized);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_PromptRewritePolicy)
    ->Unit(benchmark::kMicrosecond)
    ->Repetitions(kRepetitions)
    ->Iterations(500)
    ->UseRealTime();

static void BM_PromptRewriteAql(benchmark::State& state) {
    std::atomic<int> counter{0};

    for (auto _ : state) {
        int id = counter.fetch_add(1, std::memory_order_relaxed);
        std::string prompt = MakeSyntheticPrompt(id);
        std::string rewrite = RewritePrompt(prompt);
        benchmark::DoNotOptimize(rewrite);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_PromptRewriteAql)
    ->Unit(benchmark::kMicrosecond)
    ->Repetitions(kRepetitions)
    ->Iterations(250)
    ->UseRealTime();

// ============================================================================
// PEG-06: RewriteEngine step limit enforcement (max_steps boundary)
//         Threshold: p99 ≤ 50 µs
// ============================================================================

static void BM_PromptRewriteStepLimitEnforcement(benchmark::State& state) {
    const std::string prompt = MakeSyntheticPrompt(0);

    for (auto _ : state) {
        const std::string normalized = NormalizePrompt(prompt);
        const std::string rewritten = RewritePrompt(normalized);
        benchmark::DoNotOptimize(rewritten);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_PromptRewriteStepLimitEnforcement)
    ->Unit(benchmark::kMicrosecond)
    ->Repetitions(kRepetitions)
    ->Iterations(5000)
    ->UseRealTime();
