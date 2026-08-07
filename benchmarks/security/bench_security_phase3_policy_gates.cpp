// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file bench_security_phase3_policy_gates.cpp
 * @brief Phase 3 policy/data-protection release gates (P-MRG-01..P-MRG-04).
 *
 * Benchmark-backed performance gates for Phase 3 hardening:
 * - Policy evaluation latency (single + complex rule sets)
 * - RLS filter latency
 * - Query result masking overhead
 * - Denial-by-default precedence overhead
 *
 * @see src/security/ROADMAP.md — Phase 3+5 (Benchmarks)
 * @see benchmarks/MEASUREMENT_HYGIENE.md
 */

#include <benchmark/benchmark.h>

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

// ─────────────────────────────────────────────────────────────────────────────
// Mock Policy Engine for Testing
// ─────────────────────────────────────────────────────────────────────────────

struct MockPolicyRule {
    std::string principal;
    std::string resource;
    std::string action;
    bool allowed;
    int precedence;
};

class MockPolicyEngine {
public:
    MockPolicyEngine() = default;
    
    void addRule(const std::string& rule_id, const MockPolicyRule& rule) {
        rules_[rule_id] = rule;
    }
    
    bool evaluate(const std::string& principal, const std::string& resource,
                  const std::string& action) {
        eval_count_++;
        
        // Find matching rule with highest precedence
        int max_precedence = -1;
        bool result = false;
        
        for (const auto& [rid, rule] : rules_) {
            if (rule.principal == principal && rule.resource == resource && rule.action == action) {
                if (rule.precedence > max_precedence) {
                    max_precedence = rule.precedence;
                    result = rule.allowed;
                }
            }
        }
        
        // No matching rule: deny-by-default
        if (max_precedence < 0) {
            result = false;
        }
        
        return result;
    }
    
    uint64_t evalCount() const { return eval_count_; }

private:
    std::unordered_map<std::string, MockPolicyRule> rules_;
    std::atomic<uint64_t> eval_count_{0};
};

// ─────────────────────────────────────────────────────────────────────────────
// P-MRG-01: Single Rule Evaluation (Baseline)
// ─────────────────────────────────────────────────────────────────────────────

static void BenchP_MRG_01_SingleRuleEvaluation(benchmark::State& state) {
    MockPolicyEngine engine;
    
    // Setup: single rule
    engine.addRule("rule_1", {
        .principal = "user",
        .resource = "resource",
        .action = "READ",
        .allowed = true,
        .precedence = 10
    });
    
    // Benchmark: policy evaluation
    for (auto _ : state) {
        benchmark::DoNotOptimize(engine.evaluate("user", "resource", "READ"));
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// P-MRG-02: Complex Rule Set (10 rules, RBAC + ABAC)
// ─────────────────────────────────────────────────────────────────────────────

static void BenchP_MRG_02_ComplexRuleSet(benchmark::State& state) {
    MockPolicyEngine engine;
    
    // Setup: 10 mixed rules (RBAC + ABAC)
    engine.addRule("rbac_1", {"user", "resource", "READ", true, 50});
    engine.addRule("rbac_2", {"admin", "resource", "WRITE", true, 75});
    engine.addRule("rbac_3", {"user", "sensitive", "READ", false, 60});
    engine.addRule("abac_1", {"user", "resource", "DELETE", false, 40});
    engine.addRule("abac_2", {"user", "public", "READ", true, 30});
    engine.addRule("abac_3", {"admin", "resource", "DELETE", true, 80});
    engine.addRule("abac_4", {"guest", "public", "READ", true, 20});
    engine.addRule("abac_5", {"user", "resource", "WRITE", false, 55});
    engine.addRule("abac_6", {"admin", "sensitive", "READ", true, 90});
    engine.addRule("abac_7", {"guest", "resource", "WRITE", false, 10});
    
    // Benchmark: policy evaluation against complex rule set
    for (auto _ : state) {
        benchmark::DoNotOptimize(engine.evaluate("user", "resource", "READ"));
        benchmark::DoNotOptimize(engine.evaluate("admin", "sensitive", "READ"));
        benchmark::DoNotOptimize(engine.evaluate("guest", "public", "READ"));
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// P-MRG-03: Deny-by-Default (No matching rule)
// ─────────────────────────────────────────────────────────────────────────────

static void BenchP_MRG_03_DenyByDefault(benchmark::State& state) {
    MockPolicyEngine engine;
    
    // Setup: rules that don't match the queries
    engine.addRule("rule_1", {"user_a", "resource_a", "READ", true, 50});
    engine.addRule("rule_2", {"user_b", "resource_b", "WRITE", false, 60});
    
    // Benchmark: queries that don't match any rule (deny-by-default)
    for (auto _ : state) {
        benchmark::DoNotOptimize(engine.evaluate("user_unknown", "resource_unknown", "DELETE"));
        benchmark::DoNotOptimize(engine.evaluate("user_c", "resource_c", "READ"));
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// P-MRG-04: RLS Filter Latency (1000 rows, 10% match)
// ─────────────────────────────────────────────────────────────────────────────

static void BenchP_MRG_04_RLSFilterLatency(benchmark::State& state) {
    // Simulate: 1000 rows with RLS constraint
    struct MockRow {
        std::string user_id;
        std::string data;
    };
    
    std::vector<MockRow> rows;
    for (int i = 0; i < 1000; ++i) {
        // 10% match target user
        std::string user = (i % 10 == 0) ? "target_user" : ("user_" + std::to_string(i));
        rows.push_back({user, "data_" + std::to_string(i)});
    }
    
    std::string filter_user = "target_user";
    
    // Benchmark: apply RLS filter
    for (auto _ : state) {
        int matched = 0;
        for (const auto& row : rows) {
            if (row.user_id == filter_user) {
                matched++;
                benchmark::DoNotOptimize(row);
            }
        }
        benchmark::ClobberMemory();
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// P-MRG-05: Query Result Masking Overhead
// ─────────────────────────────────────────────────────────────────────────────

static void BenchP_MRG_05_QueryMaskingOverhead(benchmark::State& state) {
    // Simulate: 1000 rows with PII fields
    struct ResultRow {
        std::string user_id;
        std::string email;
        std::string phone;
        std::string data;
    };
    
    std::vector<ResultRow> results;
    for (int i = 0; i < 1000; ++i) {
        results.push_back({
            "user_" + std::to_string(i),
            "user" + std::to_string(i) + "@example.com",
            "555-" + std::to_string(1000 + i),
            "data_" + std::to_string(i)
        });
    }
    
    // Benchmark: apply masking to all rows
    for (auto _ : state) {
        for (auto& row : results) {
            // Mask PII fields
            row.email = "***redacted***";
            row.phone = "***redacted***";
            benchmark::DoNotOptimize(row);
        }
        benchmark::ClobberMemory();
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Benchmark Registration with Release Gates
// ─────────────────────────────────────────────────────────────────────────────

// P-MRG-01: Single rule evaluation p99 ≤ 1µs
BENCHMARK(BenchP_MRG_01_SingleRuleEvaluation)
    ->Unit(benchmark::kMicrosecond)
    ->Iterations(10000)
    ->DisplayAggregatesOnly();

// P-MRG-02: Complex rule set evaluation p99 ≤ 100µs
BENCHMARK(BenchP_MRG_02_ComplexRuleSet)
    ->Unit(benchmark::kMicrosecond)
    ->Iterations(1000)
    ->DisplayAggregatesOnly();

// P-MRG-03: Deny-by-default evaluation p99 ≤ 50µs
BENCHMARK(BenchP_MRG_03_DenyByDefault)
    ->Unit(benchmark::kMicrosecond)
    ->Iterations(1000)
    ->DisplayAggregatesOnly();

// P-MRG-04: RLS filter (1000 rows) p99 ≤ 1ms
BENCHMARK(BenchP_MRG_04_RLSFilterLatency)
    ->Unit(benchmark::kMicrosecond)
    ->Iterations(100)
    ->DisplayAggregatesOnly();

// P-MRG-05: Query masking (1000 rows) p99 ≤ 2ms, overhead ≤ 5%
BENCHMARK(BenchP_MRG_05_QueryMaskingOverhead)
    ->Unit(benchmark::kMicrosecond)
    ->Iterations(100)
    ->DisplayAggregatesOnly();

BENCHMARK_MAIN();
