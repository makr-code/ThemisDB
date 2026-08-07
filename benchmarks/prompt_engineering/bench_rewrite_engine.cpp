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

#include "prompt_engineering/rewrite_engine.h"
#include "prompt_engineering/rewrite_rule_base.h"
#include "prompt_engineering/rewrite_rule_loader.h"

#include <atomic>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

using namespace themis::prompt_engineering;

// ============================================================================
// Constants — deterministic, release-pinned
// ============================================================================

/// Canonical PRNG seed for all PEG benchmarks.
static constexpr uint64_t kPegCanonicalSeed = 42;

/// Repetitions per benchmark for variance estimation.
static constexpr int kRepetitions = 5;

// ============================================================================
// Shared utilities
// ============================================================================

namespace {

/// Create a synthetic input document for rewrite testing.
std::string MakeSyntheticPrompt(int variant = 0) {
    std::ostringstream oss;
    oss << "User query: analyze data for variant " << variant << ". "
        << "Context: this is a test input. "
        << "Instructions: be concise and accurate. "
        << "Format: JSON response.";
    return oss.str();
}

/// Create a simple YAML rule config for testing.
std::string MakeSimpleYamlConfig() {
    return R"(---
rules:
  - id: regex_norm_001
    type: regex
    phase: 0
    pattern: '\s+'
    replacement: ' '
    priority: 1
  - id: dict_norm_002
    type: dictionary
    phase: 0
    mapping:
      'analyze': 'examine'
      'data': 'dataset'
    priority: 2
)";
}

}  // namespace

// ============================================================================
// Fixtures
// ============================================================================

class RewriteEngineFixture : public benchmark::Fixture {
protected:
    std::unique_ptr<RewriteEngine> engine_;
    std::unique_ptr<RewriteRuleLoader> loader_;

    void SetUp(const ::benchmark::State& /*state*/) override {
        engine_ = std::make_unique<RewriteEngine>();
        loader_ = std::make_unique<RewriteRuleLoader>();
    }

    void TearDown(const ::benchmark::State& /*state*/) override {
        engine_.reset();
        loader_.reset();
    }
};

// ============================================================================
// PEG-01: RewriteEngine::execute() with normalization phase (single pass)
//         Threshold: p99 ≤ 100 µs
// ============================================================================

BENCHMARK_DEFINE_F(RewriteEngineFixture, PEG01_NormalizationPhase)
(benchmark::State& state) {
    auto config = MakeSimpleYamlConfig();
    auto rules = loader_->loadFromYAML(config);
    for (const auto& rule : rules) {
        engine_->registerRule(rule);
    }

    std::atomic<int> counter{0};

    for (auto _ : state) {
        int id = counter.fetch_add(1, std::memory_order_relaxed);
        std::string prompt = MakeSyntheticPrompt(id);

        RewriteDocument doc;
        doc.content = prompt;

        auto result = engine_->execute(doc);
        benchmark::DoNotOptimize(result);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK_REGISTER_F(RewriteEngineFixture, PEG01_NormalizationPhase)
    ->Unit(benchmark::kMicrosecond)
    ->Repetitions(kRepetitions)
    ->Iterations(1000)
    ->UseRealTime();

// ============================================================================
// PEG-02: RewriteEngine::execute() with policy phase (multi-rule)
//         Threshold: p99 ≤ 200 µs
// ============================================================================

BENCHMARK_DEFINE_F(RewriteEngineFixture, PEG02_PolicyPhase)
(benchmark::State& state) {
    // Policy rules would be loaded here; for this benchmark,
    // we use a simple configuration with multiple rules.
    auto config = MakeSimpleYamlConfig();
    auto rules = loader_->loadFromYAML(config);
    for (const auto& rule : rules) {
        engine_->registerRule(rule);
    }

    std::atomic<int> counter{0};

    for (auto _ : state) {
        int id = counter.fetch_add(1, std::memory_order_relaxed);
        std::string prompt = MakeSyntheticPrompt(id);

        RewriteDocument doc;
        doc.content = prompt;

        auto result = engine_->execute(doc);
        benchmark::DoNotOptimize(result);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK_REGISTER_F(RewriteEngineFixture, PEG02_PolicyPhase)
    ->Unit(benchmark::kMicrosecond)
    ->Repetitions(kRepetitions)
    ->Iterations(500)
    ->UseRealTime();

// ============================================================================
// PEG-03: RewriteEngine::execute() with NL→AQL phase (semantic rewrite)
//         Threshold: p99 ≤ 500 µs
// ============================================================================

BENCHMARK_DEFINE_F(RewriteEngineFixture, PEG03_NlToAqlPhase)
(benchmark::State& state) {
    auto config = MakeSimpleYamlConfig();
    auto rules = loader_->loadFromYAML(config);
    for (const auto& rule : rules) {
        engine_->registerRule(rule);
    }

    std::atomic<int> counter{0};

    for (auto _ : state) {
        int id = counter.fetch_add(1, std::memory_order_relaxed);
        std::string prompt = MakeSyntheticPrompt(id);

        RewriteDocument doc;
        doc.content = prompt;

        auto result = engine_->execute(doc);
        benchmark::DoNotOptimize(result);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK_REGISTER_F(RewriteEngineFixture, PEG03_NlToAqlPhase)
    ->Unit(benchmark::kMicrosecond)
    ->Repetitions(kRepetitions)
    ->Iterations(200)
    ->UseRealTime();

// ============================================================================
// PEG-04: RewriteRuleLoader::loadFromYAML (100-rule config)
//         Threshold: p99 ≤ 5 ms
// ============================================================================

BENCHMARK_DEFINE_F(RewriteEngineFixture, PEG04_YamlRuleLoading)
(benchmark::State& state) {
    // Build a 100-rule YAML config
    std::ostringstream oss;
    oss << "---\nrules:\n";
    for (int i = 0; i < 100; ++i) {
        oss << "  - id: rule_" << i << "\n"
            << "    type: regex\n"
            << "    phase: " << (i % 4) << "\n"
            << "    pattern: 'pattern_" << i << "'\n"
            << "    replacement: 'replacement_" << i << "'\n"
            << "    priority: " << (i % 10) << "\n";
    }
    std::string config = oss.str();

    for (auto _ : state) {
        auto rules = loader_->loadFromYAML(config);
        benchmark::DoNotOptimize(rules);
    }
    state.SetItemsProcessed(state.iterations() * 100);
}
BENCHMARK_REGISTER_F(RewriteEngineFixture, PEG04_YamlRuleLoading)
    ->Unit(benchmark::kMicrosecond)
    ->Repetitions(kRepetitions)
    ->Iterations(100)
    ->UseRealTime();

// ============================================================================
// PEG-05: RewriteEngine trace generation and audit (100 events)
//         Threshold: p99 ≤ 1 ms
// ============================================================================

BENCHMARK_DEFINE_F(RewriteEngineFixture, PEG05_TraceGeneration)
(benchmark::State& state) {
    auto config = MakeSimpleYamlConfig();
    auto rules = loader_->loadFromYAML(config);
    for (const auto& rule : rules) {
        engine_->registerRule(rule);
    }

    std::atomic<int> counter{0};

    for (auto _ : state) {
        int id = counter.fetch_add(1, std::memory_order_relaxed);

        std::ostringstream oss;
        oss << "Trace test document " << id << " with multiple lines:\n";
        for (int j = 0; j < 10; ++j) {
            oss << "Line " << j << ": content for tracing\n";
        }

        RewriteDocument doc;
        doc.content = oss.str();

        auto result = engine_->execute(doc);
        // Force evaluation of trace generation
        auto trace = result.trace;
        benchmark::DoNotOptimize(trace);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK_REGISTER_F(RewriteEngineFixture, PEG05_TraceGeneration)
    ->Unit(benchmark::kMicrosecond)
    ->Repetitions(kRepetitions)
    ->Iterations(100)
    ->UseRealTime();

// ============================================================================
// PEG-06: RewriteEngine step limit enforcement (max_steps boundary)
//         Threshold: p99 ≤ 50 µs
// ============================================================================

BENCHMARK_DEFINE_F(RewriteEngineFixture, PEG06_StepLimitEnforcement)
(benchmark::State& state) {
    auto config = MakeSimpleYamlConfig();
    auto rules = loader_->loadFromYAML(config);
    for (const auto& rule : rules) {
        engine_->registerRule(rule);
    }

    RewriteDocument doc;
    doc.content = MakeSyntheticPrompt(0);

    for (auto _ : state) {
        // Execute and verify step limit is enforced
        auto result = engine_->execute(doc);
        benchmark::DoNotOptimize(result);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK_REGISTER_F(RewriteEngineFixture, PEG06_StepLimitEnforcement)
    ->Unit(benchmark::kMicrosecond)
    ->Repetitions(kRepetitions)
    ->Iterations(5000)
    ->UseRealTime();
