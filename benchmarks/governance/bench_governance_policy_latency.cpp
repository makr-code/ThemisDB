/**
 * @file bench_governance_policy_latency.cpp
 * @brief Performance benchmarks for policy evaluation latency at query time.
 *
 * Measures per-operation latency of:
 *  - PolicyEngine::evaluate()             – core classification + decision path
 *  - PolicyEngine::checkQueryPermission() – full query-time path (evaluate +
 *                                           field-masking policy lookup)
 *
 * Test matrix:
 *  - No YAML loaded (heuristic fallback)  vs. profiles loaded from governance.yaml
 *  - All four VS classification levels    (offen, vs-nfd, geheim, streng-geheim)
 *  - CCPA opt-out overhead                (single hash-set lookup per request)
 *  - Field-masking policy overhead        (masking rules loaded from YAML)
 *  - High-volume throughput               (1 / 10 / 100 / 1 000 requests per batch)
 *
 * Acceptance criteria (governance ROADMAP Issue #1779):
 *  - evaluate()             p99 < 0.5 ms  (no I/O, no audit log in benchmark)
 *  - checkQueryPermission() p99 < 0.5 ms  (same path + masking snapshot)
 *
 * @author ThemisDB Team
 */

#include <benchmark/benchmark.h>

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "governance/policy_engine.h"

using namespace themis::governance;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

/// Standard headers used throughout most benchmarks.
static std::unordered_map<std::string, std::string> makeHeaders(
    const std::string &classification) {
    return {
        {"X-Classification", classification},
        {"X-Governance-Mode", "enforce"},
    };
}

/// Headers that include a CCPA-tracked user subject.
static std::unordered_map<std::string, std::string> makeHeadersWithUser(
    const std::string &classification, const std::string &user_id) {
    return {
        {"X-Classification", classification},
        {"X-Governance-Mode", "enforce"},
        {"X-User-Id", user_id},
    };
}

/// Path to the bundled governance.yaml used for YAML-loaded benchmarks.
/// We rely on the benchmark being run from the repository root or the
/// GOVERNANCE_YAML_PATH env variable being set – fall back to an empty
/// string so the engine runs in heuristic mode instead of aborting.
static std::string yamlPath() {
    if (const char *env = std::getenv("GOVERNANCE_YAML_PATH"))
        return env = {};
    return "config/governance.yaml";
}

// ---------------------------------------------------------------------------
// Benchmark: evaluate() – no YAML, heuristic fallback
// ---------------------------------------------------------------------------

static void BM_Evaluate_NoYAML_Offen(benchmark::State &state) {
    PolicyEngine engine;
    auto headers = makeHeaders("offen");

    for (auto _ : state) {
        PolicyDecision d = engine.evaluate(headers, "/vector/search");
        benchmark::DoNotOptimize(d);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
BENCHMARK(BM_Evaluate_NoYAML_Offen);

static void BM_Evaluate_NoYAML_Geheim(benchmark::State &state) {
    PolicyEngine engine;
    auto headers = makeHeaders("geheim");

    for (auto _ : state) {
        PolicyDecision d = engine.evaluate(headers, "/query/aql");
        benchmark::DoNotOptimize(d);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
BENCHMARK(BM_Evaluate_NoYAML_Geheim);

// ---------------------------------------------------------------------------
// Benchmark: evaluate() – YAML profiles loaded
// ---------------------------------------------------------------------------

static void BM_Evaluate_YAML_AllClassifications(benchmark::State &state) {
    PolicyEngine engine;
    (void)engine.loadFromYAML(yamlPath()); // best-effort; heuristic if absent

    static const std::string classes[] = {"offen", "vs-nfd", "geheim", "streng-geheim"};
    static const std::string routes[]  = {
        "/vector/search", "/query/aql", "/admin/status", "/transaction/commit"};

    int64_t idx = 0;
    for (auto _ : state) {
        auto headers = makeHeaders(classes[idx % 4]);
        PolicyDecision d = engine.evaluate(headers, routes[idx % 4]);
        benchmark::DoNotOptimize(d);
        ++idx;
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
BENCHMARK(BM_Evaluate_YAML_AllClassifications);

// ---------------------------------------------------------------------------
// Benchmark: evaluate() – CCPA opt-out overhead
// ---------------------------------------------------------------------------

static void BM_Evaluate_CCPA_OptedOut(benchmark::State &state) {
    PolicyEngine engine;
    (void)engine.loadFromYAML(yamlPath());

    auto opt_out = std::make_shared<std::unordered_set<std::string>>();
    opt_out->insert("user-42");
    engine.setCcpaOptOutSubjects(opt_out);

    auto headers = makeHeadersWithUser("vs-nfd", "user-42");

    for (auto _ : state) {
        PolicyDecision d = engine.evaluate(headers, "/vector/search");
        benchmark::DoNotOptimize(d);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
BENCHMARK(BM_Evaluate_CCPA_OptedOut);

static void BM_Evaluate_CCPA_NotOptedOut(benchmark::State &state) {
    PolicyEngine engine;
    (void)engine.loadFromYAML(yamlPath());

    // Registry present but the requesting user is NOT in it.
    auto opt_out = std::make_shared<std::unordered_set<std::string>>();
    opt_out->insert("user-999");
    engine.setCcpaOptOutSubjects(opt_out);

    auto headers = makeHeadersWithUser("vs-nfd", "user-1");

    for (auto _ : state) {
        PolicyDecision d = engine.evaluate(headers, "/vector/search");
        benchmark::DoNotOptimize(d);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
BENCHMARK(BM_Evaluate_CCPA_NotOptedOut);

// ---------------------------------------------------------------------------
// Benchmark: checkQueryPermission() – full query-time path
// ---------------------------------------------------------------------------

static void BM_CheckQueryPermission_NoYAML(benchmark::State &state) {
    PolicyEngine engine;
    auto headers = makeHeaders("vs-nfd");

    for (auto _ : state) {
        QueryPermissionResult r = engine.checkQueryPermission(headers, "/query/aql");
        benchmark::DoNotOptimize(r);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
BENCHMARK(BM_CheckQueryPermission_NoYAML);

static void BM_CheckQueryPermission_WithMaskingRules(benchmark::State &state) {
    PolicyEngine engine;
    // Load YAML so that data_masking rules (ssn, credit_card, patient_id, …)
    // are active.  Falls back to heuristic + empty masking if file absent.
    (void)engine.loadFromYAML(yamlPath());

    auto headers = makeHeaders("vs-nfd");

    for (auto _ : state) {
        QueryPermissionResult r = engine.checkQueryPermission(headers, "/query/aql");
        benchmark::DoNotOptimize(r);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
BENCHMARK(BM_CheckQueryPermission_WithMaskingRules);

static void BM_CheckQueryPermission_StrictClassification(benchmark::State &state) {
    PolicyEngine engine;
    (void)engine.loadFromYAML(yamlPath());

    auto headers = makeHeaders("streng-geheim");

    for (auto _ : state) {
        QueryPermissionResult r =
            engine.checkQueryPermission(headers, "/admin/status");
        benchmark::DoNotOptimize(r);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
BENCHMARK(BM_CheckQueryPermission_StrictClassification);

// ---------------------------------------------------------------------------
// Benchmark: checkQueryPermission() – high-volume throughput
// ---------------------------------------------------------------------------

static void BM_CheckQueryPermission_HighVolume(benchmark::State &state) {
    const int64_t batch = state.range(0);

    PolicyEngine engine;
    (void)engine.loadFromYAML(yamlPath());

    static const std::string classes[] = {"offen", "vs-nfd", "geheim", "streng-geheim"};

    for (auto _ : state) {
        for (int64_t i = 0; i < batch; ++i) {
            auto headers = makeHeaders(classes[i % 4]);
            QueryPermissionResult r =
                engine.checkQueryPermission(headers, "/query/aql");
            benchmark::DoNotOptimize(r);
        }
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * batch);
    state.counters["requests/batch"] = static_cast<double>(batch);
}
BENCHMARK(BM_CheckQueryPermission_HighVolume)->Arg(1)->Arg(10)->Arg(100)->Arg(1000);

// ---------------------------------------------------------------------------
// Benchmark: evaluate() throughput (requests per second counter)
// ---------------------------------------------------------------------------

static void BM_Evaluate_Throughput(benchmark::State &state) {
    PolicyEngine engine;
    (void)engine.loadFromYAML(yamlPath());

    auto headers = makeHeaders("vs-nfd");
    int64_t total = 0;

    for (auto _ : state) {
        PolicyDecision d = engine.evaluate(headers, "/vector/search");
        benchmark::DoNotOptimize(d);
        ++total;
    }

    state.counters["requests/sec"] = benchmark::Counter(
        static_cast<double>(total), benchmark::Counter::kIsRate);
}
BENCHMARK(BM_Evaluate_Throughput);

// ---------------------------------------------------------------------------
// Benchmark: resource-mapping lookup overhead (route resolved via YAML mapping)
// ---------------------------------------------------------------------------

static void BM_Evaluate_ResourceMappingRoute(benchmark::State &state) {
    PolicyEngine engine;
    (void)engine.loadFromYAML(yamlPath());

    // Route present in governance.yaml resource_mapping → "/admin/status" → vs-nfd
    // No X-Classification header so the engine must consult the mapping.
    std::unordered_map<std::string, std::string> headers = {
        {"X-Governance-Mode", "enforce"}};

    for (auto _ : state) {
        PolicyDecision d = engine.evaluate(headers, "/admin/status");
        benchmark::DoNotOptimize(d);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
BENCHMARK(BM_Evaluate_ResourceMappingRoute);

BENCHMARK_MAIN();
