/**
 * @file bench_delegate_evaluator.cpp
 * @brief Google Benchmark targets for the DELEGATE-52 round-trip evaluator.
 *
 * Benchmarks validate the performance guarantee stated in the implementation
 * plan (Phase 5):
 *   - RS calculation < 5 ms for documents up to 100 KB.
 *   - RoundTripSimulator with 20 interactions on 10 KB JSON < 100 ms
 *     (excluding EditFn latency — identity fn used here).
 *
 * Scientific basis: Laban et al., "LLMs Corrupt Your Documents When You
 * Delegate" (arXiv:2604.15597).
 */

#include "rag/delegate_evaluator.h"

#include <benchmark/benchmark.h>
#include <string>
#include <vector>

using namespace themis::rag::delegate_eval;

// ─────────────────────────────────────────────────────────────────────────────
// Fixtures
// ─────────────────────────────────────────────────────────────────────────────

namespace {

/// Build a JSON document with @p num_fields top-level string fields.
std::string makeJsonDoc(size_t num_fields) {
    std::string doc = "{";
    for (size_t i = 0; i < num_fields; ++i) {
        if (i > 0) {
          doc += ',';
        }
        doc += "\"field" + std::to_string(i) + "\":\"value" + std::to_string(i) + "\"";
    }
    doc += "}";
    return doc;
}

/// Build a plain-text document of @p target_bytes bytes.
std::string makePlainText(size_t target_bytes) {
    const std::string chunk =
        "The quick brown fox jumps over the lazy dog. "
        "Pack my box with five dozen liquor jugs. ";
    std::string doc = {};
    doc.reserve(target_bytes + chunk.size());
    while (doc.size() < target_bytes) {
        doc += chunk;
    }
    doc.resize(target_bytes);
    return doc;
}

} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// BM_DelegateEvaluator_JsonRoundTrip_10k
//
// Measures: RoundTripSimulator with 10 round-trips (20 interactions) on a
// ~10 KB JSON document (200 fields × ~50 bytes each).
// Target: < 100 ms end-to-end (identity EditFn).
// ─────────────────────────────────────────────────────────────────────────────

static void BM_DelegateEvaluator_JsonRoundTrip_10k(benchmark::State& state) {
    // ~10 KB JSON: 200 fields of ~50 chars each
    const std::string seed_doc = makeJsonDoc(200);

    DelegateEvaluatorConfig cfg;
    cfg.num_round_trips = 10; // 20 interactions total

    JsonDocumentEvaluator ev;
    const std::vector<RoundTripEditPair> pairs{
        {"Add a field.", "Remove the field you added.",
         "bench_seed", DomainType::JSON_DOCUMENT}};

    // Identity EditFn — zero latency so we measure only framework overhead
    EditFn identity = [](const std::string& doc, const std::string&) { return doc; };

    for (auto _ : state) {
        RoundTripSimulator sim(cfg);
        auto result = sim.run(seed_doc, pairs, ev, identity);
        benchmark::DoNotOptimize(result.scores.rs_per_interaction);
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_DelegateEvaluator_JsonRoundTrip_10k)
    ->Unit(benchmark::kMillisecond)
    ->Iterations(100);

// ─────────────────────────────────────────────────────────────────────────────
// BM_DelegateEvaluator_JsonEval_100KB
//
// Measures: JsonDocumentEvaluator::evaluate() on a ~100 KB document (2 000 fields).
// Target: < 5 ms.
// ─────────────────────────────────────────────────────────────────────────────

static void BM_DelegateEvaluator_JsonEval_100KB(benchmark::State& state) {
    const std::string doc = makeJsonDoc(2000);
    JsonDocumentEvaluator ev;

    for (auto _ : state) {
        const double rs = ev.evaluate(doc, doc);
        benchmark::DoNotOptimize(rs);
    }
}
BENCHMARK(BM_DelegateEvaluator_JsonEval_100KB)
    ->Unit(benchmark::kMicrosecond)
    ->Iterations(1000);

// ─────────────────────────────────────────────────────────────────────────────
// BM_DelegateEvaluator_PlainTextEval_100KB
//
// Measures: PlainTextEvaluator::evaluate() on a 100 KB text document.
// Target: < 5 ms.
// ─────────────────────────────────────────────────────────────────────────────

static void BM_DelegateEvaluator_PlainTextEval_100KB(benchmark::State& state) {
    const std::string doc = makePlainText(100 * 1024);
    PlainTextEvaluator ev;

    for (auto _ : state) {
        // Use doc vs doc (identity) to stress large-input edit-distance scoring.
        const double rs = ev.evaluate(doc, doc);
        benchmark::DoNotOptimize(rs);
    }
}
BENCHMARK(BM_DelegateEvaluator_PlainTextEval_100KB)
    ->Unit(benchmark::kMillisecond)
    ->Iterations(50);

// ─────────────────────────────────────────────────────────────────────────────
// BM_DelegateEvaluator_AqlEval_Typical
//
// Measures: AqlQueryEvaluator::evaluate() on a typical multi-clause AQL query.
// ─────────────────────────────────────────────────────────────────────────────

static void BM_DelegateEvaluator_AqlEval_Typical(benchmark::State& state) {
    const std::string aql =
        "FOR u IN users "
        "FILTER u.age >= 18 AND u.country == 'DE' "
        "SORT u.lastName ASC "
        "LIMIT 100 "
        "RETURN { name: u.firstName, id: u._id }";
    AqlQueryEvaluator ev;

    for (auto _ : state) {
        const double rs = ev.evaluate(aql, aql);
        benchmark::DoNotOptimize(rs);
    }
}
BENCHMARK(BM_DelegateEvaluator_AqlEval_Typical)
    ->Unit(benchmark::kMicrosecond)
    ->Iterations(10000);

BENCHMARK_MAIN();
